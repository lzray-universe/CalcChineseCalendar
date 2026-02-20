
# CalcChineseCalendar（lunar）

> 一个基于 NASA/NAIF SPICE & JPL Ephemeris 的中国传统历法/天文历工具：可计算农历月、二十四节气、月相（朔/上弦/望/下弦），并提供 CLI、可嵌入的 C API（DLL/SO）与 JSON/CSV/ICS 等多种输出格式。

---

## 适用场景

- **生成某一年/某几年**的：
  - 农历月列表（含闰月、起止时刻）
  - 二十四节气时刻
  - 月相时刻（朔/上弦/望/下弦）
- **导出日历文件（.ics）**：把节气/月相写入系统日历（Google/Outlook/Apple Calendar）
- **查询某一时刻**的月相信息、照明率、太阳/月球黄经、并给出“最近的节气/相位事件”
- **公历 ↔ 农历**互转（含批处理：从 stdin / 文件读取多行）
- **日/月份视图**：给出某日（或某月每日）的农历、月相、当天事件摘要
- **节日/黄历**：春节/中秋/端午等固定节日、以及“某日黄历式摘要”（当日事件 + 当年节日落点）
- **检查星历文件可用性**、覆盖时间范围、以及工具自检
- **shell 补全脚本生成**：bash/zsh/fish/powershell

---

## 项目结构

- `src/`：CLI 与核心实现
- `include/lunar/`：对外头文件（C++ API、C API）
- `lunar`：CLI 可执行程序（CMake target）
- `lunar_dll`：动态库（导出 C API，便于其它语言绑定）

---

## 依赖与精度说明

release中可直接下载Windows、linux版本。

### 1) 必需：NAIF CSPICE
项目通过 CSPICE 读取并计算 JPL DE 星历（`.bsp`）。构建时需要：
- 头文件：`SpiceUsr.h`
- 库：`cspice`、`csupport`（静态或动态均可，CMake 里按 `find_library` 查找）

> **注意**：本仓库的 CMake 期望 CSPICE 目录形如：  
> `.../cspice/include/SpiceUsr.h`  
> `.../cspice/lib/(lib)cspice.*` & `.../cspice/lib/(lib)csupport.*`

默认查找逻辑（见 `CMakeLists.txt`）：
- Windows：`<repo>/dependent/cspice`
- Linux/macOS：优先 `$LUNAR_DEP_ROOT/cspice`（默认 `/usr/local/cspice`），找不到则回退 `<repo>/dependent-linux/cspice`

### 2) 可选：ERFA（更好的岁差/章动长期模型）
`-DLUNAR_USE_ERFA=ON` 时会链接 ERFA（见 `src/frames.cpp`），用于更精细的岁差/章动计算（尤其长期跨度）。
- 期望路径：`$LUNAR_DEP_ROOT/erfa` 或 `<repo>/dependent/erfa`
- 需要 `include/erfa.h` 以及 `src/` 中的 ERFA C 源

### 3) 星历文件（`.bsp`）
运行时需要一个 JPL DE 的 `.bsp` 文件，例如 `de440s.bsp / de442s.bsp`（覆盖 1850–2150 年，约 31MB）。

项目内置 `lunar download` 可直接下载 NAIF 公开地址（需要系统有 `curl` 或 `wget`）。

---

## 快速开始

### 1) 获取星历文件（推荐：de442s）
```bash
# 列出可下载项
lunar download list

# 下载到当前目录（或用 --dir 指定目录）
lunar download get de442s --dir .
````

### 2) 生成 2025 年节气 + 月相（并导出 ICS）

```bash
lunar calendar ./de442s.bsp 2025 --format ics --out cal_2025.ics
```

### 3) 看 2025 年的农历月（JSON）

```bash
lunar months ./de442s.bsp 2025 --format json --out months_2025.json --pretty 1
```

### 4) 查询某时刻月相（含最近事件）

```bash
lunar at ./de442s.bsp 2025-06-01T00:00:00+08:00 --format json --events 1
```

### 5) 公历转农历（或农历转公历）

```bash
# 公历 -> 农历
lunar convert ./de442s.bsp 2026-02-18 --format txt

