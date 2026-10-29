// Reproducible expected vectors from the actual JavaScript prototype.
import {readFile,readdir} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
import * as m from '../model.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const files=(await readdir(path.join(root,'data/recipes/curated'))).filter(f=>f.endsWith('.json')).sort();
const recipes=await Promise.all(files.map(f=>readFile(path.join(root,'data/recipes/curated',f),'utf8').then(JSON.parse)));
const history=Array.from({length:20},(_,i)=>({id:recipes[i%5].id}));
const context={history,favorites:[recipes[4].id],wanted:[recipes[8].id],difficulty:2};
const weights=[];
for(const age of [0,1,2,3,6,7,13,14,15,99999])for(let flags=0;flags<16;flags++){
 const r={id:'r',difficulty:flags&8?3:1,season:flags&2?[10]:[],tags:[]};
 const h=age===99999?[]:[{id:'r'},...Array.from({length:age},()=>({id:'x'}))];
 weights.push({age,flags,value:m.recipeWeight(r,{history:h,favorites:flags&1?['r']:[],wanted:flags&4?['r']:[],difficulty:2})});
}
const draws=[];
for(const combo of [0,1,2,3])for(const seed of [0,1,42,321,0xffffffff]){
 const conditions={combo,minutes:60},result=m.recommend(recipes,conditions,context,seed);
 const reroll=m.recommend(recipes,conditions,context,seed,result.ids);
 draws.push({combo,seed,minutes:60,result,reroll});
}
const source_sha256={};
for(const name of ['prototype/model.mjs','prototype/state.mjs'])source_sha256[name]=createHash('sha256').update((await readFile(path.join(root,name),'utf8')).replaceAll('\r\n','\n')).digest('hex');
const input=recipes.map(r=>({id:r.id,ingredients:r.ingredients.map(x=>x.id),main:m.mainIds(r),method:m.primaryMethod(r),minutes:r.minutes,weight:m.recipeWeight(r,context),courses:r.course}));
process.stdout.write(JSON.stringify({schema:'kitchen-v02-parity',source_sha256,weights,recipes:input,draws},null,2)+'\n');
