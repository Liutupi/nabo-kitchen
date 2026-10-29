import {drawSeed,levelFor,scaleQty} from './model.mjs';
export const STORAGE_KEY='nabo-kitchen-prototype-v1';
export function initialState(day='2026-10-28'){
  return {version:2,drawCounter:0,conditions:{people:2,minutes:30,combo:0},settings:{sound:false,volume:50,rerolls:3,excluded:[],lessSpicy:false,lessOil:false,difficulty:2},fridge:[],favorites:[],wanted:[],history:[],cooked:{},streak:{lastDay:'',days:0},shopping:[],progress:{},daily:{seed:drawSeed(0),rerollsUsed:0,result:null},device:'browser-01'};
}
export function migrateState(value,ids){
  const base=initialState(),s={...base,...value,version:2,settings:{...base.settings,...value?.settings},conditions:{...base.conditions,...value?.conditions}};
  if(value?.version===1)s.history=[...(s.history||[])].sort((a,b)=>String(a.day||'').localeCompare(String(b.day||'')));
  s.history=(s.history||[]).map(x=>({id:x.id}));
  s.cooked=Object.fromEntries(Object.entries(s.cooked||{}).map(([id,x])=>[id,{count:x.count}]));
  s.drawCounter=Number(value?.version===2&&Number.isInteger(value.drawCounter)?value.drawCounter:s.daily?.rerollsUsed||0)>>>0;
  s.daily={...base.daily,...s.daily};delete s.daily.day;s.streak={lastDay:'',days:0};
  if(ids){const valid=new Set(ids);s.history=s.history.filter(x=>valid.has(x.id));for(const k of ['cooked','progress'])s[k]=Object.fromEntries(Object.entries(s[k]).filter(([id])=>valid.has(id)));for(const k of ['favorites','wanted'])s[k]=s[k].filter(id=>valid.has(id));if(s.daily.result?.ids.some(id=>!valid.has(id)))s.daily.result=null;s.timerItems=(s.timerItems||[]).filter(t=>!t.source||valid.has(t.source.split(':')[0]));}
  return s;
}
export function restore(storage,key=STORAGE_KEY,unused,ids){
  try{const s=JSON.parse(storage.getItem(key));if(s?.version===1||s?.version===2)return migrateState(s,ids);}catch{}
  return initialState();
}
export function save(storage,state,key=STORAGE_KEY){try{storage.setItem(key,JSON.stringify(state));return true;}catch{return false;}}
// Compatibility entry point: calendar changes never invalidate results or progress.
export function ensureDay(){}
export function stats(s){
  const unique=Object.keys(s.cooked).length,total=Object.values(s.cooked).reduce((n,x)=>n+x.count,0);
  return {unique,total,streak:0,today:0,level:levelFor(unique,total)};
}
export function session(s,recipe){
  return s.progress[recipe.id]??=( {index:0,visited:[],timerSteps:[],completed:false} );
}
export function completeStep(s,recipe,day){
  const p=session(s,recipe);if(p.completed)return {complete:true,newCompletion:false};
  if(!p.visited.includes(p.index))p.visited.push(p.index);
  if(p.index<recipe.steps.length-1){p.index++;return {complete:false};}
  if(recipe.steps.some((_,i)=>!p.visited.includes(i)))return {complete:false,error:'还有步骤未完成'};
  p.completed=true;s.history.push({id:recipe.id});s.history=s.history.slice(-60);
  s.cooked[recipe.id]={count:(s.cooked[recipe.id]?.count||0)+1};
  s.daily.result=null;
  return {complete:true,newCompletion:true};
}
export function addShopping(s,recipe,people){
  for(const item of recipe.ingredients){
    const qty=scaleQty(item.qty,recipe.servings,people),key=`${item.id}|${item.unit}`;
    const old=s.shopping.find(x=>x.key===key&&!x.bought);
    if(old&&typeof old.qty==='number'&&typeof qty==='number')old.qty=Math.round((old.qty+qty)*100)/100;
    else s.shopping.push({key,uid:Math.max(0,...s.shopping.map(x=>x.uid||0))+1,id:item.id,unit:item.unit,qty,bought:false});
  }
}
export const sortedShopping = s => [...s.shopping].sort((a,b)=>Number(a.bought)-Number(b.bought));
export function clearRecords(s){s.history=[];s.cooked={};s.streak={lastDay:'',days:0};s.progress={};}
