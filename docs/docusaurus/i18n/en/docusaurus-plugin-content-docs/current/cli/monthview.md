---
title: monthview
description: Parameters and output for a monthly daily view.
---

# monthview

Generates a daily view for one Gregorian month.

## Syntax

```bash
lunar monthview [bsp] <YYYY-MM>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<YYYY-MM>`: target Gregorian month.
- `--tz`: display timezone.
- `--lunar-day-tz`: civil-day timezone used for lunar date mapping.
- `--format`: `json`, `txt`, or `csv`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.
- `--astro`: include astronomy events.
- `--astro-mode`: event scope.
- `--astro-pick`: targets for `pick` mode.
- `--astro-lat`: observer latitude.
- `--astro-lon`: observer longitude.
- `--astro-height`: observer height in meters.

## Output

- `json`: month input, daily records, lunar dates, Huangli summaries, and optional astronomy events.
- `txt`: readable daily summaries.
- `csv`: one row per Gregorian day.

## Examples

```bash
lunar monthview @series 2025-09 --format txt
lunar monthview ./de442.bsp 2025-09 --format csv --out month.csv
```