# 农历 -> 公历（指定闰月用 --leap 1）
lunar convert ./de442s.bsp --from-lunar 2025 8 15 --leap 0 --format json
```

---

## CLI 总览

### 基本用法

```bash
lunar <subcommand> [args...] [options...]
```

常见子命令（完整列表见 `lunar --help`）：

| 命令           | 用途                          |
| ------------ | --------------------------- |
| `months`     | 输出某年/年份区间的农历月（或公历月枚举）       |
| `calendar`   | 输出某年/年份区间：节气 + 月相（可选包含月份）   |
| `year`       | 输出某一年的节气 + 月相 +（可选）农历月      |
| `event`      | 计算单个事件（某节气 / 某月相，靠近某日期）     |
| `download`   | 列出/下载星历 BSP                 |
| `at`         | 查询某时刻的月相/照明率等（支持批处理）        |
| `convert`    | 公历 ↔ 农历互转（支持批处理）            |
| `day`        | 某日：农历 + 月相 +（可选）当天事件        |
| `monthview`  | 某月每日：农历/照明率/事件摘要            |
| `next`       | 从某时刻起，取接下来 N 个事件            |
| `range`      | 取某时间段内的事件                   |
| `search`     | 简易搜索（目前只支持 `next ...` 形式）   |
| `festival`   | 某农历年的传统节日落点（春节/中秋等）         |
| `almanac`    | 某日“黄历摘要”（农历+月相+当天事件+当天节日）   |
| `info`       | 星历文件信息与覆盖范围                 |
| `selftest`   | 自检（快速 sanity check）         |
| `config`     | 查看/设置默认配置（写入 `lun_cfg.txt`） |
| `completion` | 生成 shell 补全脚本               |

---

## 输入时间格式

很多命令接受 `<time>` 或 `<dt_or_tm>`。解析规则由 `parse_iso()` 决定（见 `src/format.cpp`）：

支持：

* `YYYY-MM-DD`
* `YYYY-MM-DDZ` 或 `YYYY-MM-DD+08:00` / `YYYY-MM-DD-05:00`
* `YYYY-MM-DDTHH:MM`
* `YYYY-MM-DDTHH:MM:SS`
* `YYYY-MM-DDTHH:MM:SS.sss`
* 以上形式均可带 `Z`/`±HH:MM` 时区后缀

若输入字符串 **没有显式时区**：

* 使用 `--input-tz`（用于 `at/convert` 等）或配置中的 `default_tz` 作为解析默认时区

---

## 时区与“农历判日”规则

* `--tz`：**仅影响输出显示**（例如 `loc_iso`、`local_iso` 的时区偏移）
* 农历规则与判日（哪一天属于哪一个农历日）**固定按 UTC+8 的民用日边界**执行

  * 例如：`convert/day/monthview/almanac` 中会出现“农历判日固定 UTC+8”的注记
  * 这意味着：即使 `--tz Z`，农历日归属仍按东八区日界判定

---

## 输出格式与通用字段

大多数命令支持 `--format`：

* `txt`：人读友好的表格/键值行
* `json`：结构化输出（顶层对象）
* `jsonl`：逐行 JSON（适合流式/大批量；部分命令支持 `--meta-once`）
* `csv`：表格输出（逗号分隔）
* `ics`：iCalendar（可导入日历）

### JSON 通用 `meta`

多数 JSON 输出都有：

```json
{
  "meta": {
    "tool": "lunar",
    "version": "lunar-cli-2026.02",
    "schema": "lunar.v1",
    "ephem": "<bsp path>",
    "tz_display": "+08:00",
    "notes": ["..."]
  },
  "data": ...
}
```

字段含义：

* `tool`：固定为 `"lunar"`
* `version`：工具版本（`tool_ver()`）
* `schema`：固定为 `"lunar.v1"`
* `ephem`：使用的 `.bsp` 文件路径
* `tz_display`：显示时区（来自 `--tz`）
* `notes`：重要提示

---

## 核心数据结构

### 1) MonthRec（农历月记录）——`months/calendar/year` 的 `months`

```json
{
  "label": "正月",
  "month_no": 1,
  "is_leap": false,
  "st_jdutc": 2460705.024987337,
  "ed_jdutc": 2460734.53112871,
  "st_utc": "2025-01-29T12:35:58.906Z",
  "st_loc": "2025-01-29T20:35:58.906+08:00",
  "ed_utc": "2025-02-28T00:44:49.521Z",
  "ed_loc": "2025-02-28T08:44:49.521+08:00"
}
```

含义：

* `label`：人读标签（含干支年/月名等）
* `month_no`：农历月序号（1–12）
* `is_leap`：是否闰月
* `st_jdutc` / `ed_jdutc`：月起止时刻（UTC Julian Day）
* `st_utc`/`ed_utc`：UTC ISO 字符串
* `st_loc`/`ed_loc`：按 `--tz` 显示的本地时间 ISO

### 2) EventRec（事件）——两种变体

#### 2.1 变体 A：calendar/year/event（带 `jd_tdb`）

```json
{
  "kind": "solar_term",
  "code": "Z1",
  "name": "雨水",
  "year": 2025,
  "jd_tdb": null,
  "jd_utc": 2460724.921229934,
  "utc_iso": "2025-02-18T10:06:34.266Z",
  "loc_iso": "2025-02-18T18:06:34.266+08:00"
}
```

* `kind`：`solar_term` 或 `lunar_phase` 或 `festival`
* `code`：事件代码（节气 code / 月相 code / 节日 m-d）
* `name`：中文名（如“立春”“望”“春节”）
* `year`：事件所属年（通常是公历年；节日用农历年）
* `jd_tdb`：TDB Julian Day（当前实现多为 `null`）
* `jd_utc`：UTC Julian Day
* `utc_iso`：UTC ISO
* `loc_iso`：按 `--tz` 显示的本地 ISO

#### 2.2 变体 B：查询类命令（`at/day/next/range/...`，不含 `jd_tdb`，字段名略不同）

```json
{
  "kind": "solar_term",
  "code": "J5",
  "name": "芒种",
  "year": 2025,
  "jd_utc": 2460831.9142496972,
  "utc_iso": "2025-06-05T09:56:31.174Z",
  "loc_iso": "2025-06-05T17:56:31.174+08:00"
}
```

字段同上，区别是没有 `jd_tdb`。

### 3) LunDate（农历日期）——`at/convert/day/almanac` 中出现

```json
{
  "lunar_year": 2025,
  "lun_mno": 5,
  "is_leap": false,
  "lun_mlab": "五月",
  "lunar_day": 6,
  "lun_label": "农历2025年五月初六"
}
```

---

## 各子命令详解（含用例与输出）


---

### 1) `months`：列出某年（或区间）的月份

**用途**

* 做“农历月/闰月”结构分析
* 给 UI 或数据库准备“农历月起止边界”

**用法**

```bash
lunar months <bsp> <years>
  [--mode lunar|gregorian]
  [--format json|txt|csv] [--out <path>] [--tz Z|+08:00|-05:00]
  [--pretty 0|1] [--quiet]
