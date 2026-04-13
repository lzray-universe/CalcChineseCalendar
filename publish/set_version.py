from __future__ import annotations

import json
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parent.parent
PYPROJECT = ROOT / "publish" / "python" / "pyproject.toml"
PY_VERSION = ROOT / "publish" / "python" / "src" / "calcchinesecalendar" / "_version.py"
NPM_PACKAGE = ROOT / "publish" / "npm" / "package.json"

PRERELEASE_MAP = {
    "a": "a",
    "alpha": "a",
    "b": "b",
    "beta": "b",
    "c": "rc",
    "pre": "rc",
    "preview": "rc",
    "rc": "rc",
    "dev": "dev",
    "post": "post",
}
NPM_PRERELEASE_MAP = {
    "a": "alpha",
    "b": "beta",
    "rc": "rc",
    "dev": "dev",
    "post": "post",
}
VERSION_CORE_RE = re.compile(r"(?P<major>\d+)\.(?P<minor>\d+)(?:\.(?P<patch>\d+))?")
PRERELEASE_RE = re.compile(
    r"^(?P<label>a|alpha|b|beta|c|pre|preview|rc|dev|post)"
    r"(?:[\s._-]*(?P<num>\d+))?",
    flags=re.IGNORECASE,
)


def replace_regex(path: Path, pattern: str, repl: str) -> None:
    text = path.read_text(encoding="utf-8")
    updated, count = re.subn(pattern, repl, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise RuntimeError(f"failed to update version in {path}")
    path.write_text(updated, encoding="utf-8")


def derive_versions_from_tag(tag: str) -> tuple[str, str]:
    match = VERSION_CORE_RE.search(tag)
    if match is None:
        raise ValueError(f"tag does not contain a version core: {tag!r}")

    major = int(match.group("major"))
    minor = int(match.group("minor"))
    patch = int(match.group("patch") or "0")
    base = f"{major}.{minor}.{patch}"

    suffix = tag[match.end():].strip()
    suffix = re.sub(r"^[\s._()+-]+", "", suffix)
    if not suffix:
        return base, base

    pre = PRERELEASE_RE.match(suffix)
    if pre is None:
        return base, base

    label = PRERELEASE_MAP[pre.group("label").lower()]
    number = pre.group("num") or "1"
    py_version = f"{base}{label}{number}"
    npm_label = NPM_PRERELEASE_MAP[label]
    npm_version = f"{base}-{npm_label}.{number}"
    return py_version, npm_version


def apply_versions(py_version: str, npm_version: str) -> None:
    replace_regex(PYPROJECT, r'^version = "[^"]+"$', f'version = "{py_version}"')
    PY_VERSION.write_text(f'__version__="{py_version}"\n', encoding="utf-8")

    package = json.loads(NPM_PACKAGE.read_text(encoding="utf-8"))
    package["version"] = npm_version
    NPM_PACKAGE.write_text(
        json.dumps(package, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    argv = sys.argv[1:]
    if len(argv) != 1 and not (len(argv) == 2 and argv[0] == "--from-tag"):
        print("usage: set_version.py <version>\n       set_version.py --from-tag <tag>",
              file=sys.stderr)
        return 2

    if len(argv) == 2:
        tag = argv[1].strip()
        if not tag:
            print("tag must not be empty", file=sys.stderr)
            return 2
        py_version, npm_version = derive_versions_from_tag(tag)
    else:
        version = argv[0].strip()
        if not version:
            print("version must not be empty", file=sys.stderr)
            return 2
        py_version = version
        npm_version = version

    apply_versions(py_version, npm_version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
