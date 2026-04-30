---
title: at
description: Parameters and output for instant queries.
---

# at

Queries lunar date, phase, lunar mansion relation, Huangli data, and optional events for one or more instants.

## Syntax

```bash
lunar at [bsp] <time>
lunar at [bsp] --time <time>
lunar at [bsp] --stdin
lunar at [bsp] --file <path>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--events 0|1] [--eot-lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--jobs N] [--meta-once 0|1]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<time>` / `--time`: query instant.
- `--stdin`: read one instant per line from stdin.
- `--file`: read one instant per line from a file.
- `--input-tz`: timezone used when input has no suffix.
- `--tz`: display timezone.
- `--lunar-day-tz`: civil-day timezone used for lunar date mapping.
- `--format`: `json`, `txt`, or `jsonl`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.
- `--events`: include related events.
- `--eot-lon`: longitude for equation-of-time output, east-positive.
- `--trad`: Huangli profile.
- `--year-boundary`: Huangli year-boundary rule.
- `--month-boundary`: Huangli month-boundary rule.
- `--leap-month-mode`: leap-month handling rule.
- `--day-boundary`: Huangli day-boundary rule.
- `--jobs`: batch concurrency parameter; output order remains input order.
- `--meta-once`: emit metadata once in batch JSONL output.

## Output

- `json`: one object with `meta`, input time, and `data`; `data` includes lunar date, Huangli, illumination percentage, phase, lunar mansion relation, UTC/local time, and optional events.
- `jsonl`: one line per input in batch mode; single-run mode falls back to `json`.
- `txt`: key-value text output.

## Examples

```bash
lunar at @series 2025-06-01T00:00:00+08:00 --format json
lunar at ./de442.bsp --file times.txt --format jsonl --meta-once 1
```
