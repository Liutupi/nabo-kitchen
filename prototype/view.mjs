import {COMBOS,fridgeMatch,cookingOrder,ingredientQty,lastAge,scaleQty} from './model.mjs';
import {session,sortedShopping,stats} from './state.mjs';
import {timeText} from './timers.mjs';
const ART='/game-art/kitchen/';
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
export const image=(path,w,h,cls='')=>`<img class="${cls}" src="${ART+path}.png" width="${w}" height="${h}" alt="">`;
const icon=(id,size=24)=>image('icons/'+id,size,size);
const clockIcon=(size=14)=>`<svg class="program-icon" width="${size}" height="${size}" viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="9" fill="#FBF1E3" stroke="#3B2730" stroke-width="2"/><path d="M12 6v6l4 3" fill="none" stroke="#FFB45E" stroke-width="2" stroke-linecap="round"/></svg>`;
const moonIcon=()=>'<svg class="program-icon" width="14" height="14" viewBox="0 0 24 24" aria-hidden="true"><path d="M15 3C10 4 7 8 7 12c0 5 4 9 9 9 2 0 4-1 5-2-6 0-10-4-10-9 0-3 2-6 4-7Z" fill="#FFB45E"/></svg>';
const bulbIcon=()=>'<svg class="program-icon" width="14" height="14" viewBox="0 0 24 24" aria-hidden="true"><path d="M8 16c0-3-3-3-3-8a7 7 0 0 1 14 0c0 5-3 5-3 8Z" fill="#FFC93F" stroke="#3B2730" stroke-width="2"/><path d="M9 19h6m-5 3h4" stroke="#3B2730" stroke-width="2" stroke-linecap="round"/></svg>';
const at=(x,y,w,h,html,cls='',extra='')=>`<div class="at ${cls}" style="left:${x}px;top:${y}px;width:${w}px;${h?`height:${h}px;`:''}${extra}">${html}</div>`;
const action=(name,content,cls='',index='')=>`<button class="${cls}" data-action="${name}" data-index="${index}">${content}</button>`;
const face=(id='happy',size=40)=>`<span class="face" style="width:${size}px;height:${size}px">${image('icons/face_'+id,size,size)}</span>`;
const portrait=(pose='C')=>`<img class="portrait" src="/prototype/assets/${pose}.png" alt="NABO">`;
const seasoningNames={oil:'食用油',salt:'盐',sugar:'糖',rock_sugar:'冰糖',steam_soy:'蒸鱼豉油',cooking_wine:'料酒',light_soy:'生抽',soy_light:'生抽',dark_soy:'老抽',soy_dark:'老抽',soy_sauce:'生抽',vinegar:'醋',starch:'淀粉',oyster_sauce:'蚝油',oyster:'蚝油',pepper:'胡椒粉',white_pepper:'白胡椒粉',sesame_oil:'香油',water:'水',black_bean:'豆豉',cola:'可乐',dried_chili:'干辣椒',star_anise:'八角'};
const seasoningQty=(r,x,p)=>String(x.qty).replace(/^(\d+(?:\.\d+)?)/,n=>scaleQty(Number(n),r.servings,p));
let dishHeroes=new Set();
export function setDishHeroes(ids){
  if(!Array.isArray(ids)||ids.some(id=>typeof id!=='string'||id.length>40||!/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(id)))throw Error('Invalid dish hero IDs');
  dishHeroes=new Set(ids);
}
export function dish(recipe,w=150,h=112,hidden=false){
  if(!recipe)return '';
  const d=recipe.dish,group=['bowl','claypot'].includes(d.vessel)?d.vessel:'plate';
  const layers=[`vessel_${d.vessel}_shadow`,`vessel_${d.vessel}`,d.sauce&&`sauce_${d.sauce}__${group}`,...d.layers,...(d.garnish||[]).map(x=>'garnish_'+x)].filter(Boolean);
  // These two original recipes used hero=true as a future art request, yet
  // shipped with layered artwork. New hero-required dishes show an honest
  // placeholder until the matching complete illustration is available.
  const legacyLayers=recipe.id==='fanqie-chaodan'||recipe.id==='qingzheng-luyu';
  const pending=d.hero===true&&!legacyLayers;
  const fallback=pending
    ? `<span class="dish-pending">${w>=70?'菜图制作中':'待配图'}</span>`
    : layers.map(id=>image('dish-layers/'+id,w,h)).join('');
  // Keep the appropriate fallback until a complete illustration loads. If a
  // requested hero fails to load, never substitute an incomplete layer image.
  const hero=dishHeroes.has(recipe.id)
    ? `<span>${fallback}</span><img src="${ART}dish-hero/hero_${recipe.id}.png" width="${w}" height="${h}" alt="" onload="this.previousElementSibling.hidden=true" onerror="this.remove()">`
    : fallback;
  return `<div class="dish ${hidden?'unknown':''}" style="width:${w}px;height:${h}px">${hero}${hidden?'<b>?</b>':''}</div>`;
}
const keyRows=(rows,focus=-1,y=216)=>rows.map((r,i)=>at(8,y+i*34,138,31,action(r[2],`<i>${r[0]}</i><span>${r[1]}</span>`,'key '+(i===focus?'selected ':'')+(r[3]?'hot':'')), 'key-wrap')).join('');
export function render(v){
  const {ui:u,state:s,recipes,ingredients,timers,now,day}=v;
  const recipe=recipes.find(r=>r.id===u.recipeId)||recipes[0],byId=Object.fromEntries(ingredients.map(x=>[x.id,x]));
  const st=stats(s,day),left=Math.max(0,s.settings.rerolls-s.daily.rerollsUsed),selected=recipes.filter(r=>u.result.includes(r.id));
  const timed=timers.items.filter(t=>t.status==='running').sort((a,b)=>timers.remaining(a)-timers.remaining(b))[0];
  const alarm=timers.alarm();
  const titles=['NABO 深夜厨房','NABO 深夜厨房','今晚怎么吃','今晚吃点啥','今天就吃它！','准备食材',recipe?.name||'跟做','计时','做好啦！','冰箱里有啥','冰箱推荐','买菜单','我的菜谱','设置'];
  const accents={2:'#FF6B5B',3:'#FF6B5B',4:'#FF6B5B',5:'#FFC93F',6:'#FFC93F',7:'#5DB7F2',8:'#4FC98A',9:'#5DB7F2',10:'#5DB7F2',11:'#4FC98A',12:'#FFC93F',13:'#C5C3D3'};
  const title=u.page===6?recipe.name:titles[u.page];
  let right=timed?action('timers',`${clockIcon()} ${timeText(timers.remaining(timed))}`):'<span class="battery" aria-label="电量">▰ 70%</span>';
  if(u.page===6)right=`${session(s,recipe).index+1} / ${recipe.steps.length}`;
  if(u.page===9)right=`已选 ${s.fridge.length}`;
  if(u.page===12)right=`${st.unique}/${recipes.length}`;
  let body='',footer='双击 ↑ 返回 · 长 ↑ 回开场';
  if(u.page===0){
    const foods=['egg','tomato','choy_sum','fish_whole','tofu','pork_rib'].map((id,i)=>{const a=Math.PI*(1.07+i*.174);return at(Math.round(120+Math.cos(a)*94-14),Math.round(257+Math.sin(a)*84-14),28,28,image('ingredients/'+id,28,28));}).join('');
    body=`<div class="stars">${Array.from({length:24},(_,i)=>`<i style="left:${i*53%238}px;top:${i*37%150}px"></i>`).join('')}</div>${at(10,42,220,90,image('ui/logo_wordmark',220,90))}${at(10,148,26,26,icon('ui_lantern',26))}${at(204,148,26,26,icon('ui_lantern',26))}${at(24,136,192,20,'今晚吃什么，我来想','center dim small')}${foods}${at(74,177,94,143,portrait())}`;
  }else if(u.page===1){
    body=at(0,38,240,70,face(now-u.lastActivity>30000?'sleepy':'happy',58)+`<div class="bubble"><b>${now-u.lastActivity>30000?'Zzz…':'今晚吃点啥？'}</b><small>${s.daily.result?'上次菜单已保留':'随时颠勺选菜'} · 不限次</small></div>`,'welcome');
    body+=at(8,114,224,58,action('home',icon('method_stir_fry',36)+'<span><b>今天吃什么</b><small>颠一下锅，马上决定</small></span>','hero '+(u.focus===0?'selected':''),0));
    [['冰箱','有啥做啥','ui_lantern_dot','#5DB7F2'],['买菜单',`${s.shopping.filter(x=>!x.bought).length} 样待买`,'prop_bowl','#4FC98A'],['我的菜谱',`${st.unique} / ${recipes.length}`,'badge_1','#FFC93F'],['设置','口味 · 声音','ui_timer_icon','#C5C3D3']].forEach((r,i)=>{
      body+=at(8+(i%2)*116,180+Math.trunc(i/2)*50,108,44,action('home',`<img src="${ART+(r[2].startsWith('prop')?'props/':'icons/')+r[2]}.png" width="24" height="24"><span>${r[0]}<small>${r[1]}</small></span>`,'menu-card '+(u.focus===i+1?'selected':''),i+1),'',`--acc:${r[3]}`);
    });footer='↑↓ 选择 · OK 进入 · 长 OK 颠勺';
  }else if(u.page===2){
    const values=[s.conditions.people===5?'5+':s.conditions.people,s.conditions.minutes?s.conditions.minutes+' 分钟':'不限',COMBOS[s.conditions.combo]];
    ['吃几个人','有多少时间','怎么吃'].forEach((name,i)=>body+=at(8,52+i*52,224,44,action('condition',`<span>${name}</span><b>${values[i]}</b>`,'setting-row '+(u.focus===i?'selected':''),i)));
    body+=at(8,224,224,42,action('draw','颠勺！','primary '+(u.focus===3?'selected':'')));footer='↑↓ 选行 · OK 切换 · 颠勺出菜';
  }else if(u.page===3){
    body=at(44,60,152,32,'今晚吃点热乎的','center')+at(44,100,152,192,portrait('K2'));
  }else if(u.page===4){
    if(u.result.length>1){
      body+=at(10,43,186,18,`${COMBOS[s.conditions.combo]} · ${selected.reduce((n,r)=>n+r.minutes,0)} 分钟`,'dim small')+at(200,43,32,18,`${u.comboFocus+1}/${u.result.length}`,'number small');
      const start=Math.min(Math.max(0,u.comboFocus-1),Math.max(0,u.result.length-3));
      u.result.slice(start,start+3).forEach((id,i)=>{const r=recipes.find(r=>r.id===id),active=start+i===u.comboFocus;body+=at(8+i*76,active?66:74,72,104,action('combo',dish(r,72,54)+`<small>${esc(r.name)}</small>`,'combo-card '+(active?'selected':''),start+i));});
      body+=at(10,185,40,40,face('happy'))+at(60,186,170,52,cookingOrder(selected).join(' → '),'bubble small');
      body+=keyRows([['↓','换一组','reroll'],['OK','开始做这道','recipe',true]],-1,246);footer='↑ 选菜 · ↓ 换一组 · OK 开做';
    }else{
      body=at(8,44,224,116,'<i class="glow one"></i><i class="glow two"></i>'+dish(recipe),'stage');
      const age=lastAge(recipe.id,s.history),recent=age<14?(age===0?'上次做过':`${age+1} 次前做过`):'';
      body+=at(12,164,220,26,esc(recipe.name),'dish-name')+at(12,192,220,20,`<span class="chip">${clockIcon()} ${recipe.minutes} 分钟</span><span class="chip">${esc(recent||recipe.tags?.[0]||recipe.cuisine)}</span>`);
      body+=keyRows([['↑','回吧台','lobby'],['↓','换一道 · 不限次','reroll'],['OK','开做！','recipe',true]]);
      body+=at(154,170,84,150,portrait())+at(150,148,82,22,'这道我拿手','bubble tiny');footer='双击 ↑ 返回 · 长 OK 收藏';
    }
    if(u.notes.length)body+=at(8,62,224,32,u.notes.map(esc).join(' · '),'relax-note small');
  }else if(u.page===5){
    body=at(8,44,224,188,recipe.ingredients.map(i=>`<div class="ingredient-line">${image('ingredients/'+i.id,32,32)}<span>${esc(byId[i.id]?.name||i.id)}</span><b>${ingredientQty(recipe,i,s.conditions.people)} ${esc(i.unit)}</b></div>`).join('')+`<div class="seasonings">${recipe.seasonings.map(x=>`${esc(x.name||seasoningNames[x.id]||x.id)} ${esc(seasoningQty(recipe,x,s.conditions.people))}`).join(' · ')}</div>`,'paper scroll');
    body+=at(8,242,224,38,action('start','OK 开始跟做','primary'));footer='↓ 加买菜单 · 长 ↓ 翻页 · OK 跟做';
  }else if(u.page===6){
    const p=session(s,recipe),step=recipe.steps[p.index],active=timers.items.find(t=>t.source===`${recipe.id}:${p.index}`);
    body=at(8,46,224,6,recipe.steps.map((_,i)=>`<i style="background:${i<=p.index?'#FFC93F':'#3a3556'}"></i>`).join(''),'progress');
    body+=at(8,60,224,150,`<div class="step-heading"><b class="step-num">${String(p.index+1).padStart(2,'0')}</b><span class="chip">${icon('heat_'+step.heat,16)}${({high:'大火',mid:'中火',low:'小火',off:'关火'})[step.heat]}</span><span class="chip">${icon('method_'+step.method,16)}${({cut:'切',mix:'拌',stir_fry:'炒',steam:'蒸',boil:'煮',braise:'焖烧',fry:'炸',marinate:'腌',blanch:'焯',stew:'炖'})[step.method]}</span></div><div id="step-text" class="step-text">${esc(step.text)}</div>${step.tip?`<div class="tip">${bulbIcon()} ${esc(step.tip)}</div>`:''}`,'step-card');
    body+=at(8,220,150,56,action('timers',`<span>${clockIcon(24)}</span><div><b class="number">${active?timeText(timers.remaining(active)):step.timer?timeText(step.timer*1000):'准备好'}</b><small>${active?'后台计时中':step.timer?'OK 开始计时':'OK 下一步'}</small></div>`,'timer-capsule'))+at(176,218,56,56,face(step.timer?'focus':p.index===recipe.steps.length-1?'wait':'go',56));
    footer='OK 下一步 · ↑ 上一步 · 长 ↓ 用料';
  }else if(u.page===7){
    const t=timers.items[u.focus%Math.max(1,timers.items.length)];
    body=at(46,46,148,148,`<svg width="148" height="148" viewBox="0 0 148 148"><circle cx="74" cy="74" r="64" fill="none" stroke="#2C2845" stroke-width="14"/><circle cx="74" cy="74" r="64" fill="none" stroke="${t?.status==='done'?'#E65152':'#5DB7F2'}" stroke-width="14" stroke-linecap="round" stroke-dasharray="402" stroke-dashoffset="${t?402-Math.trunc(timers.remaining(t)*402/t.totalMs):402}" transform="rotate(-90 74 74)"/></svg>`);
    body+=at(0,96,240,44,t?timeText(timers.remaining(t)):'00:00','timer-number number')+at(20,140,200,18,t?esc(t.label)+(t.status==='paused'?' · 已暂停':''):'还没有计时器','center dim small');
    body+=timers.items.map((x,i)=>at(8,202+i*27,138,24,action('timer-select',`${i+1} ${esc(x.label)} <b>${timeText(timers.remaining(x))}</b>`,'timer-row '+(x.id===t?.id?'selected':''),i))).join('')+at(154,170,84,150,portrait());
    footer='↑↓ 切换 · 长 OK 暂停 / 继续';
  }else if(u.page===8){
    body=at(40,48,160,120,dish(recipe,160,120))+at(142,44,64,64,icon('ui_stamp_done',64)+'<b>完成</b>','stamp');
    body+=at(12,176,220,26,`${esc(recipe.name)} <small>第 ${s.cooked[recipe.id]?.count||0} 次</small>`,'dish-name')+at(12,208,150,20,`我的菜谱 ${st.unique} / ${recipes.length}`,'dim small')+at(12,238,132,22,`累计做饭 ${st.total} 次`,'chip')+at(154,170,84,150,portrait())+at(110,148,82,26,'吃饭了！','bubble small');footer='OK 回吧台';
  }else if(u.page===9){
    const cats=[['meat','肉'],['seafood','海鲜'],['egg_tofu','蛋豆'],['veg','蔬菜'],['staple','主食']],cat=u.fridgeCategory||0;
    body=at(8,44,224,22,cats.map(([id,name],i)=>action('category',name,'category '+(cat===i?'active ':'')+(!u.fridgeItems&&cat===i?'selected':''),id)).join(''),'categories');
    const category=ingredients.filter(i=>i.cat===cats[cat][0]),start=u.fridgeItems?Math.trunc(u.focus/8)*8:0;
    category.slice(start,start+8).forEach((i,j)=>body+=at(8+j%4*58,74+Math.trunc(j/4)*72,52,66,action('fridge',image('ingredients/'+i.id,40,40)+`<small>${esc(i.name.replace(/（.*?）/g,''))}</small>${s.fridge.includes(i.id)?'<b class="check"><i class="check-mark"></i></b>':''}`,'ingredient-card '+(u.fridgeItems&&start+j===u.focus?'selected ':'')+(s.fridge.includes(i.id)?'checked':''),start+j)));
    const matches=fridgeMatch(recipes,s.fridge,s.settings.excluded);
    body+=at(10,224,40,40,face('focus'))+at(60,226,170,52,`这些能做 <b>${matches.filter(x=>!x.missing.length).length}</b> 道菜<br>长按 OK 看推荐`,'bubble small');footer=u.fridgeItems?'↑↓ 选食材 · OK 勾选 · 双 ↑ 分类':'↑↓ 选分类 · OK 进入 · 双 ↑ 返回';
  }else if(u.page===10){
    const list=fridgeMatch(recipes,s.fridge,s.settings.excluded),start=Math.trunc(u.focus/4)*4;
    body=list.length?list.slice(start,start+4).map((x,i)=>at(8,46+i*57,224,52,action('recommendation',dish(x.recipe,48,36)+`<span>${esc(x.recipe.name)}<small>${x.missing.length?'缺 1 样：'+esc(byId[x.missing[0]]?.name):'齐了'}</small></span>`,'recipe-row '+(start+i===u.focus?'selected':''),start+i))).join(''):at(12,90,216,90,'没有只缺 1 样以内的菜<br>双击 ↑ 补选食材','center');footer='↑↓ 选择 · OK 看菜谱';
  }else if(u.page===11){
    const items=sortedShopping(s),start=Math.trunc(u.focus/5)*5;
    body=at(14,50,212,232,`<i class="tape"></i>${items.slice(start,start+5).map((i,j)=>action('shopping',`<span class="box">${i.bought?'<i class="check-mark"></i>':''}</span>${image('ingredients/'+i.id,30,30)}<span>${esc(byId[i.id]?.name||i.id)}</span><small>${i.qty} ${esc(i.unit)}</small>`,'shop-row '+(start+j===u.focus?'selected ':'')+(i.bought?'bought':''),i.uid)).join('')}${!items.length?'<p>买菜单是空的<br>从用料页加入食材</p>':''}${action('clear-bought','清空已买','clear-bought '+(u.focus===items.length?'selected':''))}`,'shopping-paper');footer='↑↓ 选择 · OK 打勾 / 清空';
  }else if(u.page===12){
    const filters=['全部','做过的','收藏','我存的'],list=recipes.filter(r=>u.bookFilter===0||u.bookFilter===1&&s.cooked[r.id]||u.bookFilter===2&&s.favorites.includes(r.id)||u.bookFilter===3&&r.source==='user');
    body=at(8,44,224,52,icon('badge_'+st.level,40)+`<div><b>${['','夜宵学徒','深夜帮厨','夜班主厨','深夜厨神'][st.level]}</b><small>累计 ${st.total} 次 · 解锁 ${st.unique} 道</small></div>`,'level-card');
    body+=at(8,99,224,22,action('book-filter',`筛选：${filters[u.bookFilter]}`,'filter-row '+(u.focus===0?'selected':'')));
    const start=Math.trunc(Math.max(0,u.focus-1)/6)*6;
    body+=list.slice(start,start+6).map((r,i)=>at(8+i%3*76,125+Math.trunc(i/3)*83,72,80,action('book',dish(r,72,54,!s.cooked[r.id])+`<small>${esc(s.cooked[r.id]?r.name:'还没做过')}</small>${s.favorites.includes(r.id)?`<b class="favorite">${icon('ui_star',12)}</b>`:''}`,'book-card '+(u.focus===start+i+1?'selected':''),start+i))).join('');
    if(!list.length)body+=at(12,158,216,50,'这个分类还没有菜谱','center dim');footer='↑↓ 选择 · OK 打开 · 长 OK 收藏';
  }else if(u.page===13){
    if(u.settingsMode==='exclude'){
      const start=Math.trunc(u.focus/5)*5;
      body=ingredients.slice(start,start+5).map((i,j)=>at(8,46+j*46,224,42,action('exclude',`${image('ingredients/'+i.id,32,32)}<span>${esc(i.name)}</span><b>${s.settings.excluded.includes(i.id)?'不吃':'可吃'}</b>`,'setting-row '+(start+j===u.focus?'selected':''),start+j))).join('');footer='↑↓ 选择 · OK 切换 · 双 ↑ 返回';
    }else{
      const list=[['声音',s.settings.sound?'开':'关'],['音量',s.settings.volume+'%'],['不吃的食材',s.settings.excluded.length+' 样'],['少辣',s.settings.lessSpicy?'开':'关'],['少油',s.settings.lessOil?'开':'关'],['难度',s.settings.difficulty],['清空记录',u.clearStage?`确认 ${u.clearStage}/2`:'需两次确认']];
      const start=Math.trunc(u.focus/5)*5;
      body=list.slice(start,start+5).map((r,i)=>at(8,46+i*46,224,42,action('setting',`<span>${r[0]}</span><b>${r[1]}</b>`,'setting-row '+(start+i===u.focus?'selected':''),start+i))).join('');footer=u.clearStage?'OK 确认 · ↑↓ 取消':'↑↓ 选择 · OK 调整';
    }
  }
  const header=u.page===0?'':`<header>${esc(title)}<span>${right}</span></header><div class="gingham" style="--acc:${accents[u.page]||'#FFB45E'}"></div>`;
  const flash=alarm&&now-alarm.deadline<2100?`<div class="alarm-border" style="animation-delay:-${Math.max(0,now-alarm.deadline)}ms"></div>`:'';
  return `${header}${body}${[0,4].includes(u.page)?'':`<footer>${footer}</footer>`}${alarm?`<button class="alarm" data-action="timers">${clockIcon()} ${esc(alarm.label)} 时间到 · OK 查看</button>${flash}`:''}${u.message?`<div class="toast" role="status">${esc(u.message)}</div>`:''}`;
}
