---
title: search
description: Parameters and output for event search expressions.
---

# search

Searches events with a restricted expression.

## Syntax

```bash
lunar search [bsp] <query>
  [--from <time>] [--count N] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv|ics|jsonl] [--out <path>]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `<query>`: search expression. Currently supports `next ...`.
- `--from`: search start; defaults to current UTC time.
- `--count`: number of events, default `1`.
- `--tz`: display timezone.
- `--format`: `json`, `txt`, `csv`, `ics`, or `jsonl`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.
- `--eclipse`: include lunar eclipse detail for full-moon rows.

## Query targets

Common targets include `full moon`, `new moon`, `first quarter`, `last quarter`, `solar term`, `lunar eclipse`, `solar eclipse`, and `eclipse`. Spaces, hyphens, and underscores are normalized.

## Output

Output matches `next`: event list, JSONL event stream, CSV table, or ICS calendar.

## Examples

```bash
lunar search @series next full moon --from 2025-06-01
lunar search ./de442.bsp "next lunar eclipse" --from 2025-01-01 --format json
```
