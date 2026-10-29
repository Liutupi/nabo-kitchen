import {dailySeed,dayNumber,levelFor,scaleQty} from './model.mjs';
export const STORAGE_KEY='nabo-kitchen-prototype-v1';
export function initialState(day='2026-10-28'){
  return {version:1,conditions:{people:2,minutes:30,combo:0},settings:{sound:false,volume:50,rerolls:3,excluded:[],lessSpicy:false,lessOil:false,difficulty:2},fridge:[],favorites:[],wanted:[],history:[],cooked:{},streak:{lastDay:'',days:0},shopping:[],progress:{},daily:{day,seed:dailySeed(day),rerollsUsed:0,result:null},device:'browser-01'};
}
export function restore(storage,key=STORAGE_KEY,day){
  const base=initialState(day);try{const s=JSON.parse(storage.getItem(key));if(s?.version===1)return {...base,...s,settings:{...base.settings,...s.settings},conditions:{...base.conditions,...s.conditions}};}catch{}
  return base;
}
export function save(storage,state,key=STORAGE_KEY){try{storage.setItem(key,JSON.stringify(state));return true;}catch{return false;}}
export function ensureDay(s,day){if(s.daily.day!==day)s.daily={day,seed:dailySeed(day,s.device),rerollsUsed:0,result:null};}
export function stats(s,day){
  const dates=[...new Set(s.history.map(h=>h.day))].sort().reverse(),today=dayNumber(day);let streak=0,cursor=today;
  if(dates.length&&dayNumber(dates[0])===today-1)cursor--;
  for(const d of dates){if(dayNumber(d)===cursor){streak++;cursor--;}else if(dayNumber(d)<cursor)break;}
  if(s.streak?.lastDay){const age=today-dayNumber(s.streak.lastDay);streak=age>=0&&age<=1?s.streak.days:0;}
  const unique=Object.keys(s.cooked).length,total=Object.values(s.cooked).reduce((n,x)=>n+x.count,0);
  return {unique,total,streak,today:s.history.filter(h=>h.day===day).length,level:levelFor(unique,streak)};
}
export function session(s,recipe){
  return s.progress[recipe.id]??=( {index:0,visited:[],timerSteps:[],completed:false} );
}
export function completeStep(s,recipe,day){
  const p=session(s,recipe);if(p.completed)return {complete:true,newCompletion:false};
  if(!p.visited.includes(p.index))p.visited.push(p.index);
  if(p.index<recipe.steps.length-1){p.index++;return {complete:false};}
  if(recipe.steps.some((_,i)=>!p.visited.includes(i)))return {complete:false,error:'还有步骤未完成'};
  p.completed=true;s.history.push({id:recipe.id,day});s.history=s.history.slice(-60);
  s.cooked[recipe.id]={count:(s.cooked[recipe.id]?.count||0)+1,lastDay:day};
  const age=s.streak.lastDay?dayNumber(day)-dayNumber(s.streak.lastDay):99999;
  s.streak={lastDay:day,days:age===0?s.streak.days:age===1?s.streak.days+1:1};
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
