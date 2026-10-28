"""Build the deterministic, bounds-checked KAB1 offline kitchen asset bundle."""
import argparse
import struct
import zlib
from pathlib import Path

FILES = ('kitchen_recipes.krc', 'kitchen_fonts.bin', 'kitchen_dish.bin',
         'kitchen_icons.bin', 'kitchen_ui.bin', 'kitchen_nabo.bin')
HEADER = struct.Struct('<4sHHII')
ENTRY = struct.Struct('<IIII')


def build(folder: Path) -> bytes:
    data = [ (folder/name).read_bytes() for name in FILES ]
    if any(not blob for blob in data):
        raise ValueError('Kitchen resource files must not be empty')
    table_size = ENTRY.size * len(FILES)
    cursor = HEADER.size + table_size
    chunks, entries = [], []
    for ident, blob in enumerate(data, 1):
        padding = (-cursor) % 4
        if padding:
            chunks.append(bytes(padding))
            cursor += padding
        entries.append(ENTRY.pack(ident, cursor, len(blob), zlib.crc32(blob)))
        chunks.append(blob)
        cursor += len(blob)
    body = b''.join(entries + chunks)
    return HEADER.pack(b'KAB1', 1, len(FILES), HEADER.size+len(body), zlib.crc32(body)) + body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-dir', type=Path, default=Path('assets/kitchen'))
    parser.add_argument('--output', type=Path, default=Path('assets/kitchen/kitchen_bundle.bin'))
    args = parser.parse_args()
    bundle = build(args.asset_dir)
    if len(bundle) > 0x500000:
        raise SystemExit('ERROR: bundle exceeds the 5 MiB kitchen asset partition')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(bundle)
    print(f'Packed {len(FILES)} resources: {len(bundle)} bytes; assets partition remaining {0x500000-len(bundle)} bytes')


if __name__ == '__main__':
    main()
