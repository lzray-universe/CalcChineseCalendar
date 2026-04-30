---
title: zodiac
description: Parameters and output for solar zodiac queries.
---

# zodiac

Queries the solar zodiac at one instant, or generates zodiac intervals for a Gregorian year.

## Syntax

```bash
lunar zodiac <bsp> --time <time>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]

lunar zodiac <bsp> --year <year>
  [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

## Parameters

- `<bsp>`: required ephemeris. This command does not currently use ephemeris auto-fill.
- `--time`: query the zodiac at one instant.
- `--year`: output intervals for a Gregorian year.
- `--input-tz`: timezone used when `--time` has no suffix.
- `--tz`: display timezone; in `--year` mode it also defines the civil-year clipping window.
- `--format`: `json`, `txt`, or `csv`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.

## Output

- `--time`: current sign, apparent solar ecliptic longitude, sign boundaries, elapsed duration, and remaining duration.
- `--year`: 12 sign intervals with start/end, clipped in-year start/end, and duration.
- Solar zodiac is computed from geocentric apparent solar ecliptic longitude with light-time correction.

## Examples

```bash
lunar zodiac @series --time 2025-03-20T18:01:00+08:00
lunar zodiac ./de442.bsp --year 2025 --format csv
```
