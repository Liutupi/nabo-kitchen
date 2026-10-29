#!/usr/bin/env python3
"""Package accepted build artifacts; an explicit spec supports later releases.

Without --release-spec, preserve the original v0.1.0 packaging contract.
This tool never accesses hardware or publishes files.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

sys.dont_write_bytecode = True
from archive_firmware import ARTIFACTS, create_archive, verify_archive
from pack_offline_release import package, verify_package

APP_SHA = "a6f4a167bf1d79d11bd7dbbf2a46d56eef5362446bbcd41313ace56effcb1202"
BUNDLE_SHA = "f1c0f215a460b6c4e7c8805527a6077ff2b8473626eb55205a77f9bda26d2931"
ELF_SHA = "698af70572f6700387fa72f9af029fdede66b95dfcc2302f6ab01eeef1b9793f"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def prepare(archive: Path, bundle: Path, output: Path, release_spec: Path | None = None,
            bootloader_archive: Path | None = None) -> None:
    root = Path(__file__).resolve().parents[1]
    archive, bundle, output = archive.resolve(), bundle.resolve(), output.resolve()
    original = verify_archive(archive)
    spec = json.loads(release_spec.read_text(encoding="utf-8")) if release_spec else {}
    version = spec.get("release", "v0.1.0")
    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", version):
        raise ValueError("release must be a stable semantic version")
    if release_spec:
        required = ("release", "application_sha256", "application_elf_sha256", "resource_sha256",
                    "runtime_source_commit", "content_source_commit", "application_embedded_build_version", "validation")
        if any(key not in spec for key in required):
            raise ValueError("release spec must identify accepted artifacts, sources and validation")
    app_sha = spec.get("application_sha256", APP_SHA)
    elf_sha = spec.get("application_elf_sha256", ELF_SHA)
    bundle_sha = spec.get("resource_sha256", BUNDLE_SHA)
    assert sha(archive / "FoloToy-AI-Passport.bin") == app_sha
    assert sha(archive / "FoloToy-AI-Passport.elf") == elf_sha
    assert sha(bundle) == bundle_sha
    boot_source = archive / "bootloader/bootloader.bin"
    if bootloader_archive:
        verify_archive(bootloader_archive)
        if not release_spec or "bootloader_sha256" not in spec:
            raise ValueError("a bootloader override requires an explicitly accepted hash")
        boot_source = bootloader_archive / "bootloader/bootloader.bin"
        assert sha(boot_source) == spec["bootloader_sha256"]
        assert sha(bootloader_archive / "partition_table/partition-table.bin") == sha(archive / "partition_table/partition-table.bin")
    output.mkdir(parents=True, exist_ok=False)
    staging = output / "verified-build-inputs"
    staging.mkdir()
    for name in ARTIFACTS:
        if name == "FoloToy-AI-Passport-full.bin":
            continue
        target = staging / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(archive / name, target)
    shutil.copyfile(boot_source, staging / "bootloader/bootloader.bin")
    shutil.copyfile(bundle, staging / "kitchen_bundle.bin")
    subprocess.run([
        sys.executable, "-m", "esptool", "--chip", "esp32c3", "merge_bin",
        "--output", "FoloToy-AI-Passport-full.bin", "--flash_mode", "dio",
        "--flash_freq", "80m", "--flash_size", "8MB",
        "0x0", "bootloader/bootloader.bin", "0x8000", "partition_table/partition-table.bin",
        "0x10000", "FoloToy-AI-Passport.bin", "0x200000", "kitchen_bundle.bin",
    ], cwd=staging, check=True)
    subprocess.run([sys.executable, str(root / "tools/verify_firmware.py"), str(staging)], check=True)
    verified = create_archive(staging, output / "debug-archive")
    verify_archive(verified)
    segments = output / "segments"
    meta = package(verified, bundle, segments)
    verify_package(verified, bundle, segments)
    full = (staging / "FoloToy-AI-Passport-full.bin").read_bytes()
    # Every byte outside the four allowlisted build segments must be erased fill.
    cursor = 0
    for segment in sorted(meta["segments"], key=lambda item: item["offset"]):
        start = segment["offset"]
        assert all(byte == 255 for byte in full[cursor:start]), "non-build data in merged gap"
        assert full[start:start + segment["size"]] == (segments / segment["file"]).read_bytes()
        cursor = start + segment["size"]
    assert all(byte == 255 for byte in full[cursor:]), "non-build trailing data"
    public = output / "public"
    public.mkdir()
    full_path = public / f"nabo-kitchen-{version}-full.bin"
    full_path.write_bytes(full)
    manifest = {
        "schema_version": 1, "release": version, "channel": "experience-prerelease",
        "source_ref": f"refs/tags/{version}",
        "repository": "https://github.com/Liutupi/nabo-kitchen",
        "runtime_source_commit": spec.get("runtime_source_commit", "c7cdc0b94a6009672333dc1736b4f1c33f0a0537"),
        "content_source_commit": spec.get("content_source_commit", "c99d979b630286ef3ec385c2e6f1485940ecb5a3"),
        "accepted_original_full_image_sha256": original["full_bin_sha256"],
        "application_embedded_build_version": spec.get("application_embedded_build_version", "f799a2f-dirty"),
        "application_sha256": app_sha, "application_bytes": (archive / "FoloToy-AI-Passport.bin").stat().st_size,
        "application_elf_sha256": elf_sha,
        "resource_sha256": bundle_sha, "resource_bytes": bundle.stat().st_size,
        "full_image": {"file": full_path.name, "bytes": len(full), "sha256": sha(full_path), "offset": 0},
        "segments": meta["segments"], "protected_ranges_for_segmented_upgrade": meta["protected_ranges"],
        "first_install_warning": "Merged 0x0 installation replaces firmware and overwrites NVS/PHY gaps. It may reset settings and records.",
        "upgrade": {"requires_exact_compatible_layout": True, "write_only": ["app.bin", "kitchen_bundle.bin"], "offsets": [65536, 2097152]},
        "provenance": ("All segments are build outputs; app and bundle match the explicitly accepted hashes. No device Flash dumps, NVS, credentials or logs are included. All merged gaps are 0xFF." if release_spec else "All segments are build outputs; app and bundle are unchanged accepted prebuilt files. No device Flash dumps, NVS, credentials or logs are included. All merged gaps are 0xFF."),
        "validation": spec.get("validation", {
            "merged_structure_and_archive": "PASS", "segmented_package_and_erased_gaps": "PASS",
            "device_resource_readback": "PASS", "device_controlled_boots": 3,
            "device_complete_frames": 6, "matching_dish_pixels": 100800,
            "physical_panel_and_speaker_observation": "NOT OBSERVED",
            "full_image_first_install": "NOT RUN",
            "bootloader_limit": "Packaged build bootloader differs from the retained compatible bootloader used for app/resource device checks.",
            "all_recipe_cooking_and_art_acceptance": "PENDING REVIEW", "long_term_use": "NOT RUN",
        }),
        "licenses": "Code follows bundled MIT notices; NABO art, character, voice, animation and recipe text are excluded. Font notices apply.",
    }
    if release_spec:
        manifest["bootloader_sha256"] = sha(boot_source)
        manifest["release_spec_sha256"] = sha(release_spec)
    dump(public / "manifest.json", manifest)
    payload = {p.name: p.read_bytes() for p in segments.iterdir() if p.is_file() and p.name != "manifest.json"}
    payload["segments.json"] = (segments / "manifest.json").read_bytes()
    payload["manifest.json"] = (public / "manifest.json").read_bytes()
    for name in ("installation.md", "installation.zh_CN.md"):
        payload[name] = (root / "docs/community" / version / name).read_bytes()
    # Keep the packaged instructions self-contained; relative repository links
    # are rewritten to the immutable release source when copied into the ZIP.
    for name in ("installation.md", "installation.zh_CN.md"):
        text = payload[name].decode("utf-8")
        text = text.replace("(release-validation.json)", f"(https://github.com/Liutupi/nabo-kitchen/blob/{version}/docs/community/{version}/release-validation.json)")
        text = text.replace("(../../development/offline-art-device-validation-20261029.json)", "(https://github.com/Liutupi/nabo-kitchen/blob/v0.1.0/docs/development/offline-art-device-validation-20261029.json)")
        payload[name] = text.encode("utf-8")
    for source, target in (("LICENSE", "LICENSE"), ("main/kitchen/LICENSE", "KITCHEN-CODE-LICENSE"),
                           ("assets/kitchen/NOTICE.md", "NOTICE.md"), ("assets/kitchen/NOTICE.zh_CN.md", "NOTICE.zh_CN.md")):
        payload[target] = (root / source).read_bytes()
    for path in sorted((root / "assets/kitchen/licenses").rglob("*")):
        if path.is_file():
            payload["licenses/" + path.relative_to(root / "assets/kitchen/licenses").as_posix()] = path.read_bytes()
    payload["SHA256SUMS.txt"] = "".join(f"{hashlib.sha256(data).hexdigest()}  {name}\n" for name, data in sorted(payload.items())).encode("ascii")
    zip_path = public / f"nabo-kitchen-{version}.zip"
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in sorted(payload.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 10, 29, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            z.writestr(info, data, compresslevel=9)
    with zipfile.ZipFile(zip_path) as z:
        assert set(z.namelist()) == set(payload)
        assert all(z.read(name) == data for name, data in payload.items())
    sums = "".join(f"{sha(path)}  {path.name}\n" for path in sorted(public.iterdir()))
    (public / "SHA256SUMS.txt").write_text(sums, encoding="ascii", newline="\n")
    dump(output / "audit.json", {"files": [{"file": p.name, "bytes": p.stat().st_size, "sha256": sha(p)} for p in sorted(public.iterdir())], "zip_entries": sorted(payload), "all_merged_gaps_ff": True, "no_device_dump_or_private_data": True, "debug_archive": verified.name})
    print(json.dumps({"public_directory": str(public), "full_image_sha256": sha(full_path), "app_sha256": app_sha, "bundle_sha256": bundle_sha}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--release-spec", type=Path)
    parser.add_argument("--bootloader-archive", type=Path,
                        help="Verified archive containing an explicitly accepted bootloader")
    args = parser.parse_args()
    prepare(args.archive, args.bundle, args.output, args.release_spec, args.bootloader_archive)
