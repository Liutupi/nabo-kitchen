"""Recheck retained device dish pixels against the unchanged production C code.

Requires a native CC compiler, CJSON_DIR, Pillow and the firmware checkout.
Reads the public capture evidence only; never connects to or writes a device.
"""
import argparse
import ctypes as ct
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

from PIL import Image

BATCH = Path(__file__).resolve().parent
SOURCE = 'c7cdc0b94a6009672333dc1736b4f1c33f0a0537'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--firmware-root', required=True, type=Path)
    args = parser.parse_args()
    firmware = args.firmware_root.resolve()
    subprocess.run(['git', '-C', str(firmware), 'diff', '--exit-code', SOURCE, '--',
                    'main/kitchen', 'tests/kitchen_view_host.c', 'tests/test_real_catalog_views.py'], check=True)
    spec = importlib.util.spec_from_file_location('device_capture_host_views', firmware / 'tests/test_real_catalog_views.py')
    views = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(views)
    views.ASSETS = BATCH / 'resource-candidate'
    metadata = json.loads((BATCH / 'device-captures/capture-validation.json').read_text(encoding='utf-8'))
    cls = views.RealCatalogViews
    cls.setUpClass()
    total = 0
    try:
        for item in metadata['checks']:
            path = BATCH / 'device-captures' / item['capture']
            assert path.resolve().is_relative_to((BATCH / 'device-captures').resolve())
            assert hashlib.sha256(path.read_bytes()).hexdigest() == item['png_sha256']
            with Image.open(path) as image:
                assert image.size == (240, 320) and image.mode == 'RGB'
                actual = image.copy()
            assert item['dish_rectangle_xywh'] == [45, 46, 150, 112]
            assert cls.dll.view_page(4, cls.ids.index(item['recipe_id'])) == 1
            pixels = (ct.c_uint16 * (240 * 320))()
            assert cls.dll.view_draw(pixels, 0, 320) == 1
            mismatches = 0
            for y in range(46, 158):
                for x in range(45, 195):
                    r, g, b = actual.getpixel((x, y))
                    value = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                    mismatches += value != pixels[y * 240 + x]
            assert mismatches == 0, (item['recipe_id'], mismatches)
            total += 16800
            print(item['recipe_id'] + ': PASS, 16800 RGB565 pixels')
    finally:
        cls.tearDownClass()
    print('PASS: %d retained device captures; %d pixels; zero mismatches' % (len(metadata['checks']), total))


if __name__ == '__main__':
    main()
