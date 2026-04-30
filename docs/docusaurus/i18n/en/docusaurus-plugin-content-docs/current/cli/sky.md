---
title: sky
description: Parameters and output for observer-site sky position queries.
---

# sky

Queries apparent positions for the Sun, Moon, and catalog targets at a given observer site and time.

## Syntax

```bash
lunar sky <bsp> <time> --lat <deg> --lon <deg>
lunar sky <bsp> --time <time> --lat <deg> --lon <deg>
  [--height <m>] [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--mode all|pick] [--pick sun,moon,Spica,HR5056,...]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

## Parameters

- `<bsp>`: required ephemeris. This command does not currently use ephemeris auto-fill.
- `<time>` / `--time`: query instant.
- `--lat`: observer latitude, required.
- `--lon`: observer longitude, required.
- `--height`: observer height in meters.
- `--input-tz`: timezone used when input has no suffix.
- `--tz`: display timezone.
- `--mode`: `all` for default targets, `pick` for selected targets.
- `--pick`: target list for `pick` mode, such as `sun,moon,Spica`.
- `--format`: `json`, `txt`, or `csv`.
- `--out`: output file path; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.

## Output

- `json`: input time, observer site, and target array. Target records contain name, equatorial coordinates, azimuth, altitude, and related apparent-position fields.
- `txt`: readable target table.
- `csv`: one row per target. Solar-system targets are listed before catalog stars.

## Examples

```bash
lunar sky @series 2025-06-01T20:00:00+08:00 --lat 31.23 --lon 121.47
lunar sky ./de442.bsp --time 2025-06-01T20:00 --input-tz +08:00 --lat 31.23 --lon 121.47 --mode pick --pick sun,moon,Spica
```
