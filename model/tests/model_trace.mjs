import {mulberry32, recipeWeight} from './fixtures/prototype-v0.1/model.mjs';
const random = [];
for (const seed of [0, 1, 42, 0x7fffffff, 0xffffffff]) {
  const next = mulberry32(seed);
  for (let i=0; i<100; ++i) random.push({seed, index:i, value:next()});
}
const weights = [];
for (const age of [0,1,2,3,6,7,13,14,15,99999])
  for (let flags=0; flags<16; ++flags) {
    const day='2026-10-28', d=new Date(`${day}T00:00:00Z`);
    d.setUTCDate(d.getUTCDate()-age);
    const past=d.toISOString().slice(0,10), recipe={id:'r',difficulty:flags&8?3:1,
      season:flags&2?[10]:[], tags:[]};
    const context={day,history:age===99999?[]:[{id:'r',day:past}],
      favorites:flags&1?['r']:[],wanted:flags&4?['r']:[],difficulty:2};
    weights.push({age,flags,value:recipeWeight(recipe,context)});
  }
process.stdout.write(JSON.stringify({random,weights}));
