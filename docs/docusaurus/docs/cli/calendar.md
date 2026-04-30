---
title: calendar
description: 年度节气、月相和农历月日历的参数与输出。
---

# calendar

生成一个或多个年份的节气、月相和可选农历月信息。

## 语法

```bash
lunar calendar [bsp] [<years>]
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--include-months 0|1] [--include-eclipses 0|1]
  [--pretty 0|1] [--quiet]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `[<years>]`：年份表达式；省略时默认 `2025`。
- `--format`：输出格式，支持 `json`、`txt`、`ics`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--tz`：显示时区。
- `--include-months`：是否附加农历月列表。
- `--include-eclipses`：是否附加月食信息。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。

## 输出

- `json`：包含 `meta` 和 `data`。单年时 `data` 为对象，多年时为数组。年度对象含 `sol_terms`、`lun_phase`，可选 `months` 和 `lunar_eclipses`。
- `txt`：按年份列出节气和月相事件，可附加农历月与月食段落。
- `ics`：导出节气和月相日历事件，不包含农历月表。

## 示例

```bash
lunar calendar @series 2025
lunar calendar ./de442.bsp 2024-2026 --format ics --out calendar.ics
```
