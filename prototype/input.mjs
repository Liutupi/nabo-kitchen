export const LONG_MS=650,DOUBLE_MS=280;
export function bindControls(dispatch,now=()=>Date.now()){
  const held=new Map();let upPending=null;
  const clearUp=()=>{if(upPending){clearTimeout(upPending.timer);upPending=null;}};
  const flushUp=()=>{if(upPending){clearUp();dispatch('UP','short');}};
  function press(key){if(held.has(key))return;if(key!=='UP')flushUp();const item={at:now(),long:false};item.timer=setTimeout(()=>{item.long=true;clearUp();dispatch(key,'long');},LONG_MS);held.set(key,item);}
  function release(key,cancel=false){const item=held.get(key);if(!item)return;clearTimeout(item.timer);held.delete(key);if(cancel||item.long)return;
    if(key==='UP'){
      if(upPending&&now()-upPending.at<=DOUBLE_MS){clearUp();dispatch('UP','double');}
      else{clearUp();upPending={at:now(),timer:setTimeout(()=>{upPending=null;dispatch('UP','short');},DOUBLE_MS)};}
    }else{clearUp();dispatch(key,'short');}
  }
  const map={ArrowUp:'UP',ArrowDown:'DOWN',Enter:'OK',' ':'OK'};
  window.addEventListener('keydown',e=>{if(map[e.key]&&!e.repeat&&!/INPUT|SELECT|TEXTAREA/.test(e.target.tagName)){e.preventDefault();press(map[e.key]);}});
  window.addEventListener('keyup',e=>{if(map[e.key]){e.preventDefault();release(map[e.key]);}});
  window.addEventListener('blur',()=>{for(const key of held.keys())release(key,true);clearUp();});
  for(const button of document.querySelectorAll('[data-key]')){
    button.addEventListener('pointerdown',e=>{e.preventDefault();button.setPointerCapture(e.pointerId);button.classList.add('pressed');press(button.dataset.key);});
    button.addEventListener('pointerup',()=>{button.classList.remove('pressed');release(button.dataset.key);});
    button.addEventListener('pointercancel',()=>{button.classList.remove('pressed');release(button.dataset.key,true);});
  }
  return {press,release};
}
