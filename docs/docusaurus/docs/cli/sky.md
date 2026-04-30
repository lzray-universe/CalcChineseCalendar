---
title: sky
description: 指定观测点天空位置查询的参数与输出。
---

# sky

查询指定时刻、指定观测点的太阳、月亮和星表目标视位置。

## 语法

```bash
lunar sky <bsp> <time> --lat <deg> --lon <deg>
lunar sky <bsp> --time <time> --lat <deg> --lon <deg>
  [--height <m>] [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--mode all|pick] [--pick sun,moon,Spica,HR5056,...]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

## 参数

- `<bsp>`：必填星历。此命令当前不使用自动补齐星历。
- `<time>` / `--time`：查询时刻。
- `--lat`：观测点纬度，必填。
- `--lon`：观测点经度，必填。
- `--height`：观测点海拔，单位米。
- `--input-tz`：解析无时区输入时使用的时区。
- `--tz`：显示时区。
- `--mode`：输出范围。`all` 输出默认全部目标，`pick` 输出指定目标。
- `--pick`：`pick` 模式下的目标列表，如 `sun,moon,Spica`。
- `--format`：输出格式，支持 `json`、`txt`、`csv`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。

## 输出

- `json`：包含输入时刻、观测点和目标数组。目标记录包含名称、赤经赤纬、方位、高度等视位置字段。
- `txt`：目标列表文本表格。
- `csv`：每个目标一行。太阳系目标排在星表恒星之前。

## 示例

```bash
lunar sky @series 2025-06-01T20:00:00+08:00 --lat 31.23 --lon 121.47
lunar sky ./de442.bsp --time 2025-06-01T20:00 --input-tz +08:00 --lat 31.23 --lon 121.47 --mode pick --pick sun,moon,Spica
```
