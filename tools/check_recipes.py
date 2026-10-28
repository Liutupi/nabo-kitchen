"""Schema, foreign-key and real image checks. Safe to import from packers/tests."""
import argparse
import csv
import json
from pathlib import Path

import _bootstrap
from jsonschema import Draft202012Validator

ROOT = _bootstrap.ROOT


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def asset_rows(root=ROOT):
    path = Path(root) / 'game-art/kitchen/ASSET_LIST.csv'
    with path.open(encoding='utf-8-sig', newline='') as stream:
        rows = list(csv.DictReader(stream))
    ids = [r['id'] for r in rows]
    if len(set(ids)) != len(ids):
        raise ValueError('duplicate asset ID in ASSET_LIST.csv')
    base = (Path(root) / 'game-art/kitchen').resolve()
    for row in rows:
        relative = Path(row['path'])
        target = (base / relative).resolve()
        if relative.is_absolute() or not target.is_relative_to(base) or relative.stem != row['id']:
            raise ValueError(f"unsafe path or ID/filename mismatch: {row['id']}")
    return {r['id']: r for r in rows}


def dish_asset_ids(dish):
    vessel = dish['vessel']
    group = 'plate' if vessel in ('plate', 'oval', 'deep') else vessel
    ids = [f'vessel_{vessel}_shadow', f'vessel_{vessel}']
    if dish['sauce'] is not None:
        ids.append(f"sauce_{dish['sauce']}__{group}")
    ids += dish['layers']
    ids += [f'garnish_{name}' for name in dish.get('garnish', [])]
    return ids


def validate_recipe(recipe, root=ROOT, require_images=True):
    root = Path(root)
    schema = read_json(root / 'data/schema/recipe.schema.json')
    errors = [f"schema {'.'.join(map(str, e.absolute_path))}: {e.message}"
              for e in Draft202012Validator(schema).iter_errors(recipe)]
    if errors:
        return errors
    ingredients = {x['id'] for x in read_json(root / 'data/ingredients.json')['ingredients']}
    seasonings = {x['id'] for x in read_json(root / 'data/seasonings.json')['seasonings']}
    assets = asset_rows(root)
    errors += [f"unknown ingredient: {x['id']}" for x in recipe['ingredients']
               if x['id'] not in ingredients]
    errors += [f"unknown seasoning: {x['id']}" for x in recipe['seasonings']
               if x['id'] not in seasonings]
    if not any(x.get('main') for x in recipe['ingredients']):
        errors.append('at least one main ingredient is required')
    expected = {x: 'dish-L2' for x in recipe['dish']['layers']}
    for ident in dish_asset_ids(recipe['dish']):
        category = expected.get(ident, 'dish-L0' if ident.startswith('vessel_')
                                else 'dish-L1' if ident.startswith('sauce_') else 'dish-L3')
        row = assets.get(ident)
        if row is None or row['category'] != category:
            errors.append(f'missing/wrong-category dish asset: {ident}')
        elif require_images and not (root / 'game-art/kitchen' / row['path']).is_file():
            errors.append(f'missing dish PNG: {ident}')
    return errors


def load_recipes(root=ROOT, directory=None, require_images=True):
    root = Path(root)
    directory = Path(directory) if directory else root / 'data/recipes/curated'
    recipes, errors, seen = [], [], set()
    for path in sorted(directory.rglob('*.json')):
        try:
            recipe = read_json(path)
            failures = validate_recipe(recipe, root, require_images)
            if path.stem != recipe.get('id'):
                failures.append('filename must equal recipe ID')
            if recipe.get('id') in seen:
                failures.append('duplicate recipe ID')
            seen.add(recipe.get('id'))
            errors.extend(f'{path.name}: {e}' for e in failures)
            if not failures:
                recipes.append(recipe)
        except (ValueError, OSError, TypeError) as error:
            errors.append(f'{path.name}: {error}')
    if not recipes and not errors:
        errors.append('no recipe JSON files found')
    if errors:
        raise ValueError('\n'.join(errors))
    return recipes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--recipes', type=Path)
    args = parser.parse_args()
    try:
        recipes = load_recipes(args.root, args.recipes or args.root / 'data/recipes')
    except ValueError as error:
        parser.exit(1, str(error) + '\n')
    print(f'Validated {len(recipes)} recipes: schema, ingredient/seasoning and all dish references/PNGs')


if __name__ == '__main__':
    main()
