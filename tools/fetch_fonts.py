"""Download checksum-pinned font build inputs into ignored local cache."""
import argparse
import hashlib
import json
from pathlib import Path
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent.parent


def fetch(root=ROOT, include_standard=False):
    root = Path(root)
    manifest = json.loads((root / 'tools/fonts/sources.json').read_text(encoding='utf-8'))
    cache = root / 'private/font-source'
    cache.mkdir(parents=True, exist_ok=True)
    for item in manifest['records']:
        name = item['name']
        if name == 'standard-gf0023-2020.pdf' and not include_standard:
            continue
        if Path(name).name != name:
            raise ValueError('invalid source filename')
        target = cache / name
        if target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == item['sha256']:
            print('verified cache: ' + name)
            continue
        request = Request(item['url'], headers={'User-Agent': 'nabo-kitchen-font-build/1'})
        with urlopen(request, timeout=90) as response:
            data = response.read(item['bytes'] + 1)
        if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest() != item['sha256']:
            raise ValueError('download size/checksum mismatch: ' + name)
        temp = target.with_suffix(target.suffix + '.part')
        temp.write_bytes(data)
        temp.replace(target)
        print('downloaded and verified: ' + name)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--include-standard', action='store_true', help='also fetch the 51 MB provenance PDF')
    args = parser.parse_args()
    fetch(args.root, args.include_standard)
