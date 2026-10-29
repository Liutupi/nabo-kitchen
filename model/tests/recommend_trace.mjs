import {primaryMethod,recipeWeight,recommend} from './fixtures/prototype-v0.1/model.mjs';
import {testRecipes} from './recipe_fixture.mjs';
const actual=await testRecipes();
const recipe=(id,course,ingredient,method,minutes=10)=>({id,course,minutes,difficulty:1,tags:[],
  ingredients:[{id:ingredient,main:true}],steps:[{method}]});
const fixture=[recipe('a',['meat','veg'],'x','steam'),recipe('b',['meat','veg'],'x','steam'),
  recipe('c',['meat'],'y','stir_fry',40),recipe('d',['veg'],'z','boil'),
  recipe('e',['soup'],'s','boil')];
const cases=[];
function add(recipes,conditions,context,seed,avoid=[]) {
  cases.push({recipes:recipes.map(r=>({id:r.id,course:r.course,minutes:r.minutes,
    ingredients:r.ingredients,method:primaryMethod(r),weight:recipeWeight(r,context)})),
    conditions,excluded:context.excluded||[],seed,avoid,
    expected:recommend(recipes,conditions,context,seed,avoid)});
}
for(const combo of [0,1,2,3]) for(const minutes of [0,10,30,120]) for(const seed of [0,1,41,0xffffffff])
  add(fixture,{combo,minutes},{day:'2026-10-28'},seed, seed===41?['a','b']:[]);
add(fixture,{combo:0,minutes:10},{excluded:['x','y','z','s']},1);
add(fixture.slice(0,2),{combo:2,minutes:30},{},1);
add([fixture[0]],{combo:0,minutes:30},{},1,['a']);
const nonmain={...fixture[0],ingredients:[...fixture[0].ingredients,{id:'scallion',main:false}]};
for(const seed of [1,2,3,4]) add([nonmain,fixture[1]],{combo:0,minutes:30},{excluded:['scallion']},seed);
const repeated={...fixture[0],ingredients:[...fixture[0].ingredients,{id:'x',main:true}]};
add([repeated,fixture[3]],{combo:1,minutes:30},{},2);
for(let i=0;i<96;i++) {
  const day='2026-10-28', selected=actual[i%actual.length];
  const context={day,history:[{id:selected.id,day:'2026-10-27'}],favorites:[actual[(i+1)%actual.length].id],
    wanted:[actual[(i+2)%actual.length].id],difficulty:1+i%3,
    excluded:i%4===0?[selected.ingredients[0].id]:[]};
  const conditions={combo:i%4,minutes:[15,30,60,120][Math.floor(i/4)%4]};
  const seed=(Math.imul(i,0x9e3779b9)+123)>>>0;
  const previous=recommend(actual,conditions,context,seed);
  add(actual,conditions,context,seed,i%2?previous.ids:[]);
}
process.stdout.write(JSON.stringify(cases));
