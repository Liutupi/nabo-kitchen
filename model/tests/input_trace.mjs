import {bindControls} from '../../prototype/input.mjs';
let now=0,next=1;const timers=new Map(),handlers={},events=[];
globalThis.setTimeout=(fn,ms)=>{const id=next++;timers.set(id,{fn,at:now+ms});return id;};
globalThis.clearTimeout=id=>timers.delete(id);
globalThis.window={addEventListener:(name,fn)=>handlers[name]=fn};
globalThis.document={querySelectorAll:()=>[]};
const controls=bindControls((key,kind)=>events.push({key,kind}),()=>now);
const trace=[];
function action(op,key='UP',delta=0){
 const end=now+delta;
 for(;;){const due=[...timers].filter(([,x])=>x.at<=end).sort((a,b)=>a[1].at-b[1].at||a[0]-b[0])[0];
  if(!due)break;now=due[1].at;timers.delete(due[0]);due[1].fn();}
 now=end;const start=events.length;
 if(op==='press')controls.press(key);
 if(op==='release')controls.release(key);
 if(op==='cancel')controls.release(key,true);
 if(op==='blur')handlers.blur();
 // Include events emitted while advancing time as well as during the action.
 trace.push({op,key,now,events:events.slice(trace.reduce((n,t)=>n+t.events.length,0))});
}
for(const [op,key,dt] of [['press','UP',0],['release','UP',50],['press','UP',100],['release','UP',100],
 ['press','DOWN',5],['release','DOWN',20],['press','UP',5],['release','UP',10],['press','OK',100],
 ['release','OK',700],['press','UP',20],['release','UP',5],['poll','UP',280],['press','OK',10],
 ['cancel','OK',10],['press','UP',5],['blur','UP',5]])action(op,key,dt);
let seed=0xa23be3c1;
for(let i=0;i<2000;i++){
 seed=(Math.imul(seed,1664525)+1013904223)>>>0;const op=['press','release','cancel','poll','blur'][(seed>>>16)%5];
 seed=(Math.imul(seed,1664525)+1013904223)>>>0;const key=['UP','DOWN','OK'][(seed>>>16)%3];
 seed=(Math.imul(seed,1664525)+1013904223)>>>0;action(op,key,(seed>>>16)%350);
}
process.stdout.write(JSON.stringify(trace));
