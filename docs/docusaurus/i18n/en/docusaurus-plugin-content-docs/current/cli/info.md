---
title: info
description: Parameters and output for version, config, and ephemeris information.
---

# info

Shows tool version, config, ephemeris file status, and coverage intervals.

## Syntax

```bash
lunar info [bsp] [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `--format`: `json` or `txt`. The default is always `txt`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.

## Output

- `json`: tool version, config values, ephemeris path, file existence, size, SPK objects, and coverage intervals.
- `txt`: the same information as key-value text.

## Examples

```bash
lunar info
lunar info ./de442.bsp --format json --out info.json
```
