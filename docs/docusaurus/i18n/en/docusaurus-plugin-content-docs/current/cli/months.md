---
title: months
description: Parameters and output for enumerating lunar months.
---

# months

Enumerates lunar months for one or more years.

## Syntax

```bash
lunar months [bsp] <years>
  [--mode lunar|gregorian]
  [--format json|txt|csv] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--include-eclipses 0|1]
  [--output <json>] [--output-txt <txt>]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select; accepts `.bsp`, `@series`, or `series`.
- `<years>`: year expression, such as `2025`, `2024-2026`, or `2024,2026,2030-2032`.
- `--mode`: year basis. `lunar` uses lunar years; `gregorian` uses Gregorian years.
- `--format`: output format, `json`, `txt`, or `csv`.
- `--out`: output file path; omitted means stdout.
- `--tz`: display timezone for local time strings.
- `--pretty`: pretty-print JSON when set to `1`.
- `--quiet`: suppress progress and file-write notices.
- `--include-eclipses`: include lunar eclipse data for the year.
- `--output`: legacy JSON output path.
- `--output-txt`: legacy text output path.

## Output

- `json`: `meta` plus `data`. Each year contains `year`, `mode`, and `months`; month records include label, month number, leap flag, start/end Julian dates, UTC times, and local times.
- `txt`: grouped by year for direct reading or simple scripts.
- `csv`: one row per lunar month.
- With `--include-eclipses`, JSON/TXT include lunar eclipse data. CSV is not suitable for nested eclipse data.

## Examples

```bash
lunar months @series 2025
lunar months ./de442.bsp 2024-2026 --mode gregorian --format json --out months.json
```
