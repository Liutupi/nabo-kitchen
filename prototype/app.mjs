import * as model from './model.mjs';
import * as store from './state.mjs';
import {Timers} from './timers.mjs';
import {bindControls} from './input.mjs';
import {render} from './view.mjs';

const testMode=new URLSearchParams(location.search).has('test');
const key=testMode?'nabo-kitchen-isolated-tests':store.STORAGE_KEY;
const testEpoch=Date.UTC(2026,9,28,12),clockKey=key+'-clock';
let testClock=testEpoch;
if(testMode){try{testClock=Number(localStorage.getItem(clockKey))||testEpoch;}catch{}}
const now=()=>testMode?testClock:Date.now(),day=()=>model.localDay(new Date(now()));
const [recipes,ingredientData]=await Promise.all([fetch('/api/recipes').then(r=>{if(!r.ok)throw Error('菜谱加载失败');return r.json();}),fetch('/data/ingredients.json').then(r=>r.json())]);
const ingredients=ingredientData.ingredients;
let state=store.restore(localStorage,key,day());
const timers=new Timers(now,state.timerItems||[]);
let ui={page:0,focus:0,stack:[],recipeId:recipes[0]?.id,result:[],notes:[],comboFocus:0,bookFilter:0,settingsMode:'',clearStage:0,lastActivity:now(),message:'',outside:false};
let messageTimeout,transitionTimeout;
const screen=document.querySelector('#screen');
const recipe=()=>recipes.find(r=>r.id===ui.recipeId)||recipes[0];
const context=()=>({...state.settings,history:state.history,favorites:state.favorites,wanted:state.wanted,day:day()});
function persist(){state.timerItems=timers.serialize();if(!store.save(localStorage,state,key))ui.message='本地保存失败，请检查浏览器存储';}
function paint(){
  const renderKey=`${ui.page}:${ui.recipeId}:${state.progress[ui.recipeId]?.index??0}`;
  const keepScroll=ui.lastRenderKey===renderKey;
  const stepScroll=keepScroll?(screen.querySelector('#step-text')?.scrollTop||0):0;
  const paperScroll=keepScroll?(screen.querySelector('.paper.scroll')?.scrollTop||0):0;
  screen.dataset.page='P'+ui.page;screen.classList.toggle('outside',ui.outside);
  screen.innerHTML=render({ui,state,recipes,ingredients,timers,now:now(),day:day()});
  if(screen.querySelector('#step-text'))screen.querySelector('#step-text').scrollTop=stepScroll;
  if(screen.querySelector('.paper.scroll'))screen.querySelector('.paper.scroll').scrollTop=paperScroll;
  ui.lastRenderKey=renderKey;
  document.querySelector('#outside-notice').hidden=!ui.outside;
  document.querySelector('#library-count').textContent=recipes.length;
}
function toast(text){ui.message=text;clearTimeout(messageTimeout);messageTimeout=setTimeout(()=>{ui.message='';paint();},2400);paint();}
function go(page,{push=true,focus=0}={}){
  if(push)ui.stack.push({page:ui.page,focus:ui.focus,recipeId:ui.recipeId});
  ui.page=page;ui.focus=focus;ui.clearStage=0;ui.settingsMode='';paint();
}
function home(){ui.stack=[];ui.outside=false;go(1,{push:false});}
function back(){
  if(ui.page===13&&ui.settingsMode){ui.settingsMode='';ui.focus=3;paint();return;}
  const prev=ui.stack.pop();if(prev){Object.assign(ui,prev);ui.clearStage=0;paint();}else home();
}
function result(ids,notes=[]){ui.result=ids;ui.notes=notes;ui.comboFocus=0;ui.recipeId=ids[0];go(4,{push:ui.page!==3});}
function draw(reroll=false){
  store.ensureDay(state,day());
  if(!reroll&&state.daily.result&&state.daily.conditions===JSON.stringify(state.conditions)){
    result(state.daily.result.ids,state.daily.result.notes);return;
  }
  const replacing=reroll||Boolean(state.daily.result);
  if(replacing&&state.daily.rerollsUsed>=state.settings.rerolls){toast('今天的重抽次数用完啦');return;}
  const used=state.daily.rerollsUsed+(replacing?1:0),seed=(state.daily.seed+Math.imul(used,0x9E3779B9))>>>0;
  const choice=model.recommend(recipes,state.conditions,context(),seed,replacing?(ui.result.length?ui.result:state.daily.result.ids):[]);
  if(!choice.ids.length){toast(choice.reason);return;}
  if(replacing)state.daily.rerollsUsed++;
  state.daily.result=choice;state.daily.conditions=JSON.stringify(state.conditions);persist();
  go(3,{push:ui.page!==4});clearTimeout(transitionTimeout);transitionTimeout=setTimeout(()=>{if(ui.page===3)result(choice.ids,choice.notes);},testMode?20:1600);
}
function openRecipe(id){ui.recipeId=id;go(5);}
function start(){const p=store.session(state,recipe());if(p.completed)delete state.progress[recipe().id];go(6);persist();}
function nextStep(){
  const r=recipe(),p=store.session(state,r),step=r.steps[p.index],source=`${r.id}:${p.index}`;
  if(step.timer&&!p.timerSteps.includes(p.index)){
    const existing=timers.items.find(t=>t.source===source);
    const outcome=existing?{timer:existing}:timers.add(step.timer,r.name,source);
    if(outcome.error){toast(outcome.error);return;}
    p.timerSteps.push(p.index);persist();paint();return;
  }
  const outcome=store.completeStep(state,r,day());persist();
  if(outcome.complete)go(8);else if(outcome.error)toast(outcome.error);else paint();
}
const bookList=()=>recipes.filter(r=>ui.bookFilter===0||ui.bookFilter===1&&state.cooked[r.id]||ui.bookFilter===2&&state.favorites.includes(r.id)||ui.bookFilter===3&&r.source==='user');
const fridgeCatalog=()=>ingredients.filter(i=>i.v===1);
function focusCount(){
  return ({1:5,2:4,4:Math.max(1,ui.result.length),9:fridgeCatalog().length,10:model.fridgeMatch(recipes,state.fridge,state.settings.excluded).length,11:state.shopping.length+1,12:bookList().length+1,13:ui.settingsMode==='exclude'?ingredients.length:8,7:Math.max(1,timers.items.length)})[ui.page]||1;
}
function toggleFavorite(id){if(!id)return;state.favorites=state.favorites.includes(id)?state.favorites.filter(x=>x!==id):[...state.favorites,id];persist();toast(state.favorites.includes(id)?'已收藏':'已取消收藏');}
function setting(index){
  ui.focus=index;
  if(index!==7)ui.clearStage=0;
  if(index===0)state.settings.sound=!state.settings.sound;
  if(index===1)state.settings.volume=(state.settings.volume+10)%110;
  if(index===2)state.settings.rerolls=state.settings.rerolls%5+1;
  if(index===3){ui.settingsMode='exclude';ui.focus=0;}
  if(index===4)state.settings.lessSpicy=!state.settings.lessSpicy;
  if(index===5)state.settings.lessOil=!state.settings.lessOil;
  if(index===6)state.settings.difficulty=state.settings.difficulty%3+1;
  if(index===7){ui.clearStage++;if(ui.clearStage===3){store.clearRecords(state);ui.clearStage=0;toast('记录已清空');}else toast(ui.clearStage===1?'确认 1/2：清空做菜记录？':'确认 2/2：再按 OK 清空');}
  persist();paint();
}
function activate(name,index){
  if(name==='home'){ui.focus=Number(index);if(ui.focus===0)go(2);else go([0,9,11,12,13][ui.focus]);}
  else if(name==='condition'){ui.focus=Number(index);const prop=['people','minutes','combo'][ui.focus],values=[[1,2,3,4,5],[15,30,60,0],[0,1,2,3]][ui.focus];state.conditions[prop]=values[(values.indexOf(state.conditions[prop])+1)%values.length];persist();paint();}
  else if(name==='draw')draw();
  else if(name==='reroll')draw(true);
  else if(name==='back')back();
  else if(name==='lobby')home();
  else if(name==='recipe')openRecipe(ui.result[ui.comboFocus]||ui.recipeId);
  else if(name==='start')start();
  else if(name==='combo'){ui.comboFocus=Number(index);ui.recipeId=ui.result[ui.comboFocus];paint();}
  else if(name==='timers'){const alarm=timers.alarm();go(7,{focus:alarm?timers.items.indexOf(alarm):0});}
  else if(name==='timer-select'){ui.focus=Number(index);paint();}
  else if(name==='category'){const i=fridgeCatalog().findIndex(i=>i.cat===index);if(i>=0)ui.focus=i;paint();}
  else if(name==='fridge'){ui.focus=Number(index);const id=fridgeCatalog()[ui.focus].id;state.fridge=state.fridge.includes(id)?state.fridge.filter(x=>x!==id):[...state.fridge,id];persist();paint();}
  else if(name==='recommendation'){const m=model.fridgeMatch(recipes,state.fridge,state.settings.excluded)[Number(index)];if(m)result([m.recipe.id]);}
  else if(name==='shopping'){const x=state.shopping.find(x=>x.uid===Number(index));if(x)x.bought=!x.bought;persist();paint();}
  else if(name==='clear-bought'){state.shopping=state.shopping.filter(x=>!x.bought);ui.focus=Math.min(ui.focus,state.shopping.length);persist();paint();}
  else if(name==='book-filter'){ui.bookFilter=(ui.bookFilter+1)%4;ui.focus=0;paint();}
  else if(name==='book'){const r=bookList()[Number(index)];if(r)result([r.id]);}
  else if(name==='setting')setting(Number(index));
  else if(name==='exclude'){ui.focus=Number(index);const id=ingredients[ui.focus].id;state.settings.excluded=state.settings.excluded.includes(id)?state.settings.excluded.filter(x=>x!==id):[...state.settings.excluded,id];persist();paint();}
}
function dispatch(key,kind='short'){
  ui.lastActivity=now();
  if(timers.tick())persist();
  if(key==='OK'&&kind==='short'&&timers.alarm()&&ui.page!==7){ui.outside=false;activate('timers');return;}
  if(ui.outside){if(key==='UP'&&kind==='long')return;home();return;}
  if(ui.page===0){home();return;}
  if(key==='UP'&&kind==='long'){ui.outside=true;persist();paint();return;}
  if(key==='UP'&&kind==='double'){back();return;}
  if(kind==='long'){
    if(key==='OK'&&ui.page===1)draw();
    else if(key==='OK'&&ui.page===9)go(10);
    else if(key==='DOWN'&&ui.page===6)go(5);
    else if(key==='DOWN'&&ui.page===5){const paper=screen.querySelector('.paper.scroll');if(paper)paper.scrollTop=paper.scrollTop+paper.clientHeight>=paper.scrollHeight-2?0:paper.scrollTop+120;}
    else if(key==='OK'&&ui.page===7){const t=timers.items[ui.focus];if(t)timers.toggle(t.id);persist();paint();}
    else if(key==='OK'&&[4,12].includes(ui.page))toggleFavorite(ui.page===4?ui.recipeId:bookList()[ui.focus-1]?.id);
    return;
  }
  if(ui.page===6){
    if(key==='OK')nextStep();
    if(key==='UP'){const p=store.session(state,recipe());p.index=Math.max(0,p.index-1);persist();paint();}
    if(key==='DOWN'){const el=document.querySelector('#step-text');if(el&&el.scrollTop+el.clientHeight<el.scrollHeight-2)el.scrollBy(0,50);else activate('timers');}
    return;
  }
  if(ui.page===4&&key==='DOWN'){draw(true);return;}
  if(ui.page===4&&key==='UP'&&ui.result.length>1){ui.comboFocus=(ui.comboFocus+ui.result.length-1)%ui.result.length;ui.recipeId=ui.result[ui.comboFocus];paint();return;}
  if(ui.page===4&&key==='UP'){home();return;}
  if(ui.page===5){if(key==='UP')screen.querySelector('.paper.scroll')?.scrollBy(0,-120);if(key==='DOWN'){store.addShopping(state,recipe(),state.conditions.people);persist();toast('食材已加入买菜单');}if(key==='OK')start();return;}
  if(ui.page===8){if(key==='OK')home();return;}
  if(key==='UP'||key==='DOWN'){ui.focus=(ui.focus+(key==='UP'?-1:1)+focusCount())%focusCount();ui.clearStage=0;paint();return;}
  if(key!=='OK')return;
  if(ui.page===1)activate('home',ui.focus);
  else if(ui.page===2)ui.focus===3?draw():activate('condition',ui.focus);
  else if(ui.page===4)activate('recipe');
  else if(ui.page===7){const t=timers.items[ui.focus];if(t?.status==='done'){timers.acknowledge(t.id);timers.cancel(t.id);ui.focus=0;persist();paint();}}
  else if(ui.page===9)activate('fridge',ui.focus);
  else if(ui.page===10)activate('recommendation',ui.focus);
  else if(ui.page===11){const x=store.sortedShopping(state)[ui.focus];activate(x?'shopping':'clear-bought',x?.uid);}
  else if(ui.page===12)activate(ui.focus?'book':'book-filter',ui.focus-1);
  else if(ui.page===13)activate(ui.settingsMode?'exclude':'setting',ui.focus);
}
screen.addEventListener('click',e=>{const b=e.target.closest('[data-action]');if(b){ui.lastActivity=now();activate(b.dataset.action,b.dataset.index);}});
bindControls(dispatch,now);document.querySelector('#reopen').onclick=home;
let second=-1;setInterval(()=>{const change=timers.tick();if(change)persist();if(change||Math.trunc(now()/1000)!==second){second=Math.trunc(now()/1000);paint();}},200);
if(!testMode)transitionTimeout=setTimeout(()=>{if(ui.page===0)home();},1200);
window.kitchenTest={
  ready:true,model,dispatch,snapshot:()=>structuredClone({state,ui,timers:timers.snapshot(),recipeCount:recipes.length,recencyPolicy:'weight-only'}),
  recommend:(conditions,ctx,seed,avoid=[])=>model.recommend(recipes,conditions,ctx,seed,avoid),
  advanceTime(ms){if(!testMode)throw Error('test mode required');testClock+=ms;localStorage.setItem(clockKey,String(testClock));timers.tick();persist();paint();},
  reset(overrides={},clockMs=testEpoch){if(!testMode)throw Error('test mode required');testClock=clockMs;localStorage.setItem(clockKey,String(testClock));clearTimeout(messageTimeout);const base=store.initialState(day());state={...base,...structuredClone(overrides),settings:{...base.settings,...overrides.settings},conditions:{...base.conditions,...overrides.conditions}};timers.items=[];timers.nextId=1;ui={...ui,page:1,focus:0,stack:[],result:[],notes:[],outside:false,clearStage:0,settingsMode:'',bookFilter:0,message:'',lastActivity:now()};persist();paint();},
  showResult(ids){if(!testMode)throw Error('test mode required');if(new Set(ids).size!==ids.length||ids.some(id=>!recipes.some(r=>r.id===id)))throw Error('known distinct recipe IDs required');result(ids);},
  visit(page,id){if(!testMode)throw Error('test mode required');if(id)ui.recipeId=id;go(page);},
  setTimer(seconds,label){if(!testMode)throw Error('test mode required');const r=timers.add(seconds,label);persist();paint();return r;}
};
persist();paint();
