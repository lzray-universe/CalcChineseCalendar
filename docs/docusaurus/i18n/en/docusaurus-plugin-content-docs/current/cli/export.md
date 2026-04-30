---
title: export
description: Parameters and output for bulk daily export.
---

# export

Exports daily data for a Gregorian month, month range, or year range.

## Syntax

```bash
lunar export [bsp] <YYYY-MM>
lunar export [bsp] --from <YYYY-MM> --to <YYYY-MM>
lunar export [bsp] --from-year <year> --to-year <year>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|jsonl|csv|txt] [--out <path>] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--jobs N] [--events 0|1] [--eclipse 0|1]
  [--scope basic|full] [--full 0|1]
  [--huangli off|folk|ziping|purple|xieji|all] [--trad folk|ziping|purple|xieji|all]
  [--lon <deg>]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<YYYY-MM>`: export one Gregorian month.
- `--from` / `--to`: inclusive Gregorian month range.
- `--from-year` / `--to-year`: inclusive year range.
- `--tz`: display timezone.
- `--lunar-day-tz`: civil-day timezone for daily boundaries.
- `--format`: `json`, `jsonl`, `csv`, or `txt`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.
- `--at`: daily sampling time, default `12:00:00`.
- `--jobs`: daily computation concurrency; `1` forces single-thread mode.
- `--events`: include solar-term and lunar-phase events.
- `--eclipse`: include eclipse events.
- `--scope`: `basic` or `full`; `full` enables eclipses, astronomy, and all Huangli schools.
- `--full`: boolean compatibility form of `--scope full`.
- `--huangli`: Huangli mode; `off` disables, `all` emits all schools.
- `--trad`: single Huangli profile alias.
- `--lon`: Huangli longitude, east-positive.
- `--year-boundary`: Huangli year-boundary rule.
- `--month-boundary`: Huangli month-boundary rule.
- `--leap-month-mode`: leap-month handling rule.
- `--day-boundary`: Huangli day-boundary rule.
- `--astro`: include astronomy events.
- `--astro-mode`: event scope.
- `--astro-pick`: targets for `pick` mode.
- `--astro-lat`: observer latitude.
- `--astro-lon`: observer longitude.
- `--astro-height`: observer height in meters.

## Output

- `json`: `meta`, input range, and `days`; each day may include Gregorian date, lunar date, Ganzhi, events, eclipses, astronomy events, and Huangli data.
- `jsonl`: one day per line.
- `csv`: one row per Gregorian day; nested data is summarized.
- `txt`: readable sections by date.

## Examples

```bash
lunar export @series 2025-09 --format jsonl --out days.jsonl
lunar export ./de442.bsp --from 2025-01 --to 2025-12 --scope full --format json
```
