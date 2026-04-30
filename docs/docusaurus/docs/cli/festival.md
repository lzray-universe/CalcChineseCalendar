---
title: festival
description: 年度节日数据的参数与输出。
---

# festival

生成指定年份的传统节日数据。

## 语法

```bash
lunar festival [bsp] <year>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<year>`：目标公历年份。
- `--tz`：显示时区。
- `--lunar-day-tz`：农历判日使用的民用日时区。
- `--format`：输出格式，支持 `json`、`txt`、`csv`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。

## 输出

- `json`：包含年份和节日数组。节日记录含日期、名称、类别和农历/公历相关信息。
- `txt`：按日期列出的节日摘要。
- `csv`：每个节日一行。

## 示例

```bash
lunar festival @series 2025
lunar festival ./de442.bsp 2025 --format csv --out festival.csv
```
