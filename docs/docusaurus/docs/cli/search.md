---
title: search
description: 事件搜索表达式的参数与输出。
---

# search

使用受限表达式查询事件。

## 语法

```bash
lunar search [bsp] <query>
  [--from <time>] [--count N] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv|ics|jsonl] [--out <path>]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<query>`：查询表达式。当前支持 `next ...`。
- `--from`：搜索起点；省略时使用当前 UTC 时刻。
- `--count`：返回数量，默认 `1`。
- `--tz`：显示时区。
- `--format`：输出格式，支持 `json`、`txt`、`csv`、`ics`、`jsonl`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。
- `--eclipse`：是否为满月行附加月食细节。

## 查询目标

常用目标包括 `full moon`、`new moon`、`first quarter`、`last quarter`、`solar term`、`lunar eclipse`、`solar eclipse`、`eclipse`。空格、连字符和下划线会归一化处理。

## 输出

输出格式与 `next` 相同：事件列表、JSONL 事件流、CSV 表格或 ICS 日历。

## 示例

```bash
lunar search @series next full moon --from 2025-06-01
lunar search ./de442.bsp "next lunar eclipse" --from 2025-01-01 --format json
```
