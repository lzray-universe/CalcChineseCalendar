---
title: next
description: 后续事件查询的参数与输出。
---

# next

从指定时刻开始查询后续若干个事件。

## 语法

```bash
lunar next [bsp] --from <time> --count N
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `--from`：起始时刻，必填。
- `--count`：返回数量，必须大于等于 `1`。
- `--kinds`：事件类型列表，逗号分隔。
- `--tz`：显示时区。
- `--format`：输出格式，支持 `json`、`txt`、`csv`、`ics`、`jsonl`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。
- `--eclipse`：是否为满月行附加月食细节。

## 输出

- `json`：包含 `meta`、查询类型和事件数组。
- `jsonl`：每个事件一行。
- `txt`：事件列表文本表格。
- `csv`：每个事件一行。
- `ics`：日历事件集合。

事件字段通常包含类型、代码、名称、年份、儒略日、UTC 时间和显示时区本地时间。

## 示例

```bash
lunar next @series --from 2025-06-01T00:00:00+08:00 --count 5
lunar next ./de442.bsp --from 2025-06-01 --count 10 --format ics --out next.ics
```
