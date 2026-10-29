import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile,readdir} from 'node:fs/promises';
import * as m from '../model.mjs';
import * as s from '../state.mjs';
import {Timers} from '../timers.mjs';
const dir=new URL('../../data/recipes/curated/',import.meta.url);
const recipes=await Promise.all((await readdir(dir)).filter(f=>f.endsWith('.json')).map(async f=>JSON.parse(await readFile(new URL(f,dir),'utf8'))));
const sample=recipes.find(r=>r.id==='fanqie-chaodan');
const day='2026-10-28';
const fixture=(id,course,method='stir_fry',ingredient=id,minutes=10)=>({id,name:id,course:[course],minutes,servings:2,difficulty:1,ingredients:[{id:ingredient,main:true,qty:2,unit:'个'}],steps:[{method,text:id,heat:'mid'}],seasonings:[],tags:[]});
test('mulberry32 stable uint32 vectors and date/device separation',()=>{
  const a=m.mulberry32(1),b=m.mulberry32(1);const seq=Array.from({length:8},()=>a());assert.deepEqual(seq,Array.from({length:8},()=>b()));
  assert.deepEqual(seq.slice(0,3),[2693262067,11749833,2265367787]);
  assert.notEqual(m.dailySeed(day,'a'),m.dailySeed(day,'b'));assert.notEqual(m.dailySeed(day),m.dailySeed('2026-10-29'));
});
test('recency weights use completion positions, ignore dates and never exclude',()=>{
  const r={...sample,season:[10]};
  for(const [age,w] of [[0,10],[1,10],[2,10],[3,40],[6,40],[7,80],[13,80],[14,100]]){
    const history=[{id:r.id,day:'1900-01-01'},...Array.from({length:age},()=>({id:'other'}))];
    assert.equal(m.recipeWeight(r,{day:'2099-12-31',history}),w);
  }
  assert.equal(m.recipeWeight({...r,difficulty:3},{day,history:[{id:r.id,day}],favorites:[r.id],wanted:[r.id],difficulty:1}),9);
  const recent=fixture('recent','meat'),other=fixture('other','meat'),ctx={day,history:[{id:'recent',day}]};
  assert.equal(m.recipeWeight(recent,ctx),10);assert.equal(m.recipeWeight(other,ctx),100);
  assert.ok(Array.from({length:1000},(_,seed)=>m.recommend([recent,other],{combo:0,minutes:30},ctx,seed)).some(x=>x.ids[0]==='recent'));
});
test('hard exclusions survive every soft relaxation and deterministic rerolls avoid same set',()=>{
  const a=fixture('a','meat'),b=fixture('b','meat');
  assert.deepEqual(m.recommend([a],{combo:0,minutes:1},{excluded:['a']},9).ids,[]);
  const one=m.recommend([a,b],{combo:0,minutes:30},{day},123);
  assert.deepEqual(one,m.recommend([a,b],{combo:0,minutes:30},{day},123));
  assert.notDeepEqual(one.ids,m.recommend([a,b],{combo:0,minutes:30},{day},123,one.ids).ids);
  const relaxed=m.recommend([fixture('slow','meat','steam','x',60)],{combo:0,minutes:15},{day},1);
  assert.deepEqual(relaxed.notes,['已放宽：总用时']);
});
test('three/four dish assignments enforce roles, uniqueness, ingredients, methods and time',()=>{
  const rs=[fixture('meat1','meat'),fixture('meat2','meat','steam'),fixture('veg','veg'),fixture('soup','soup','boil')];
  for(const combo of [2,3]){const r=m.recommend(rs,{combo,minutes:60},{day},42);assert.equal(r.ids.length,combo+1);assert.equal(new Set(r.ids).size,r.ids.length);assert.deepEqual(r.notes,[]);}
  const overlap=[fixture('m','meat','steam','shared'),fixture('v','veg','stir_fry','shared')];
  assert.deepEqual(m.recommend(overlap,{combo:1,minutes:30},{day},1).notes,['已放宽：主料重复']);
  assert.ok(m.recommend([fixture('m','meat')],{combo:2,minutes:60},{day},1).reason.includes('无法组成'));
});
test('fridge scores main ingredients only, ignores seasonings and hides missing two',()=>{
  const r={...sample,ingredients:[{id:'egg',main:true},{id:'tomato',main:true},{id:'scallion',main:false}]};
  assert.equal(m.fridgeMatch([r],[]).length,0);assert.deepEqual(m.fridgeMatch([r],['egg'])[0].missing,['tomato']);
  assert.equal(m.fridgeMatch([r],['egg','tomato'])[0].score,1000);assert.equal(m.fridgeMatch([r],['egg'],['scallion']).length,0);
});
test('shopping scales and merges only same unit; bought entries sink and remain separate',()=>{
  const st=s.initialState(day);s.addShopping(st,sample,4);s.addShopping(st,sample,4);
  assert.equal(st.shopping.find(x=>x.id==='egg').qty,12);assert.equal(st.shopping.length,sample.ingredients.length);
  st.shopping[0].bought=true;s.addShopping(st,sample,2);assert.equal(st.shopping.length,sample.ingredients.length+1);
  assert.equal(s.sortedShopping(st).at(-1).bought,true);assert.equal(new Set(st.shopping.map(x=>x.uid)).size,st.shopping.length);
});
test('full walk required for completion; repeated cooking increments lifetime but not unlock count',()=>{
  const st=s.initialState(day),r={...sample,steps:sample.steps.slice(0,3)};
  s.session(st,r).index=2;assert.equal(s.completeStep(st,r,day).complete,false);assert.equal(st.history.length,0);
  st.progress={};for(let i=0;i<3;i++)s.completeStep(st,r,day);
  assert.equal(st.history.length,1);assert.equal(s.completeStep(st,r,day).newCompletion,false);
  delete st.progress[r.id];for(let i=0;i<3;i++)s.completeStep(st,r,'2026-10-29');
  assert.deepEqual(s.stats(st,'2026-10-29'),{unique:1,total:2,streak:0,today:0,level:1});
  st.history=[];assert.equal(s.stats(st,day).total,2);assert.equal(s.stats(st,day).unique,1);
});
test('persistence corruption recovery and date rollover do not erase sessions',()=>{
  const st=s.initialState(day);st.progress[sample.id]={index:2,visited:[0,1],timerSteps:[]};let value;
  const memory={getItem:()=>value,setItem:(_,x)=>value=x};s.save(memory,st);assert.equal(s.restore(memory,undefined,day).progress[sample.id].index,2);
  s.ensureDay(st,'2026-10-29');assert.equal(st.daily.rerollsUsed,0);assert.equal(st.progress[sample.id].index,2);
  value='{broken';assert.deepEqual(s.restore(memory,undefined,day),s.initialState(day));
});
test('three total timers persist, pause/resume and expire independent of pages',()=>{
  let now=1000;const t=new Timers(()=>now);for(let i=0;i<3;i++)assert.ok(t.add(5+i,'t'+i).timer);
  assert.ok(t.add(30,'fourth').error);now+=2000;t.toggle(1);assert.equal(t.remaining(t.items[0]),3000);
  now+=10000;t.tick();assert.equal(t.items[0].status,'paused');assert.equal(t.items[1].status,'done');assert.ok(t.add(1,'still fourth').error);
  const restored=new Timers(()=>now,t.serialize());restored.toggle(1);now+=3000;restored.tick();assert.equal(restored.items[0].status,'done');
  restored.cancel(2);assert.ok(restored.add(10,'replacement').timer);
});
test('approved thirty-menu library can form real three/four-dish combinations',()=>{
  assert.ok(recipes.length>=30);
  for(const combo of [2,3]){
    const choice=m.recommend(recipes,{combo,minutes:60},{day},321),selected=choice.ids.map(id=>recipes.find(r=>r.id===id));
    assert.equal(choice.ids.length,combo+1);assert.equal(new Set(choice.ids).size,choice.ids.length);
    assert.ok(selected.some(r=>r.course.includes('veg')));assert.ok(selected.some(r=>r.course.includes('soup')));
    assert.ok(selected.filter(r=>r.course.includes('meat')).length>=(combo===3?2:1));
  }
});
test('v2 levels use unique dishes and total completions',()=>{
  for(const [unique,total,level] of [[0,0,1],[9,59,1],[10,10,2],[1,60,3],[40,40,3],[100,199,3],[99,200,3],[100,200,4]])assert.equal(m.levelFor(unique,total),level);
});
test('restored running timers pause with the last saved remaining duration',()=>{
  let now=1000;const first=new Timers(()=>now);first.add(10,'恢复测试');now=6000;
  const saved=first.serialize();now=86400000;
  const restored=new Timers(()=>now,saved);assert.equal(restored.items[0].status,'paused');assert.equal(restored.remaining(restored.items[0]),5000);
  restored.toggle(1);now+=2000;assert.equal(restored.remaining(restored.items[0]),3000);
});
test('legacy date history migrates once and retired references are filtered',()=>{
  const old={...s.initialState(),version:1,history:[{id:'b',day:'2026-10-29'},{id:'a',day:'2026-10-27'},{id:'retired',day:'2026-10-28'}],cooked:{a:{count:4,lastDay:'2026-10-27'},retired:{count:1}},favorites:['a','retired'],progress:{a:{index:1},retired:{index:2}},daily:{day:'2026-10-29',result:{ids:['a','retired']},rerollsUsed:5}};
  const st=s.migrateState(old,['a','b']);assert.deepEqual(st.history,[{id:'a'},{id:'b'}]);assert.deepEqual(st.cooked,{a:{count:4}});assert.deepEqual(st.favorites,['a']);assert.equal(st.progress.a.index,1);assert.equal(st.daily.result,null);assert.equal(st.drawCounter,5);assert.equal(st.version,2);
  assert.equal(m.drawSeed(7,'unit-1'),m.drawSeed(7,'unit-1'));assert.notEqual(m.drawSeed(7,'unit-1'),m.drawSeed(8,'unit-1'));
});
