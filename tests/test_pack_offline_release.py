"""Safety tests for preparing segmented offline firmware; no device access."""
import hashlib
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import pack_offline_release as pack
from verify_firmware import ENTRY


class PackOfflineReleaseTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.archive = self.root / "archive"
        (self.archive / "bootloader").mkdir(parents=True)
        (self.archive / "partition_table").mkdir()
        table = b"".join(ENTRY.pack(0x50AA, kind, subtype, start, size, name.encode().ljust(16, b"\0"), 0)
                         for name, (kind, subtype, start, size) in pack.EXPECTED_PARTITIONS.items())
        table += b"\xeb\xeb" + b"\xff" * 14 + hashlib.md5(table).digest()
        table = table.ljust(0xC00, b"\xff")
        self.bundle = self.root / "source.bin"
        self.bundle.write_bytes(b"kitchen-assets")
        contents = (b"boot", table, b"firmware", self.bundle.read_bytes())
        merged = bytearray(b"\xff" * (0x200000 + len(contents[-1])))
        for (source, _, offset, _), data in zip(pack.SPECS, contents):
            if source != "kitchen_bundle.bin":
                (self.archive / source).write_bytes(data)
            merged[offset:offset + len(data)] = data
        (self.archive / pack.FULL_BIN).write_bytes(merged)
        (self.archive / "flash_args").write_text("--flash_size 8MB\n" + "".join(
            f"0x{offset:x} {source}\n" for source, _, offset, _ in pack.SPECS), encoding="utf-8")
        self.verify = patch.object(pack, "verify_archive", return_value={"full_bin_sha256": "1" * 64, "app_elf_sha256": "2" * 64})
        self.verify.start()
        self.addCleanup(self.verify.stop)
        self.addCleanup(self.temp.cleanup)

    def test_checked_package_records_hashes_and_erase_ranges(self):
        destination = self.root / "release"
        manifest = pack.package(self.archive, self.bundle, destination)
        self.assertEqual(len(manifest["segments"]), 4)
        for segment in manifest["segments"]:
            data = (destination / segment["file"]).read_bytes()
            self.assertEqual(hashlib.sha256(data).hexdigest(), segment["sha256"])
            self.assertEqual(segment["erase_end_exclusive"] % pack.SECTOR, 0)
            for start, end, _ in pack.PROTECTED:
                self.assertFalse(segment["offset"] < end and segment["erase_end_exclusive"] > start)
        self.assertEqual(pack.verify_package(self.archive, self.bundle, destination), manifest)
        with self.assertRaises(ValueError):
            pack.package(self.archive, self.bundle, destination)

    def test_verification_rejects_modified_segment(self):
        destination = self.root / "release"
        pack.package(self.archive, self.bundle, destination)
        (destination / "app.bin").write_bytes(b"tampered")
        with self.assertRaisesRegex(ValueError, "differs from verified source"):
            pack.verify_package(self.archive, self.bundle, destination)

    def test_mismatched_bundle_rejected_before_creating_output(self):
        self.bundle.write_bytes(b"wrong-assets")
        destination = self.root / "release"
        with self.assertRaisesRegex(ValueError, "differs from archived merged"):
            pack.package(self.archive, self.bundle, destination)
        self.assertFalse(destination.exists())

    def test_unexpected_partition_and_offset_rejected(self):
        args = self.archive / "flash_args"
        args.write_text(args.read_text().replace("0x200000", "0x700000"))
        with self.assertRaisesRegex(ValueError, "offsets"):
            pack.checked_segments(self.archive, self.bundle)
        args.write_text(args.read_text().replace("0x700000", "0x200000"))
        table = self.archive / "partition_table/partition-table.bin"
        data = bytearray(table.read_bytes())
        data[8] ^= 1
        table.write_bytes(data)
        with self.assertRaises(ValueError):
            pack.checked_segments(self.archive, self.bundle)

    def test_erase_range_cannot_expand_into_nvs(self):
        (self.archive / "bootloader/bootloader.bin").write_bytes(b"x" * 0x8001)
        with self.assertRaisesRegex(ValueError, "allowed flash range"):
            pack.checked_segments(self.archive, self.bundle)


if __name__ == "__main__":
    unittest.main()
