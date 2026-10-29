"""Assemble only an art resource candidate; never write to firmware or a device."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

BATCH = Path(__file__).resolve().parent
ROOT = BATCH.parents[4]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--firmware-root', required=True, type=Path)
    parser.add_argument('--packed', type=Path, default=ROOT / 'build/kitchen-art-review')
    parser.add_argument('--out', type=Path, default=BATCH / 'resource-candidate')
    args = parser.parse_args()
    runtime, packed, out = args.firmware_root.resolve(), args.packed.resolve(), args.out.resolve()
    # This command may only write inside its own review batch.
    assert out.is_relative_to(BATCH.resolve()) and out != BATCH.resolve()
    queue = json.loads((BATCH / 'manifest.json').read_text(encoding='utf-8'))
    assert all(item['status'] == 'review' for item in queue['items'])
    report = json.loads((packed / 'static_report.json').read_text(encoding='utf-8'))
    assert report['roundtrip'] and report['packages']['kitchen_dish.bin']['assets'] == 226
    assert report['asset_csv_sha256'] == hashlib.sha256((ROOT / 'game-art/kitchen/ASSET_LIST.csv').read_bytes()).hexdigest(), 'Repack after CSV edits or line-ending conversion'
    out.mkdir(parents=True, exist_ok=True)
    for name in ('kitchen_recipes.krc', 'kitchen_fonts.bin', 'kitchen_icons.bin', 'kitchen_ui.bin', 'kitchen_nabo.bin', 'NOTICE.md', 'NOTICE.zh_CN.md'):
        shutil.copyfile(runtime / 'assets/kitchen' / name, out / name)
    shutil.copytree(runtime / 'assets/kitchen/licenses', out / 'licenses', dirs_exist_ok=True)
    shutil.copyfile(packed / 'kitchen_dish.bin', out / 'kitchen_dish.bin')
    for name in ('kitchen_icons.bin', 'kitchen_ui.bin'):
        assert (out / name).read_bytes() == (packed / name).read_bytes(), name
    subprocess.run([sys.executable, str(runtime / 'tools/pack_kitchen_assets.py'), '--asset-dir', str(out), '--output', str(out / 'kitchen_bundle.bin')], check=True)
    shutil.copyfile(packed / 'static_report.json', BATCH / 'static_report.json')
    print('Candidate assembled; still requires verify_runtime.py and physical-device acceptance.')


if __name__ == '__main__':
    main()
