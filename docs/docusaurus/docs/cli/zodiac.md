---
title: zodiac
description: 太阳星座查询的参数与输出。
---

# zodiac

查询指定时刻所属太阳星座，或生成某公历年的星座区间。

## 语法

```bash
lunar zodiac <bsp> --time <time>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]

lunar zodiac <bsp> --year <year>
  [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

## 参数

- `<bsp>`：必填星历。此命令当前不使用自动补齐星历。
- `--time`：查询指定时刻所属星座。
- `--year`：输出该公历年内的星座区间。
- `--input-tz`：解析无时区 `--time` 输入时使用的时区。
- `--tz`：显示时区；在 `--year` 模式下也决定公历年裁剪窗口。
- `--format`：输出格式，支持 `json`、`txt`、`csv`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。

## 输出

- `--time` 输出当前星座、太阳视黄经、星座起止时刻、已过时长和剩余时长。
- `--year` 输出 12 个星座区间，包含区间起止、本年裁剪起止和持续时间。
- 太阳星座按地心太阳视黄经计算，包含光行时修正。

## 示例

```bash
lunar zodiac @series --time 2025-03-20T18:01:00+08:00
lunar zodiac ./de442.bsp --year 2025 --format csv
```
