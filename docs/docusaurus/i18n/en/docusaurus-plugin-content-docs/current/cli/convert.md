---
title: convert
description: Parameters and output for Gregorian/lunar conversion.
---

# convert

Converts between Gregorian date/time and lunar date.

## Syntax

```bash
lunar convert [bsp] <dt_or_tm>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --from-lunar <lunar_year> <month_no> <day> [--leap 0|1]
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --stdin
lunar convert [bsp] --file <path>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--jobs N] [--meta-once 0|1]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<dt_or_tm>`: Gregorian date or date-time input.
- `--from-lunar`: lunar-to-Gregorian mode; followed by lunar year, month number, and day.
- `--leap`: whether the lunar month is leap.
- `--stdin`: read Gregorian inputs from stdin.
- `--file`: read Gregorian inputs from a file.
- `--input-tz`: timezone used when Gregorian input has no suffix.
- `--tz`: display timezone.
- `--lunar-day-tz`: civil-day timezone used for lunar mapping.
- `--format`: `json`, `txt`, or `jsonl`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.
- `--jobs`: batch concurrency parameter.
- `--meta-once`: emit metadata once in batch JSONL output.

## Output

- `json`: input, converted Gregorian date/time, lunar date, leap-month flag, and related date boundaries.
- `jsonl`: one conversion per line in batch mode; single-run mode falls back to `json`.
- `txt`: key-value conversion result.

## Examples

```bash
lunar convert @series 2025-06-01
lunar convert ./de442.bsp --from-lunar 2025 5 6 --leap 0 --format json
```
