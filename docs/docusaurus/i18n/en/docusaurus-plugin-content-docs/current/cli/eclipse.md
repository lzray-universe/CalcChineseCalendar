---
title: eclipse
description: Parameters and output for solar/lunar eclipse and visibility queries.
---

# eclipse

Queries solar eclipses, lunar eclipses, and eclipses visible near a given site and time.

## Syntax

```bash
lunar [--eclipse-method modern|legacy] eclipse [bsp] --near <YYYY-MM-DD> [--kind lunar|solar]
  [--stage any|umb|total]
  [--stage any|central]
  [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1]
  [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--tz Z|+08:00|-05:00] [--format json|txt|geojson]
  [--out <path>] [--pretty 0|1] [--quiet]

lunar eclipse [bsp] --visible-near <time> --point-lat <deg> --point-lon <deg>
  [--kind lunar|solar|both] [--visible-years <years>]
  [--stage any|umb|total|central] [--sample-min <minutes>]
  [--point-height <m>]
  [--tz Z|+08:00|-05:00] [--format json|txt]
  [--out <path>] [--pretty 0|1] [--quiet]
```

## Parameters

- `[bsp]`: optional ephemeris. Omit to auto-select.
- `--eclipse-method`: global eclipse algorithm, `modern` or `legacy`.
- `--near`: nearest eclipse to the date.
- `--visible-near`: nearest visible eclipse around the supplied instant and site.
- `--kind`: `lunar`, `solar`, or `both`.
- `--stage`: stage filter. Lunar uses `any`, `umb`, `total`; solar uses `any`, `central`.
- `--sample-min`: visibility sampling step in minutes.
- `--point-lat`: observer latitude.
- `--point-lon`: observer longitude.
- `--point-height`: observer height in meters.
- `--point-refine`: refine point visibility boundary.
- `--global-vis`: compute global grid visibility.
- `--global`: compatibility alias for `--global-vis`.
- `--global-format`: global visibility format, `json` or `geojson`.
- `--grid-lat-step`: global grid latitude step.
- `--grid-lon-step`: global grid longitude step.
- `--tz`: display timezone.
- `--format`: normal mode supports `json`, `txt`, `geojson`; `--visible-near` supports `json`, `txt`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress progress and file-write notices.

## Output

- `json`: eclipse type, stage, maximum time, contact times, visibility, and optional global grid.
- `txt`: readable eclipse summary.
- `geojson`: global visibility FeatureCollection; selecting this format enables global visibility computation.

## Examples

```bash
lunar eclipse @series --near 2025-09-07 --format json
lunar eclipse ./de442.bsp --kind solar --near 2026-08-12 --format json
lunar eclipse ./de442.bsp --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```
