export const DAY_MS = 86400000;
export const COMBOS = ['一道菜', '一荤一素', '一荤一素一汤', '两荤一素一汤'];
export const ROLES = [[], ['meat', 'veg'], ['meat', 'veg', 'soup'], ['meat', 'meat', 'veg', 'soup']];
export const dayNumber = day => Math.trunc(Date.parse(`${day}T00:00:00Z`) / DAY_MS);
export const localDay = (date = new Date()) => `${date.getFullYear()}-${String(date.getMonth()+1).padStart(2,'0')}-${String(date.getDate()).padStart(2,'0')}`;
export function hash32(text) { let h=2166136261; for(const c of text){h=Math.imul(h^c.charCodeAt(0),16777619);} return h>>>0; }
export function mulberry32(seed) {
  let a=seed>>>0;
  return () => {a=(a+0x6D2B79F5)>>>0; let t=Math.imul(a^(a>>>15),a|1);t^=t+Math.imul(t^(t>>>7),t|61);return (t^(t>>>14))>>>0;};
}
export const dailySeed = (day,device='browser-01') => hash32(`${day}|${device}`);
export const drawSeed = (counter,device='browser-01') => (hash32(device)+Math.imul(counter>>>0,0x9E3779B9))>>>0;
export function lastAge(id,history){for(let i=history.length-1;i>=0;i--)if(history[i].id===id)return history.length-1-i;return 99999;}
export function recipeWeight(recipe,context={}) {
  const {history=[],favorites=[],wanted=[],difficulty=2}=context;
  const age=lastAge(recipe.id,history);let w=100;
  w=Math.trunc(w*(age<3?10:age<7?40:age<14?80:100)/100);
  if(favorites.includes(recipe.id))w=Math.trunc(w*150/100);
  // Seasonal weighting is disabled without a trusted date.
  if(wanted.includes(recipe.id)||recipe.tags?.includes('想试试'))w=Math.trunc(w*120/100);
  if(recipe.difficulty>difficulty)w=Math.trunc(w*50/100);
  return Math.max(1,w);
}
export const mainIds = r => r.ingredients.filter(i=>i.main).map(i=>i.id);
export function primaryMethod(recipe){
  const counts={};for(const s of recipe.steps)if(!['cut','mix','marinate','blanch'].includes(s.method))counts[s.method]=(counts[s.method]||0)+1;
  return Object.keys(counts).sort((a,b)=>counts[b]-counts[a]||a.localeCompare(b))[0]||'mix';
}
export function fridgeMatch(recipes,have,excluded=[]) {
  if(!have.length)return [];
  return recipes.filter(r=>!r.ingredients.some(i=>excluded.includes(i.id))).map(recipe=>{
    const required=mainIds(recipe),missing=required.filter(id=>!have.includes(id));
    return {recipe,missing,score:required.length?Math.trunc((required.length-missing.length)*1000/required.length):1000};
  }).filter(x=>x.missing.length<=1).sort((a,b)=>b.score-a.score||a.recipe.id.localeCompare(b.recipe.id)).slice(0,10);
}
function rolePools(pool,roles,seed,bounded){return roles.map(role=>{const all=pool.filter(r=>r.course.includes(role));if(!bounded)return all;const random=mulberry32((seed^Math.imul(0x9E3779B9,({meat:1,veg:2,soup:4}[role]+1)))>>>0),reservoir=[];all.forEach((r,i)=>{const slot=i<12?i:random()%(i+1);if(slot<12)reservoir[slot]=r;});return pool.filter(r=>reservoir.includes(r));});}
function assignments(pools,index=0,chosen=[]){if(index===pools.length)return [chosen];return pools[index].filter(r=>!chosen.some(x=>x.id===r.id)).flatMap(r=>assignments(pools,index+1,[...chosen,r]));}
export function recommend(recipes,conditions,context={},seed=1,avoid=[]) {
  const {minutes=30,combo=0}=conditions,{excluded=[]}=context;
  const hardPool=recipes.filter(r=>!r.ingredients.some(i=>excluded.includes(i.id)));
  if(!hardPool.length)return {ids:[],notes:[],reason:'没有符合忌口的菜谱'};
  const roles=ROLES[combo]||[],groups=combo?assignments(rolePools(hardPool,roles,seed,recipes.length>64)):hardPool.map(r=>[r]);
  if(!groups.length)return {ids:[],notes:[],reason:'现有菜谱无法组成这个搭配，请调整条件'};
  // Relax only the stated soft constraint at each tier. Exclusions and distinct
  // recipe IDs remain hard. Tier order is a prototype implementation choice.
  // Product decision 2026-10-28: recency affects weight only, never eligibility.
  const relax=[[],['main'],['main','method'],['main','method','time']];
  const labels={main:'已放宽：主料重复',method:'已放宽：做法相同',time:'已放宽：总用时'};
  for(const tier of relax){
    const seen=new Set();let candidates=groups.filter(g=>{
      const key=g.map(r=>r.id).sort().join('|');if(seen.has(key))return false;seen.add(key);
      if(!tier.includes('time')&&minutes&&g.reduce((s,r)=>s+r.minutes,0)>minutes)return false;
      if(g.length>1&&!tier.includes('main')){const m=g.flatMap(mainIds);if(new Set(m).size!==m.length)return false;}
      if(g.length>1&&!tier.includes('method')&&new Set(g.map(primaryMethod)).size===1)return false;
      return true;
    });
    if(!candidates.length)continue;
    const different=candidates.filter(g=>g.map(r=>r.id).sort().join('|')!==[...avoid].sort().join('|'));
    if(different.length)candidates=different;
    const weights=candidates.map(g=>g.reduce((n,r)=>n+recipeWeight(r,context),0));
    const sum=weights.reduce((a,b)=>a+b,0);let pick=mulberry32(seed)()%sum,index=0;
    while(pick>=weights[index])pick-=weights[index++];
    const selected=candidates[index],main=selected.flatMap(mainIds);
    const actual=tier.filter(k=>k==='main'?new Set(main).size!==main.length:k==='method'?selected.length>1&&new Set(selected.map(primaryMethod)).size===1:minutes&&selected.reduce((s,r)=>s+r.minutes,0)>minutes);
    return {ids:selected.map(r=>r.id),notes:actual.map(k=>labels[k]),seed:seed>>>0,reason:''};
  }
  return {ids:[],notes:[],reason:'没有满足条件的菜谱'};
}
export function scaleQty(qty,servings,people){return typeof qty==='number'?Math.trunc((qty*people*100+Math.trunc(servings/2))/servings)/100:qty;}
export function ingredientQty(recipe,item,people){return scaleQty(item.qty,recipe.servings,people);}
export function levelFor(unique,total){if(unique>=100&&total>=200)return 4;if(unique>=40||total>=60)return 3;if(unique>=10)return 2;return 1;}
export function cookingOrder(recipes){return [...recipes].sort((a,b)=>Number(primaryMethod(b)==='steam')-Number(primaryMethod(a)==='steam')||b.minutes-a.minutes).map(r=>r.name);}