```

* `<years>` 支持：`2025` 或 `2024-2026`
* `--mode lunar`：输出农历月（默认）
* `--mode gregorian`：输出公历月枚举（用于对照/调试）

**JSON 输出**

```json
{
  "meta": {...},
  "data": [
    {
      "year": 2025,
      "mode": "lunar",
      "months": [ MonthRec, ... ]
    }
  ]
}
```

**CSV 输出列**

* `year,mode,label,month_no,is_leap,st_jdutc,ed_jdutc,st_utc,st_loc,ed_utc,ed_loc`

---

### 2) `calendar`：某年/区间的节气 + 月相（可选月表）

**用途**

* 生成全年事件列表（节气/月相），做日历导出或提醒系统
* 同时包含月份边界（`--include-months 1`）

**用法**

```bash
lunar calendar <bsp> [<years>]
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz ...]
  [--include-months 0|1] [--pretty 0|1] [--quiet]
```

**TXT 输出**

* 先给“year summary”，再分区列出 `[solar_terms]`、`[lunar_phases]`、`[months]`（若包含）

**JSON 输出**

```json
{
  "meta": {...},
  "data": [
    {
      "year": 2025,
      "mode": "lunar",
      "sol_terms": [ EventRec(A), ... ],
      "lun_phase": [ EventRec(A), ... ],
      "months": [ MonthRec, ... ]   // 仅 include-months=1
    }
  ]
}
```

**ICS 输出**

* 生成 iCalendar，事件标题为 `name`（例如“立春”“朔”），可导入日历

---

### 3) `year`：单年输出（calendar 的单年便利版）

**用法**

```bash
lunar year <bsp> <year>
  [--months 0|1]
  [--format json|txt|ics] [--out ...] [--tz ...]
  [--pretty 0|1] [--quiet]
