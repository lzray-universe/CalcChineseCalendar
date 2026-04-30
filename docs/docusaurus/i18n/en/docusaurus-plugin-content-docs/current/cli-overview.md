---
sidebar_position: 4
title: Command-line usage
description: Complete usage reference for the lunar command-line tool, including global options, ephemeris selection, configuration, output formats, and commands.
---

# Command-line usage

`lunar` is the command-line entry point for CalcChineseCalendar. It generates lunar calendar data, solar terms, lunar phases, eclipses, Huangli data, festivals, solar zodiac intervals, and observer-site sky positions. This page follows the current CLI implementation. In the command reference, `[bsp]` means the positional ephemeris argument may be omitted; `<bsp>` means it must be provided explicitly.

## Basic form

```bash
lunar --help
lunar --version
lunar [--lang zh|zht|en|ja|ko] [--eclipse-method modern|legacy] <command> ...
lunar
```

Running without arguments starts interactive mode. Every subcommand supports `--help` for the help text compiled into the current binary.

Compatible legacy syntax:

```bash
lunar <bsp> <years> [months options...]
```

This is equivalent to:

```bash
lunar months <bsp> <years> [months options...]
```

## Global options

```bash
--lang zh|zht|en|ja|ko
--eclipse-method modern|legacy
--bsp <path>
--bsp=<path>
```

- `--lang` controls output language. It also accepts common aliases such as `zh-cn`, `zh-hans`, `zh-tw`, `zh-hant`, `en-us`, `ja-jp`, and `ko-kr`.
- `--eclipse-method` selects the eclipse calculation method. The default is `modern`.
- `--bsp` explicitly supplies the ephemeris path for commands that need ephemerides and support automatic ephemeris insertion.

Examples:

```bash
lunar day 2025-06-01 --bsp ./de442.bsp
lunar --lang en day @series 2025-06-01 --format json
```

## Ephemeris argument

`<bsp>` or `[bsp]` may be:

- a `.bsp` file path, for example `./de442.bsp`
- `@series`
- `series`

`@series` and `series` use the built-in VSOP87A + ELPMPP02 series model. This requires the build option `LUNAR_ENABLE_SERIES_FALLBACK`; the default build enables it.

The following commands allow `[bsp]` to be omitted. When omitted, the CLI selects an ephemeris automatically:

```text
months calendar year event at convert day monthview export next range search eclipse festival almanac info
```

The following commands do not require BSP:

```text
download config completion
```

The following commands still require an explicit positional `<bsp>`:

```text
zodiac sky
```

Automatic ephemeris selection collects and deduplicates candidates in this order:

1. `def_bsp` from config
2. `bsp_list` from config
3. `.bsp` files under `bsp_dir`
4. `.bsp` files under the current working directory

If the command implies a time range, the CLI prefers a BSP that fully covers the range; otherwise it chooses the BSP with the largest overlap. If no BSP is available and series fallback is enabled, it uses `@series`. If series fallback is disabled, the command fails and suggests `--bsp` or `lunar config set def_bsp`.

## Time, timezone, and output

Supported time formats:

```text
YYYY-MM-DD
YYYY-MM-DDZ
YYYY-MM-DD+08:00
YYYY-MM-DDTHH:MM
YYYY-MM-DDTHH:MM:SS
YYYY-MM-DDTHH:MM:SS.sss
```

The time forms above may include a `Z`, `+HH:MM`, or `-HH:MM` timezone suffix. If the input has no timezone suffix, parsing first uses `--input-tz`, then the config value `default_tz`.

Common output options:

```bash
--format txt|json|jsonl|csv|ics|geojson
--out <path>
--pretty 0|1
--quiet
--tz Z|+08:00|-05:00
--lunar-day-tz Z|+08:00|-05:00
```

- `--tz` usually affects only display time. `zodiac --year` is an exception because it also defines the civil-year clipping window.
- `--lunar-day-tz` controls the civil-day boundary used for lunar date mapping. If omitted, the CLI reads `default_lunar_day_tz`; if that is empty, it infers from language, using `+09:00` for `ja` / `ko` and `+08:00` otherwise.
- `geojson` is used only for eclipse-related output.
- Most JSON outputs contain `meta`; common fields include `tool`, `version`, `schema`, `ephem`, `tz_display`, and `notes`.

## Configuration file

The CLI stores defaults in `lun_cfg.txt`.

