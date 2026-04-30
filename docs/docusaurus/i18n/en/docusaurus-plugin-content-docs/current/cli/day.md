---
title: day
description: Parameters and output for single-day calendar, Huangli, and astronomy queries.
---

# day

Queries lunar date, lunar phase, Huangli data, and optional astronomy events for one Gregorian date.

## Syntax

```bash
lunar day [bsp] <YYYY-MM-DD>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<YYYY-MM-DD>`: target Gregorian date.
- `--tz`: display timezone.
- `--lunar-day-tz`: civil-day timezone used for lunar date mapping.
- `--format`: `json`, `txt`, `csv`, or `jsonl`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.
- `--at`: sampling time within the day.
- `--events`: include same-day solar-term and lunar-phase events.
- `--lon`: longitude for Huangli calculations, east-positive.
- `--trad`: Huangli rule profile.
- `--year-boundary`: Huangli year-boundary rule.
- `--month-boundary`: Huangli month-boundary rule.
- `--leap-month-mode`: leap-month handling rule.
- `--day-boundary`: Huangli day-boundary rule.
- `--astro`: include astronomy events.
- `--astro-mode`: event scope, `less`, `all`, or `pick`.
- `--astro-pick`: targets for `pick` mode.
- `--astro-lat`: observer latitude.
- `--astro-lon`: observer longitude.
- `--astro-height`: observer height in meters; requires latitude and longitude.

## Output

- `json` / `jsonl`: `meta`, `input`, and `data`. `data` includes `lunar_date`, `huangli`, illumination percentage, phase name, lunar mansion relation, sampling UTC/local time, `events`, and `astro_events`.
- `txt`: key-value output plus event tables.
- `csv`: one summary row with lunar date, phase, sampling time, event summary, and Huangli fields.

## Examples

```bash
lunar day @series 2025-06-01
lunar day ./de442.bsp 2025-01-31 --trad ziping --format json
lunar day ./de442.bsp 2025-06-01 --astro 1 --astro-lat 31.23 --astro-lon 121.47
```
