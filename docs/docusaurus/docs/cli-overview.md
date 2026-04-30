---
sidebar_position: 4
title: 命令行使用方法
description: lunar 命令行工具的完整用法、全局参数、星历选择、配置、输出格式与命令参考。
---

# 命令行使用方法

`lunar` 是 CalcChineseCalendar 的命令行入口，用于生成农历、节气、月相、食象、黄历、节日、星座和观测位置等数据。本文按当前代码实现整理命令语法；命令参考中写作 `[bsp]` 的位置参数可省略，写作 `<bsp>` 的位置参数必须显式提供。

## 基本形式

```bash
lunar --help
lunar --version
lunar [--lang zh|zht|en|ja|ko] [--eclipse-method modern|legacy] <command> ...
lunar
```

无参数运行会进入交互模式。所有子命令都支持 `--help` 查看当前编译版本内置的帮助文本。

兼容旧语法：

```bash
lunar <bsp> <years> [months options...]
```

该形式等价于：

```bash
lunar months <bsp> <years> [months options...]
```

## 全局参数

```bash
--lang zh|zht|en|ja|ko
--eclipse-method modern|legacy
--bsp <path>
--bsp=<path>
```

- `--lang` 控制输出语言，也接受 `zh-cn`、`zh-hans`、`zh-tw`、`zh-hant`、`en-us`、`ja-jp`、`ko-kr` 等常见别名。
- `--eclipse-method` 控制食象计算方法，默认 `modern`。
- `--bsp` 可为所有需要星历且支持自动补齐星历的命令提供显式星历路径。

示例：

```bash
lunar day 2025-06-01 --bsp ./de442.bsp
lunar --lang en day @series 2025-06-01 --format json
```

## 星历参数

`<bsp>` 或 `[bsp]` 可使用以下值：

- `.bsp` 文件路径，例如 `./de442.bsp`
- `@series`
- `series`

`@series` 和 `series` 使用内置 VSOP87A + ELPMPP02 级数模型，要求构建时启用 `LUNAR_ENABLE_SERIES_FALLBACK`。默认构建启用该能力。

以下命令支持省略 `[bsp]`，省略时会自动选择星历：

```text
months calendar year event at convert day monthview export next range search eclipse festival almanac info
```

以下命令不需要 BSP：

```text
download config completion
```

以下命令当前仍要求显式传入位置参数 `<bsp>`：

```text
zodiac sky
```

自动选择星历时，候选来源按以下顺序收集并去重：

1. 配置项 `def_bsp`
2. 配置项 `bsp_list`
3. 配置项 `bsp_dir` 目录下的 `.bsp` 文件
4. 当前工作目录下的 `.bsp` 文件

如果命令可推断时间区间，优先选择完整覆盖该区间的 BSP；否则选择重叠最多的 BSP。没有可用 BSP 且启用级数回退时，自动使用 `@series`；未启用级数回退时，命令会报错并提示使用 `--bsp` 或 `lunar config set def_bsp`。

## 时间、时区与输出

支持的时间格式：

```text
YYYY-MM-DD
YYYY-MM-DDZ
YYYY-MM-DD+08:00
YYYY-MM-DDTHH:MM
YYYY-MM-DDTHH:MM:SS
YYYY-MM-DDTHH:MM:SS.sss
```

上述带时间的形式均可追加 `Z` 或 `+HH:MM` / `-HH:MM` 时区后缀。输入不带时区时，优先使用命令参数 `--input-tz`，再使用配置项 `default_tz`。

常用输出参数：

```bash
--format txt|json|jsonl|csv|ics|geojson
--out <path>
--pretty 0|1
--quiet
--tz Z|+08:00|-05:00
--lunar-day-tz Z|+08:00|-05:00
```

- `--tz` 通常只影响显示时区；`zodiac --year` 例外，它也决定公历年裁剪窗口。
- `--lunar-day-tz` 控制农历日期映射所用的民用日边界。未指定时读取 `default_lunar_day_tz`；再为空时按语言推断，`ja` / `ko` 为 `+09:00`，其余为 `+08:00`。
- `geojson` 仅用于食象相关输出。
- 多数 JSON 输出包含 `meta`，常见字段包括 `tool`、`version`、`schema`、`ephem`、`tz_display` 和 `notes`。

## 配置文件

CLI 使用 `lun_cfg.txt` 保存默认参数。