```bash
lunar config show [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

Supported config keys:

```text
def_bsp
bsp_dir
bsp_list
default_tz
default_lang
default_lunar_day_tz
def_fmt
hli_trad
hli_year_boundary
hli_month_boundary
hli_leap_month_mode
hli_day_boundary
def_prety
```

Notes:

- `bsp_list` accepts comma-separated or semicolon-separated values.
- When `def_bsp` is set, the value is appended to `bsp_list` if it is not already present.
- `default_lang` only allows `zh`, `zht`, `en`, `ja`, and `ko`.
- `def_fmt` allows `txt`, `json`, `csv`, `jsonl`, and `ics`.
- `def_prety` uses `0` or `1`.
- `default_lunar_day_tz` can be cleared with `default`, `auto`, or `inherit` to restore automatic inference.
- `hli_*` values provide default Huangli rules for `day`, `almanac`, and related export commands.

Examples:

```bash
lunar config set def_bsp ./de442.bsp
lunar config set default_tz +08:00
lunar config set default_lang en
lunar config set default_lunar_day_tz +09:00
lunar config set hli_trad xieji
lunar config show --format json
```

## Calendar and date commands

### months

Enumerate lunar months.

```bash
lunar months [bsp] <years>
  [--mode lunar|gregorian]
  [--format json|txt|csv] [--out <path>] [--tz +08:00|Z|-05:00]
  [--pretty 0|1] [--quiet] [--include-eclipses 0|1]
  [--output <json>] [--output-txt <txt>]
```

`<years>` supports `2025`, `2024-2026`, and `2024,2026,2030-2032`. `--output` and `--output-txt` are legacy options and cannot be used together with `--out`.

```bash
lunar months @series 2025
lunar months ./de442.bsp 2024-2026 --mode gregorian --format json --out months.json
```

### calendar

Generate yearly solar-term, lunar-phase, and optional lunar-month data.

```bash
lunar calendar [bsp] [<years>]
  [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]
  [--include-months 0|1] [--include-eclipses 0|1]
  [--pretty 0|1] [--quiet]
```

If `<years>` is omitted, the default is `2025`. `ics` exports only solar-term and lunar-phase events.

```bash
lunar calendar @series 2025
lunar calendar ./de442.bsp 2024-2026 --format ics --out calendar.ics
```

### year

Generate a single-year calendar summary.

```bash
lunar year [bsp] <year>
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]
  [--pretty 0|1] [--quiet]
```

```bash
lunar year @series 2025 --format json --out year-2025.json
```

### day

Query one day for lunar date, Ganzhi, solar-term or lunar-phase events, optional Huangli data, and optional astronomy events.

```bash
lunar day [bsp] <YYYY-MM-DD>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

`--astro-lat` and `--astro-lon` must be provided together. `--trad` and the boundary / mode parameters also accept common ASCII aliases.

```bash
lunar day @series 2025-06-01
lunar day ./de442.bsp 2025-01-31 --trad ziping --format json
lunar day ./de442.bsp 2025-06-01 --astro 1 --astro-lat 31.23 --astro-lon 121.47
```

### monthview

Generate a daily view for one Gregorian month.

```bash
lunar monthview [bsp] <YYYY-MM>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

```bash
lunar monthview @series 2025-09 --format txt
lunar monthview ./de442.bsp 2025-09 --format csv --out month.csv
```

### export

Export daily data for one month, a month range, or a year range.

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

Ranges are inclusive by Gregorian month under `--lunar-day-tz`. `--scope full` enables eclipse events, astronomy events, and all Huangli schools. JSON and JSONL preserve nested daily structures; CSV is a flat summary format.

```bash
lunar export @series 2025-09 --format jsonl --out days.jsonl
lunar export ./de442.bsp --from 2025-01 --to 2025-12 --scope full --format json
```

## Conversion and instant queries

### at

Query full calendar and astronomy data for a specific instant.

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

`--stdin` and `--file` are mutually exclusive. In single-run mode, `--format jsonl` falls back to `json`. `--eot-lon` uses east-positive longitude and outputs apparent minus mean solar time.

```bash
lunar at @series 2025-06-01T00:00:00+08:00 --format json
lunar at ./de442.bsp --file times.txt --format jsonl --meta-once 1
```

### convert

Convert between Gregorian and lunar dates.

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

In `--from-lunar` mode, the positional `<dt_or_tm>` must not be supplied. In batch mode, `--stdin` and `--file` are mutually exclusive.

```bash
lunar convert @series 2025-06-01
lunar convert ./de442.bsp --from-lunar 2025 5 6 --leap 0 --format json
```

## Events, search, and eclipses

### event

Query one solar-term, lunar-phase, or eclipse event.

```bash
lunar event [bsp] solar-term <code> <year>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-phase <new_moon|fst_qtr|full_moon|lst_qtr> --near <YYYY-MM-DD>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-eclipse --near <YYYY-MM-DD>
  [--stage any|umb|total] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet]

lunar event [bsp] solar-eclipse --near <YYYY-MM-DD>
  [--stage any|central] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet]
```

`solar-term` uses `<code>` values `J1..J12` and `Z1..Z12`. `lunar-eclipse` and `solar-eclipse` forward to the `eclipse` command, so validation rules stay consistent.

```bash
lunar event @series solar-term Z2 2025
lunar event @series lunar-phase full_moon --near 2025-09-07
lunar event ./de442.bsp solar-eclipse --near 2026-08-12 --format json
```

### next

Query upcoming events from a starting instant.

```bash
lunar next [bsp] --from <time> --count N
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

