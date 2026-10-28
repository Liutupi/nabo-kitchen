"""Pack all CSV static assets to KSP1 pools and verify lossless indexed roundtrips."""
import argparse
import hashlib
import json
import re
from pathlib import Path

import _bootstrap  # noqa: F401
from PIL import Image, ImageDraw, ImageFont
from asset_codec import index_image, pack_sprites, prepare_image, unpack_sprites
from check_recipes import ROOT, asset_rows, dish_asset_ids, load_recipes

BUDGETS = {'dish': 200000, 'icons': 120000, 'ui': 40000}
SEAFOOD_EXTRA = frozenset(('crab', 'scallop', 'oyster_meat', 'mussel',
                           'razor_clam', 'abalone', 'octopus', 'salmon'))


def profile(row):
    ident, category = row['id'], row['category']
    # A full dish illustration deliberately uses a separate category from
    # dish-L*: recipe_codec includes every `dish-` asset in the KRC dictionary.
    # Adding an optional hero must not renumber existing recipe layer IDs.
    if category == 'hero':
        if len(ident) > 45 or not re.fullmatch(r'hero_[a-z0-9]+(?:-[a-z0-9]+)*', ident):
            raise ValueError(f'invalid hero asset ID: {ident}')
        if row['path'] != f'dish-hero/{ident}.png':
            raise ValueError(f'invalid hero asset path: {ident}')
        # A dedicated palette avoids forcing unrelated dishes into 64 colours.
        return 'dish', ident, (160, 120), False, False
    if category.startswith('dish-'):
        return 'dish', category, (160, 120), False, False
    if category in ('costume', 'style', 'grok-start'):
        size = (150, 150) if category == 'costume' else (240, 320) if category == 'style' else (192, 192)
        return 'audit', category, size, True, category != 'style'
    if ident == 'logo_wordmark':
        return 'ui', 'startup', (220, 90), False, False
    if ident == 'ui_lantern':
        return 'ui', 'startup', (26, 26), False, False
    if category == 'ingredient' and ident in SEAFOOD_EXTRA:
        # Keep the original ingredient palette unchanged when adding seafood.
        return 'icons', 'seafood-' + ident, (40, 40), False, False
    if category in ('ingredient', 'face'):
        return 'icons', category, (40, 40), category == 'face', False
    if category == 'prop':
        return 'icons', 'prop', (32, 32), False, False
    dimensions = {'ui_lantern_dot': (12, 16), 'ui_timer_icon': (16, 16),
                  'ui_star': (12, 12), 'ui_stamp_done': (56, 56)}
    if ident in dimensions:
        return 'icons', 'ui-glyph', dimensions[ident], False, False
    if ident.startswith('badge_'):
        return 'icons', 'badge', (40, 40), False, False
    if ident in ('ui_fridge', 'ui_shopping', 'ui_recipe_book', 'ui_settings'):
        return 'icons', 'menu-icon', (40, 40), False, False
    if ident.startswith(('heat_', 'method_')):
        return 'icons', 'method-heat', (24, 24), False, False
    raise ValueError(f'no explicit packing profile for {ident}')


