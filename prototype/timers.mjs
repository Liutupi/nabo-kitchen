export class Timers{
  constructor(now=()=>Date.now(),saved=[]){this.now=now;this.items=saved.map(t=>({...t,status:t.status==='running'?'paused':t.status}));this.nextId=Math.max(0,...this.items.map(x=>x.id))+1;}
  add(seconds,label,source=''){
    if(!Number.isInteger(seconds)||seconds<1)return {error:'计时时间需为正整数秒'};
    if(this.items.length>=3)return {error:'已有 3 个计时，请先停止一个'};
    const item={id:this.nextId++,label,source,totalMs:seconds*1000,leftMs:seconds*1000,deadline:this.now()+seconds*1000,status:'running',acknowledged:false};this.items.push(item);return {timer:item};
  }
  remaining(t){return t.status==='running'?Math.max(0,t.deadline-this.now()):Math.max(0,t.leftMs);}
  tick(){let changed=false;for(const t of this.items)if(t.status==='running'&&this.remaining(t)<=0){t.status='done';t.leftMs=0;changed=true;}return changed;}
  toggle(id){const t=this.items.find(t=>t.id===id);if(!t)return;if(t.status==='running'){t.leftMs=this.remaining(t);t.status='paused';}else if(t.status==='paused'){t.deadline=this.now()+t.leftMs;t.status='running';}}
  cancel(id){this.items=this.items.filter(t=>t.id!==id);}
  alarm(){this.tick();return this.items.find(t=>t.status==='done'&&!t.acknowledged);}
  acknowledge(id){const t=this.items.find(t=>t.id===id);if(t)t.acknowledged=true;}
  snapshot(){return this.items.map(t=>({...t,remainingMs:this.remaining(t)}));}
  serialize(){return this.items.map(t=>({...t,leftMs:this.remaining(t)}));}
}
export const timeText = ms => {const s=Math.ceil(Math.max(0,ms)/1000);return `${Math.trunc(s/60).toString().padStart(2,'0')}:${(s%60).toString().padStart(2,'0')}`;};
