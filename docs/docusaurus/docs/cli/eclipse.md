---
title: eclipse
description: 日食、月食和地点可见食查询的参数与输出。
---

# eclipse

查询日食、月食，以及给定地点附近可见的食象。

## 语法

```bash
lunar [--eclipse-method modern|legacy] eclipse [bsp] --near <YYYY-MM-DD> [--kind lunar|solar]
  [--stage any|umb|total]
  [--stage any|central]
  [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1]
  [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--tz Z|+08:00|-05:00] [--format json|txt|geojson]
  [--out <path>] [--pretty 0|1] [--quiet]

lunar eclipse [bsp] --visible-near <time> --point-lat <deg> --point-lon <deg>
  [--kind lunar|solar|both] [--visible-years <years>]
  [--stage any|umb|total|central] [--sample-min <minutes>]
  [--point-height <m>]
  [--tz Z|+08:00|-05:00] [--format json|txt]
  [--out <path>] [--pretty 0|1] [--quiet]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `--eclipse-method`：全局食象算法，`modern` 或 `legacy`。
- `--near`：查询距离日期最近的食象。
- `--visible-near`：查询指定地点附近可见的最近食象。
- `--kind`：食象类型，`lunar`、`solar` 或 `both`。
- `--stage`：筛选食象阶段。月食支持 `any`、`umb`、`total`；日食支持 `any`、`central`。
- `--sample-min`：可见性采样步长，单位分钟。
- `--point-lat`：观测点纬度。
- `--point-lon`：观测点经度。
- `--point-height`：观测点海拔，单位米。
- `--point-refine`：是否细化地点可见性边界。
- `--global-vis`：是否计算全球网格可见性。
- `--global`：`--global-vis` 的兼容别名。
- `--global-format`：全球可见性输出格式，`json` 或 `geojson`。
- `--grid-lat-step`：全球网格纬度步长。
- `--grid-lon-step`：全球网格经度步长。
- `--tz`：显示时区。
- `--format`：输出格式。普通查询支持 `json`、`txt`、`geojson`；`--visible-near` 支持 `json`、`txt`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制进度和写文件提示。

## 输出

- `json`：包含食象类型、阶段、最大食时刻、接触时刻、可见性和可选全球网格。
- `txt`：人类可读的食象摘要。
- `geojson`：全球可见性 FeatureCollection；使用该格式会开启全球可见性计算。

## 示例

```bash
lunar eclipse @series --near 2025-09-07 --format json
lunar eclipse ./de442.bsp --kind solar --near 2026-08-12 --format json
lunar eclipse ./de442.bsp --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```
