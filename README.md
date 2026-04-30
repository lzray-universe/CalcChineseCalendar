[中文说明](README_zh.md)

# CalcChineseCalendar (lunar)

CalcChineseCalendar is a calendar and astronomy computation tool that prefers JPL DE BSP ephemerides and can automatically fall back to the built-in VSOP87A + ELPMPP02 models when BSP files are unavailable. It provides:

- CLI tool `lunar`
- Shared library `lunar_dll` (exports the C API)
- Reusable C++ computation interfaces (`lunar::core`)
- Python package: `calcchinesecalendar`, install with `pip install calcchinesecalendar` and use via `import calcchinesecalendar`
- npm package: `calcchinesecalendar`, install with `npm install calcchinesecalendar` and use via `import ... from "calcchinesecalendar"`

<div align="center">

  <p>
    <a href="https://github.com/lzray-universe/CalcChineseCalendar/actions/workflows/CI.yml">
      <img src="https://github.com/lzray-universe/CalcChineseCalendar/actions/workflows/CI.yml/badge.svg" alt="CI">
    </a>
    <img src="https://img.shields.io/github/v/release/lzray-universe/CalcChineseCalendar" alt="Release">
    <img src="https://img.shields.io/github/last-commit/lzray-universe/CalcChineseCalendar" alt="Last commit">
  </p>

  <p>
    <a href="https://tokei.kojix2.net/github/lzray-universe/CalcChineseCalendar">
      <img src="https://img.shields.io/endpoint?url=https%3A%2F%2Ftokei.kojix2.net%2Fbadge%2Fgithub%2Flzray-universe%2FCalcChineseCalendar%2Flines" alt="Lines of Code">
    </a>
    <img src="https://img.shields.io/github/repo-size/lzray-universe/CalcChineseCalendar" alt="Repo size">
    <a href="LICENSE">
      <img src="https://img.shields.io/github/license/lzray-universe/CalcChineseCalendar" alt="License">
    </a>
    <a href="https://deepwiki.com/lzray-universe/CalcChineseCalendar">
      <img src="https://deepwiki.com/badge.svg" alt="DeepWiki">
    </a>
  </p>

  <p>
    <img src="https://img.shields.io/badge/platform-Windows-blue" alt="Windows">
    <img src="https://img.shields.io/badge/platform-Linux-green" alt="Linux">
    <img src="https://img.shields.io/badge/platform-macOS-lightgrey" alt="macOS">
    <img src="https://img.shields.io/badge/platform-WebAssembly-orange" alt="WebAssembly">
  </p>

</div>

## Table of Contents

