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

Global solar-eclipse `mag` is the greatest surface magnitude at the instant of
greatest eclipse, not geocentric disc overlap. Partial eclipses use the mean
penumbral lunar radius `k1=0.2724880`; total, annular, and hybrid eclipses use
the mean-minimum lunar radius `k2=0.2722810`, matching the Five Millennium
Canon convention. `obscuration` is the fraction of the solar-disc area covered.

## Batch solar-eclipse refresh

When a greatest-eclipse `JD TDB` and the previous type are already known, an
existing catalog can be refreshed in a persistent ephemeris process:

```bash
lunar eclipse-magnitude ./de441.bsp --input maxima.tsv --out magnitudes.tsv
```

Input columns are `id jd_tdb_max type`, where `type` is `P`, `A`, `T`, or `H`.
The default TSV output is `id jd_tdb_max corrected_type catalog_mag
catalog_obscuration`, including boundary-type reclassification.  With `--full
--tz +08:00`, each line is an NDJSON object containing the complete solar
eclipse: compatibility and catalog magnitudes, every contact, time scales,
distances, gamma, and the complete Besselian elements/cubic polynomials.  The
known `JD TDB` is the high-precision greatest-eclipse seed; full mode rebuilds
and validates the remaining parameters around it.

```bash
lunar eclipse-magnitude ./de441.bsp --input maxima.tsv --out eclipses.ndjson --full --tz +08:00
```

## Examples

```bash
lunar eclipse @series --near 2025-09-07 --format json
lunar eclipse ./de442.bsp --kind solar --near 2026-08-12 --format json
lunar eclipse ./de442.bsp --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```
