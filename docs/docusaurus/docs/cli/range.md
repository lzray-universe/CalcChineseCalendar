---
title: range
description: 区间事件查询的参数与输出。
---

# range

查询指定时间区间内的节气、月相和食象事件。

## 语法

```bash
lunar range [bsp] --from <time> --to <time>
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `--from`：区间起点，必填。
- `--to`：区间终点，必填，不能早于 `--from`。
- `--kinds`：事件类型列表，逗号分隔。
- `--tz`：显示时区。
- `--format`：输出格式，支持 `json`、`txt`、`csv`、`ics`、`jsonl`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。
- `--eclipse`：是否为满月行附加月食细节。

## 输出

- `json`：包含 `meta`、查询类型和区间内事件数组。
- `jsonl`：每个事件一行。
- `txt`：事件列表文本表格。
- `csv`：每个事件一行。
- `ics`：日历事件集合。

## 示例

```bash
lunar range @series --from 2025-01-01 --to 2025-12-31
lunar range ./de442.bsp --from 2025-02-01 --to 2025-03-01 --format csv
```
