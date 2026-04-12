from __future__ import annotations

import json
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parent.parent
PYPROJECT = ROOT / "publish" / "python" / "pyproject.toml"
PY_VERSION = ROOT / "publish" / "python" / "src" / "lunar" / "_version.py"
NPM_PACKAGE = ROOT / "publish" / "npm" / "package.json"


def replace_regex(path: Path, pattern: str, repl: str) -> None:
    text = path.read_text(encoding="utf-8")
    updated, count = re.subn(pattern, repl, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise RuntimeError(f"failed to update version in {path}")
    path.write_text(updated, encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: set_version.py <version>", file=sys.stderr)
        return 2

    version = sys.argv[1].strip()
    if not version:
        print("version must not be empty", file=sys.stderr)
        return 2

    replace_regex(PYPROJECT, r'^version = "[^"]+"$', f'version = "{version}"')
    PY_VERSION.write_text(f'__version__="{version}"\n', encoding="utf-8")

    package = json.loads(NPM_PACKAGE.read_text(encoding="utf-8"))
    package["version"] = version
    NPM_PACKAGE.write_text(
        json.dumps(package, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
