import {test} from 'node:test';
import assert from 'node:assert/strict';
import {heroRecipeIds} from '../dish-heroes.mjs';
import {dish,setDishHeroes} from '../view.mjs';

test('only registered and present hero PNGs are advertised',()=>{
  const csv=['id,category,path',
    'hero_egg-tomato,hero,dish-hero/hero_egg-tomato.png,160x120,640x480',
    'hero_missing,hero,dish-hero/hero_missing.png,160x120,640x480',
    'hero_bad,hero,../hero_bad.png,160x120,640x480',
    'hero_other,dish-L2,dish-hero/hero_other.png,160x120,640x480'].join('\n');
  assert.deepEqual(heroRecipeIds(csv,['hero_egg-tomato.png','hero_bad.png','hero_other.png']),['egg-tomato']);
});

test('hero replaces layered dish on load, absent or failed hero keeps fallback',()=>{
  const recipe={id:'egg-tomato',dish:{vessel:'plate',sauce:'tomato',layers:['egg__scrambled'],garnish:[]}};
  setDishHeroes([]);
  assert.match(dish(recipe),/dish-layers\/egg__scrambled/);
  assert.doesNotMatch(dish(recipe),/dish-hero/);
  setDishHeroes(['egg-tomato']);
  const markup=dish(recipe);
  assert.match(markup,/dish-hero\/hero_egg-tomato\.png/);
  assert.match(markup,/dish-layers\/egg__scrambled/);
  assert.match(markup,/onload="this\.previousElementSibling\.hidden=true"/);
  assert.match(markup,/onerror="this\.remove\(\)"/);
  assert.throws(()=>setDishHeroes(['../escape']));
  setDishHeroes([]);
});

test('new hero-required dishes use a placeholder while legacy hero requests keep layers',()=>{
  const recipe={id:'baicai-huajia-tang',dish:{vessel:'bowl',sauce:'clear_soup',layers:['napa_cabbage__boiled'],hero:true}};
  setDishHeroes([]);
  assert.match(dish(recipe),/菜图制作中/);
  assert.doesNotMatch(dish(recipe),/dish-layers/);
  assert.match(dish(recipe,48,36),/待配图/);
  for(const id of ['fanqie-chaodan','qingzheng-luyu']){
    const legacy={...recipe,id};
    assert.match(dish(legacy),/dish-layers\/napa_cabbage__boiled/);
    assert.doesNotMatch(dish(legacy),/菜图制作中/);
  }
  setDishHeroes(['baicai-huajia-tang']);
  const markup=dish(recipe);
  assert.match(markup,/dish-hero\/hero_baicai-huajia-tang\.png/);
  assert.match(markup,/菜图制作中/);
  assert.doesNotMatch(markup,/dish-layers/);
  assert.match(markup,/onerror="this\.remove\(\)"/);
  setDishHeroes([]);
});
