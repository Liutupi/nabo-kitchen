"""Pack/unpack/check KRC1; defaults to every current curated JSON, never candidates."""
import argparse
import hashlib
import json
from pathlib import Path

from check_recipes import ROOT, load_recipes
from recipe_codec import canonical, pack_recipes, unpack_recipes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    pack = sub.add_parser('pack')
    pack.add_argument('--root', type=Path, default=ROOT)
    pack.add_argument('--recipes', type=Path)
    pack.add_argument('--out', type=Path, default=ROOT / 'build/kitchen/kitchen_recipes.krc')
    pack.add_argument('--font-chars', type=Path, help='UTF-8 repertoire; reject any missing recipe glyph')
    unpack = sub.add_parser('unpack')
    unpack.add_argument('input', type=Path)
    unpack.add_argument('--out', type=Path, required=True)
    check = sub.add_parser('check')
    check.add_argument('input', type=Path)
    args = parser.parse_args()
    try:
        if args.command == 'pack':
            recipes = load_recipes(args.root, args.recipes)
            used = set(''.join(canonical(r).decode('utf-8') for r in recipes))
            missing = sorted(used - set(args.font_chars.read_text(encoding='utf-8'))) if args.font_chars else []
            if missing:
                raise ValueError('font repertoire missing: ' + ''.join(missing))
            blob = pack_recipes(recipes, args.root)
            decoded, info = unpack_recipes(blob)
            if decoded != sorted(recipes, key=lambda x: x['id']):
                raise ValueError('recipe roundtrip differs')
            args.out.parent.mkdir(parents=True, exist_ok=True)
            args.out.write_bytes(blob)
            args.out.with_suffix('.chars.txt').write_text(''.join(sorted(used)), encoding='utf-8')
            report = {'format': 'KRC1', 'packages': {args.out.name: {'bytes': len(blob), 'budget_bytes': 150000,
                       'within_budget': len(blob) <= 150000, 'recipes': len(recipes), 'sha256': hashlib.sha256(blob).hexdigest()}},
                      'roundtrip': True,
                      'source_content_sha256': {r['id']: hashlib.sha256(canonical(r)).hexdigest() for r in recipes},
                      'source_schema_sha256': hashlib.sha256((args.root / 'data/schema/recipe.schema.json').read_bytes()).hexdigest(),
                      'font_coverage': 'checked' if args.font_chars else 'pending font subset repertoire',
                      **info}
            args.out.with_suffix('.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
            print(json.dumps({k: v for k, v in report.items() if k in ['packages', 'roundtrip', 'font_coverage']}, ensure_ascii=False))
        else:
            recipes, info = unpack_recipes(args.input.read_bytes())
            if args.command == 'unpack':
                args.out.mkdir(parents=True, exist_ok=True)
                for recipe in recipes:
                    (args.out / (recipe['id'] + '.json')).write_text(json.dumps(recipe, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
            print(f'KRC1 verified: {len(recipes)} recipes, 16-byte index, heatshrink8/4, max decoded {max(r["decoded_bytes"] for r in info["records"])} bytes')
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
