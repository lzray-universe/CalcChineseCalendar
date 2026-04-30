---
title: year
description: 单年历法摘要的参数与输出。
---

# year

生成单个年份的节气、月相、农历月等历法摘要。

## 语法

```bash
lunar year [bsp] <year>
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<year>`：目标年份。
- `--mode`：年份口径。`lunar` 按农历年，`gregorian` 按公历年。
- `--format`：输出格式，支持 `json`、`txt`、`ics`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--tz`：显示时区。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。

## 输出

- `json`：包含 `meta` 和 `data`。`data` 含 `year`、`mode`、`sol_terms`、`lun_phase`、`months`。
- `txt`：人类可读的年度事件和月份摘要。
- `ics`：节气和月相日历事件。

## 示例

```bash
lunar year @series 2025 --format json --out year-2025.json
```
