---
title: export
description: 批量导出逐日数据的参数与输出。
---

# export

按公历月或年份范围批量导出逐日数据。

## 语法

```bash
lunar export [bsp] <YYYY-MM>
lunar export [bsp] --from <YYYY-MM> --to <YYYY-MM>
lunar export [bsp] --from-year <year> --to-year <year>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|jsonl|csv|txt] [--out <path>] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--jobs N] [--events 0|1] [--eclipse 0|1]
  [--scope basic|full] [--full 0|1]
  [--huangli off|folk|ziping|purple|xieji|all] [--trad folk|ziping|purple|xieji|all]
  [--lon <deg>]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<YYYY-MM>`：导出单个公历月。
- `--from` / `--to`：导出公历月闭区间。
- `--from-year` / `--to-year`：导出年份闭区间。
- `--tz`：显示时区。
- `--lunar-day-tz`：逐日边界使用的民用日时区。
- `--format`：输出格式，支持 `json`、`jsonl`、`csv`、`txt`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。
- `--at`：每日采样时间，默认 `12:00:00`。
- `--jobs`：逐日计算并发数；`1` 为单线程。
- `--events`：是否包含节气、月相等日事件。
- `--eclipse`：是否包含食象事件。
- `--scope`：`basic` 为基础导出，`full` 会开启食象、天象和全部黄历流派。
- `--full`：`--scope full` 的兼容布尔形式。
- `--huangli`：黄历输出模式，`off` 关闭，`all` 输出全部流派。
- `--trad`：黄历流派别名，作用同单流派 `--huangli`。
- `--lon`：黄历计算经度，东经为正。
- `--year-boundary`：黄历年界规则。
- `--month-boundary`：黄历月界规则。
- `--leap-month-mode`：闰月处理规则。
- `--day-boundary`：黄历日界规则。
- `--astro`：是否输出天象事件。
- `--astro-mode`：天象范围。
- `--astro-pick`：`pick` 模式下的目标列表。
- `--astro-lat`：观测点纬度。
- `--astro-lon`：观测点经度。
- `--astro-height`：观测点海拔，单位米。

## 输出

- `json`：包含 `meta`、输入范围和 `days` 数组；每日含公历日期、农历、干支、事件、食象、天象和黄历数据。
- `jsonl`：每日一行，适合流式处理。
- `csv`：每个公历日一行，嵌套数据压缩为摘要字段。
- `txt`：按日期分段的人类可读输出。

## 示例

```bash
lunar export @series 2025-09 --format jsonl --out days.jsonl
lunar export ./de442.bsp --from 2025-01 --to 2025-12 --scope full --format json
```
