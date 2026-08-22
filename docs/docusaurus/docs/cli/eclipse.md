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

日食全局 `mag` 是最大食时刻地表最大食分，不是地心圆面重叠量。偏食采用
`k1=0.2724880` 的平均半影月球半径；全食、环食和全环食采用
`k2=0.2722810` 的平均最小月球半径，与 Five Millennium Canon 的约定一致。
`obscuration` 是太阳圆面被遮蔽的面积比例。

## 批量刷新日食数据

已知最大食的 `JD TDB` 和旧类型时，可以在一个常驻星历进程中批量刷新目录：

```bash
lunar eclipse-magnitude ./de441.bsp --input maxima.tsv --out magnitudes.tsv
```

输入列为 `id jd_tdb_max type`，其中 `type` 为 `P`、`A`、`T` 或 `H`；默认 TSV
输出为 `id jd_tdb_max corrected_type catalog_mag catalog_obscuration`，会重新判断
边界食类型。增加 `--full --tz +08:00` 后，每行输出一个 NDJSON 对象，包含完整
日食详情：兼容食分、标准目录食分、全部接触点、时间尺度、距离、gamma 和完整
贝塞尔参数/三次多项式。`JD TDB` 是已知食甚的高精度种子，完整模式仍会围绕它
重建并校验所有参数。

```bash
lunar eclipse-magnitude ./de441.bsp --input maxima.tsv --out eclipses.ndjson --full --tz +08:00
```

## 示例

```bash
lunar eclipse @series --near 2025-09-07 --format json
lunar eclipse ./de442.bsp --kind solar --near 2026-08-12 --format json
lunar eclipse ./de442.bsp --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```
