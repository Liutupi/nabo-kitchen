import * as s from './fixtures/prototype-v0.1/state.mjs';
import {dayNumber} from './fixtures/prototype-v0.1/model.mjs';
const ids=['a','b','c'], recipes=ids.map(id=>({id,steps:[{},{},{}]}));
const state=s.initialState('2026-10-28'); let current=0;
const trace=[];
function add(op,value=0,day='2026-10-28'){
 let result=null;
 if(op==='select')current=value;
 if(op==='reset')delete state.progress[ids[current]];
 if(op==='index')s.session(state,recipes[current]).index=value;
 if(op==='step'){
  const r=s.completeStep(state,recipes[current],day);
  result=r.newCompletion?'complete':r.error?'incomplete':r.complete?'already':'advanced';
 }
 const p=state.progress[ids[current]] || {index:0,visited:[],completed:false};
 trace.push({op,value,day:dayNumber(day),current,result,session:{index:p.index,completed:p.completed,
   visited:p.visited.reduce((x,i)=>x|(1<<i),0)},
   history:state.history.map(h=>({id:ids.indexOf(h.id),day:dayNumber(h.day)})),
   cooked:Object.fromEntries(Object.entries(state.cooked).map(([id,v])=>[ids.indexOf(id),{count:v.count,day:dayNumber(v.lastDay)}])),
   stats:s.stats(state,day)});
}
add('index',2);add('step');add('reset');add('step');add('step');add('step');add('step');
add('select',1);add('step',0,'2026-10-29');add('step',0,'2026-10-29');add('step',0,'2026-10-29');
add('reset',0,'2026-10-29');add('step',0,'2026-10-29');add('step',0,'2026-10-29');add('step',0,'2026-10-29');
add('select',2,'2026-11-02');add('step',0,'2026-11-02');add('step',0,'2026-11-02');add('step',0,'2026-11-02');
for(let i=0;i<65;i++){const date=new Date(Date.UTC(2026,10,3+i)).toISOString().slice(0,10);add('reset',0,date);add('step',0,date);add('step',0,date);add('step',0,date);}
add('stats',0,'2027-01-09');add('stats',0,'2025-01-01');
process.stdout.write(JSON.stringify(trace));