```bash
lunar config show [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

支持的配置键：

```text
def_bsp
bsp_dir
bsp_list
default_tz
default_lang
default_lunar_day_tz
def_fmt
hli_trad
hli_year_boundary
hli_month_boundary
hli_leap_month_mode
hli_day_boundary
def_prety
```

说明：

- `bsp_list` 支持逗号或分号分隔。
- 设置 `def_bsp` 后，如果该值不在 `bsp_list` 中，会自动追加。
- `default_lang` 仅允许 `zh`、`zht`、`en`、`ja`、`ko`。
- `def_fmt` 允许 `txt`、`json`、`csv`、`jsonl`、`ics`。
- `def_prety` 使用 `0` 或 `1`。
- `default_lunar_day_tz` 可用 `default`、`auto` 或 `inherit` 清空并恢复自动推断。
- `hli_*` 配置为 `day`、`almanac` 和相关导出命令提供默认黄历规则。

示例：

```bash
lunar config set def_bsp ./de442.bsp
lunar config set default_tz +08:00
lunar config set default_lang en
lunar config set default_lunar_day_tz +09:00
lunar config set hli_trad xieji
lunar config show --format json
```

## 子命令详细页

每个子命令的参数用法和输出解释见独立页面：

| 命令 | 说明 |
| --- | --- |
| [months](./cli/months) | 枚举农历月 |
| [calendar](./cli/calendar) | 年度节气、月相和农历月信息 |
| [year](./cli/year) | 单年历法摘要 |
| [event](./cli/event) | 单个节气、月相或食象事件 |
| [download](./cli/download) | 列出和下载 BSP |
| [at](./cli/at) | 指定时刻综合查询 |
| [convert](./cli/convert) | 公历与农历互转 |
| [day](./cli/day) | 单日历法、黄历和天象 |
| [monthview](./cli/monthview) | 单月逐日视图 |
| [export](./cli/export) | 批量逐日导出 |
| [next](./cli/next) | 后续事件查询 |
| [range](./cli/range) | 区间事件查询 |
| [search](./cli/search) | 事件表达式搜索 |
| [eclipse](./cli/eclipse) | 日食、月食和可见性 |
| [festival](./cli/festival) | 年度节日 |
| [almanac](./cli/almanac) | 单日黄历 |
| [info](./cli/info) | 版本、配置和星历信息 |
| [config](./cli/config) | 配置读写 |
| [completion](./cli/completion) | shell 补全脚本 |
| [zodiac](./cli/zodiac) | 太阳星座 |
| [sky](./cli/sky) | 观测点天空位置 |

## 日历与日期命令

### months

枚举农历月。详细参数与输出见 [months](./cli/months)。

```bash
lunar months [bsp] <years>
  [--mode lunar|gregorian]
  [--format json|txt|csv] [--out <path>] [--tz +08:00|Z|-05:00]
  [--pretty 0|1] [--quiet] [--include-eclipses 0|1]
  [--output <json>] [--output-txt <txt>]
```

`<years>` 支持 `2025`、`2024-2026`、`2024,2026,2030-2032`。`--output` 和 `--output-txt` 是旧参数，不能与 `--out` 同时使用。

```bash
lunar months @series 2025
lunar months ./de442.bsp 2024-2026 --mode gregorian --format json --out months.json
```

### calendar

生成年度节气、月相和可选农历月信息。详细参数与输出见 [calendar](./cli/calendar)。

```bash
lunar calendar [bsp] [<years>]
  [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]
  [--include-months 0|1] [--include-eclipses 0|1]
  [--pretty 0|1] [--quiet]
```

省略 `<years>` 时默认使用 `2025`。`ics` 只导出节气和月相事件。

```bash
lunar calendar @series 2025
lunar calendar ./de442.bsp 2024-2026 --format ics --out calendar.ics
```

### year

生成单年历法摘要。详细参数与输出见 [year](./cli/year)。

```bash
lunar year [bsp] <year>
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz +08:00|Z|-05:00]
  [--pretty 0|1] [--quiet]
```

```bash
lunar year @series 2025 --format json --out year-2025.json
```

### day

查询单日农历、干支、节气/月相、可选黄历与天象事件。详细参数与输出见 [day](./cli/day)。

```bash
lunar day [bsp] <YYYY-MM-DD>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

`--astro-lat` 与 `--astro-lon` 必须同时提供。`--trad` 和各类 boundary / mode 参数也接受常见 ASCII 别名。

