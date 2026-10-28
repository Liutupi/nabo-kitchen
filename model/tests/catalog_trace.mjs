import {readFile,readdir} from 'node:fs/promises';
import {primaryMethod} from '../../prototype/model.mjs';
const dir=new URL('../../data/recipes/curated/',import.meta.url);
const list=await Promise.all((await readdir(dir)).filter(f=>f.endsWith('.json')).sort().map(async f=>JSON.parse(await readFile(new URL(f,dir),'utf8'))));
process.stdout.write(JSON.stringify(list.map(r=>({id:r.id,name:r.name,primary:primaryMethod(r)}))));
