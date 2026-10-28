"""Verify a frozen checkpoint, optionally compare its recorded source tree."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def content(path, record):
    data = path.read_bytes()
    if record.get('normalization') == 'utf8-lf':
        data = data.decode('utf-8').replace('\r\n', '\n').encode('utf-8')
    return data


def checked_path(base, name):
    base = base.resolve()
    target = (base / name).resolve()
    if not target.is_relative_to(base):
        raise ValueError('manifest path escapes root: ' + name)
    return target


def verify(checkpoint, source_root=None):
    checkpoint = Path(checkpoint)
    manifest = json.loads((checkpoint / 'manifest.json').read_text(encoding='utf-8'))
    groups = [(checkpoint, manifest['files'])]
    if source_root is not None:
        groups.append((Path(source_root), manifest['source_files']))
    count = 0
    for base, files in groups:
        for name, record in files.items():
            path = checked_path(base, name)
            if not path.is_file():
                raise ValueError('missing/changed file: ' + str(path))
            data = content(path, record)
            if len(data) != record['bytes'] or hashlib.sha256(data).hexdigest() != record['sha256']:
                raise ValueError('missing/changed file: ' + str(path))
            count += 1
    return count


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint', nargs='?', type=Path, default=ROOT / 'checkpoints/2026-10-28')
    parser.add_argument('--sources', action='store_true', help='also check current repository sources against frozen inputs')
    args = parser.parse_args()
    try:
        print('Verified SHA-256 and sizes for', verify(args.checkpoint, ROOT if args.sources else None), 'files')
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + '\n')
