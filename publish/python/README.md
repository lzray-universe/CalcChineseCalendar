# calcchinesecalendar

`calcchinesecalendar` packages the `lunar` engine as a native Python extension
for Chinese calendar, almanac, and astronomy computations.

It is intended for direct use from Python code and for installation from PyPI.

The upstream C++ repository and the full reference documentation are available
at:

- GitHub: https://github.com/lzray-universe/CalcChineseCalendar/

If you need the full CLI reference, C++ API, C API, i18n details, repository
layout, or release artifacts, use the GitHub repository as the primary
reference.

## Installation

```bash
pip install calcchinesecalendar
```

## Quick start

```python
import calcchinesecalendar as ccc

print(ccc.version())

data = ccc.day("@series", "2025-06-01")
print(data["data"]["phase_name"])

ganzhi = ccc.ganzhi("@series", "2025-09-07")
print(ganzhi["data"]["day"]["text"])
```

## Using BSP ephemerides

The first argument of the public APIs is the ephemeris selector:

- use `@series` to force the built-in VSOP87A + ELPMPP02 fallback
- pass a BSP file path such as `de442.bsp` to use JPL DE ephemerides

Example:

```python
import calcchinesecalendar as ccc

data = ccc.day("de442.bsp", "2025-06-01")
print(data["data"]["phase_name"])
```

## Public API overview

The package exposes:

- native core APIs such as `core_day`, `ganzhi`, `ganzhi_month`, and `calc_eot`
- high-level helpers such as `day`, `monthview`, `at`, `convert`, `search`,
  `eclipse`, `festival`, `almanac`, and `info`
- raw `run` and `run_json` access for advanced command-style use
- a `Lunar` client class for shared defaults such as `lang` and
  `eclipse_method`

## High-level helper example

```python
import calcchinesecalendar as ccc

result = ccc.search(
    "@series",
    "next full_moon",
    from_time="2025-06-01T00:00:00+08:00",
    count=2,
)

for item in result["data"]["items"]:
    print(item["time"]["iso"])
```

## `Lunar` client example

```python
import calcchinesecalendar as ccc

lunar = ccc.Lunar(lang="en", eclipse_method="fast")
result = lunar.day("@series", "2025-06-01")
print(result["data"]["phase_name"])
```

## Notes

- Python package name: `calcchinesecalendar`
- Import name: `calcchinesecalendar`
- Python 3.9 or newer is required
- The package bundles the native extension; a compiler is not required for
  normal wheel installation

For the full CLI, C++ API, C API, i18n documentation, and repository-level
usage notes, see the GitHub C++ repository:

- https://github.com/lzray-universe/CalcChineseCalendar/
