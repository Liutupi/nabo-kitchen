const test=require('node:test');
const assert=require('node:assert/strict');
const path=require('node:path');
const os=require('node:os');
const fs=require('node:fs');
const {spawn}=require('node:child_process');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||path.join(os.homedir(),'.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright'));
const root=path.resolve(__dirname,'../..');
let child,browser,page,base;
test.before(async()=>{
  child=spawn(process.execPath,[path.join(root,'prototype/server.mjs'),'0'],{cwd:root,stdio:['ignore','pipe','pipe']});
  base=await new Promise((resolve,reject)=>{
    const timeout=setTimeout(()=>reject(Error('server start timeout')),10000);
    child.stdout.on('data',d=>{const m=String(d).match(/http:\/\/127\.0\.0\.1:\d+/);if(m){clearTimeout(timeout);resolve(m[0]);}});
    child.on('error',reject);
  });
  const edge=process.env.EDGE_PATH||'/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge';
  browser=await chromium.launch({headless:true,...(fs.existsSync(edge)?{executablePath:edge}:{})});
  page=await browser.newPage();
  await page.goto(base+'/?test=1');
  await page.waitForFunction(()=>window.kitchenTest?.ready);
});
test.after(async()=>{await browser?.close();child?.kill();});

test('registered hero is visible; recipes without a hero keep their original layers',async()=>{
  await page.evaluate(()=>window.kitchenTest.showResult(['huanggua-chaodan']));
  const hero=page.locator('#screen .dish img[src*="dish-hero/hero_huanggua-chaodan.png"]');
  await hero.waitFor({state:'visible'});
  await page.waitForFunction(()=>{
    const img=document.querySelector('#screen .dish img[src*="dish-hero/"]');
    return img?.naturalWidth>0 && img.previousElementSibling?.hidden;
  });
  assert.equal(await hero.evaluate(img=>img.previousElementSibling.hidden),true);
  await page.evaluate(()=>window.kitchenTest.showResult(['fanqie-chaodan']));
  assert.equal(await page.locator('#screen .dish img[src*="dish-hero/"]').count(),0);
  assert.ok(await page.locator('#screen .dish img[src*="dish-layers/"]').count()>0);
});

test('a hero that fails to load leaves the fallback layers visible',async()=>{
  await page.route('**/dish-hero/hero_huanggua-chaodan.png',route=>route.abort());
  await page.reload();
  await page.waitForFunction(()=>window.kitchenTest?.ready);
  await page.evaluate(()=>window.kitchenTest.showResult(['huanggua-chaodan']));
  await page.waitForFunction(()=>{
    const dish=document.querySelector('#screen .dish');
    return dish && !dish.querySelector('img[src*="dish-hero/"]');
  });
  const fallback=page.locator('#screen .dish span').first();
  assert.equal(await fallback.evaluate(el=>el.hidden),false);
  assert.ok(await fallback.locator('img').count()>0);
});

test('new required art prefers its image and missing art shows a placeholder',async()=>{
  await page.unrouteAll();
  await page.reload();
  await page.waitForFunction(()=>window.kitchenTest?.ready);
  await page.evaluate(()=>window.kitchenTest.showResult(['fanqie-huajia-tang']));
  const hero=page.locator('#screen .dish img[src*="dish-hero/hero_fanqie-huajia-tang.png"]');
  await hero.waitFor({state:'visible'});
  await page.waitForFunction(()=>{
    const img=document.querySelector('#screen .dish img[src*="dish-hero/"]');
    return img?.naturalWidth>0 && img.previousElementSibling?.hidden;
  });
  await page.evaluate(()=>window.kitchenTest.showResult(['baicai-huajia-tang']));
  const missing=page.locator('#screen .dish .dish-pending');
  assert.equal(await missing.innerText(),'菜图制作中');
  assert.equal(await page.locator('#screen .dish img[src*="dish-layers/"]').count(),0);
  for(const id of ['fanqie-chaodan','qingzheng-luyu']){
    await page.evaluate(id=>window.kitchenTest.showResult([id]),id);
    assert.equal(await page.locator('#screen .dish .dish-pending').count(),0);
    assert.ok(await page.locator('#screen .dish img[src*="dish-layers/"]').count()>0);
  }
});

test('a required hero that fails to load reveals its placeholder',async()=>{
  await page.route('**/dish-hero/hero_fanqie-huajia-tang.png',route=>route.abort());
  await page.reload();
  await page.waitForFunction(()=>window.kitchenTest?.ready);
  await page.evaluate(()=>window.kitchenTest.showResult(['fanqie-huajia-tang']));
  await page.waitForFunction(()=>!document.querySelector('#screen .dish img[src*="dish-hero/"]'));
  const pending=page.locator('#screen .dish .dish-pending');
  assert.equal(await pending.innerText(),'菜图制作中');
  assert.equal(await page.locator('#screen .dish img[src*="dish-layers/"]').count(),0);
});
