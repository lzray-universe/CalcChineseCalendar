---
title: calendar
description: Parameters and output for yearly solar terms, lunar phases, and lunar months.
---

# calendar

Generates solar terms, lunar phases, and optional lunar-month data for one or more years.

## Syntax

```bash
lunar calendar [bsp] [<years>]
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--include-months 0|1] [--include-eclipses 0|1]
  [--pretty 0|1] [--quiet]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `[<years>]`: year expression; defaults to `2025`.
- `--format`: output format, `json`, `txt`, or `ics`.
- `--out`: output file path; omitted means stdout.
- `--tz`: display timezone.
- `--include-months`: include lunar month records.
- `--include-eclipses`: include lunar eclipse data.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.

## Output

- `json`: `meta` plus `data`. A single year is an object; multiple years are an array. Year objects include `sol_terms`, `lun_phase`, optional `months`, and optional `lunar_eclipses`.
- `txt`: readable yearly event sections.
- `ics`: calendar events for solar terms and lunar phases only.

## Examples

```bash
lunar calendar @series 2025
lunar calendar ./de442.bsp 2024-2026 --format ics --out calendar.ics
```
