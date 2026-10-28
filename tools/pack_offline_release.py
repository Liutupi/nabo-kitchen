#!/usr/bin/env python3
"""Create a checked, segmented offline-kitchen package; never access a device."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True
from archive_firmware import APP, FULL_BIN, parse_flash_args, safe_path, unique_json_object, verify_archive
from verify_firmware import FLASH_SIZE, parse_partition_table

SECTOR = 0x1000
PROTECTED = ((0x9000, 0x10000, "nvs-and-phy"), (0x700000, 0x800000, "files"))
EXPECTED_PARTITIONS = {
    "nvs": (1, 2, 0x9000, 0x6000),
    "phy_init": (1, 1, 0xF000, 0x1000),
    "factory": (0, 0, 0x10000, 0x1F0000),
    "kitchen_assets": (1, 0x82, 0x200000, 0x500000),
    "files": (1, 0x82, 0x700000, 0x100000),
}
SPECS = (
    ("bootloader/bootloader.bin", "bootloader.bin", 0, 0x8000),
    ("partition_table/partition-table.bin", "partition-table.bin", 0x8000, 0x9000),
    (f"{APP}.bin", "app.bin", 0x10000, 0x200000),
    ("kitchen_bundle.bin", "kitchen_bundle.bin", 0x200000, 0x700000),
)


def checked_segments(archive: Path, bundle_path: Path) -> tuple[dict, list[tuple[str, bytes, int]]]:
    archive = safe_path(archive)
    bundle_path = safe_path(bundle_path)
    meta = verify_archive(archive)
    args = parse_flash_args((archive / "flash_args").read_bytes())
    if args != {source: start for source, _, start, _ in SPECS}:
        raise ValueError("flash_args images or offsets differ from the approved offline layout")
    table = (archive / "partition_table/partition-table.bin").read_bytes()
    partitions, md5 = parse_partition_table(table)
    layout = {p.label: (p.kind, p.subtype, p.offset, p.size) for p in partitions}
    if not md5 or len(layout) != len(partitions) or layout != EXPECTED_PARTITIONS:
        raise ValueError("partition table differs from the offline layout")
    if not bundle_path.is_file():
        raise ValueError("kitchen bundle does not exist")
    merged = (archive / FULL_BIN).read_bytes()
    images = []
    ranges = []
    for source, name, start, limit in SPECS:
        data = bundle_path.read_bytes() if source == "kitchen_bundle.bin" else (archive / source).read_bytes()
        if not data or start % SECTOR:
            raise ValueError(f"invalid image or unaligned offset: {name}")
        end = start + len(data)
        erase_end = (end + SECTOR - 1) // SECTOR * SECTOR
        if erase_end > limit or erase_end > FLASH_SIZE:
            raise ValueError(f"image exceeds its allowed flash range: {name}")
        if merged[start:end] != data:
            raise ValueError(f"{name} differs from archived merged image")
        if any(start < protected_end and erase_end > protected_start for protected_start, protected_end, _ in PROTECTED):
            raise ValueError(f"{name} would erase protected storage")
        if any(start < previous_end and erase_end > previous_start for previous_start, previous_end in ranges):
            raise ValueError(f"{name} overlaps another image's erase range")
        ranges.append((start, erase_end))
        images.append((name, data, start))
    return meta, images


def build_manifest(meta: dict, images: list[tuple[str, bytes, int]]) -> dict:
    return {
        "schema_version": 1,
        "target": "esp32c3",
        "flash_size_bytes": FLASH_SIZE,
        "full_image_sha256": meta["full_bin_sha256"],
        "app_elf_sha256": meta["app_elf_sha256"],
        "protected_ranges": [{"name": name, "start": start, "end_exclusive": end} for start, end, name in PROTECTED],
        "warning": "Segmented write only; flashing the merged image at 0 overwrites stored data. Device identity, security and explicit authorization must be checked separately.",
        "segments": [
            {"file": name, "offset": start, "size": len(data), "end_exclusive": start + len(data),
             "erase_end_exclusive": (start + len(data) + SECTOR - 1) // SECTOR * SECTOR,
             "sha256": hashlib.sha256(data).hexdigest()}
            for name, data, start in images
        ],
    }


def package(archive: Path, bundle_path: Path, destination: Path) -> dict:
    meta, images = checked_segments(archive, bundle_path)
    manifest = build_manifest(meta, images)
    destination = safe_path(destination)
    if destination.exists():
        raise ValueError("destination already exists; choose a new directory")
    if not destination.parent.is_dir():
        raise ValueError("destination parent must exist")
    temporary = Path(tempfile.mkdtemp(prefix=".offline-package-", dir=destination.parent))
    try:
        for name, data, _ in images:
            (temporary / name).write_bytes(data)
        (temporary / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        # Exclusive creation avoids overwriting an existing path, including a race.
        os.mkdir(destination)
        try:
            for name, _, _ in images:
                os.replace(temporary / name, destination / name)
            os.replace(temporary / "manifest.json", destination / "manifest.json")
        except BaseException:
            shutil.rmtree(destination)
            raise
    finally:
        shutil.rmtree(temporary)
    return manifest


def verify_package(archive: Path, bundle_path: Path, destination: Path) -> dict:
    meta, images = checked_segments(archive, bundle_path)
    expected = build_manifest(meta, images)
    destination = safe_path(destination)
    if not destination.is_dir():
        raise ValueError("package directory does not exist")
    names = {name for name, _, _ in images} | {"manifest.json"}
    if {p.name for p in destination.iterdir()} != names:
        raise ValueError("package contains missing or unexpected entries")
    manifest_path = safe_path(destination / "manifest.json")
    actual = json.loads(manifest_path.read_text(encoding="utf-8"), object_pairs_hook=unique_json_object)
    if actual != expected:
        raise ValueError("package manifest differs from verified source artifacts")
    for name, data, _ in images:
        image = safe_path(destination / name)
        if not image.is_file() or image.read_bytes() != data:
            raise ValueError(f"package image differs from verified source: {name}")
    return expected


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true", help="recheck existing package against verified sources")
    parser.add_argument("archive", type=Path, help="verified content-addressed debug archive")
    parser.add_argument("bundle", type=Path, help="matching kitchen_bundle.bin")
    parser.add_argument("destination", type=Path, help="new directory for the segmented package")
    args = parser.parse_args()
    try:
        manifest = (verify_package if args.verify else package)(args.archive, args.bundle, args.destination)
    except (OSError, UnicodeError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Segmented package {'verified' if args.verify else 'created'}: {args.destination}")
    print(f"Full image SHA256: {manifest['full_image_sha256']}")
    print(f"ELF SHA256: {manifest['app_elf_sha256']}")
    for image in manifest["segments"]:
        print(f"{image['file']} 0x{image['offset']:06x} {image['size']} bytes {image['sha256']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