```

JSON 结构与 `calendar` 类似，但顶层 `data` 是单个对象（不是数组）。

---

### 4) `event`：计算单个事件

**用途**

* 只想算“某一年某节气”的精确时刻
* 或算“靠近某个日期的某个月相”（比如“离 2025-06-01 最近的满月”）

**用法（节气）**

```bash
lunar event <bsp> solar-term <code> <year>
  [--format json|txt|ics] [--out ...] [--tz ...] [--pretty 0|1] [--quiet]
```

**用法（月相）**

```bash
lunar event <bsp> lunar-phase <code> --near YYYY-MM-DD
  [--format json|txt|ics] ...
```

* `solar-term code` / `lunar-phase code` 的合法值可参考工具内置定义（也可先用 `calendar/year` 输出查看）

JSON 输出：`{ meta, data: EventRec(A) }`

---

### 5) `download`：下载星历

**用法**

```bash
lunar download list
lunar download get <id> [--dir <path>]
```

`list` 输出列：

* `id`：`de440/de440s/de441p1/de441p2/de442/de442s`
* `size`：大约体积
* `range`：覆盖年份范围
* `url`：下载地址（NAIF 公共站点）

---

### 6) `at`：查询某时刻（单次 / 批处理）

**用途**

* 输入一个时间点，得到：

  * 太阳/月球黄经（rad）
  * 角距（elongation），照明率（ill_pct）
  * 月相名（phase_name）
  * 该时刻对应的农历日期（按 UTC+8 判日）
  * 可选：最近的节气/相位事件（`--events 1`）

**单次用法**

```bash
lunar at <bsp> <time>
  [--input-tz Z|+08:00|-05:00] [--tz Z|+08:00|-05:00]
  [--events 0|1]
  [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
```

**JSON 输出（单次）**

```json
{
  "meta": {
    "tool": "lunar",
    "version": "lunar-cli-2026.02",
    "schema": "lunar.v1",
    "ephem": "/var/task/de442.bsp",
    "tz_display": "+08:00",
    "notes": []
  },
  "input": {
    "time_raw": "2025-06-01T00:00:00+08:00",
    "input_tz": "+08:00",
    "display_tz": "+08:00",
    "jd_utc": 2460827.1666666665,
    "jd_tdb": 2460827.1674674074,
    "utc_iso": "2025-05-31T16:00:00.000Z",
    "loc_iso": "2025-06-01T00:00:00.000+08:00"
  },
  "data": {
    "sun_lam": 1.2296133758553403,
    "moon_lam": 2.2879127151349525,
    "elongation_rad": 1.0582993392796123,
    "elongation_deg": 60.63608560220537,
    "ill_frac": 0.25482252256039406,
    "ill_pct": 25.482252256039406,
    "waxing": true,
    "phase_name": "wax_cres",
    "lunar_date": {
      "lunar_year": 2025,
      "lun_mno": 5,
      "is_leap": false,
      "lun_mlab": "五月",
      "lunar_day": 6,
      "lun_label": "农历2025年五月初六"
    },
    "near_ev": {
      "st_prev": {
        "kind": "solar_term",
        "code": "Z4",
        "name": "小满",
        "year": 2025,
        "jd_utc": 2460816.2879417166,
        "utc_iso": "2025-05-20T18:54:38.164Z",
        "loc_iso": "2025-05-21T02:54:38.164+08:00"
      },
      "st_next": {
        "kind": "solar_term",
        "code": "J5",
        "name": "芒种",
        "year": 2025,
        "jd_utc": 2460831.9142496972,
        "utc_iso": "2025-06-05T09:56:31.174Z",
        "loc_iso": "2025-06-05T17:56:31.174+08:00"
      },
      "lp_prev": {
        "kind": "lunar_phase",
        "code": "new_moon",
        "name": "朔",
        "year": 2025,
        "jd_utc": 2460822.6266325763,
        "utc_iso": "2025-05-27T03:02:21.055Z",
        "loc_iso": "2025-05-27T11:02:21.055+08:00"
      },
      "lp_next": {
        "kind": "lunar_phase",
        "code": "fst_qtr",
        "name": "上弦",
        "year": 2025,
        "jd_utc": 2460829.653450671,
        "utc_iso": "2025-06-03T03:40:58.138Z",
        "loc_iso": "2025-06-03T11:40:58.138+08:00"
      }
    }
  }
}
```

字段说明：

* `sun_lam` / `moon_lam`：太阳/月球黄经（弧度）
* `elongation_*`：日月黄经差（角距）
* `illum_*`：月面照明率（0–1 / 0–100）
* `waxing`：是否在“盈”（照明率上升），用黄经变化率近似判断
* `phase_name`：由角距判断的相位名称（如“朔”“望”“上弦”“下弦”“盈凸月”等）
* `near_events`：最近节气/月相事件（仅 `--events 1` 时出现）

#### 批处理模式（stdin / file）

```bash
# 从文件读多行时间
lunar at <bsp> --file times.txt --format jsonl --meta-once 1

# 从 stdin 读多行时间
cat times.txt | lunar at <bsp> --stdin --format jsonl --meta-once 1
```

* `--format jsonl`：每行一个 JSON（便于流式处理）
* `--meta-once 1`：只在第一行输出一个 `{"meta": ...}`，后续行只输出数据行对象

jsonl 数据行结构：

* `line_no`：输入行号（从 1 开始）
* `raw`：原始行文本
* `input` / `data`：成功时输出
* `error.message`：失败时输出错误信息

---

### 7) `convert`：公历 ↔ 农历互转（单次 / 批处理）

**重要提示**：农历判日固定按 UTC+8 民用日边界；`--tz` 只影响显示。

#### 公历 -> 农历

```bash
lunar convert <bsp> <dt_or_tm>
  [--input-tz ...] [--tz ...]
  [--format json|txt|jsonl] [--out ...] [--pretty 0|1] [--quiet]
```

#### 农历 -> 公历

```bash
lunar convert <bsp> --from-lunar <lunar_year> <month_no> <day> [--leap 0|1]
  [--tz ...] [--format json|txt|jsonl] ...
```

**JSON 输出（单次）**

```json
{
  "meta": {...},
  "input": {
    "direction": "greg2lun" | "lun2greg",
    "input_tz": "+08:00",
    "jd_utc": 2460xxx.x
  },
  "data": {
    "lunar_date": LunDate,
    "gcst_date": "YYYY-MM-DD",
    "gcst_jd": 2460xxx.x
  }
}
```

* `gcst_date`：对应的“UTC+8 民用日”（即“中原时区日”的公历日期）
* `gcst_jd`：该民用日 00:00（UTC+8）对应的 UTC Julian Day（实现上为 `cstday_jd`）

#### 批处理

```bash
# 公历批处理：文件中每行一个时间
lunar convert <bsp> --file dates.txt --format jsonl --meta-once 1

# 农历批处理：每行 "lunar_year month_no day [leap]"
lunar convert <bsp> --from-lunar 0 0 0 --stdin --format jsonl --meta-once 1
```

> 注意：当开启 `--from-lunar` 批处理时，实际读取的是 stdin/file 中每行的“农历三元组”，命令行里那组三元组仅用于满足解析器要求，不会被使用；可填 `0 0 0`。

---

### 8) `day`：某日摘要

**用途**

* 给某个公历日（YYYY-MM-DD）生成：

  * 农历日期（按 UTC+8 判日）
  * 指定采样时刻（默认 12:00:00）处的照明率/相位名
  * （可选）当天发生的节气/月相事件

**用法**

```bash
lunar day <bsp> <YYYY-MM-DD>
  [--at HH:MM:SS]
  [--events 0|1]
  [--tz ...] [--format json|txt|csv|jsonl] [--out ...]
```

**JSON 输出**

```json
{
  "meta": {
    "tool": "lunar",
    "version": "lunar-cli-2026.02",
    "schema": "lunar.v1",
    "ephem": "/var/task/de442.bsp",
    "tz_display": "+08:00",
    "notes": ["农历判日固定UTC+8"]
  },
  "input": {
    "date": "2025-06-01",
    "smp_time": "12:00:00",
    "smp_jdutc": 2460827.6666666665
  },
  "data": {
    "lunar_date": {
      "lunar_year": 2025,
      "lun_mno": 5,
      "is_leap": false,
      "lun_mlab": "五月",
      "lunar_day": 6,
      "lun_label": "农历2025年五月初六"
    },
    "ill_pct": 30.259265163809857,
    "phase_name": "wax_cres",
    "smp_uiso": "2025-06-01T04:00:00.000Z",
    "smp_liso": "2025-06-01T12:00:00.000+08:00",
    "events": []
  }
}
```

**CSV 列**

* `date,lun_label,lun_mlab,is_leap,lunar_day,ill_pct,phase_name,smp_tlociso,ev_sum`

---

### 9) `monthview`：某月每日摘要

**用途**

* 输出某 `YYYY-MM` 每天的：

  * 农历日期标签
  * 是否闰月、月名
  * 照明率（中午采样）
  * 当天事件名称摘要（用 `|` 拼接）

**用法**

```bash
lunar monthview <bsp> <YYYY-MM>
  [--tz ...] [--format json|txt|csv] [--out ...]
```

**JSON 输出**
`data` 是数组，每行一个公历日：

```json
{
  "greg_date": "2025-09-01",
  "lun_label": "农历2025年七月初十",
  "is_leap": false,
  "lun_mlab": "七月",
  "ill_pct": 58.637611311571355,
  "ev_sum": ""
}
```

**CSV 列**

* `greg_date,lun_label,is_leap,lun_m_label,ill_pct,ev_sum`

---

### 10) `next` / `range` / `search`：事件检索

#### `next`：从某时刻起取接下来 N 个事件

```bash
lunar next <bsp> --from <time> --count N
  [--kinds solar_term,lunar_phase]
  [--tz ...] [--format json|txt|csv|ics|jsonl] [--out ...]
```

#### `range`：取时间段内事件

```bash
lunar range <bsp> --from <time> --to <time>
  [--kinds solar_term,lunar_phase]
  [--tz ...] [--format json|txt|csv|ics|jsonl] [--out ...]
```

#### `search`：简易语法（当前只支持以 `next` 开头）

```bash
lunar search <bsp> "<query>" [--from <time>] [--count N]
```

示例：

```bash
lunar search ./de442s.bsp "next full_moon" --from 2025-06-01
lunar search ./de442s.bsp "next solar_term 立春" --from 2025-01-01 --format json
```

输出：

* `json`：`{ meta, data: [EventRec(B), ...] }`
* `jsonl`：每行 `{ meta, data: EventRec(B) }`
* `csv`：`kind,code,name,year,jd_utc,utc_iso,loc_iso`
* `ics`：日历文件

---

### 11) `festival`：传统节日落点

内置节日：

* 春节（1-1）、元宵（1-15）、端午（5-5）、七夕（7-7）、中秋（8-15）、重阳（9-9）、腊八（12-8）、除夕（12-last）

**用法**

```bash
lunar festival <bsp> <year>
  [--tz ...] [--format json|txt|csv] [--out ...]
```

输出是 `EventRec(B)` 列表（`kind="festival"`，`code="m-d"` 或 `"12-last"`）。

---

### 12) `almanac`：某日黄历式摘要

**用法**

```bash
lunar almanac <bsp> <YYYY-MM-DD>
  [--tz ...] [--format json|txt|csv] [--out ...]
```

**JSON 输出**

```json
{
  "meta": {
    "tool": "lunar",
    "version": "lunar-cli-2026.02",
    "schema": "lunar.v1",
    "ephem": "/var/task/de442.bsp",
    "tz_display": "+08:00",
    "notes": ["农历判日固定UTC+8"]
  },
  "input": {
    "date": "2025-09-17"
  },
  "data": {
    "lunar_date": {
      "lunar_year": 2025,
      "lun_mno": 7,
      "is_leap": false,
      "lun_mlab": "七月",
      "lunar_day": 26,
      "lun_label": "农历2025年七月廿六"
    },
    "ill_pct": 21.516017482142264,
    "phase_name": "wan_cres",
    "events": [],
    "festivals": []
  }
}
```

CSV 列：

* `date,lun_label,ill_pct,phase_name,events,festivals`

  * `events/festivals` 用 `|` 拼接摘要

---

### 13) `info`：星历文件信息

**用法**

```bash
lunar info <bsp> [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
```

`json.data`：

* `exists`：文件是否存在
* `fsize_b`：文件大小（字节，超过 int 上限会截断）
* `leap_last`：内置闰秒表最后日期（当前为 `2017-01-01`）
* `delt_str`：ΔT/闰秒处理策略说明
* `spk_cov`：若可解析星历覆盖范围则给出：

  * `jd_tstart` / `jd_tdb_end`
  * `u_sisoap` / `u_eisoap`：覆盖范围的 UTC ISO（近似换算）
* `tool_ver` / `build_time`

---

### 14) `selftest`：自检

**用法**

```bash
lunar selftest <bsp> [--format json|txt] [--out ...]
```

`json.data`：

* `pass`：整体是否通过
* `cases[]`：每个测试用例的 `id/pass/message`

---

### 15) `config`：默认配置（写入 `lun_cfg.txt`）

配置文件名固定：`lun_cfg.txt`（当前工作目录下）。

**查看**

```bash
lunar config show [--format json|txt]
```

**设置**

```bash
lunar config set <key> <value>
# key: def_bsp | bsp_dir | default_tz | def_fmt | def_prety
```

* `def_bsp`：默认 bsp 文件路径（交互模式会优先用）
* `bsp_dir`：bsp 搜索目录（交互模式）
* `default_tz`：无时区输入时的默认解析时区
* `def_fmt`：默认输出格式
* `def_prety`：默认 JSON pretty（拼写沿用实现）

---

### 16) `completion`：生成补全脚本

```bash
lunar completion bash > lunar-completion.bash
lunar completion zsh  > _lunar
lunar completion fish > lunar.fish
lunar completion powershell > lunar-completion.ps1
```

---

## 交互模式

直接运行：

```bash
lunar
```

会进入交互式菜单（`src/interact.cpp`），可引导选择 bsp、设置默认输出/时区，并执行 `months/calendar/at/convert` 等操作。

---

## 构建（CMake）

> 下面给的是推荐方式。可以根据自己的 CSPICE 安装方式调整。

### Linux/macOS

从此处下载[NAIF CSPICE Toolkit for linux GCC](https://naif.jpl.nasa.gov/pub/naif/toolkit//C/PC_Linux_GCC_64bit/packages/cspice.tar.Z)

可选下载ERFA:

- [仓库地址](https://github.com/liberfa/erfa)

1. 确保 CSPICE 安装在 `/usr/local/cspice`，或设置环境变量：

```bash
export LUNAR_DEP_ROOT=/path/to/deps   # 其中包含 cspice/ 与可选的 erfa/
```

2. 构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

可选开启 ERFA：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLUNAR_USE_ERFA=ON
cmake --build build -j
```

### Windows

推荐使用MSVC环境，可从[此处下载Visual Studio](https://visualstudio.microsoft.com/)。

从此处下载[NAIF CSPICE Toolkit for MSVC](https://naif.jpl.nasa.gov/pub/naif/toolkit//C/PC_Windows_VisualC_64bit/packages/cspice.zip)

推荐重新编译CSPICE：

- 解压CSPICE，找到`makeall.bat`所在目录。

- 打开x64 Native Tools Command Prompt for VS，cd到`makeall.bat`所在目录，直接运行`makeall.bat`。

- 待执行完毕后将`makeall.bat`所在目录所有文件拷贝到`<repo>/dependent/cspice/`中

可选下载ERFA:

- [仓库地址](https://github.com/liberfa/erfa)
- 解压，将所有文件拷贝到`<repo>/dependent/erfa/`中。

然后：

```powershell
  # 1) 配置（启用 ERFA），若不用可关闭ERFA
  VS26:cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DLUNAR_USE_ERFA=ON
  VS22:cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DLUNAR_USE_ERFA=ON

  # 2) 先编译 ERFA 静态库，若不用可关闭忽略
  cmake --build build --config Release --target erfa --parallel

  # 3) 再编译主程序（静态 CRT + USE_ERFA）
  cmake --build build --config Release --target lunar --parallel

```

---

## 作为库使用（C API / DLL）

动态库目标：`lunar_dll`（导出见 `include/lunar/c_api.h`）

核心导出：

* `lunar_main(argc, argv)`：以“嵌入方式”调用 CLI（便于从其它语言复用 CLI）
* `lunar_run_command(jsonRequestUtf8, outBuf, outCap)`：传入 JSON 请求，返回 JSON 响应（便于做稳定 API）
* `lunar_last_error()`：获取上次错误信息
* `lunar_tool_ver()`：版本号

### `lunar_run_command` 请求/响应

请求示例：

```json
{
  "cmd": "at",
  "ephem": "./de442s.bsp",
  "time": "2025-06-01T00:00:00+08:00",
  "input_tz": "+08:00",
  "tz": "+08:00",
  "format": "json",
  "pretty": true,
  "events": true
}
```

响应：

* 成功：`{"ok":true,"output":"<utf8 text>" ...}`
* 失败：`{"ok":false,"error":"..."}`
  （详见 `src/c_api.cpp` 的实现逻辑；输出内容与 CLI 对应命令一致）

---

## 已知限制 / 备注

* **闰秒表**：内置表到 `2017-01-01`（见 `info` 输出），至2026年暂无新增闰秒，超出部分用近似策略（`delt_str`）
* **农历判日固定 UTC+8**：这是一种实现选择，用于与常见中国民用农历一致，因为中国是农历的最大使用国；并非随 `--tz` 改变
* `.bsp` 覆盖范围受所选星历文件限制（`info` 可查看 coverage）
* 批处理模式的 `--jobs` 当前只作为参数接受，执行仍为顺序

---

## License

MIT LICENSE

---

## 致谢

* NASA/NAIF SPICE Toolkit
* JPL Development Ephemeris
* ERFA / IAU SOFA 
