---
title: range
description: Parameters and output for time-range event queries.
---

# range

Queries solar-term, lunar-phase, and eclipse events within a time range.

## Syntax

```bash
lunar range [bsp] --from <time> --to <time>
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `--from`: range start, required.
- `--to`: range end, required and not earlier than `--from`.
- `--kinds`: comma-separated event types.
- `--tz`: display timezone.
- `--format`: `json`, `txt`, `csv`, `ics`, or `jsonl`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.
- `--eclipse`: include lunar eclipse detail for full-moon rows.

## Output

- `json`: `meta`, query type, and events inside the range.
- `jsonl`: one event per line.
- `txt`: readable event table.
- `csv`: one event per row.
- `ics`: calendar event collection.

## Examples

```bash
lunar range @series --from 2025-01-01 --to 2025-12-31
lunar range ./de442.bsp --from 2025-02-01 --to 2025-03-01 --format csv
```
