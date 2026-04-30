---
title: at
description: 指定时刻综合查询的参数与输出。
---

# at

查询指定时刻的农历、月相、月宿、黄历和可选事件。

## 语法

```bash
lunar at [bsp] <time>
lunar at [bsp] --time <time>
lunar at [bsp] --stdin
lunar at [bsp] --file <path>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--events 0|1] [--eot-lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--jobs N] [--meta-once 0|1]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<time>` / `--time`：查询时刻。
- `--stdin`：从标准输入逐行读取时刻。
- `--file`：从文件逐行读取时刻。
- `--input-tz`：解析无时区输入时使用的时区。
- `--tz`：显示时区。
- `--lunar-day-tz`：农历判日使用的民用日时区。
- `--format`：输出格式，支持 `json`、`txt`、`jsonl`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。
- `--events`：是否附加相关事件。
- `--eot-lon`：真太阳时差计算经度，东经为正。
- `--trad`：黄历规则流派。
- `--year-boundary`：黄历年界规则。
- `--month-boundary`：黄历月界规则。
- `--leap-month-mode`：闰月处理规则。
- `--day-boundary`：黄历日界规则。
- `--jobs`：批量模式并发参数；当前执行保持顺序输出。
- `--meta-once`：批量 JSONL 输出时是否只输出一次元信息。

## 输出

- `json`：单次查询输出一个对象，含 `meta`、输入时刻和 `data`。`data` 包含农历日期、黄历、月亮照明比例、月相、月宿关系、采样 UTC/本地时间和可选事件。
- `jsonl`：批量模式每个输入时刻一行；单次模式会回退为 `json`。
- `txt`：键值形式输出查询结果。

## 示例

```bash
lunar at @series 2025-06-01T00:00:00+08:00 --format json
lunar at ./de442.bsp --file times.txt --format jsonl --meta-once 1
```
