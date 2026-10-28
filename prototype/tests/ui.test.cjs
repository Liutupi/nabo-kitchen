const test=require('node:test');const assert=require('node:assert/strict');const path=require('node:path');const os=require('node:os');const fs=require('node:fs');const {spawn}=require('node:child_process');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||path.join(os.homedir(),'.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright'));
const root=path.resolve(__dirname,'../..');const proof=path.join(root,'private/prototype-qa');fs.mkdirSync(proof,{recursive:true});
let child,browser,context,page,base;const errors=[],requests=[];
test.before(async()=>{
  child=spawn(process.execPath,[path.join(root,'prototype/server.mjs'),'0'],{cwd:root,stdio:['ignore','pipe','pipe'],windowsHide:true});
  base=await new Promise((resolve,reject)=>{const timeout=setTimeout(()=>reject(Error('server start timeout')),10000);child.stdout.on('data',d=>{const m=String(d).match(/http:\/\/127\.0\.0\.1:\d+/);if(m){clearTimeout(timeout);resolve(m[0]);}});child.on('error',reject);});
  const edge=process.env.EDGE_PATH||'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe';
  browser=await chromium.launch({headless:true,...(fs.existsSync(edge)?{executablePath:edge}:{})});
  context=await browser.newContext({viewport:{width:900,height:740},reducedMotion:'reduce'});page=await context.newPage();
  page.on('pageerror',e=>errors.push(String(e)));page.on('response',r=>{if(r.status()>=400&&!r.url().endsWith('favicon.ico'))requests.push([r.status(),r.url()]);});
  await page.goto(base+'/?test=1');await page.waitForFunction(()=>window.kitchenTest?.ready);
});
test.after(async()=>{await browser?.close();child?.kill();});
const call=(name,...args)=>page.evaluate(([name,args])=>window.kitchenTest[name](...args),[name,args]);
const snap=()=>call('snapshot');
test('all fourteen pages render locally with real images, fonts, no runtime errors',async()=>{
  await call('reset');
  for(let i=0;i<=13;i++){
    await call('visit',i,'fanqie-chaodan');
    await page.evaluate(async()=>{await document.fonts.ready;await Promise.all([...document.querySelectorAll('#screen img')].map(i=>i.decode()));});
    assert.equal(await page.locator('#screen').getAttribute('data-page'),'P'+i);
    const metrics=await page.locator('#screen').evaluate(el=>({width:el.clientWidth,height:el.clientHeight,bad:[...el.querySelectorAll('img')].some(i=>!i.naturalWidth),font:document.fonts.check('500 18px KitchenHan')}));
    assert.deepEqual(metrics,{width:240,height:320,bad:false,font:true});
    await page.locator('#screen').screenshot({path:path.join(proof,`P${i}.png`)});
  }
  assert.deepEqual(errors,[]);assert.deepEqual(requests,[]);
});
test('actual keyboard gestures: double UP is atomic; long UP leaves but timers keep going',async()=>{
  await call('reset');await page.keyboard.press('Enter');assert.equal((await snap()).ui.page,2);
  await page.keyboard.press('ArrowUp');await page.waitForTimeout(90);await page.keyboard.press('ArrowUp');await page.waitForTimeout(350);assert.equal((await snap()).ui.page,1);
  await call('setTimer',30,'test');await page.keyboard.down('ArrowUp');await page.waitForTimeout(720);await page.keyboard.up('ArrowUp');
  assert.equal((await snap()).ui.outside,true);await call('advanceTime',10000);assert.ok((await snap()).timers[0].remainingMs<21000);
  await page.locator('#reopen').click();assert.equal((await snap()).ui.outside,false);
});
test('short OK starts step timer then advances, step progress persists after return/reload',async()=>{
  await call('reset');await call('visit',6,'fanqie-chaodan');
  await call('dispatch','OK');assert.equal((await snap()).state.progress['fanqie-chaodan'].index,1);
  await call('dispatch','OK');await call('dispatch','OK');assert.equal((await snap()).state.progress['fanqie-chaodan'].index,3);
  await call('dispatch','OK');let s=await snap();assert.equal(s.state.progress['fanqie-chaodan'].index,3);assert.equal(s.timers.length,1);
  await call('dispatch','OK');assert.equal((await snap()).state.progress['fanqie-chaodan'].index,4);
  await call('dispatch','UP','double');await page.reload();await page.waitForFunction(()=>window.kitchenTest?.ready);
  assert.equal((await snap()).state.progress['fanqie-chaodan'].index,4);assert.equal((await snap()).timers.length,1);
  await call('visit',6,'fanqie-chaodan');await call('dispatch','OK');assert.equal((await snap()).ui.page,8);assert.equal((await snap()).state.cooked['fanqie-chaodan'].count,1);
  await page.locator('#screen').screenshot({path:path.join(proof,'P8-complete.png')});
});
test('timer limit, long OK pause/resume and top alarm route across any page',async()=>{
  await call('reset');for(let i=0;i<3;i++)assert.ok((await call('setTimer',5+i,'timer'+i)).timer);
  assert.ok((await call('setTimer',30,'fourth')).error);await call('visit',7);await call('dispatch','OK','long');assert.equal((await snap()).timers[0].status,'paused');
  await page.locator('#screen').screenshot({path:path.join(proof,'P7-three-timers.png')});
  await call('dispatch','OK','long');assert.equal((await snap()).timers[0].status,'running');await call('visit',9);await call('advanceTime',8000);
  assert.equal(await page.locator('.alarm').count(),1);await call('dispatch','OK');assert.equal((await snap()).ui.page,7);await call('dispatch','OK');assert.equal((await snap()).timers.length,2);
});
test('fridge toggles, shopping merges, favorites and two clear confirmations persist',async()=>{
  await call('reset',{history:[{id:'fanqie-chaodan',day:'2026-10-28'}],cooked:{'fanqie-chaodan':{count:1,lastDay:'2026-10-28'}}});await call('visit',9);await call('dispatch','OK');await call('dispatch','OK','long');assert.equal((await snap()).ui.page,10);
  await call('visit',5,'fanqie-chaodan');await call('dispatch','DOWN');await call('dispatch','DOWN');let s=await snap();assert.equal(s.state.shopping.find(x=>x.id==='egg').qty,6);
  await call('visit',12);await call('dispatch','DOWN');await call('dispatch','OK','long');assert.equal((await snap()).state.favorites.length,1);
  await call('visit',13);for(let i=0;i<7;i++)await call('dispatch','DOWN');await call('dispatch','OK');await call('dispatch','OK');assert.equal((await snap()).state.history.length,1);
  await call('dispatch','OK');assert.equal((await snap()).state.history.length,0);assert.equal((await snap()).state.favorites.length,1);
  await page.reload();await page.waitForFunction(()=>window.kitchenTest?.ready);assert.equal((await snap()).state.shopping.find(x=>x.id==='egg').qty,6);
});
test('day cache survives reopening and reroll allowance cannot be bypassed by changing conditions',async()=>{
  await call('reset');await call('dispatch','OK','long');await page.waitForTimeout(60);const first=await snap();assert.equal(first.ui.page,4);
  await call('visit',1);await call('dispatch','OK','long');assert.deepEqual((await snap()).ui.result,first.ui.result);assert.equal((await snap()).state.daily.rerollsUsed,0);
  for(let i=0;i<3;i++){await call('dispatch','DOWN');await page.waitForTimeout(50);}
  assert.equal((await snap()).state.daily.rerollsUsed,3);await call('visit',2);await page.locator('[data-action=condition][data-index="0"]').click();await page.locator('[data-action=draw]').click();assert.equal((await snap()).state.daily.rerollsUsed,3);
  assert.ok((await snap()).ui.message.includes('用完'));
});
test('four-dish result keeps all four unique recipes and scrolls a three-card viewport',async()=>{
  const data=await (await fetch(base+'/api/recipes')).json();assert.ok(data.length>=4,'approved30 menu data must be loaded');
  const ids=data.slice(0,4).map(x=>x.id);await call('reset',{conditions:{combo:3}});await call('showResult',ids);
  assert.equal(await page.locator('.combo-card').count(),3);assert.deepEqual((await snap()).ui.result,ids);
  await call('dispatch','UP');assert.equal((await snap()).ui.comboFocus,3);assert.equal((await snap()).ui.recipeId,ids[3]);
  assert.equal(await page.locator('.combo-card').count(),3);assert.ok((await page.locator('#screen').innerText()).includes(data[3].name));
  await page.locator('#screen').screenshot({path:path.join(proof,'four-dish.png')});
  assert.equal(await page.locator('#screen footer').count(),0);
  const box=await page.locator('[data-action=recipe]').boundingBox();const screen=await page.locator('#screen').boundingBox();assert.ok(box.y+box.height<=screen.y+320);
});
test('alarms route from cover and outside kitchen; canceled single UP is never lost to another key',async()=>{
  await call('reset');await call('setTimer',1,'cover alarm');await call('visit',0);await call('advanceTime',1100);await call('dispatch','OK');assert.equal((await snap()).ui.page,7);
  await call('reset');await call('setTimer',1,'outside alarm');await call('dispatch','UP','long');await call('advanceTime',1100);await call('dispatch','OK');assert.equal((await snap()).ui.page,7);assert.equal((await snap()).ui.outside,false);
  await call('reset');await page.keyboard.press('ArrowUp');await page.keyboard.press('ArrowDown');await page.waitForTimeout(350);assert.equal((await snap()).ui.focus,0);
});
test('touch physical controls work at mobile size, with silent media and native screen pixels',async()=>{
  const mobile=await browser.newContext({viewport:{width:390,height:800},hasTouch:true,reducedMotion:'reduce'}),p=await mobile.newPage();
  await p.goto(base+'/?test=1');await p.waitForFunction(()=>window.kitchenTest?.ready);await p.evaluate(()=>window.kitchenTest.reset());
  const down=await p.locator('[data-key=DOWN]').boundingBox();await p.touchscreen.tap(down.x+down.width/2,down.y+down.height/2);
  assert.equal((await p.evaluate(()=>window.kitchenTest.snapshot())).ui.focus,1);
  const ok=await p.locator('[data-key=OK]').boundingBox();await p.touchscreen.tap(ok.x+ok.width/2,ok.y+ok.height/2);
  assert.equal((await p.evaluate(()=>window.kitchenTest.snapshot())).ui.page,9);
  const screen=await p.locator('#screen').boundingBox();assert.equal(screen.width,240);assert.equal(screen.height,320);
  assert.ok(await p.evaluate(()=>[...document.querySelectorAll('audio,video')].every(x=>x.paused||x.muted)));
  await p.locator('.workbench').screenshot({path:path.join(proof,'mobile.png')});await mobile.close();
});
test('all thirty recipes and every instruction can render without missing delivered art',async()=>{
  const data=await (await fetch(base+'/api/recipes')).json();
  for(const r of data){
    await call('reset');await call('showResult',[r.id]);
    await page.evaluate(async()=>Promise.all([...document.querySelectorAll('#screen img')].map(i=>i.decode())));
    for(let index=0;index<r.steps.length;index++){
      await call('reset',{progress:{[r.id]:{index,visited:[],timerSteps:[]}}});await call('visit',6,r.id);
      await page.evaluate(async()=>Promise.all([...document.querySelectorAll('#screen img')].map(i=>i.decode())));
      // Query and measure in one browser task; the one-second clock redraw may
      // otherwise detach a Playwright locator's previous element handle.
      const dimensions=await page.evaluate(()=>{const e=document.querySelector('#step-text');return {text:e.textContent,width:e.clientWidth,height:e.clientHeight,font:getComputedStyle(e).fontSize};});
      assert.equal(dimensions.text,r.steps[index].text,`${r.id}:${index}`);
      assert.equal(dimensions.width,202,`${r.id}:${index}`);assert.ok(dimensions.height<=100);assert.equal(dimensions.font,'18px');
    }
  }
  assert.deepEqual(errors,[]);assert.deepEqual(requests,[]);
});
