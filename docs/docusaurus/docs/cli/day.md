---
title: day
description: 单日历法、黄历与天象查询的参数与输出。
---

# day

查询一个公历日的农历日期、月相、黄历和可选天象事件。

## 语法

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

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<YYYY-MM-DD>`：目标公历日期。
- `--tz`：显示时区。
- `--lunar-day-tz`：农历判日使用的民用日时区。
- `--format`：输出格式，支持 `json`、`txt`、`csv`、`jsonl`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。
- `--at`：日内采样时间，默认由实现使用的日采样逻辑决定。
- `--events`：是否输出当日节气、月相等事件。
- `--lon`：黄历计算使用的经度，东经为正。
- `--trad`：黄历规则流派。
- `--year-boundary`：黄历年界规则。
- `--month-boundary`：黄历月界规则。
- `--leap-month-mode`：闰月处理规则。
- `--day-boundary`：黄历日界规则。
- `--astro`：是否输出天象事件。
- `--astro-mode`：天象范围。`less` 为常用集合，`all` 为全部，`pick` 为指定目标。
- `--astro-pick`：`pick` 模式下的目标列表。
- `--astro-lat`：观测点纬度。
- `--astro-lon`：观测点经度。
- `--astro-height`：观测点海拔，单位米；需同时提供经纬度。

## 输出

- `json` / `jsonl`：包含 `meta`、`input` 和 `data`。`data` 含 `lunar_date`、`huangli`、月亮照明比例、月相名、月宿关系、采样 UTC/本地时间、`events` 和 `astro_events`。
- `txt`：键值形式输出输入参数、农历、黄历、事件列表和天象事件列表。
- `csv`：单行摘要，含日期、农历标签、月相、采样时间、事件摘要和黄历字段。

## 示例

```bash
lunar day @series 2025-06-01
lunar day ./de442.bsp 2025-01-31 --trad ziping --format json
lunar day ./de442.bsp 2025-06-01 --astro 1 --astro-lat 31.23 --astro-lon 121.47
```
