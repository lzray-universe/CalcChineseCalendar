---
title: festival
description: Parameters and output for yearly festival data.
---

# festival

Generates festival data for a given year.

## Syntax

```bash
lunar festival [bsp] <year>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<year>`: target Gregorian year.
- `--tz`: display timezone.
- `--lunar-day-tz`: civil-day timezone used for lunar date mapping.
- `--format`: `json`, `txt`, or `csv`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.

## Output

- `json`: year plus festival array; records include date, name, category, and related Gregorian/lunar fields.
- `txt`: readable festival list by date.
- `csv`: one row per festival.

## Examples

```bash
lunar festival @series 2025
lunar festival ./de442.bsp 2025 --format csv --out festival.csv
```
