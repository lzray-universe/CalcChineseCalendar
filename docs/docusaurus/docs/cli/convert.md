---
title: convert
description: 公历与农历互转的参数与输出。
---

# convert

在公历日期时间和农历日期之间转换。

## 语法

```bash
lunar convert [bsp] <dt_or_tm>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --from-lunar <lunar_year> <month_no> <day> [--leap 0|1]
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --stdin
lunar convert [bsp] --file <path>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--jobs N] [--meta-once 0|1]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `<dt_or_tm>`：公历日期或日期时间。
- `--from-lunar`：农历转公历，后接农历年、月序、日。
- `--leap`：农历月是否为闰月。
- `--stdin`：从标准输入逐行读取公历输入。
- `--file`：从文件逐行读取公历输入。
- `--input-tz`：解析无时区公历输入时使用的时区。
- `--tz`：显示时区。
- `--lunar-day-tz`：农历判日使用的民用日时区。
- `--format`：输出格式，支持 `json`、`txt`、`jsonl`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。
- `--jobs`：批量模式并发参数。
- `--meta-once`：批量 JSONL 输出时是否只输出一次元信息。

## 输出

- `json`：包含输入、转换后的公历日期时间、农历日期、闰月标记和相关时间边界。
- `jsonl`：批量模式每个输入一行；单次模式会回退为 `json`。
- `txt`：键值形式输出转换结果。

## 示例

```bash
lunar convert @series 2025-06-01
lunar convert ./de442.bsp --from-lunar 2025 5 6 --leap 0 --format json
```
