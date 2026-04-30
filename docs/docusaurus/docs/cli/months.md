---
title: months
description: 枚举农历月的参数与输出。
---

# months

枚举一个或多个年份内的农历月。

## 语法

```bash
lunar months [bsp] <years>
  [--mode lunar|gregorian]
  [--format json|txt|csv] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--include-eclipses 0|1]
  [--output <json>] [--output-txt <txt>]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择；可用 `.bsp`、`@series` 或 `series`。
- `<years>`：年份表达式，支持 `2025`、`2024-2026`、`2024,2026,2030-2032`。
- `--mode`：年份口径。`lunar` 按农历年，`gregorian` 按公历年。
- `--format`：输出格式，支持 `json`、`txt`、`csv`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--tz`：显示时区，只影响输出中的本地时间文本。
- `--pretty`：JSON 是否格式化，`1` 为格式化。
- `--quiet`：抑制进度和写文件提示。
- `--include-eclipses`：是否附加当年相关月食数据。
- `--output`：旧 JSON 输出路径参数，保留兼容。
- `--output-txt`：旧文本输出路径参数，保留兼容。

## 输出

- `json`：包含 `meta` 和 `data`。每年数据含 `year`、`mode`、`months`，月记录含 `label`、`month_no`、`is_leap`、起止儒略日、UTC 时间和本地时间。
- `txt`：按年份分段，适合直接阅读或简单脚本处理。
- `csv`：每个农历月一行，字段为年份、模式、月名、月序、闰月标记和起止时间。
- 开启 `--include-eclipses` 时，JSON/TXT 会追加月食信息；CSV 不适合承载嵌套食象数据。

## 示例

```bash
lunar months @series 2025
lunar months ./de442.bsp 2024-2026 --mode gregorian --format json --out months.json
```
