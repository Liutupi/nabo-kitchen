// Only explicitly registered, present hero PNGs are candidates for the prototype.
const recipeId = /^[a-z0-9]+(?:-[a-z0-9]+)*$/;

export function heroRecipeIds(csv, filenames) {
  const files = new Set(filenames);
  const ids = new Set();
  for (const line of csv.split(/\r?\n/)) {
    // The first three ASSET_LIST fields cannot contain commas: IDs and safe
    // relative paths have fixed names. Later quoted note fields are irrelevant.
    const [id, category, path] = line.split(',', 3);
    if (category !== 'hero' || !id?.startsWith('hero_')) continue;
    const recipe = id.slice(5);
    if (recipe.length > 40 || !recipeId.test(recipe)) continue;
    if (path === `dish-hero/${id}.png` && files.has(`${id}.png`)) ids.add(recipe);
  }
  return [...ids].sort();
}
