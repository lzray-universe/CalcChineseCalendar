---
title: almanac
description: Parameters and output for one-day Huangli queries.
---

# almanac

Queries Huangli summary and auspicious / inauspicious items for a date.

## Syntax

```bash
lunar almanac [bsp] <YYYY-MM-DD>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
  [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<YYYY-MM-DD>`: target Gregorian date.
- `--tz`: display timezone.
- `--lunar-day-tz`: civil-day timezone used for lunar date mapping.
- `--format`: `json`, `txt`, or `csv`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.
- `--lon`: Huangli longitude, east-positive.
- `--trad`: Huangli rule profile.
- `--year-boundary`: Huangli year-boundary rule.
- `--month-boundary`: Huangli month-boundary rule.
- `--leap-month-mode`: leap-month handling rule.
- `--day-boundary`: Huangli day-boundary rule.

## Output

- `json`: input date, lunar date, Ganzhi, Huangli rules, auspicious/inauspicious items, and related structured fields.
- `txt`: key-value Huangli summary.
- `csv`: one summary row.

## Examples

```bash
lunar almanac @series 2025-09-17
lunar almanac ./de442.bsp 2025-09-17 --trad xieji --format json
```
