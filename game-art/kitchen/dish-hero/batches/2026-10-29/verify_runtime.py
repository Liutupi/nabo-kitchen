"""Verify an art candidate against the unchanged production C resource readers.

No firmware build, serial access or device write. Requires CC and CJSON_DIR.
The existing 17-hero fixture stays unchanged; this candidate expects every recipe
to resolve a hero instead of expecting the old missing-image placeholder.
"""
import argparse
import ctypes as ct
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware-root", required=True, type=Path)
    parser.add_argument("--assets", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    firmware, assets = args.firmware_root.resolve(), args.assets.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    packer = load("art_candidate_bundle", firmware / "tools/pack_kitchen_assets.py")
    blob = (assets / "kitchen_bundle.bin").read_bytes()
    assert packer.build(assets) == blob, "Bundle must rebuild byte-for-byte"
    assert len(blob) <= 0x500000, "Exceeds actual offline resource partition"
    unchanged = [name for name in packer.FILES if name != "kitchen_dish.bin"]
    for name in unchanged:
        assert (assets / name).read_bytes() == (firmware / "assets/kitchen" / name).read_bytes(), name
    revision = subprocess.check_output(["git", "-C", str(firmware), "rev-parse", "HEAD"], text=True).strip()
    subprocess.run(["git", "-C", str(firmware), "diff", "--exit-code", "HEAD", "--", "main/kitchen", "tests/kitchen_view_host.c", "tests/test_real_catalog_views.py", "tools/pack_kitchen_assets.py"], check=True)

    class Blob(ct.Structure):
        _fields_ = [("data", ct.c_void_p), ("size", ct.c_size_t)]

    class Assets(ct.Structure):
        _fields_ = [(name, Blob) for name in ("recipes", "fonts", "dish", "icons", "ui", "animation")]

    with tempfile.TemporaryDirectory() as temp:
        library = Path(temp) / "candidate_bundle.so"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC", "-I", str(firmware / "main/kitchen"), str(firmware / "main/kitchen/kitchen_bundle.c"), "-o", str(library)], check=True)
        dll = ct.CDLL(str(library))
        dll.kb_open.argtypes = [ct.c_void_p, ct.c_size_t, ct.POINTER(Assets)]
        dll.kb_open.restype = ct.c_bool
        callback = ct.CFUNCTYPE(None, ct.c_void_p)
        dll.kb_open_cooperative.argtypes = [ct.c_void_p, ct.c_size_t, ct.POINTER(Assets), callback, ct.c_void_p]
        dll.kb_open_cooperative.restype = ct.c_bool
        buffer = ct.create_string_buffer(blob)
        parsed = Assets()
        calls = []
        progress = callback(lambda context: calls.append(context))
        assert dll.kb_open_cooperative(buffer, len(blob), ct.byref(parsed), progress, 123)
        expected = (len(blob) - 16 + 4095) // 4096
        for (field, _), filename in zip(Assets._fields_, packer.FILES):
            item = getattr(parsed, field)
            assert ct.string_at(item.data, item.size) == (assets / filename).read_bytes(), filename
            expected += (item.size + 4095) // 4096
        assert calls == [123] * expected
        bad = bytearray(blob)
        bad[-1] ^= 1
        corrupt = ct.create_string_buffer(bytes(bad))
        assert not dll.kb_open(corrupt, len(bad), ct.byref(Assets()))
        for length in (0, 15, 111, len(blob) - 1):
            assert not dll.kb_open(buffer, length, ct.byref(Assets()))
        if os.name == "nt":
            from _ctypes import FreeLibrary
            FreeLibrary(dll._handle)

    views = load("art_candidate_views", firmware / "tests/test_real_catalog_views.py")
    views.ASSETS = assets
    output = args.out
    candidate_count = []

    class CompleteHeroCandidate(views.RealCatalogViews):
        def test_hero_and_placeholder_compatibility(self):
            # Strengthen the old fixture contract: all real recipes now have art.
            for index, ident in enumerate(self.ids):
                self.assertEqual(self.dll.view_has_hero(index), 1, ident)
                self.assertEqual(self.dll.view_compare_hero_required(index, 1), 1, ident)
            candidate_count[:] = [len(self.ids)]

        def test_representative_candidate_screens(self):
            from PIL import Image
            selected = ["fanqie-chaodan", "qingzheng-luyu", "jidan-chaofan", "qingjiao-chao-youyu", "baicai-doufu", "baiqie-ji"]
            gallery = Image.new("RGB", (720, 640))
            pixels = (ct.c_uint16 * (240 * 320))()
            for position, ident in enumerate(selected):
                self.assertEqual(self.dll.view_page(4, self.ids.index(ident)), 1)
                self.assertEqual(self.dll.view_draw(pixels, 0, 320), 1)
                frame = Image.new("RGB", (240, 320))
                frame.putdata([(((v >> 11) * 255 + 15) // 31, (((v >> 5) & 63) * 255 + 31) // 63, ((v & 31) * 255 + 15) // 31) for v in pixels])
                gallery.paste(frame, ((position % 3) * 240, (position // 3) * 320))
            gallery.save(output / "host-rendered-candidate.png")

    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(CompleteHeroCandidate))
    report = {"kind": "host-only art resource candidate; NOT FLASHED", "firmware_source_commit": revision, "bundle_sha256": hashlib.sha256(blob).hexdigest(), "bundle_bytes": len(blob), "partition_bytes": 0x500000, "partition_remaining_bytes": 0x500000 - len(blob), "deterministic_bundle": True, "production_c_bundle_reader": "PASS", "cooperative_crc_callbacks": expected, "corrupt_and_truncated_bundle_rejection": "PASS", "unchanged_payloads": unchanged, "hero_recipes_verified_in_production_c": candidate_count[0] if candidate_count else 0, "render_tests_run": result.testsRun, "render_tests_passed": result.wasSuccessful(), "host_gallery_sha256": sha(output / "host-rendered-candidate.png"), "files": {name: {"bytes": (assets / name).stat().st_size, "sha256": sha(assets / name)} for name in (*packer.FILES, "kitchen_bundle.bin")}, "unverified": ["new resources on physical device", "per-dish human visual and cooking acceptance", "long-term use"]}
    (output / "runtime-validation.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if not result.wasSuccessful():
        raise SystemExit(1)


if __name__ == "__main__":
    main()