```bash
lunar day @series 2025-06-01
lunar day ./de442.bsp 2025-01-31 --trad ziping --format json
lunar day ./de442.bsp 2025-06-01 --astro 1 --astro-lat 31.23 --astro-lon 121.47
```

### monthview

生成单个公历月的逐日视图。详细参数与输出见 [monthview](./cli/monthview)。

```bash
lunar monthview [bsp] <YYYY-MM>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat <deg> --astro-lon <deg> [--astro-height <m>]]
```

```bash
lunar monthview @series 2025-09 --format txt
lunar monthview ./de442.bsp 2025-09 --format csv --out month.csv
```

### export

批量导出公历月或年份范围内的逐日数据。详细参数与输出见 [export](./cli/export)。

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

区间按 `--lunar-day-tz` 下的公历月闭区间导出。`--scope full` 会开启食事件、天象事件和全部黄历流派；JSON / JSONL 保留逐日嵌套结构，CSV 为扁平摘要格式。

```bash
lunar export @series 2025-09 --format jsonl --out days.jsonl
lunar export ./de442.bsp --from 2025-01 --to 2025-12 --scope full --format json
```

## 转换与时刻查询

### at

查询指定时刻的综合历法与天象数据。详细参数与输出见 [at](./cli/at)。

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

`--stdin` 与 `--file` 互斥。单次模式下 `--format jsonl` 会自动回退为 `json`。`--eot-lon` 使用东经为正的经度，输出视太阳时与平太阳时差值。

```bash
lunar at @series 2025-06-01T00:00:00+08:00 --format json
lunar at ./de442.bsp --file times.txt --format jsonl --meta-once 1
```

### convert

公历与农历互转。详细参数与输出见 [convert](./cli/convert)。

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

`--from-lunar` 模式不能再传位置参数 `<dt_or_tm>`。批模式下 `--stdin` 与 `--file` 互斥。

```bash
lunar convert @series 2025-06-01
lunar convert ./de442.bsp --from-lunar 2025 5 6 --leap 0 --format json
```

## 事件、搜索与食象

### event

查询单个节气、月相或食象事件。详细参数与输出见 [event](./cli/event)。

```bash
lunar event [bsp] solar-term <code> <year>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-phase <new_moon|fst_qtr|full_moon|lst_qtr> --near <YYYY-MM-DD>
  [--format json|txt|ics] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]

lunar event [bsp] lunar-eclipse --near <YYYY-MM-DD>
  [--stage any|umb|total] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet]

lunar event [bsp] solar-eclipse --near <YYYY-MM-DD>
  [--stage any|central] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet]
```

`solar-term` 的 `<code>` 使用 `J1..J12`、`Z1..Z12`。`lunar-eclipse` 和 `solar-eclipse` 会转发到 `eclipse` 命令，因此校验规则保持一致。

```bash
lunar event @series solar-term Z2 2025
lunar event @series lunar-phase full_moon --near 2025-09-07
lunar event ./de442.bsp solar-eclipse --near 2026-08-12 --format json
```

### next

从指定时刻开始查询后续事件。详细参数与输出见 [next](./cli/next)。

```bash
lunar next [bsp] --from <time> --count N
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

`--from` 必填，`--count` 必须大于等于 `1`。`--kinds` 未指定时使用全部事件类型。

```bash
lunar next @series --from 2025-06-01T00:00:00+08:00 --count 5
lunar next ./de442.bsp --from 2025-06-01 --count 10 --format ics --out next.ics
```

### range

查询时间区间内的事件。详细参数与输出见 [range](./cli/range)。

```bash
lunar range [bsp] --from <time> --to <time>
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz Z|+08:00|-05:00] [--format json|txt|csv|ics|jsonl]
  [--out <path>] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

`--from` 与 `--to` 必填，且 `--to` 不能早于 `--from`。

```bash
lunar range @series --from 2025-01-01 --to 2025-12-31
lunar range ./de442.bsp --from 2025-02-01 --to 2025-03-01 --format csv
```

### search

使用受限查询表达式搜索事件。详细参数与输出见 [search](./cli/search)。

