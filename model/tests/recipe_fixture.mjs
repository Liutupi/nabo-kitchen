import {readFile} from 'node:fs/promises';

// Browser-parity tests must use exactly the IDs encoded in the tested package.
// With no override, the independently frozen 30-dish KRC is the baseline.
export async function testRecipes() {
  let ids;
  if (process.env.KITCHEN_TEST_RECIPE_IDS) {
    ids=JSON.parse(process.env.KITCHEN_TEST_RECIPE_IDS);
  } else {
    const data=await readFile(new URL('../../checkpoints/2026-10-28/packages/kitchen_recipes.krc',import.meta.url));
    const offset=data.readUInt32LE(20), length=data.readUInt32LE(24);
    ids=JSON.parse(data.subarray(offset,offset+length).toString('utf8')).ids;
    if (ids.length!==30) throw Error('Frozen recipe fixture must contain exactly 30 recipes');
  }
  if (!Array.isArray(ids) || !ids.length || new Set(ids).size!==ids.length ||
      ids.some(id=>typeof id!=='string' || !/^[a-z0-9-]+$/.test(id)))
    throw Error('Invalid test recipe IDs');
  const dir=new URL('../../data/recipes/curated/',import.meta.url);
  return Promise.all(ids.map(async id=>{
    const recipe=JSON.parse(await readFile(new URL(`${id}.json`,dir),'utf8'));
    if (recipe.id!==id) throw Error(`Recipe ID mismatch: ${id}`);
    return recipe;
  }));
}

export async function testIngredients() {
  let ids;
  if (process.env.KITCHEN_TEST_INGREDIENT_IDS) {
    ids=JSON.parse(process.env.KITCHEN_TEST_INGREDIENT_IDS);
  } else {
    const data=await readFile(new URL('../../checkpoints/2026-10-28/packages/kitchen_recipes.krc',import.meta.url));
    const offset=data.readUInt32LE(20), length=data.readUInt32LE(24);
    ids=JSON.parse(data.subarray(offset,offset+length).toString('utf8')).ingredients;
    if (ids.length!==50) throw Error('Frozen ingredient fixture must contain exactly 50 ingredients');
  }
  if (!Array.isArray(ids) || !ids.length || new Set(ids).size!==ids.length ||
      ids.some(id=>typeof id!=='string')) throw Error('Invalid test ingredient IDs');
  const source=JSON.parse(await readFile(new URL('../../data/ingredients.json',import.meta.url),'utf8'));
  const wanted=new Set(ids), ingredients=source.ingredients.filter(item=>wanted.has(item.id));
  if (ingredients.length!==ids.length || new Set(ingredients.map(item=>item.id)).size!==ids.length)
    throw Error('Ingredient fixture does not match package dictionary');
  if (JSON.stringify(ingredients.map(item=>item.id).sort())!==JSON.stringify(ids))
    throw Error('Ingredient ordinals differ from package dictionary');
  return {...source,ingredients};
}
