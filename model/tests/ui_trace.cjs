const {spawn}=require('node:child_process');
const path=require('node:path');const os=require('node:os');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||path.join(os.homedir(),'.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright'));
const root=path.resolve(__dirname,'../..');
(async()=>{
  const server=spawn(process.execPath,[path.join(root,'prototype/server.mjs'),'0'],{cwd:root,stdio:['ignore','pipe','pipe']});
  let browser;
  try{
    const base=await new Promise((resolve,reject)=>{let timeout=setTimeout(()=>reject(Error('server timeout')),10000);
      server.stdout.on('data',d=>{const match=String(d).match(/http:\/\/127\.0\.0\.1:\d+/);if(match){clearTimeout(timeout);resolve(match[0]);}});server.on('error',reject);});
    browser=await chromium.launch({headless:true,executablePath:process.env.EDGE_PATH||'/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge'});
    const page=await browser.newPage();await page.goto(base+'/?test=1');await page.waitForFunction(()=>window.kitchenTest?.ready);
    const ids=(await (await fetch(base+'/api/recipes')).json()).map(x=>x.id);
    if(process.env.KITCHEN_TEST_RECIPE_IDS && JSON.stringify(ids)!==JSON.stringify(JSON.parse(process.env.KITCHEN_TEST_RECIPE_IDS)))throw Error('Browser recipe set differs from KRC');
    const ingredients=(await (await fetch(base+'/data/ingredients.json')).json()).ingredients.map(x=>x.id).sort();
    const mask=(values,table)=>values.reduce((n,x)=>n|(1n<<BigInt(table.indexOf(x))),0n).toString();
    await page.evaluate(()=>window.kitchenTest.reset());
    let now=Date.UTC(2026,9,28,12);const trace=[];
    async function snapshot(op,key='',kind='short'){
      const a=await page.evaluate(()=>window.kitchenTest.snapshot());const p=a.state.progress[a.ui.recipeId]||{index:0,visited:[],completed:false};
      const notes=a.ui.notes.reduce((n,x)=>n|({'已放宽：主料重复':1,'已放宽：做法相同':2,'已放宽：总用时':4}[x]||0),0);
      trace.push({op,key,kind,now,expected:{page:a.ui.page,focus:a.ui.focus,recipe:ids.indexOf(a.ui.recipeId),
        result:a.ui.result.map(x=>ids.indexOf(x)),notes,result_focus:a.ui.comboFocus,rerolls:a.state.daily.rerollsUsed,
        index:p.index,visited:p.visited.reduce((n,i)=>n|(1<<i),0),completed:+p.completed,timers:a.timers.length,outside:+a.ui.outside,
        fridge:mask(a.state.fridge,ingredients),favorites:mask(a.state.favorites,ids),excluded:mask(a.state.settings.excluded,ingredients),
        shopping_count:a.state.shopping.length,settings_mode:+(a.ui.settingsMode==='exclude'),book_filter:a.ui.bookFilter}});
    }
    async function key(k,kind='short',transition=false){
      await page.evaluate(([k,kind])=>window.kitchenTest.dispatch(k,kind),[k,kind]);
      if(transition){now+=1600;await page.evaluate(()=>window.kitchenTest.advanceTime(1600));await page.waitForTimeout(40);}
      await snapshot(transition?'key_transition':'key',k,kind);
    }
    await snapshot('init');
    await key('OK');await key('DOWN');await key('DOWN');await key('DOWN');await key('OK','short',true);
    await key('DOWN','short',true);await key('OK');await key('DOWN');await key('OK');await key('OK');await key('OK');
    await key('UP');await key('OK');await key('UP','double');await key('UP','long');await key('DOWN');
    await key('DOWN');await key('OK');await key('OK');await key('DOWN');await key('OK');await key('OK','long');
    await key('UP','double');await key('UP','double');await key('DOWN');await key('OK');await key('OK');
    await key('UP','double');await key('DOWN');await key('OK');await key('OK');await key('UP','double');await key('DOWN');await key('OK');
    await key('DOWN');await key('DOWN');await key('DOWN');await key('OK');await key('OK');await key('UP','double');
    process.stdout.write(JSON.stringify(trace));
  }finally{if(browser)await browser.close();server.kill();}
})().catch(e=>{console.error(e);process.exit(1);});
