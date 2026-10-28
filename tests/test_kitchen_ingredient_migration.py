"""Compile real/synthetic KST1 migration cases and a missing-ID mutation."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SYNTHETIC = ROOT / "tests/fixtures/kitchen_metadata_synthetic_58.c"
BASE = [
    str(ROOT / "tests/test_kitchen_ingredient_migration.c"),
    str(ROOT / "main/kitchen/kitchen_save.c"),
    str(ROOT / "main/kitchen/kitchen_session.c"),
    str(ROOT / "main/kitchen/kitchen_timer.c"),
]


def compile_and_run(metadata: Path, executable: Path, *args: str) -> None:
    subprocess.run(
        [os.environ.get("CC", "cc"), "-O2", "-std=c11", "-Wall", "-Wextra", "-Werror",
         f"-I{ROOT / 'main/kitchen'}", *BASE, str(metadata), "-o", str(executable)],
        check=True, cwd=ROOT,
    )
    subprocess.run([str(executable), *args], check=True, cwd=ROOT)


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="kitchen-ingredient-migration-") as directory:
        temp = Path(directory)
        compile_and_run(SYNTHETIC, temp / "synthetic")
        text = SYNTHETIC.read_text(encoding="utf-8")
        old, new = '{"shrimp", "虾", "seafood", 1}', '{"shrimp_missing", "虾", "seafood", 1}'
        if text.count(old) != 1:
            raise ValueError("synthetic mutation target changed")
        missing = temp / "missing.c"
        missing.write_text(text.replace(old, new), encoding="utf-8")
        compile_and_run(missing, temp / "missing", "--expect-missing")
        compile_and_run(ROOT / "main/kitchen/kitchen_metadata.c", temp / "real")


if __name__ == "__main__":
    main()