def build(root=ROOT, out=None, decode_pngs=True):
    root = Path(root)
    out = Path(out) if out else root / 'build/kitchen'
    out.mkdir(parents=True, exist_ok=True)
    pools, sources, coverage = {}, {}, []
    for row in asset_rows(root).values():
        if not row['path'].endswith('.png'):
            continue
        source = root / 'game-art/kitchen' / row['path']
        if not source.is_file():
            raise ValueError(f"missing source PNG: {row['id']}")
        source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
        pool, group, size, character, key = profile(row)
        prepared = prepare_image(source, size, character, key)
        pools.setdefault(pool, []).append({'id': row['id'], 'group': group, 'prepared': prepared})
        with Image.open(source) as image:
            source_size = list(image.size)
            if row['category'] == 'hero' and image.size != (640, 480):
                raise ValueError(f"{row['id']}: hero source must be 640x480")
            expected_source = re.fullmatch(r'(\d+)x(\d+)', row['source'])
            if expected_source and image.size != tuple(map(int, expected_source.groups())):
                raise ValueError(f"{row['id']}: source dimensions differ from ASSET_LIST.csv")
            if pool != 'audit' and ('A' not in image.getbands() or image.getchannel('A').getextrema()[0] != 0):
                raise ValueError(f"{row['id']}: runtime source needs real transparent alpha")
            if row['category'] == 'hero' and image.getchannel('A').getextrema()[1] == 0:
                raise ValueError(f"{row['id']}: hero source is fully transparent")
        sources[row['id']] = {'path': row['path'], 'source_size': source_size,
                              'display_frame': list(size), 'pool': pool, 'palette_group': group,
                              'source_sha256': source_hash, **prepared.stats}
        coverage.append(row['id'])
    packages, decoded = {}, {}
    for pool, entries in sorted(pools.items()):
        blob = pack_sprites(entries)
        pack = unpack_sprites(blob)
        name = 'kitchen_' + pool + '.bin' if pool != 'audit' else 'review_static.ksp'
        (out / name).write_bytes(blob)
        for entry in entries:
            ident = entry['id']
            expected = index_image(entry['prepared'], pack.groups[entry['group']])
            if expected != pack.assets[ident]['indices']:
                raise ValueError(f'{ident}: full-frame crop/index roundtrip mismatch')
            sources[ident]['crop_xywh'] = list(pack.assets[ident]['crop'])
            sources[ident]['payload_bytes'] = pack.assets[ident]['encoded_bytes']
            if decode_pngs:
                folder = out / 'decoded' / pool
                folder.mkdir(parents=True, exist_ok=True)
                pack.image(ident).save(folder / (ident + '.png'))
        budget = BUDGETS.get(pool)
        packages[name] = {'bytes': len(blob), 'budget_bytes': budget,
                          'within_budget': len(blob) <= budget if budget else None,
                          'runtime': pool != 'audit', 'assets': len(entries),
                          'palette_entries': {g: len(p) for g, p in pack.groups.items()},
                          'sha256': hashlib.sha256(blob).hexdigest()}
        decoded[pool] = pack
    for ident, item in sources.items():
        source = root / 'game-art/kitchen' / item['path']
        if hashlib.sha256(source.read_bytes()).hexdigest() != item['source_sha256']:
            raise ValueError(f'{ident}: source changed while packing; rebuild from stable inputs')
    report = {'format': 'KSP1', 'static_csv_coverage': len(coverage),
              'asset_csv_sha256': hashlib.sha256((root / 'game-art/kitchen/ASSET_LIST.csv').read_bytes()).hexdigest(),
              'source_originals_modified': False, 'roundtrip': True, 'packages': packages,
              'runtime_static_bytes': sum(p['bytes'] for p in packages.values() if p['runtime']),
              'audit_note': 'costume A/B/C, style board and K1/K2 are review previews; no fabricated animation frames',
              'other_budget_inputs': {'kitchen_nabo.bin': 'separate animation pipeline',
                                      'fonts': 'separate subset pipeline',
                                      'hero_art': 'optional registered hero sprites share the dish pool; absent heroes use layers'},
              'firmware_total_complete': False, 'assets': sources}
    (out / 'static_report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    return report, decoded


def composite_recipes(pack, recipes, out):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    font_path = ROOT / 'game-art/kitchen/reference/ui-mockup/NotoSansSC-Medium.ttf'
    try:
        font = ImageFont.truetype(str(font_path), 14)
    except OSError:
        try:
            font = ImageFont.truetype('C:/Windows/Fonts/msyh.ttc', 14)
        except OSError:
            font = ImageFont.load_default()
    cols, row_height = 3, 170
    rows = (len(recipes) + cols - 1) // cols
    boards = {bg: Image.new('RGB', (660, rows * row_height), bg) for bg in ('#2C2845', '#FBF1E3')}
    records = []
    for number, recipe in enumerate(recipes):
        image = Image.new('RGBA', (160, 120))
        hero = 'hero_' + recipe['id']
        layers = [hero] if hero in pack.assets else dish_asset_ids(recipe['dish'])
        for ident in layers:
            if ident not in pack.assets or pack.assets[ident]['frame'] != (160, 120):
                raise ValueError(f'missing/invalid packed recipe layer: {ident}')
            image.alpha_composite(pack.image(ident))
        image.save(out / (recipe['id'] + '.png'))
        x, y = (number % cols) * 220, (number // cols) * row_height
        for bg, board in boards.items():
            board.paste(image, (x + 30, y + 10), image)
            ImageDraw.Draw(board).text((x + 15, y + 135), recipe['name'], font=font,
                                      fill='#FBF1E3' if bg == '#2C2845' else '#15122A')
        records.append({'id': recipe['id'], 'layers': layers, 'frame': [160, 120],
                        'decoded_rgba_sha256': hashlib.sha256(image.tobytes()).hexdigest()})
    for bg, board in boards.items():
        board.save(out / ('contact_dark.png' if bg == '#2C2845' else 'contact_cream.png'))
    (out / 'composites.json').write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding='utf-8')
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    pack = sub.add_parser('pack')
    pack.add_argument('--root', type=Path, default=ROOT)
    pack.add_argument('--out', type=Path, default=ROOT / 'build/kitchen')
    pack.add_argument('--strict-budget', action='store_true')
    for command in ('check', 'unpack'):
        item = sub.add_parser(command)
        item.add_argument('input', type=Path)
        if command == 'unpack':
            item.add_argument('--out', type=Path, required=True)
    composite = sub.add_parser('composite')
    composite.add_argument('--input', type=Path, default=ROOT / 'build/kitchen/kitchen_dish.bin')
    composite.add_argument('--recipes', type=Path)
    composite.add_argument('--root', type=Path, default=ROOT)
    composite.add_argument('--out', type=Path, default=ROOT / 'build/kitchen/proofs/recipes')
    args = parser.parse_args()
    try:
        if args.command == 'pack':
            report, decoded = build(args.root, args.out)
            print(json.dumps({k: report[k] for k in ('static_csv_coverage', 'roundtrip', 'packages')}, ensure_ascii=False))
            if args.strict_budget and any(p['within_budget'] is False for p in report['packages'].values()):
                parser.exit(2, 'one or more static pools exceed the documented budget\n')
        else:
            decoded = unpack_sprites(args.input.read_bytes())
            if args.command == 'unpack':
                args.out.mkdir(parents=True, exist_ok=True)
                for ident in decoded.assets:
                    decoded.image(ident).save(args.out / (ident + '.png'))
            elif args.command == 'composite':
                records = composite_recipes(decoded, load_recipes(args.root, args.recipes), args.out)
                print(f'Composed {len(records)} recipes from decoded KSP1 indices at160x120')
            else:
                print(f'KSP1 verified: {len(decoded.assets)} assets, {len(decoded.groups)} palettes, max64 incl transparent0')
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
