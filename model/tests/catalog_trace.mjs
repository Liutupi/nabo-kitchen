import {primaryMethod} from './fixtures/prototype-v0.1/model.mjs';
import {testRecipes} from './recipe_fixture.mjs';
const list=await testRecipes();
process.stdout.write(JSON.stringify(list.map(r=>({id:r.id,name:r.name,primary:primaryMethod(r)}))));
