# calcchinesecalendar

`calcchinesecalendar` packages the `lunar` engine as a native Python
extension.

The public module is imported as `calcchinesecalendar`.

```python
import calcchinesecalendar

data=calcchinesecalendar.day("@series","2025-06-01")
print(data["data"]["phase_name"])
```

The package exposes:

- native core APIs such as `core_day`, `ganzhi`, `ganzhi_month`, and `calc_eot`
- typed helpers such as `day`, `monthview`, `at`, `convert`, `search`,
  `eclipse`, `festival`, `almanac`, and `info`
- raw `run` / `run_json` access for advanced commands
- a `Lunar` client class for shared global defaults such as `lang` and
  `eclipse_method`
