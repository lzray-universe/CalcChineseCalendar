---
title: almanac
description: 单日黄历查询的参数与输出。
---

# almanac

查询指定日期的黄历摘要与宜忌信息。

## 语法

```bash
lunar almanac [bsp] <YYYY-MM-DD>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
  [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<YYYY-MM-DD>`：目标公历日期。
- `--tz`：显示时区。
- `--lunar-day-tz`：农历判日使用的民用日时区。
- `--format`：输出格式，支持 `json`、`txt`、`csv`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。
- `--lon`：黄历计算经度，东经为正。
- `--trad`：黄历规则流派。
- `--year-boundary`：黄历年界规则。
- `--month-boundary`：黄历月界规则。
- `--leap-month-mode`：闰月处理规则。
- `--day-boundary`：黄历日界规则。

## 输出

- `json`：包含输入日期、农历日期、干支、黄历规则、宜忌、神煞等结构化字段。
- `txt`：键值形式输出黄历摘要。
- `csv`：单行黄历摘要，适合表格处理。

## 示例

```bash
lunar almanac @series 2025-09-17
lunar almanac ./de442.bsp 2025-09-17 --trad xieji --format json
```
