---
title: year
description: Parameters and output for a single-year calendar summary.
---

# year

Generates a single-year calendar summary.

## Syntax

```bash
lunar year [bsp] <year>
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<year>`: target year.
- `--mode`: `lunar` or `gregorian` year basis.
- `--format`: output format, `json`, `txt`, or `ics`.
- `--out`: output file path; omitted means stdout.
- `--tz`: display timezone.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.

## Output

- `json`: `meta` plus `data`; `data` includes `year`, `mode`, `sol_terms`, `lun_phase`, and `months`.
- `txt`: readable yearly summary.
- `ics`: calendar events for solar terms and lunar phases.

## Example

```bash
lunar year @series 2025 --format json --out year-2025.json
```
