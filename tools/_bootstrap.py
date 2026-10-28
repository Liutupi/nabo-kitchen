"""Optional task-local dependencies; a normal venv works without this folder."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VENDOR = ROOT / 'private/t1/vendor'
if VENDOR.is_dir():
    sys.path.insert(0, str(VENDOR))