- [1. Feature Overview](#1-feature-overview)
- [2. Build](#2-build)
- [3. Runtime Preparation: BSP Ephemerides (Optional but Preferred)](#3-runtime-preparation-bsp-ephemerides-optional-but-preferred)
- [4. CLI Entry and Global Options](#4-cli-entry-and-global-options)
- [5. Automatic BSP Selection](#5-automatic-bsp-selection)
- [6. Configuration File `lun_cfg.txt`](#6-configuration-file-lun_cfgtxt)
- [7. Time Format and Timezone Rules](#7-time-format-and-timezone-rules)
- [8. Output Formats](#8-output-formats)
- [9. Command Reference](#9-command-reference)
- [10. Interactive Mode](#10-interactive-mode)
- [11. i18n (Simplified Chinese / Traditional Chinese / English / Japanese / Korean)](#11-i18n-simplified-chinese--traditional-chinese--english--japanese--korean)
- [12. C++ API (Library-First)](#12-c-api-library-first)
- [13. C API](#13-c-api)
- [14. Repository Layout](#14-repository-layout)
- [15. Acknowledgements](#15-acknowledgements)
- [16. Quick Examples](#16-quick-examples)
- [17. Output Field Codes](#17-output-field-codes)
- [18. Main Functions and Structure Fields](#18-main-functions-and-structure-fields)

## 1. Feature Overview

- Enumerate lunar months: `months`
- Full-year solar term / lunar phase calendars: `calendar`, `year`
- Solve a single event: `event`
- Download ephemerides: `download`
- Query a specific time and batch processing: `at`
- Gregorian/lunar conversion and batch processing: `convert`
- Solar zodiac and observed sky positions: `zodiac`, `sky`
- Single-day, single-month, and batch day export views: `day`, `monthview`, `export`
- Event lookup: `next`, `range`, `search`
- Lunar and solar eclipses: `eclipse`
- Traditional festivals and almanac summary: `festival`, `almanac`
- Ephemeris information: `info`
- Config management and completion script generation: `config`, `completion`
- Interactive mode (start with no arguments)

## 2. Build

### 2.1 Requirements

- CMake 3.20+
- A compiler with C++20 support
- BSP files (`.bsp`) are preferred at runtime; if not found, the program can automatically fall back to the built-in VSOP87A + ELPMPP02
- `download get` requires `curl` or `wget` to exist on the system

### 2.2 Windows (Visual Studio)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

### 2.3 Linux / macOS (Ninja example)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 2.4 WebAssembly (Emscripten)

```powershell
git clone https://github.com/emscripten-core/emsdk.git D:\tools\emsdk
D:\tools\emsdk\emsdk install latest
D:\tools\emsdk\emsdk activate latest
cmd /c "call D:\tools\emsdk\emsdk_env.bat && D:\tools\emsdk\upstream\emscripten\emcmake.bat cmake -S . -B build_wasm -G Ninja -DLUNAR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release"
cmd /c "call D:\tools\emsdk\emsdk_env.bat && cmake --build build_wasm --parallel"
```

Notes:

- The outputs are `build_wasm/lunar.js` and `build_wasm/lunar.wasm`
- The compatibility wrapper also generates `build_wasm/lunar_worker.js`
- The existing CLI entry is preserved; on Node you can directly run `node build_wasm/lunar.js --version`
- wasm builds enable exception support, filesystem support, exported `FS`/`callMain`, and memory growth by default
- On Node, the current working directory is automatically mounted into the wasm virtual filesystem, so relative `.bsp` paths in the current directory can be used directly
- On Windows, absolute drive-letter paths such as `D:\path\to\file.bsp` are also handled automatically
- In browser workers, `lunar_worker.js` can be used directly and invoked with CLI-style `argv`
- The worker supports two input forms: `files` to write in-memory files, or `mounts` to mount `File`/`Blob` through `WORKERFS`
- `lunar_worker.js` preserves CLI semantics: it exits automatically after a single run; create a new worker for the next command
- If you do not need BSP, you can keep using `@series`

Browser worker example:

```js
const worker=new Worker('./lunar_worker.js');

worker.onmessage=({data})=>{
  if(data.type==='ready'){
    worker.postMessage({
      argv:[
        'day','2025-06-01',
        '--bsp','/ephem/'+spkFile.name,
        '--format','txt'
      ],
      mounts:[
        {path:'/ephem',files:[spkFile]}
      ]
    });
    return;
  }
  if(data.type==='result'){
    console.log(data.exit_code);
    console.log(data.stdout);
    console.error(data.stderr);
  }
};
```

Browser workers can also write in-memory files first:

```js
worker.postMessage({
  argv:[
    'day','2025-06-01',
    '--bsp','/ephem/de442s.bsp',
    '--format','txt'
  ],
  files:[
    {path:'/ephem/de442s.bsp',data:spkArrayBuffer}
  ]
});
```

### 2.5 Outputs

- Executable: `lunar`
- Shared library: `lunar_dll` (output name `lunar`, with platform-specific extension)
- Test executable: `lunar_tests` (when `LUNAR_BUILD_TESTS` is enabled)
- wasm builds additionally generate: `lunar.js`, `lunar.wasm`, `lunar_worker.js`

### 2.6 Optional Build Switches

- `-DLUNAR_ENABLE_DIMENSION_TYPES=ON|OFF`
  - `ON` (default): enables zero-overhead compile-time dimension-tagged vector types
  - `OFF`: falls back to the plain `Vec3` alias without changing interface behavior
- `-DLUNAR_ENABLE_SERIES_FALLBACK=ON|OFF`
  - `ON` (default): automatically switches to built-in VSOP87A + ELPMPP02 when BSP is unavailable
  - `OFF`: keeps the old behavior and requires a valid BSP
- `-DLUNAR_ENABLE_THREADS=ON|OFF`
  - `ON` (default): enables internal multithreaded execution paths
  - `OFF`: keeps single-threaded execution without changing results
- `-DLUNAR_BUILD_TESTS=ON|OFF`
  - `ON` (default): builds gtest/ctest targets
  - `OFF`: does not build test targets
- `-DLUNAR_ENABLE_IPO=ON|OFF`
  - `ON` (default): enables IPO/LTO under Release / RelWithDebInfo
- `-DLUNAR_ENABLE_FAST_MATH=ON|OFF`
  - `OFF` (default): keeps strict floating-point semantics
- `-DLUNAR_VERSION=<text>`
  - Default: `test`
  - `lunar --version`, `info`, and the C API version string all use this value
- Release builds should pass the version through `-DLUNAR_VERSION=<release tag>`; if omitted, the unified version defaults to `test`

### 2.7 Run Tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

If you use a multi-config generator such as Visual Studio, you can use:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

If `LUNAR_ENABLE_SERIES_FALLBACK` is disabled, you can specify the BSP used by tests through the environment variable `LUNAR_TEST_BSP`.

## 3. Runtime Preparation: BSP Ephemerides (Optional but Preferred)

### 3.1 Download List

```bash
lunar download list
```

Currently built-in IDs:

- `de440` (about 114MB, 1550-2650)
- `de440s` (about 31MB, 1850-2150)
- `de441p1` (about 1.5GB, -13200-1969)
- `de441p2` (about 1.5GB, 1969-17191)
- `de442` (about 114MB, 1550-2650)
- `de442s` (about 31MB, 1850-2150)

### 3.2 Download Command

```bash
lunar download get de442s --dir .
```

If your current network environment cannot directly reach the official JPL NAIF site, you can manually download the corresponding BSP file through the mirror `https://naifproxy.lzray.cloud/`. The mirror keeps the same directory structure as the official site, for example `https://naifproxy.lzray.cloud/pub/naif/generic_kernels/spk/planets/de442s.bsp`.

## 4. CLI Entry and Global Options

### 4.1 Main Entry

```bash
lunar --help
lunar --version
lunar <command> [args...]
```

Starting without arguments enters interactive mode.

### 4.2 Global Options

- `--eclipse-method modern|legacy`
- `--lang zh|zht|en|ja|ko`

`--lang` also accepts common aliases:

- `zh-cn`/`cn`/`zh-hans`
- `zh-tw`/`zh-hk`/`zh-hant`/`tw`/`hk`/`hant`
- `en-us`/`us`
- `ja-jp`/`jp`
- `ko-kr`/`kr`

### 4.3 Global BSP Override

All commands that require ephemerides support explicit override:

- `--bsp <path>`
- `--bsp=<path>`

Example:

```bash
lunar day 2025-06-01 --bsp ./de442.bsp
```

### 4.4 Compatible Syntax

The following is also valid and is equivalent to `months`:

```bash
lunar <bsp> <years> [months options...]
```

## 5. Automatic BSP Selection

Before command dispatch, `entry` calls `prep_cmd_args()` to auto-fill `bsp`.

Notes:

- Commands written as `[bsp]` in the reference below allow that positional argument to be omitted; when omitted, the automatic selection logic in this section is used
- `zodiac` and `sky` do not currently use this auto-fill behavior and still require an explicit positional `<bsp>`

### 5.1 Which Commands Require BSP

- `months calendar year event at convert day monthview export next range search eclipse festival almanac info`

Commands that do not require BSP:

- `download config completion`

### 5.2 Candidate Source Order

Candidates are collected and deduplicated in the following order, keeping only files that actually exist:

1. `def_bsp`
2. `bsp_list`
3. All `.bsp` files under `bsp_dir`
4. All `.bsp` files under the current working directory

### 5.3 Selection Strategy

- If the command implies a time range, prefer a BSP that fully covers that range
- If none fully covers the range, choose the BSP with the largest overlap
- If no range can be inferred, use the first item in the candidate list
- If no candidate is available and `LUNAR_ENABLE_SERIES_FALLBACK=ON`, automatically switch to built-in VSOP87A + ELPMPP02
- If no candidate is available and `LUNAR_ENABLE_SERIES_FALLBACK=OFF`, throw an error and suggest `--bsp` or `lunar config set def_bsp`

## 6. Configuration File `lun_cfg.txt`

### 6.1 Supported Keys

- `bsp_dir`
- `def_bsp`
- `bsp_list`
- `default_tz`
- `default_lang`
- `default_lunar_day_tz`
- `def_fmt`
- `hli_trad`
- `hli_year_boundary`
- `hli_month_boundary`
- `hli_leap_month_mode`
- `hli_day_boundary`
- `def_prety`

### 6.2 Commands

```bash
lunar config show [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

### 6.3 Details

- `bsp_list` supports comma-separated or semicolon-separated values
- If `def_bsp` is set and is not already in `bsp_list`, it is automatically appended
- `default_lang` only allows `zh|zht|en|ja|ko`
- `default_lunar_day_tz` overrides the timezone used for lunar day boundaries; if empty, it is inferred from the language default (`ja/ko -> +09:00`, others `+08:00`)
- `def_fmt` allows `txt|json|csv|jsonl|ics`
- `hli_trad`, `hli_year_boundary`, `hli_month_boundary`, `hli_leap_month_mode`, and `hli_day_boundary` act as the default almanac rules for `day` / `almanac`
- The `hli_*` keys above support writing back stable English keys; except for `hli_trad`, they also support `default` to restore built-in defaults
- `def_prety` uses `0|1`

## 7. Time Format and Timezone Rules

### 7.1 `parse_iso` Support

- `YYYY-MM-DD`
- `YYYY-MM-DDZ`
- `YYYY-MM-DD+08:00` / `YYYY-MM-DD-05:00`
- `YYYY-MM-DDTHH:MM`
- `YYYY-MM-DDTHH:MM:SS`
- `YYYY-MM-DDTHH:MM:SS.sss`
- All of the above may also include `Z` or `±HH:MM`

If the input has no timezone suffix, the parser uses the parsing timezone from the command arguments (for example `--input-tz`) or `default_tz` from the config.

### 7.2 `--tz` and Lunar Day Boundaries

- In most commands, `--tz` only affects the display timezone; `zodiac --year` is an exception and also determines the civil-year clipping window
- Lunar day boundaries first use the command-line `--lunar-day-tz`; if not explicitly provided, they read `default_lunar_day_tz` from config, and if that is still absent, infer a language-based default (`ja/ko -> +09:00`, others `+08:00`)

## 8. Output Formats

Commands may output, depending on their supported scope:

- `txt`
- `json`
- `jsonl`
- `csv`
- `ics`
- `geojson` (eclipse-related)

Most JSON outputs contain `meta`, and the core fields usually include:

- `tool`
- `version`
- `schema`
- `ephem`
- `tz_display`
- `notes`

## 9. Command Reference

Notes:

- Commands written as `[bsp]` in this section may omit that positional argument; if explicitly provided, the explicit value takes precedence

### 9.1 months

```bash
lunar months [bsp] <years>
  [--mode lunar|gregorian]
  [--format json|txt|csv] [--out <path>] [--tz +08:00|Z|-05:00]
  [--pretty 0|1] [--quiet] [--include-eclipses 0|1]
  [--output <json>] [--output-txt <txt>]   # deprecated
```

Notes:

- `<years>` supports `2025`, `2024-2026`, `2024,2026,2030-2032`
- When `--include-eclipses 1` is enabled, `--format csv` cannot be used directly
- `--output/--output-txt` are legacy interfaces and cannot be used together with `--out`
- When computing multiple years and `--quiet` is not set, stderr only outputs year progress

### 9.2 calendar

```bash
lunar calendar [bsp] [<years>]
  [--format json|txt|ics] [--out <path>] [--tz ...]
  [--include-months 0|1] [--include-eclipses 0|1] [--pretty 0|1] [--quiet]
```

Notes:

- If `<years>` is omitted, the default is `2025`
- `ics` only exports solar-term and lunar-phase events
- When computing multiple years and `--quiet` is not set, stderr only outputs year progress

### 9.3 year

```bash
lunar year [bsp] <year>
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz ...]
  [--pretty 0|1] [--quiet]
```

### 9.4 event

```bash
lunar event [bsp] solar-term <code> <year>
lunar event [bsp] lunar-phase <new_moon|fst_qtr|full_moon|lst_qtr> --near <YYYY-MM-DD>
  [--format json|txt|ics] [--out <path>] [--tz ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]

# You can also forward directly to the eclipse command:
lunar event [bsp] lunar-eclipse --near <YYYY-MM-DD>
  [--stage any|umb|total] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz ...] [--pretty 0|1] [--quiet]
lunar event [bsp] solar-eclipse --near <YYYY-MM-DD>
  [--stage any|central] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz ...] [--pretty 0|1] [--quiet]
```

Notes:

- `solar-term` uses codes `J1..J12` and `Z1..Z12`
- `lunar-phase` requires `--near`
- `--eclipse 1` appends lunar-eclipse detail fields to full-moon event output
- The `lunar-eclipse` / `solar-eclipse` categories are forwarded directly to `eclipse`, so their parameters and validation rules stay consistent with `eclipse`

### 9.5 download

```bash
lunar download list
lunar download get <id> [--dir <path>] [--quiet]
```

If the official NAIF site is unavailable, you can manually obtain the BSP file from the mirror `https://naifproxy.lzray.cloud/` and then load it through `--bsp`, the config file, or the working directory.

### 9.6 at

```bash
lunar at [bsp] <time>
lunar at [bsp] --time <time>
lunar at [bsp] --stdin
lunar at [bsp] --file <path>
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--events 0|1] [--eot-lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--jobs N] [--meta-once 0|1]
```

Notes:

- In batch mode, `--stdin` and `--file` are mutually exclusive
- In batch mode, if a positional `<time>` is provided at the same time, that positional argument is ignored
- In single-run mode, `--format jsonl` automatically falls back to `json`
- `--jobs` is accepted, but the current executor still runs sequentially
- `--lunar-day-tz` controls the civil-day boundary used for lunar date mapping; the almanac rule parameters share the same parser as `day` / `almanac`
- Default `tz/format/pretty` values come from `lun_cfg.txt` (`default_tz/def_fmt/def_prety`)

### 9.7 convert

```bash
lunar convert [bsp] <dt_or_tm>
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --from-lunar <lunar_year> <month_no> <day> [--leap 0|1]
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --stdin
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--jobs N] [--meta-once 0|1]

lunar convert [bsp] --file <path>
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--jobs N] [--meta-once 0|1]
```

Notes:

- In `--from-lunar` mode, you cannot also pass the positional `<dt_or_tm>`
- `--stdin` and `--file` are mutually exclusive
- In single-run mode, `--format jsonl` automatically falls back to `json`
- `--lunar-day-tz` affects the civil-day boundary used both for Gregorian-to-lunar and lunar-to-Gregorian conversion
- Default `tz/format/pretty` values come from config

### 9.8 day

```bash
lunar day [bsp] <YYYY-MM-DD>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv|jsonl] [--out ...] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

Notes:

- `--astro-lat` and `--astro-lon` must appear together
- `--astro-height` can only be used when latitude and longitude are already provided
- The implementation path is `lunar::core::compute_day` + `lunar::core::format_day_output`
- Default `tz/format/pretty` values come from config
- Additional almanac rule arguments are also supported: `--lunar-day-tz`, `--trad`, `--year-boundary`, `--month-boundary`, `--leap-month-mode`, `--day-boundary`
- `--trad/--year-boundary/--month-boundary/--leap-month-mode/--day-boundary` also accept common ASCII aliases

### 9.9 monthview

```bash
lunar monthview [bsp] <YYYY-MM>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

Notes:

- `--astro-lat` and `--astro-lon` must appear together
- Default `tz/format/pretty` values come from config
- `--lunar-day-tz ...` is also supported to explicitly override the lunar day-boundary timezone

### 9.9.1 export

```bash
lunar export [bsp] <YYYY-MM>
lunar export [bsp] --from <YYYY-MM> --to <YYYY-MM>
lunar export [bsp] --from-year <year> --to-year <year>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|jsonl|csv|txt] [--out ...] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--jobs N] [--events 0|1] [--eclipse 0|1]
  [--scope basic|full] [--huangli off|folk|ziping|purple|xieji|all] [--lon <deg>]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

Notes:

- Ranges are inclusive by civil month under `--lunar-day-tz`
- Each exported day includes Gregorian date, lunar date, year/month/day Ganzhi, and same-day lunar-phase or solar-term events with exact local time when present
- Optional data can add eclipse events, astronomy events, and Huangli payloads; `--huangli all` emits every supported Huangli school
- `--scope full` expands to eclipse events, less-mode astronomy events, and all Huangli schools
- JSON and JSONL preserve nested daily payloads; CSV is a flat summary format
- `--jobs 1` forces the single-thread daily loop; larger values use the threaded daily loop when internal threading is enabled

### 9.10 next

```bash
lunar next [bsp] --from <time> --count N
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz ...] [--format json|txt|csv|ics|jsonl]
  [--out ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

Notes:

- `--from` is required, and `--count >= 1`
- By default, `kinds` is the union of the four categories; the current base event pool comes from solar terms and lunar phases, and `--eclipse 1` appends lunar-eclipse detail fields for full-moon rows
- Default `tz/format/pretty` values come from config

### 9.11 range

```bash
lunar range [bsp] --from <time> --to <time>
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz ...] [--format json|txt|csv|ics|jsonl]
  [--out ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

Notes:

- `--from` and `--to` are required, and `--to >= --from`
- If `--kinds` includes eclipse categories, the corresponding eclipse events are loaded
- Default `tz/format/pretty` values come from config

### 9.12 search

```bash
lunar search [bsp] <query>
  [--from <time>] [--count N] [--tz ...]
  [--format json|txt|csv|ics|jsonl] [--out ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

Notes:

- Currently only queries beginning with `next ...` are supported
- Query words may be passed as one quoted argument (`"next full moon"`) or as
  separate CLI words (`next full moon`); spaces, hyphens, and underscores are
  normalized for supported event names
- Internally the query is parsed into restricted event-search conditions instead of forwarding the full text directly to `next`
- Defaults are `from=current UTC time` and `count=1`; `tz/format/pretty` come from config

### 9.13 eclipse

```bash
lunar eclipse [bsp] --near <YYYY-MM-DD> [--kind lunar|solar]
  [--stage any|umb|total] (lunar)
  [--stage any|central]   (solar)
  [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1]
  [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--tz ...] [--format json|txt|geojson] [--out ...] [--pretty 0|1] [--quiet]

lunar eclipse [bsp] --visible-near <time> --point-lat <deg> --point-lon <deg>
  [--kind lunar|solar|both] [--visible-years <years>]
  [--stage any|umb|total|central] [--sample-min <minutes>] [--point-height <m>]
  [--tz ...] [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
```

Notes:

- Either `--near` or `--visible-near` is required
- `--kind` defaults to `lunar`; with `--visible-near`, omitted `--kind` searches both lunar and solar eclipses
- `--point-lat` and `--point-lon` must appear together
- `--point-height` and `--point-refine` can only be used together with `--point-lat` and `--point-lon`
- `--visible-near` finds the closest eclipse visible at the supplied point and requires `--point-lat` plus `--point-lon`
- `--visible-near` is mutually exclusive with `--near`, and does not support `--global-vis` or `--format geojson`
- `--global` is a compatibility alias of `--global-vis`
- `--global-format`, `--grid-lat-step`, and `--grid-lon-step` require `--global-vis 1`, or using `--format geojson` directly
- `--format geojson` forces global-visibility computation on
- Default `tz/format/pretty` values come from config; when `def_fmt` is not `json|txt|geojson`, it falls back to `json`

### 9.14 festival

```bash
lunar festival [bsp] <year>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
```

Default `tz/format/pretty` values come from config.

Additional notes:

- `--lunar-day-tz ...` is also supported to explicitly override the lunar day-boundary timezone

### 9.15 almanac

```bash
lunar almanac [bsp] <YYYY-MM-DD>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet] [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
```

Default `tz/format/pretty` values come from config.

Additional notes:

- `--lunar-day-tz`, `--trad`, `--year-boundary`, `--month-boundary`, `--leap-month-mode`, and `--day-boundary` are also supported
- Common aliases such as `--trad xieji` and `--trad old_almanac` are available; the various boundary / mode parameters also accept stable English keys and common ASCII aliases

### 9.16 info

```bash
lunar info [bsp] [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
```

Notes:

- The default format is always `txt` and does not read `def_fmt`
- Output includes file existence, size, SPK coverage interval, and related information

### 9.17 config

```bash
lunar config show [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

### 9.18 completion

```bash
lunar completion bash|zsh|fish|powershell
```

The script is written to stdout and can be redirected to a file.

### 9.19 zodiac

```bash
lunar zodiac <bsp> --time <time>
  [--input-tz ...] [--tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]

lunar zodiac <bsp> --year <year>
  [--tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
```

Notes:

- These two commands still specify the ephemeris through the positional `<bsp>` argument and do not use the earlier auto-fill semantics used by `day/range/...`
- Choose one of `--time` or `--year`
- `--input-tz` is only valid in `--time` mode
- `--time` mode outputs the current solar zodiac, ecliptic boundary interval, elapsed time, and remaining time at the given instant
- `--year` mode outputs the 12 zodiac intervals clipped to the civil-year window under the current display timezone, together with the clipped duration of each interval
- Solar zodiac is calculated from the geocentric apparent solar ecliptic longitude, including light-time correction

### 9.20 sky

```bash
lunar sky <bsp> <time> --lat <deg> --lon <deg>
lunar sky <bsp> --time <time> --lat <deg> --lon <deg>
  [--height <m>] [--input-tz ...] [--tz ...]
  [--mode all|pick] [--pick sun,moon,Spica,HR5056,...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
```

Notes:

- This command still specifies the ephemeris through the positional `<bsp>` argument
- Output is the topocentric apparent position at the given time and observer site
- `--lat` and `--lon` must be provided together
- In `--mode pick`, you can use `--pick` to specify a subset such as `sun,moon,Spica`
- Solar-system targets are output before catalog stars

## 10. Interactive Mode

### 10.1 Start

Run `lunar` directly with no arguments.

### 10.2 Startup Flow

- Read `lun_cfg.txt`
- If `def_bsp` exists, prompt whether to continue using it
- Optionally specify an extra directory to scan for `.bsp`
- If no usable ephemeris is found, enter the download screen
- If `LUNAR_ENABLE_SERIES_FALLBACK` is enabled, interactive mode can also switch to `@series`

### 10.3 Menu

- `1` months
- `2` calendar
- `3` at
- `4` zodiac
- `5` convert
- `6` day
- `7` next
- `8` festival
- `9` info
- `10` monthview
- `11` range
- `12` search
- `13` eclipse
- `14` almanac
- `15` config
- `16` completion
- `17` sky
- `18` export
- `d` switch / download BSP
- `l` switch language
- `h` help
- `q` quit

## 11. i18n (Simplified Chinese / Traditional Chinese / English / Japanese / Korean)

### 11.1 Language Sources

- Command line: `--lang zh|zht|en|ja|ko`
- Config default: `default_lang`
- Interactive mode: `l` in the main menu can switch immediately and write back to `default_lang`

Priority: command line overrides config default.

### 11.2 Coverage

- Interactive UI text (`src/i18n/catalog_interact.cpp`)
- Event name translation (`src/i18n/catalog_event.cpp`)
- Body / astronomy-related text (`src/i18n/catalog_astro.cpp`)
- Almanac field translation (`src/i18n/catalog_almanac.cpp`)
- Some CLI prompts and notes

Structured JSON key names remain stable and do not change with language.

## 12. C++ API (Library-First)

### 12.1 Headers

- `include/lunar/core.hpp`
- `include/lunar/models.hpp`
- `include/lunar/day_formatter.hpp`

### 12.2 Typical Call

```cpp
#include "lunar/core.hpp"
#include "lunar/day_formatter.hpp"

lunar::core::DayComputeOptions opt;
opt.ephem = "./de442.bsp";
opt.date_text = "2025-06-01";
opt.tz = "+08:00";
opt.include_events = true;

DayResult result = lunar::core::compute_day(opt);
lunar::core::format_day_output(std::cout, result, "json", true);
```

`AtData` and `DayResult` are publicly declared in `include/lunar/models.hpp`.

## 13. C API

Header: `include/lunar/c_api.h`

### 13.1 Common Interfaces

- `lunar_tool_ver`
- `lunar_last_error`
- `lunar_clear_error`
- `lunar_run`
- `lunar_root_batch`

### 13.2 Core Computation Interfaces

- `lunar_calc_eot`
- `lunar_core_day`

### 13.3 Command Wrapper Interfaces

Currently wrapped:

- `month cal year event dl at conv zodiac day mview export next range search eclipse fest alm info cfg comp`

Additional notes:

- Extra `zodiac` wrappers are now exported as `lunar_cmd_zodiac` and `lunar_use_zodiac`
- `export` and `eclipse` are available as standalone command wrappers
- `sky` is still recommended to be called through `lunar_run`

### 13.4 Return Codes

- `0`: success
- `2`: argument error (`std::invalid_argument`)
- `1`: other errors

## 14. Repository Layout

```text
include/lunar/          Public headers (CLI/C++ API/C API)
src/cli/                months/calendar/year/event/download implementation
src/query/              at/convert/day/monthview/next/... implementation
src/emscripten/         wasm preload scripts and worker wrapper
src/i18n/               i18n catalogs and entries
src/interact.cpp        Interactive mode
src/global_context.cpp  Global config and automatic BSP selection
src/entry.cpp           Top-level CLI argument parsing and command dispatch
src/cli.cpp             Aggregated build entry for the cli module
src/query.cpp           Aggregated build entry for the query module
src/c_api.cpp           C API export implementation
tests/                  GoogleTest / CTest cases
vsop87a/                Planetary series fallback model
elpmpp02/               Lunar series fallback model
lun_cfg.txt             Runtime config file
```

## 15. Acknowledgements

- Thanks to [@ytliu0](https://github.com/ytliu0) for long-term contributions to Chinese calendar and astronomical calculation materials
- Thanks to JPL NAIF for publicly releasing the DE BSP ephemerides that provide high-quality base data for this project
- Thanks to the Hipparcos (HIP) catalog and related public data compilations, which serve as the reference basis for the star entries and numbering system used by this project

## 16. Quick Examples

```bash
# Set the default ephemeris and language
lunar config set def_bsp ./de442.bsp
lunar config set default_lang zh

# New syntax: no need to enter <bsp> manually
lunar day 2025-06-01 --format json

# Override the ephemeris for this run
lunar day 2025-06-01 --bsp ./de440s.bsp --format txt

# Range events
lunar range --from 2025-01-01 --to 2025-12-31 --format json --kinds solar_term,lunar_phase

# Batch daily export
lunar export --from 2025-01 --to 2025-12 --scope full --format jsonl --out days.jsonl

# Solar zodiac
lunar zodiac ./de442.bsp --time 2025-03-20T18:01:00+08:00 --format json

# Sky positions at a specific site
lunar sky ./de442.bsp --time 2025-06-01T20:00 --input-tz +08:00 --lat 31.23 --lon 121.47 --mode pick --pick sun,moon,Spica

# Eclipse calculation
lunar eclipse --near 2025-09-07 --kind lunar --global-vis 1 --global-format geojson --format json

# Nearest visible eclipse at a site
lunar eclipse --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```

## 17. Output Field Codes

Notes:

- This section specifically explains abbreviations, codes, and fields such as `*_code`, `*_key`, `*_codes`, and `*_mask_hex` in structured output
- Obvious semantic fields such as `year`, `month`, `name`, `data`, `events`, and `input` are not repeated here
- Field names follow the current implementation in `src/query/core/base.cpp`, `src/query/core/events.cpp`, `src/query/day_output.cpp`, `src/cli/output.cpp`, `src/query/cmd_day_mview.cpp`, `src/query/cmd_export.cpp`, and `src/query/cmd_eclipse.cpp`

### 17.1 Common Time and Sampling Fields

| Field | Main locations | Description |
| --- | --- | --- |
| `jd_utc` | General | UTC Julian day. |
| `jd_tdb` | General | TDB Julian day. |
| `utc_iso` | General | UTC ISO 8601 time. |
| `loc_iso` | General | ISO 8601 time converted into the current display timezone. |
| `tz_display` | `meta` | Display timezone used by the current output. |
| `input_tz` | `at.input` | Parsing timezone applied to the original input string. |
| `lunar_day_tz` | `at/day/almanac/monthview/export` | Timezone used for lunar day boundaries. |
| `smp_time` | `day.input` | Sampling time used by `day` within that date, default `12:00:00`. |
| `smp_jdutc` | `day.input` | UTC Julian day of the `day` sample point. |
| `smp_uiso` | `day.data` / `day.csv` / `export.csv` | UTC ISO time of the sample point. |
| `smp_liso` | `day.data` / `day.csv` / `export.csv` | ISO time of the sample point under the display timezone. |
| `st_jdutc` / `ed_jdutc` | `months` | UTC Julian day of the lunar month start / end. |
| `st_utc` / `ed_utc` | `months` | UTC ISO time of the lunar month start / end. |
| `st_loc` / `ed_loc` | `months` | Local ISO time of the lunar month start / end under the display timezone. |
| `jd_start_utc` / `jd_end_utc` | `eclipse.global_vis` | Start / end UTC Julian day of the global-visibility sampling window. |
| `utc_start_iso` / `utc_end_iso` | `eclipse.global_vis` | Start / end UTC ISO time of the global-visibility sampling window. |
| `loc_start_iso` / `loc_end_iso` | `eclipse.global_vis` | Start / end local ISO time of the global-visibility sampling window. |

### 17.2 Astronomical Geometry and Lunar-Phase Fields

| Field | Main locations | Description |
| --- | --- | --- |
| `lam_s` / `sun_lam` | `at.data` | Apparent solar ecliptic longitude in radians. |
| `lam_m` / `moon_lam` | `at.data` | Apparent lunar ecliptic longitude in radians. |
| `elong` / `elongation_rad` | `at.data` | Solar-lunar ecliptic longitude difference in radians. |
| `elong_deg` / `elongation_deg` | `at.data` | Solar-lunar ecliptic longitude difference in degrees. |
| `ill_frac` | `at/day` | Illuminated lunar fraction, usually within `0..1`. |
| `ill_pct` | `at/day/monthview/export` | Illuminated lunar percentage. |
| `phase_name` | `at/day` | Lunar phase name. |
| `moon_dist_km` | `event/eclipse` | Earth-Moon distance in kilometers. |
| `moon_xg` | `at/day` | Summary object for the moon's star-lodge position. |
| `moon_xg_region` | `monthview` | Name of the moon's star-lodge region. |
| `moon_xg_star` | `monthview` | Reference star name used to match that region. |
| `sep_deg` / `moon_xg_sep_deg` | `moon_xg` / `monthview` | Angular separation between the moon and the reference star, in degrees. |
| `eot` | `at.data` | Equation-of-time object. |
| `eot_lon_deg` | `at.input` | Longitude in degrees used when calculating the equation of time. |
| `lon_deg` | `day.input` / `huangli` / `eot` | Longitude in degrees used by almanac and true-solar-time calculations. |
| `eot_minutes` | `eot` / `huangli` | Equation of time in minutes. |
| `eot_seconds` | `eot` | Equation of time in seconds. |
| `true_solar_minutes` | `huangli` | Local true solar time expressed as minutes. |
| `ra_deg` | `sky/eclipse` | Right ascension in degrees. |
| `dec_deg` | `sky/eclipse` | Declination in degrees. |
| `az_deg` | `sky` | Azimuth in degrees. |
| `alt_deg` | `sky` | Altitude in degrees. |
| `mag_v` | `sky` | Apparent magnitude in the V band. |
| `region` | `sky` | Star-lodge or region name. |
| `is_solar_system` | `sky` | Whether the target belongs to the solar system. |
| `is_juxing` | `sky` | Whether the entry is a grouped-star record. |
| `sun_lam_deg` | `zodiac` | Apparent solar ecliptic longitude in degrees. |
| `sign_index` / `sign_order` | `zodiac` | Zero-based sign index / one-based display order. |
| `sign_code` / `sign_name` | `zodiac` | Stable sign code / localized sign name. |
| `term_code` | `zodiac` | Solar-term code corresponding to the sign start. |
| `start_lambda_deg` / `end_lambda_deg` | `zodiac` | Ecliptic longitude boundaries of that sign interval, in degrees. |
| `sign_offset_deg` | `zodiac.point` | Ecliptic longitude offset already traversed inside the current sign interval, in degrees. |
| `elapsed_sec` / `remain_sec` / `span_sec` | `zodiac.point` | Elapsed / remaining / total duration in seconds. |
| `interval_count` | `zodiac.year` | Number of intervals in the year summary, usually 12. |
| `in_year_start_loc_iso` / `in_year_end_loc_iso` | `zodiac.year` | Local start / end time of that sign interval clipped to the civil-year window of the current display timezone. |
| `in_year_dur_sec` / `in_year_dur_days` | `zodiac.year` | Duration of that interval inside the current civil-year window. |
| `clipped_start` / `clipped_end` | `zodiac.year` | Whether the interval start / end is clipped by the year window. |
| `sd_deg` | `eclipse` | Apparent semidiameter in degrees. |
| `ehp_deg` | `eclipse` | Equatorial horizontal parallax in degrees. |

### 17.3 Lunar Date, Ganzhi, and Bazi Fields

| Field | Main locations | Description |
| --- | --- | --- |
| `lun_mno` | `lunar_date` | Lunar month number. |
| `lun_mlab` / `lun_m_label` | `lunar_date` / `monthview.csv` | Lunar month label text, such as First Month or Leap Second Month. |
| `lun_label` | `lunar_date` / `monthview` | Full lunar date label. |
| `is_leap` | General | Whether the month is leap; boolean or `0/1`. |
| `gz` | almanac/ganzhi | Short Ganzhi text, commonly seen in fields such as `m_gz`, `d_gz`, and `h_gz`. |
| `y_lun_gz` / `year_lunar` | `day.csv` / `huangli` | Year Ganzhi computed using the lunar new year. |
| `y_lchun_gz` / `year_lchun` | `day.csv` / `huangli` | Year Ganzhi computed using Li Chun. |
| `y_rule_gz` / `year_rule` | `day.csv` / `huangli` | Year Ganzhi actually used by the current rule set. |
| `m_gz` / `month` | `day.csv` / `huangli` | Month Ganzhi. |
| `d_gz` / `day` | `day.csv` / `huangli` | Day Ganzhi. |
| `h_gz` / `hour_clock` | `day.csv` / `huangli` | Hour Ganzhi based on clock time. |
| `h_true_gz` / `hour_true_solar` | `day.csv` / `huangli` | Hour Ganzhi based on true solar time. |
| `index` | `GzNode` output | Index within the 60-cycle Ganzhi sequence. |
| `stem` | `GzNode` output | Heavenly stem index. |
| `branch` | `GzNode` output | Earthly branch index. |
| `bazi_clock` | `huangli` | Bazi string generated from clock time. |
| `bazi_true` | `huangli` | Bazi string generated from true solar time. |

### 17.4 Almanac, Deities, and Yi/Ji Fields

| Field | Main locations | Description |
| --- | --- | --- |
| `rule_profile` | `huangli` | Name of the almanac rule profile. |
| `year_boundary` | `huangli` | Rule used to change the year pillar. |
| `month_boundary` | `huangli` | Rule used to change the month pillar. |
| `leap_month_mode` | `huangli` | How leap months are handled in the month pillar. |
| `day_boundary` | `huangli` | Rule used to change the day pillar. |
| `jianchu` | `huangli` | One of the twelve Jianchu values. |
| `duty_god` | `huangli` | The day's duty deity. |
| `duty_is_yellow` | `huangli` | Whether the duty deity belongs to the yellow path. |
| `duty_tag` | `huangli` | Simplified category tag of the duty deity. |
| `clash` | `huangli` | Clash description for the day branch. |
| `chong_sha` | `huangli` | Combined clash-and-sha description. |
| `zodiac_day` | `huangli` | Zodiac animal of the day. |
| `six_he` | `huangli` | Corresponding branch in the six-harmony pair. |
| `three_he` | `huangli` | Three-harmony group. |
| `pengzu` | `huangli` | Pengzu taboo text. |
| `nayin` | `huangli` | Nayin. |
| `wuxing_day` / `wx_day` | `huangli` / struct | Five-element description for the day. |
| `fetal_god` | `huangli` | Fetal deity direction description. |
| `meridian` | `huangli` | Meridian assigned to the day. |
| `lucky_dir` | `huangli` | Lucky deity direction. |
| `wealth_dir` | `huangli` | Wealth deity direction. |
| `mascot_dir` | `huangli` | Fortune deity direction. |
| `sun_noble_dir` | `huangli` | Yang noble deity direction. |
| `moon_noble_dir` | `huangli` | Yin noble deity direction. |
| `xiu28` | `huangli` | Name of the 28 xiu. |
| `xiu28_mod28` | `huangli` | Xiu-order text produced by the mod-28 rule. |
| `xiu_star` | `huangli` | Stable identifier of the 28-xiu star. |
| `good_gods` | `huangli` | Array of auspicious deity names. |
| `bad_gods` | `huangli` | Array of inauspicious deity names. |
| `yi` | `huangli` | Array of recommended activities. |
| `ji` | `huangli` | Array of forbidden activities. |
| `yi_ji_level` | `huangli` | Strength level of the Yi/Ji result. |
| `yi_ji_rule` | `huangli` | Rule description used to determine Yi/Ji. |
| `hour_jx` | `huangli` | Array of hourly luck results. |
| `slot` | `hour_jx` | Time slot label, such as Zi hour or Chou hour. |
| `slot_index` | `hour_jx` | Index of the time slot. |
| `gz_index` | `hour_jx` | 60-cycle Ganzhi index of that hour slot. |
| `luck` | `hour_jx` | Hourly luck text. |
| `is_bad` | `hour_jx` | Whether that hour slot is inauspicious. |

### 17.5 Event, Nearby Event, and Summary Fields

| Field | Main locations | Description |
| --- | --- | --- |
| `kind` | `event` / `near_ev` / `eclipse` / `export` | Machine-usable event category. |
| `code` | `event` / `near_ev` | Stable code inside that event category. |
| `st_prev` / `st_next` | `near_ev` | Previous / next solar-term event. |
| `lp_prev` / `lp_next` | `near_ev` | Previous / next lunar-phase event. |
| `ev_sum` | `monthview` | Daily event-name summary joined by `|`. |
| `astro_ev_sum` | `monthview` | Daily astronomy-event summary joined by `|`. |
| `stage_window` | `eclipse.visibility` | Stage window used by visibility statistics, such as `any`, `umb`, `total`, or `central`. |
| `sample_count` | `eclipse.visibility` | Number of visibility samples. |
| `first_visible` / `last_visible` | `eclipse.visibility` | Earliest / latest visible instant at the site or grid point. |
| `visible` | `eclipse.visibility` | Whether it is visible. |
| `has_eclipse` | `solar.point_vis` | Whether an actual solar eclipse occurs at the site. |
| `central` | `solar.point_vis` | Whether the site lies on the central eclipse path. |
| `eclipses` | `export.day` | Eclipse events assigned to the exported civil day. |
| `astro_events` | `export.day` | Astronomy events assigned to the exported civil day. |
| `huangli` | `export.day` | Huangli payload for the selected school or all schools. |

### 17.6 Eclipse-Related Fields

| Field | Main locations | Description |
| --- | --- | --- |
| `pen_mag` | `lunar_eclipse` | Penumbral magnitude. |
| `umb_mag` | `lunar_eclipse` | Umbral magnitude. |
| `mag` | `solar_eclipse` | Geocentric apparent-disk eclipse magnitude; use `max_mag` for site or grid magnitude. |
| `obscuration` | `solar_eclipse` | Geocentric apparent-disk obscuration fraction. |
| `gamma` | `lunar_eclipse` / `solar_eclipse` | Normalized offset of eclipse center relative to the shadow axis. |
| `eps_deg` | `lunar_eclipse` | Geometric angle parameter of the lunar eclipse, in degrees. |
| `rp_re` | `eclipse` | Shadow-zone radius parameter in Earth radii. |
| `ru_re` | `eclipse` | Another shadow-zone radius parameter in Earth radii. |
| `opp_rp_re` / `opp_ru_re` | `lunar_eclipse` | Shadow-zone radius parameters at opposition, in Earth radii. |
| `dur_pen_sec` | `lunar_eclipse` | Duration of the penumbral stage in seconds. |
| `dur_umb_sec` | `lunar_eclipse` | Duration of the umbral stage in seconds. |
| `dur_tot_sec` | `lunar_eclipse` | Duration of totality in seconds. |
| `dt_max_sec` | `eclipse` | `TDB-UTC` difference at maximum eclipse, in seconds. |
| `sep_max_deg` | `solar_eclipse` | Angular separation between sun and moon centers at maximum eclipse, in degrees. |
| `sun_sd_max_deg` | `solar_eclipse` | Apparent solar semidiameter at maximum eclipse, in degrees. |
| `moon_sd_max_deg` | `solar_eclipse` | Apparent lunar semidiameter at maximum eclipse, in degrees. |
| `besselian` | `solar_eclipse` | Solar-eclipse Besselian element object with fundamental-plane coordinates, shadow radii, cone angles, and time polynomials. |
| `x` / `y` | `solar_eclipse.besselian` | Shadow-axis coordinates in the fundamental plane at maximum eclipse, in Earth radii. |
| `d_deg` / `mu_deg` | `solar_eclipse.besselian` | Declination and Greenwich hour angle of the shadow axis, in degrees. |
| `l1` / `l2` | `solar_eclipse.besselian` | Penumbral / umbral shadow radius in the fundamental plane, in Earth radii. |
| `tan_f1` / `tan_f2` | `solar_eclipse.besselian` | Tangents of the penumbral / umbral cone angles. |
| `x_dot` / `y_dot` / `d_dot_deg` / `mu_dot_deg` / `l1_dot` / `l2_dot` | `solar_eclipse.besselian` | Hourly derivatives of the corresponding Besselian elements. |
| `coefficients` | `solar_eclipse.besselian` | Cubic polynomial coefficient arrays `[c0,c1,c2,c3]`, evaluated in TDB hours from `epoch`. |
| `sun_geo` / `moon_geo` | `lunar_eclipse` | Geometric-parameter objects for the sun / moon. |
| `lib` | `lunar_eclipse` | Lunar libration parameter object. |
| `l_deg` / `b_deg` / `c_deg` | `lib` | Lunar libration angular parameters, in degrees. |
| `p1` / `u1` / `u2` / `u3` / `u4` / `p4` | `lunar_eclipse` | Node objects for each contact instant of the lunar eclipse. |
| `opp` | `lunar_eclipse` | Node object for full moon / opposition. |
| `max` | `eclipse` | Node object for maximum eclipse. |
| `c1` / `c2` / `c3` / `c4` | `solar_eclipse` / `solar.point_vis` | Node objects for first contact / second contact / third contact / fourth contact of the solar eclipse. |
| `solar_eclipse_c1_loc_iso` / `solar_eclipse_c2_loc_iso` / `solar_eclipse_max_loc_iso` / `solar_eclipse_c3_loc_iso` / `solar_eclipse_c4_loc_iso` | `next/range/search.csv` | Flat local ISO columns for solar-eclipse contact nodes. |
| `zen_lat_deg` / `zen_lon_deg` | `eclipse.node` | Zenith latitude / longitude corresponding to that node. |
| `pa_deg` | `eclipse.node` | Position angle of that node. |
| `axis_deg` | `eclipse.node` | Axis-angle parameter of that node. |
| `max_mag` | `solar.point_vis` / `solar.global_vis` | Maximum eclipse magnitude at the site or grid point. |
| `max_obscuration` | `solar.point_vis` | Maximum obscuration fraction at the site. |
| `max_sun_alt_deg` | `solar.point_vis` / `solar.global_vis` | Solar altitude at maximum eclipse. |
| `visible_target` | `eclipse.visible_near` | Target instant and distance metadata for nearest-visible search. |
| `visible_delta_days` | `eclipse.visible_near` | Absolute distance in days between the target time and the selected visible eclipse. |

### 17.7 `code` / `key` / `mask` Fields

| Field | Corresponding text field | Description |
| --- | --- | --- |
| `rule_profile_code` | `rule_profile` | Integer enum value of the almanac rule profile. |
| `rule_profile_key` | `rule_profile` | Stable English key of the almanac rule profile. |
| `year_boundary_code` | `year_boundary` | Integer enum value of the year-boundary rule. |
| `year_boundary_key` | `year_boundary` | Stable English key of the year-boundary rule. |
| `month_boundary_code` | `month_boundary` | Integer enum value of the month-boundary rule. |
| `month_boundary_key` | `month_boundary` | Stable English key of the month-boundary rule. |
| `leap_month_mode_code` | `leap_month_mode` | Integer enum value of the leap-month handling rule. |
| `leap_month_mode_key` | `leap_month_mode` | Stable English key of the leap-month handling rule. |
| `day_boundary_code` | `day_boundary` | Integer enum value of the day-boundary rule. |
| `day_boundary_key` | `day_boundary` | Stable English key of the day-boundary rule. |
| `jianchu_code` | `jianchu` | Integer code of the Jianchu value. |
| `duty_god_code` | `duty_god` | Integer code of the duty deity. |
| `duty_tag_code` | `duty_tag` | Code of the broad duty-tag category. |
| `clash_branch_code` | `clash` | Earthly branch code corresponding to the clash branch. |
| `sha_dir_code` | `chong_sha` | Direction code of the Sha direction. |
| `zodiac_day_code` | `zodiac_day` | Zodiac-animal code of the day. |
| `six_he_branch_code` | `six_he` | Earthly branch code of the six-harmony counterpart. |
| `three_he_group_code` | `three_he` | Code of the three-harmony group. |
| `nayin_code` | `nayin` | Nayin code. |
| `fetal_god_code` | `fetal_god` | Direction code of the fetal deity. |
| `meridian_code` | `meridian` | Meridian code. |
| `lucky_dir_code` | `lucky_dir` | Lucky deity direction code. |
| `wealth_dir_code` | `wealth_dir` | Wealth deity direction code. |
| `mascot_dir_code` | `mascot_dir` | Fortune deity direction code. |
| `sun_noble_dir_code` | `sun_noble_dir` | Yang noble deity direction code. |
| `moon_noble_dir_code` | `moon_noble_dir` | Yin noble deity direction code. |
| `xiu28_code` | `xiu28` | 28-xiu code. |
| `xiu28_mod28_code` | `xiu28_mod28` | mod-28 xiu-order code. |
| `good_god_codes` | `good_gods` | Integer-code array corresponding to the array of auspicious deity names. |
| `bad_god_codes` | `bad_gods` | Integer-code array corresponding to the array of inauspicious deity names. |
| `yi_codes` | `yi` | Integer-code array corresponding to the array of recommended activities. |
| `ji_codes` | `ji` | Integer-code array corresponding to the array of forbidden activities. |
| `good_god_mask_hex` | `good_gods` | Bitmap of auspicious deities as a hexadecimal string. |
| `bad_god_mask_hex` | `bad_gods` | Bitmap of inauspicious deities as a hexadecimal string. |
| `yi_mask_hex` | `yi` | Bitmap array of recommended activities as hexadecimal strings. |
| `ji_mask_hex` | `ji` | Bitmap array of forbidden activities as hexadecimal strings. |
| `yi_ji_rule_code` | `yi_ji_rule` | Code of the Yi/Ji decision rule. |

## 18. Main Functions and Structure Fields

Notes:

- This section only lists public headers and the main interfaces directly involved in the current README; small internal helper functions are not expanded
- Interface declarations follow `include/lunar/core.hpp`, `include/lunar/models.hpp`, `include/lunar/cli.hpp`, `include/lunar/cli_query.hpp`, `include/lunar/day_formatter.hpp`, and `include/lunar/c_api.h`

### 18.1 Main Functions

| Function | Location | Purpose |
| --- | --- | --- |
| `run_cli_args` | `include/lunar/entry.hpp` | Top-level CLI entry that receives an argument array and performs top-level dispatch. |
| `cli_month` / `cli_cal` / `cli_year` / `cli_event` / `cli_dl` | `include/lunar/cli.hpp` | Legacy CLI entries that use argument structs. |
| `cli_at` / `cli_conv` | `include/lunar/cli_query.hpp` | Legacy CLI entries with argument structs for `at` / `convert`. |
| `cmd_month` / `cmd_cal` / `cmd_year` / `cmd_event` / `cmd_dl` | `include/lunar/cli.hpp` | Command entries that directly receive string arrays. |
| `cmd_at` / `cmd_conv` / `cmd_zodiac` / `cmd_sky` / `cmd_day` / `cmd_mview` / `cmd_export` / `cmd_next` / `cmd_range` / `cmd_search` / `cmd_eclipse` / `cmd_fest` / `cmd_alm` / `cmd_info` / `cmd_cfg` / `cmd_comp` | `include/lunar/cli_query.hpp` | Direct entries of each subcommand. |
| `lunar::calc_sky_pos` | `include/lunar/star.hpp` | Computes the sky-position list for a given time and observer site. |
| `calc_solar_zodiac_at` / `calc_solar_zodiac_year` | `include/lunar/solar_zodiac.hpp` | Computes the solar zodiac at a single instant or a full-year zodiac interval summary. |
| `lunar::core::compute_day` | `include/lunar/core.hpp` | Computes lunar date, almanac, lunar phase, events, and optional astronomy data for a given day. |
| `lunar::core::format_day_output` | `include/lunar/day_formatter.hpp` | Formats `DayResult` as `json/txt/csv/jsonl`. |
| `lunar::core::compute_ganzhi` | `include/lunar/core.hpp` | Computes the year/month/day Ganzhi summary for a single instant. |
| `lunar::core::compute_ganzhi_month` | `include/lunar/core.hpp` | Computes the day-by-day Ganzhi summary for an entire month. |
| `lunar_tool_ver` | `include/lunar/c_api.h` | Returns the library / tool version string. |
| `lunar_last_error` / `lunar_clear_error` | `include/lunar/c_api.h` | Reads or clears the latest C API error message. |
| `lunar_run` | `include/lunar/c_api.h` | Calls the top-level CLI entry in C style. |
| `lunar_root_batch` | `include/lunar/c_api.h` | Batch root-solving entry. |
| `lunar_calc_eot` | `include/lunar/c_api.h` | Computes the equation of time and writes it into `lunar_eot_result`. |
| `lunar_core_day` | `include/lunar/c_api.h` | Computes the single-day core summary and writes it into `lunar_day_summary`. |
| `lunar_hli_rules_init` | `include/lunar/c_api.h` | Initializes `lunar_hli_rules` with default values. |
| `lunar_core_ganzhi` | `include/lunar/c_api.h` | Computes the Ganzhi summary for a single instant and writes it into `lunar_ganzhi_summary`. |
| `lunar_core_ganzhi_month` | `include/lunar/c_api.h` | Computes the month-long Ganzhi summary and writes it into `lunar_ganzhi_month_summary`. |

### 18.2 CLI Argument Structures

| Structure | Key fields | Description |
| --- | --- | --- |
| `MonthsArgs` | `ephem`, `years`, `mode`, `format`, `out`, `tz`, `pretty`, `quiet` | Parameters of the `months` command. |
| `CalArgs` | `ephem`, `years_arg`, `has_years`, `format`, `inc_month`, `tz` | Parameters of the `calendar` command. |
| `YearArgs` | `ephem`, `year`, `mode`, `format`, `tz` | Parameters of the `year` command. |
| `EventArgs` | `ephem`, `category`, `code`, `year`, `near_date`, `format`, `tz` | Parameters of the `event` command. |
| `DlArgs` | `action`, `id`, `dir`, `quiet` | Parameters of the download command. |
| `AtArgs` | `ephem`, `time_raw`, `input_tz`, `tz`, `lunar_day_tz`, `calc_eot`, `eot_lon_deg`, `hli_rules`, `jobs` | Parameters of the `at` command. |
| `ConvArgs` | `ephem`, `in_value`, `from_lunar`, `lunar_year`, `lun_mno`, `lunar_day`, `leap`, `tz`, `lunar_day_tz` | Parameters of the `convert` command. |

### 18.3 C++ Computation Option and Result Structures

| Structure | Key fields | Description |
| --- | --- | --- |
| `GanzhiComputeOptions` | `ephem`, `date_text`, `at_time`, `tz`, `lunar_day_tz`, `hli_rules` | Input of single-instant Ganzhi computation. |
| `GanzhiSummary` | `year`, `month`, `day`, `hli_rules` | Result of single-instant Ganzhi computation. |
| `GanzhiMonthComputeOptions` | `ephem`, `year`, `month`, `at_time`, `tz`, `lunar_day_tz`, `hli_rules` | Input of month-long Ganzhi computation. |
| `GanzhiMonthSummary` | `year`, `month`, `years`, `months`, `days`, `hli_rules` | Day-by-day Ganzhi result for a whole month. |
| `DayComputeOptions` | `ephem`, `date_text`, `at_time`, `tz`, `lunar_day_tz`, `include_events`, `include_astro`, `astro_mode_text`, `astro_pick_csv`, `astro_lat_deg`, `astro_lon_deg`, `astro_height_m`, `has_astro_site`, `hli_lon_deg`, `hli_rules` | Full input of `compute_day`. |
| `DayResult` | `ephem`, `date_text`, `at_time`, `tz`, `lunar_day_tz`, `hli_lon_deg`, `astro_obs`, `at_data`, `day_events`, `astro_events` | Full result of `compute_day`. |

### 18.4 Main Data Structures

| Structure | Key fields | Description |
| --- | --- | --- |
| `LunDate` | `lunar_year`, `lun_mno`, `is_leap`, `lun_mlab`, `lunar_day`, `lun_label`, `cst_year`, `cst_month`, `cst_day`, `cstday_jd` | Lunar date object, also retaining the civil-day information used for day-boundary decisions. |
| `NearEvt` | `has`, `event` | A single nearby-event slot. |
| `NearEvents` | `solar_prev`, `solar_next`, `phase_prev`, `phase_next` | Previous/next solar terms and previous/next lunar phases. |
| `AtData` | `time_raw`, `tz_in`, `display_tz`, `lunar_day_tz`, `jd_utc`, `jd_tdb`, `utc_iso`, `local_iso`, `lam_s`, `lam_m`, `elong`, `elong_deg`, `ill_frac`, `ill_pct`, `waxing`, `phase_name`, `lunar_date`, `near_ev`, `eot`, `moon_xg`, `hli` | Core result container used by `at/day/almanac`. |
| `EventRec` | `kind`, `code`, `name`, `year`, `jd_tdb`, `jd_utc`, `utc_iso`, `loc_iso` | A single event record. |
| `MonthRec` | `label`, `month_no`, `is_leap`, `st_jdutc`, `ed_jdutc`, `st_utc`, `st_loc`, `ed_utc`, `ed_loc` | A single lunar-month interval record. |
| `HliRuleSet` | `profile_code`, `year_boundary`, `month_boundary`, `leap_month_mode`, `day_boundary` | Almanac rule set. |
| `GzNode` | `stem`, `branch`, `text` | A single Ganzhi node. |
| `HliHour` | `slot_index`, `gz_index`, `is_bad`, `slot`, `gz`, `luck` | A single hourly luck result. |
| `HliData` | `y_lun`, `y_lchun`, `y_rule`, `m_gz`, `d_gz`, `h_gz`, `h_gz_true`, `bazi_clock`, `bazi_true`, `rule_profile_code`, `year_boundary_code`, `month_boundary_code`, `leap_month_mode_code`, `day_boundary_code`, `good_gods`, `bad_gods`, `yi`, `ji`, `hour_jx`, `eot_min`, `tst_min` | Full almanac result object. |
| `HliInput` | `jd_utc`, `gy/gm/gd`, `hh/mm/ss`, `tz_off`, `lon_deg`, `lun_year`, `lun_month`, `lun_day`, `lun_leap`, `phase_name`, `moon_xg`, `rules` | Almanac computation input object. |
| `EoTData` | `jd_utc`, `jd_tdb`, `lon_deg`, `lon_rad`, `apparent_solar_time_rad`, `mean_solar_time_rad`, `eot_rad`, `eot_minutes`, `eot_seconds` | Equation-of-time result. |
| `AppLon` | `eph`, `frame_bias`, `prec_cache`, `r1n_cache`, `rot_cache` | Calculator object for apparent solar/lunar longitude and equation of time. |
| `MoonXg` | `region`, `star_id`, `star_name`, `sep_deg` | Result of the moon's star-lodge assignment. |
| `AstroObs` | `has_site`, `lat_deg`, `lon_deg`, `h_m` | Astronomy observation site. |
| `SkyPos` | `kind`, `code`, `name`, `region`, `mag_v`, `ra_deg`, `dec_deg`, `az_deg`, `alt_deg` | Output record of a sky position. |
| `SolarZodiacDef` | `index`, `code`, `term_code`, `start_lambda_rad`, `end_lambda_rad` | Fixed ecliptic interval definition of a single solar zodiac sign. |
| `SolarZodiacPoint` | `sun_lam_deg`, `sign_index`, `sign_code`, `term_code`, `sign_start_jd_utc`, `sign_end_jd_utc`, `elapsed_sec`, `remain_sec` | Solar-zodiac result for a single instant. |
| `SolarZodiacYearInterval` | `sign_index`, `sign_code`, `term_code`, `in_year_start_jd_utc`, `in_year_end_jd_utc`, `in_year_dur_sec`, `clipped_start`, `clipped_end` | Summary of a single sign interval in year mode. |
| `SolarZodiacYearSummary` | `year`, `tz_off`, `year_start_jd_utc`, `year_end_jd_utc`, `intervals` | Full-year solar-zodiac summary result. |
| `AstroEvt` | `kind`, `code`, `name`, `jd_utc`, `detail` | Astronomy event record. |

### 18.5 Eclipse Structures

| Structure | Key fields | Description |
| --- | --- | --- |
| `EclipseGeoCoord` | `ra_deg`, `dec_deg`, `sd_deg`, `ehp_deg` | Celestial-body geometric parameters used in eclipse computation. |
| `EclipseLibration` | `l_deg`, `b_deg`, `c_deg` | Lunar libration parameters. |
| `EclipsePointMeta` | `zen_lat_deg`, `zen_lon_deg`, `pa_deg`, `axis_deg` | Geometric metadata of a contact point. |
| `LunarEclipse` | `has`, `type`, `jd_tdb_p1/u1/u2/u3/u4/p4/opp/max`, `pen_mag`, `umb_mag`, `dur_pen_sec`, `dur_umb_sec`, `dur_tot_sec`, `gamma`, `eps_deg`, `sun_geo`, `moon_geo`, `lib` | Full lunar-eclipse result. |
| `LunarEclipsePointVis` | `stage_window`, `lat_deg`, `lon_deg`, `height_m`, `visible`, `max_alt_deg`, `first_jd_utc`, `last_jd_utc`, `sample_count` | Point-visibility result of a lunar eclipse. |
| `LunarEclipseGlobalVis` | `stage_window`, `jd_start_utc`, `jd_end_utc`, `lat_step_deg`, `lon_step_deg`, `sample_count`, `points` | Global grid visibility result of a lunar eclipse. |
| `SolarBesselianElements` | `x`, `y`, `d_deg`, `mu_deg`, `l1`, `l2`, `tan_f1`, `tan_f2`, `*_coeff` | Solar-eclipse Besselian elements and cubic time polynomials. |
| `SolarEclipse` | `has`, `type`, `jd_tdb_c1/c2/c3/c4/max`, `mag`, `obscuration`, `gamma`, `sep_max_deg`, `sun_sd_max_deg`, `moon_sd_max_deg`, `moon_dist_km`, `sun_dist_km`, `besselian` | Full solar-eclipse result. |
| `SolarEclipsePointVis` | `stage_window`, `lat_deg`, `lon_deg`, `height_m`, `has_eclipse`, `visible`, `central`, `max_mag`, `max_obscuration`, `max_sun_alt_deg`, `c1_jd_utc/c2_jd_utc/c3_jd_utc/c4_jd_utc/max_jd_utc` | Point-visibility result of a solar eclipse. |
| `SolarEclipseGlobalVis` | `stage_window`, `jd_start_utc`, `jd_end_utc`, `lat_step_deg`, `lon_step_deg`, `sample_count`, `points` | Global grid visibility result of a solar eclipse. |

### 18.6 C API Structures

| Structure | Key fields | Description |
| --- | --- | --- |
| `lunar_eot_result` | `jd_utc`, `jd_tdb`, `lon_deg`, `lon_rad`, `apparent_solar_time_rad`, `mean_solar_time_rad`, `eot_rad`, `eot_minutes`, `eot_seconds` | Equation-of-time result in the C API. |
| `lunar_day_summary` | `lunar_year`, `lun_mno`, `lunar_day`, `is_leap`, `ill_pct`, `phase_name`, `lun_label` | Single-day summary in the C API. |
| `lunar_hli_rules` | `profile_code`, `year_boundary`, `month_boundary`, `leap_month_mode`, `day_boundary` | Almanac rule set in the C API. |
| `lunar_ganzhi_node` | `index`, `stem`, `branch`, `text` | A single Ganzhi node in the C API. |
| `lunar_ganzhi_summary` | `year`, `month`, `day`, `rule_profile_code`, `year_boundary_code`, `month_boundary_code`, `leap_month_mode_code`, `day_boundary_code` | Single-instant Ganzhi summary in the C API. |
| `lunar_ganzhi_month_summary` | `year`, `month`, `day_count`, `years[31]`, `months[31]`, `days[31]`, `*_code` fields | Month-long Ganzhi summary in the C API. |