`--from` is required, and `--count` must be at least `1`. If `--kinds` is omitted, all event types are used.

```bash
lunar next @series --from 2025-06-01T00:00:00+08:00 --count 5
lunar next ./de442.bsp --from 2025-06-01 --count 10 --format ics --out next.ics
```

### range

Query events inside a time range.

```bash
lunar range [bsp] --from <time> --to <time>
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

`--from` and `--to` are required, and `--to` must not be earlier than `--from`.

```bash
lunar range @series --from 2025-01-01 --to 2025-12-31
lunar range ./de442.bsp --from 2025-02-01 --to 2025-03-01 --format csv
```

### search

Search events with a restricted query expression.

```bash
lunar search [bsp] <query>
  [--from <time>] [--count N] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv|ics|jsonl] [--out <path>]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

Currently, supported queries begin with `next ...`. The query may be passed as one quoted argument or split into multiple CLI words. Supported event names normalize spaces, hyphens, and underscores.

```bash
lunar search @series next full moon --from 2025-06-01
lunar search ./de442.bsp "next lunar eclipse" --from 2025-01-01 --format json
```

### eclipse

Query solar eclipses, lunar eclipses, and the nearest eclipse visible from a point.

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

Use either `--near` or `--visible-near`. `--point-lat` and `--point-lon` must be provided together. `--format geojson` forces global-visibility computation. `--visible-near` does not support `geojson` or global-visibility options.

```bash
lunar eclipse @series --near 2025-09-07 --format json
lunar eclipse ./de442.bsp --kind solar --near 2026-08-12 --format json
lunar eclipse ./de442.bsp --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```

## Huangli, festivals, and astronomy extensions

### festival

Generate festival data for a year.

```bash
lunar festival [bsp] <year>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

```bash
lunar festival @series 2025
lunar festival ./de442.bsp 2025 --format csv --out festival.csv
```

### almanac

Query Huangli summary and auspicious / inauspicious items for a date.

```bash
lunar almanac [bsp] <YYYY-MM-DD>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
  [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
```

```bash
lunar almanac @series 2025-09-17
lunar almanac ./de442.bsp 2025-09-17 --trad xieji --format json
```

### zodiac

Query solar zodiac data.

```bash
lunar zodiac <bsp> --time <time>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]

lunar zodiac <bsp> --year <year>
  [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

Choose one of `--time` or `--year`. Solar zodiac is calculated from geocentric apparent solar ecliptic longitude with light-time correction.

```bash
lunar zodiac @series --time 2025-03-20T18:01:00+08:00
lunar zodiac ./de442.bsp --year 2025 --format csv
```

### sky

Query topocentric sky position for an observer site.

```bash
lunar sky <bsp> <time> --lat <deg> --lon <deg>
lunar sky <bsp> --time <time> --lat <deg> --lon <deg>
  [--height <m>] [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--mode all|pick] [--pick sun,moon,Spica,HR5056,...]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

`--lat` and `--lon` must be provided together. In `--mode pick`, use `--pick` to select output targets. Solar-system targets are listed before catalog stars.

```bash
lunar sky @series 2025-06-01T20:00:00+08:00 --lat 31.23 --lon 121.47
lunar sky ./de442.bsp --time 2025-06-01T20:00 --input-tz +08:00 --lat 31.23 --lon 121.47 --mode pick --pick sun,moon,Spica
```

## Utility commands

### info

Show version, config, ephemeris file status, and coverage interval.

```bash
lunar info [bsp] [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
```

`info` defaults to `txt` and does not use `def_fmt` as its default format.

```bash
lunar info
lunar info ./de442.bsp --format json --out info.json
```

### download

List and download BSP files from the built-in download table.

```bash
lunar download list
lunar download get <id> [--dir <path>] [--quiet]
```

```bash
lunar download list
lunar download get de442s --dir ./ephem
```

### completion

Generate shell completion scripts.

```bash
lunar completion bash|zsh|fish|powershell
```

The script is written to stdout and can be redirected as required by the shell.

```bash
lunar completion powershell > lunar-completion.ps1
lunar completion bash > lunar-completion.bash
```

## Interactive mode

Run `lunar` directly to enter interactive mode. Startup reads `lun_cfg.txt`, checks the default BSP, can scan an extra directory, can download a BSP, and can switch to `@series` when series fallback is enabled.

The interactive menu contains:

```text
1 months
2 calendar
3 at
4 zodiac
5 convert
6 day
7 next
8 festival
9 info
10 monthview
11 range
12 search
13 eclipse
14 almanac
15 config
16 completion
17 sky
18 export
d switch / download BSP
l switch language
h help
q quit
```
