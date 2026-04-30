---
title: monthview
description: 单月逐日视图的参数与输出。
---

# monthview

生成一个公历月的逐日历法视图。

## 语法

```bash
lunar monthview [bsp] <YYYY-MM>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<YYYY-MM>`：目标公历月。
- `--tz`：显示时区。
- `--lunar-day-tz`：农历判日使用的民用日时区。
- `--format`：输出格式，支持 `json`、`txt`、`csv`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。
- `--astro`：是否附加天象事件。
- `--astro-mode`：天象范围。
- `--astro-pick`：`pick` 模式下的目标列表。
- `--astro-lat`：观测点纬度。
- `--astro-lon`：观测点经度。
- `--astro-height`：观测点海拔，单位米。

## 输出

- `json`：包含月份输入、逐日数据、农历日期、黄历摘要和可选天象事件。
- `txt`：按日期逐日输出摘要。
- `csv`：每个公历日一行，适合表格处理。

## 示例

```bash
lunar monthview @series 2025-09 --format txt
lunar monthview ./de442.bsp 2025-09 --format csv --out month.csv
```