```bash
lunar search [bsp] <query>
  [--from <time>] [--count N] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv|ics|jsonl] [--out <path>]
  [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

当前支持以 `next ...` 开头的查询。查询可作为一个带引号参数传入，也可拆成多个 CLI 词；事件名会统一处理空格、连字符和下划线。

```bash
lunar search @series next full moon --from 2025-06-01
lunar search ./de442.bsp "next lunar eclipse" --from 2025-01-01 --format json
```

### eclipse

查询日食、月食以及给定地点附近可见食。详细参数与输出见 [eclipse](./cli/eclipse)。

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

`--near` 与 `--visible-near` 二选一。`--point-lat` 与 `--point-lon` 必须同时提供。`--format geojson` 会强制开启全局可见性计算；`--visible-near` 不支持 `geojson` 或全局可见性参数。

```bash
lunar eclipse @series --near 2025-09-07 --format json
lunar eclipse ./de442.bsp --kind solar --near 2026-08-12 --format json
lunar eclipse ./de442.bsp --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```

## 黄历、节日与天文扩展

### festival

生成指定年份节日数据。详细参数与输出见 [festival](./cli/festival)。

```bash
lunar festival [bsp] <year>
  [--tz Z|+08:00|-05:00] [--lunar-day-tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

```bash
lunar festival @series 2025
lunar festival ./de442.bsp 2025 --format csv --out festival.csv
```

### almanac

查询指定日期黄历摘要与宜忌信息。详细参数与输出见 [almanac](./cli/almanac)。

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

```bash
lunar almanac @series 2025-09-17
lunar almanac ./de442.bsp 2025-09-17 --trad xieji --format json
```

### zodiac

查询太阳星座。详细参数与输出见 [zodiac](./cli/zodiac)。

```bash
lunar zodiac <bsp> --time <time>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]

lunar zodiac <bsp> --year <year>
  [--tz Z|+08:00|-05:00]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

`--time` 与 `--year` 二选一。太阳星座按地心太阳视黄经计算，已包含光行时修正。

```bash
lunar zodiac @series --time 2025-03-20T18:01:00+08:00
lunar zodiac ./de442.bsp --year 2025 --format csv
```

### sky

查询指定观测点的地平坐标和视位置。详细参数与输出见 [sky](./cli/sky)。

```bash
lunar sky <bsp> <time> --lat <deg> --lon <deg>
lunar sky <bsp> --time <time> --lat <deg> --lon <deg>
  [--height <m>] [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--mode all|pick] [--pick sun,moon,Spica,HR5056,...]
  [--format json|txt|csv] [--out <path>] [--pretty 0|1] [--quiet]
```

`--lat` 与 `--lon` 必须同时提供。`--mode pick` 可用 `--pick` 指定输出目标；太阳系目标会排在星表恒星之前输出。

```bash
lunar sky @series 2025-06-01T20:00:00+08:00 --lat 31.23 --lon 121.47
lunar sky ./de442.bsp --time 2025-06-01T20:00 --input-tz +08:00 --lat 31.23 --lon 121.47 --mode pick --pick sun,moon,Spica
```

## 工具命令

### info

查看版本、配置、星历文件状态和覆盖区间。详细参数与输出见 [info](./cli/info)。

```bash
lunar info [bsp] [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
```

`info` 默认输出 `txt`，不读取 `def_fmt` 作为默认格式。

```bash
lunar info
lunar info ./de442.bsp --format json --out info.json
```

### download

列出和下载内置下载表中的 BSP 文件。详细参数与输出见 [download](./cli/download)。

```bash
lunar download list
lunar download get <id> [--dir <path>] [--quiet]
```

```bash
lunar download list
lunar download get de442s --dir ./ephem
```

### completion

生成 shell 补全脚本。详细参数与输出见 [completion](./cli/completion)。

```bash
lunar completion bash|zsh|fish|powershell
```

脚本输出到 stdout，可按 shell 要求重定向到文件。

```bash
lunar completion powershell > lunar-completion.ps1
lunar completion bash > lunar-completion.bash
```

## 交互模式

直接运行 `lunar` 会进入交互模式。启动流程会读取 `lun_cfg.txt`，检查默认 BSP，可扫描额外目录、下载 BSP 或在启用级数回退时改用 `@series`。

交互菜单包含：

```text
1 months
2 calendar
3 at
4 zodiac
5 convert
6 day
7 next
8 festival
9 info
10 monthview
11 range
12 search
13 eclipse
14 almanac
15 config
16 completion
17 sky
18 export
d switch / download BSP
l switch language
h help
q quit
```
