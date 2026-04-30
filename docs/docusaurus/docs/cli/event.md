---
title: event
description: 单个节气、月相与食象事件的参数与输出。
---

# event

查询单个节气、月相，或转发查询食象事件。

## 语法

```bash
lunar event [bsp] solar-term <code> <year>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-phase <new_moon|fst_qtr|full_moon|lst_qtr> --near <YYYY-MM-DD>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-eclipse --near <YYYY-MM-DD> [eclipse options...]
lunar event [bsp] solar-eclipse --near <YYYY-MM-DD> [eclipse options...]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `solar-term`：节气事件类型。
- `<code>`：节气代码，支持 `J1..J12` 和 `Z1..Z12`。
- `<year>`：节气查询年份。
- `lunar-phase`：月相事件类型。
- `<new_moon|fst_qtr|full_moon|lst_qtr>`：月相代码。
- `--near`：寻找距离该日期最近的月相或食象。
- `lunar-eclipse`：转发到 `eclipse --kind lunar` 语义。
- `solar-eclipse`：转发到 `eclipse --kind solar` 语义。
- `--format`：普通事件支持 `json`、`txt`、`ics`；食象转发支持 `json`、`txt`、`geojson`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--tz`：显示时区。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。
- `--eclipse`：查询满月时是否附加月食细节。

## 输出

- `json`：包含 `meta` 和单个事件对象。事件字段含类型、代码、名称、年份、`jd_utc`、UTC 时间和本地时间；满月可附加月地距离。
- `txt`：单个事件的可读摘要。
- `ics`：单个日历事件。
- 食象转发模式的输出与 `eclipse` 命令一致。

## 示例

```bash
lunar event @series solar-term Z2 2025
lunar event @series lunar-phase full_moon --near 2025-09-07
lunar event ./de442.bsp solar-eclipse --near 2026-08-12 --format json
```
