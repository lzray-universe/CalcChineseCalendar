---
title: next
description: Parameters and output for upcoming event queries.
---

# next

Queries the next events after a starting instant.

## Syntax

```bash
lunar next [bsp] --from <time> --count N
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `--from`: starting instant, required.
- `--count`: number of events, must be at least `1`.
- `--kinds`: comma-separated event types.
- `--tz`: display timezone.
- `--format`: `json`, `txt`, `csv`, `ics`, or `jsonl`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.
- `--eclipse`: include lunar eclipse detail for full-moon rows.

## Output

- `json`: `meta`, query type, and event array.
- `jsonl`: one event per line.
- `txt`: readable event table.
- `csv`: one event per row.
- `ics`: calendar event collection.

Event fields usually include kind, code, name, year, Julian date, UTC time, and local display time.

## Examples

```bash
lunar next @series --from 2025-06-01T00:00:00+08:00 --count 5
lunar next ./de442.bsp --from 2025-06-01 --count 10 --format ics --out next.ics
```
