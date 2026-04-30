---
title: event
description: Parameters and output for single solar-term, lunar-phase, and eclipse events.
---

# event

Queries one solar-term event, lunar-phase event, or forwards to eclipse queries.

## Syntax

```bash
lunar event [bsp] solar-term <code> <year>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-phase <new_moon|fst_qtr|full_moon|lst_qtr> --near <YYYY-MM-DD>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-eclipse --near <YYYY-MM-DD> [eclipse options...]
lunar event [bsp] solar-eclipse --near <YYYY-MM-DD> [eclipse options...]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `solar-term`: solar-term event mode.
- `<code>`: solar-term code, `J1..J12` or `Z1..Z12`.
- `<year>`: target year for solar-term lookup.
- `lunar-phase`: lunar-phase event mode.
- `<new_moon|fst_qtr|full_moon|lst_qtr>`: phase code.
- `--near`: nearest date for phase or eclipse lookup.
- `lunar-eclipse`: forwards to lunar eclipse behavior.
- `solar-eclipse`: forwards to solar eclipse behavior.
- `--format`: regular events support `json`, `txt`, `ics`; eclipse forwarding supports `json`, `txt`, `geojson`.
- `--out`: output file path; omitted means stdout.
- `--tz`: display timezone.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.
- `--eclipse`: include lunar eclipse detail when querying full moon.

## Output

- `json`: `meta` plus one event object. Event fields include kind, code, name, year, `jd_utc`, UTC time, and local time; full moon may include Moon distance.
- `txt`: readable event summary.
- `ics`: one calendar event.
- Eclipse forwarding output matches the `eclipse` command.

## Examples

```bash
lunar event @series solar-term Z2 2025
lunar event @series lunar-phase full_moon --near 2025-09-07
lunar event ./de442.bsp solar-eclipse --near 2026-08-12 --format json
```
