import {readFile,readdir} from 'node:fs/promises';
import {fridgeMatch} from '../../prototype/model.mjs';
import {initialState,addShopping} from '../../prototype/state.mjs';
const dir=new URL('../../data/recipes/curated/',import.meta.url);
const recipes=await Promise.all((await readdir(dir)).filter(f=>f.endsWith('.json')).sort().map(async f=>JSON.parse(await readFile(new URL(f,dir),'utf8'))));
const ingredients=JSON.parse(await readFile(new URL('../../data/ingredients.json',import.meta.url),'utf8')).ingredients.map(x=>x.id).sort();
const mask=xs=>xs.reduce((n,x)=>n|(1n<<BigInt(ingredients.indexOf(x))),0n).toString();
let seed=0x87654321;const fridge=[];
for(let n=0;n<120;n++){
  const have=[],excluded=[];
  for(const id of ingredients){seed=(Math.imul(seed,1664525)+1013904223)>>>0;if((seed>>>28)<n%16)have.push(id);
    seed=(Math.imul(seed,1664525)+1013904223)>>>0;if((seed>>>28)===0&&n%3===0)excluded.push(id);}
  fridge.push({have:mask(have),excluded:mask(excluded),ids:fridgeMatch(recipes,have,excluded).map(x=>recipes.indexOf(x.recipe))});
}
const state=initialState(),shopping=[];
const units=['克','个','根','瓣','块','片','条','只','把','颗','朵','段','斤','适量'];
function snap(op,recipe=0,people=2){shopping.push({op,recipe,people,items:state.shopping.map(x=>({uid:x.uid,qty100:Math.round(x.qty*100),
  id:ingredients.indexOf(x.id),unit:units.indexOf(x.unit),bought:+x.bought}))});}
for(const [i,p] of [[0,2],[0,4],[1,3]]){addShopping(state,recipes[i],p);snap('add',i,p);}
state.shopping[0].bought=true;snap('bought');addShopping(state,recipes[0],2);snap('add',0,2);
process.stdout.write(JSON.stringify({fridge,shopping}));
