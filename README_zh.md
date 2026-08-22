# CalcChineseCalendar（lunar）

CalcChineseCalendar 是一个优先使用 JPL DE BSP 星历、在缺失 BSP 时可自动回退到仓库内置 VSOP87A + ELPMPP02 的历法与天文计算工具，提供：

- 命令行工具 `lunar`
- 动态库 `lunar_dll`（导出 C API）
- 面向 C++ 的可复用计算接口（`lunar::core`）
- Python 包：`calcchinesecalendar`，可通过 `pip install calcchinesecalendar` 安装，并以 `import calcchinesecalendar` 使用
- npm 包：`calcchinesecalendar`，可通过 `npm install calcchinesecalendar` 安装，并以 `import ... from "calcchinesecalendar"` 使用
<div align="center">

  <p>
    <a href="https://github.com/lzray-universe/CalcChineseCalendar/actions/workflows/CI.yml">
      <img src="https://github.com/lzray-universe/CalcChineseCalendar/actions/workflows/CI.yml/badge.svg" alt="CI">
    </a>
    <img src="https://img.shields.io/github/v/release/lzray-universe/CalcChineseCalendar" alt="Release">
    <img src="https://img.shields.io/github/last-commit/lzray-universe/CalcChineseCalendar" alt="Last commit">
  </p>

  <p>
    <a href="https://tokei.kojix2.net/github/lzray-universe/CalcChineseCalendar">
      <img src="https://img.shields.io/endpoint?url=https%3A%2F%2Ftokei.kojix2.net%2Fbadge%2Fgithub%2Flzray-universe%2FCalcChineseCalendar%2Flines" alt="Lines of Code">
    </a>
    <img src="https://img.shields.io/github/repo-size/lzray-universe/CalcChineseCalendar" alt="Repo size">
    <a href="LICENSE">
      <img src="https://img.shields.io/github/license/lzray-universe/CalcChineseCalendar" alt="License">
    </a>
    <a href="https://deepwiki.com/lzray-universe/CalcChineseCalendar">
      <img src="https://deepwiki.com/badge.svg" alt="DeepWiki">
    </a>
  </p>

  <p>
    <img src="https://img.shields.io/badge/platform-Windows-blue" alt="Windows">
    <img src="https://img.shields.io/badge/platform-Linux-green" alt="Linux">
    <img src="https://img.shields.io/badge/platform-macOS-lightgrey" alt="macOS">
    <img src="https://img.shields.io/badge/platform-WebAssembly-orange" alt="WebAssembly">
  </p>

</div>

## 目录

- [1. 功能概览](#1-功能概览)
- [2. 构建](#2-构建)
- [3. 运行前准备：BSP 星历（可选但优先）](#3-运行前准备bsp-星历可选但优先)
- [4. CLI 入口与全局参数](#4-cli-入口与全局参数)
- [5. 自动 BSP 选择机制](#5-自动-bsp-选择机制)
- [6. 配置文件 `lun_cfg.txt`](#6-配置文件-lun_cfgtxt)
- [7. 时间格式与时区规则](#7-时间格式与时区规则)
- [8. 输出格式](#8-输出格式)
- [9. 命令参考](#9-命令参考)
- [10. 交互模式](#10-交互模式)
- [11. i18n（中简/中繁/英/日/韩）](#11-i18n中简中繁英日韩)
- [12. C++ API（Library-First）](#12-c-api库优先)
- [13. C API](#13-c-api)
- [14. 仓库结构](#14-仓库结构)
- [15. 致谢](#15-致谢)
- [16. 快速示例](#16-快速示例)
- [17. 输出字段代号说明](#17-输出字段代号说明)
- [18. 主要函数与结构体变量说明](#18-主要函数与结构体变量说明)

## 1. 功能概览

- 农历月枚举：`months`
- 年度节气/月相日历：`calendar`、`year`
- 单事件求解：`event`
- 星历下载：`download`
- 时刻查询与批处理：`at`
- 公历/农历互转与批处理：`convert`
- 太阳星座与观测天空位置：`zodiac`、`sky`
- 单日、单月与批量逐日导出：`day`、`monthview`、`export`
- 事件检索：`next`、`range`、`search`
- 日月食：`eclipse`
- 传统节日与黄历摘要：`festival`、`almanac`
- 星历信息：`info`
- 配置管理与补全脚本：`config`、`completion`
- 交互式模式（无参数启动）

## 2. 构建

### 2.1 环境要求

- CMake 3.20+
- 支持 C++20 的编译器
- 运行时优先使用 BSP 文件（`.bsp`）；若未找到可自动回退到内置 VSOP87A + ELPMPP02
- `download get` 需要系统中存在 `curl` 或 `wget`

### 2.2 Windows（Visual Studio）

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

### 2.3 Linux / macOS（Ninja 示例）

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 2.4 WebAssembly（Emscripten）

```powershell
git clone https://github.com/emscripten-core/emsdk.git D:\tools\emsdk
D:\tools\emsdk\emsdk install latest
D:\tools\emsdk\emsdk activate latest
cmd /c "call D:\tools\emsdk\emsdk_env.bat && D:\tools\emsdk\upstream\emscripten\emcmake.bat cmake -S . -B build_wasm -G Ninja -DLUNAR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release"
cmd /c "call D:\tools\emsdk\emsdk_env.bat && cmake --build build_wasm --parallel"
```

说明：

- 产物为 `build_wasm/lunar.js` 与 `build_wasm/lunar.wasm`
- 兼容包装还会生成 `build_wasm/lunar_worker.js`
- 保留现有 CLI 入口；Node 下可直接执行 `node build_wasm/lunar.js --version`
- wasm 构建默认启用异常支持、文件系统支持、`FS`/`callMain` 导出与内存增长
- Node 下会自动把当前工作目录挂载到 wasm 虚拟文件系统，当前目录内相对 `.bsp` 路径可直接使用
- Windows 下会自动兼容 `D:\path\to\file.bsp` 这类盘符绝对路径
- 浏览器 Worker 下可直接使用 `lunar_worker.js`，按 CLI 的 `argv` 方式调用
- Worker 支持两种输入：`files` 写入内存文件，或 `mounts` 通过 `WORKERFS` 挂载 `File`/`Blob`
- `lunar_worker.js` 保持 CLI 语义：单次执行后自动退出；下一条命令请新建 Worker
- 若不需要 BSP，可继续使用 `@series`

浏览器 Worker 示例：

```js
const worker=new Worker('./lunar_worker.js');

worker.onmessage=({data})=>{
  if(data.type==='ready'){
    worker.postMessage({
      argv:[
        'day','2025-06-01',
        '--bsp','/ephem/'+spkFile.name,
        '--format','txt'
      ],
      mounts:[
        {path:'/ephem',files:[spkFile]}
      ]
    });
    return;
  }
  if(data.type==='result'){
    console.log(data.exit_code);
    console.log(data.stdout);
    console.error(data.stderr);
  }
};
```

浏览器 Worker 也可先写入内存文件：

```js
worker.postMessage({
  argv:[
    'day','2025-06-01',
    '--bsp','/ephem/de442s.bsp',
    '--format','txt'
  ],
  files:[
    {path:'/ephem/de442s.bsp',data:spkArrayBuffer}
  ]
});
```

### 2.5 产物

- 可执行程序：`lunar`
- 动态库：`lunar_dll`（输出名为 `lunar`，对应平台扩展名）
- 测试程序：`lunar_tests`（启用 `LUNAR_BUILD_TESTS` 时）
- wasm 构建还会生成：`lunar.js`、`lunar.wasm`、`lunar_worker.js`

### 2.6 可选构建开关

- `-DLUNAR_ENABLE_DIMENSION_TYPES=ON|OFF`
  - `ON`（默认）：启用零开销的编译期物理量标签向量类型
  - `OFF`：退回到普通 `Vec3` 别名，接口行为不变
- `-DLUNAR_ENABLE_SERIES_FALLBACK=ON|OFF`
  - `ON`（默认）：未找到 BSP 时自动切换到内置 VSOP87A + ELPMPP02
  - `OFF`：保持旧行为，必须提供可用 BSP
- `-DLUNAR_ENABLE_THREADS=ON|OFF`
  - `ON`（默认）：启用内部多线程执行路径
  - `OFF`：保持单线程，计算结果不变
- `-DLUNAR_BUILD_TESTS=ON|OFF`
  - `ON`（默认）：构建 gtest/ctest 测试目标
  - `OFF`：不构建测试目标
- `-DLUNAR_ENABLE_IPO=ON|OFF`
  - `ON`（默认）：在 Release / RelWithDebInfo 下启用 IPO/LTO
- `-DLUNAR_ENABLE_FAST_MATH=ON|OFF`
  - `OFF`（默认）：保持严格浮点语义
- `-DLUNAR_VERSION=<text>`
  - 默认：`test`
  - `lunar --version`、`info` 和 C API 版本字符串都使用这个值
- Release 构建应通过 `-DLUNAR_VERSION=<release tag>` 传入版本；未传时统一为 `test`

### 2.7 运行测试

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

若使用 Visual Studio 这类多配置生成器，可改为：

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

若关闭 `LUNAR_ENABLE_SERIES_FALLBACK`，可通过环境变量 `LUNAR_TEST_BSP` 指定测试所用 BSP。

## 3. 运行前准备：BSP 星历（可选但优先）

### 3.1 下载列表

```bash
lunar download list
```

当前内置 ID：

- `de440`（约 114MB，1550-2650）
- `de440s`（约 31MB，1850-2150）
- `de441p1`（约 1.5GB，-13200-1969）
- `de441p2`（约 1.5GB，1969-17191）
- `de442`（约 114MB，1550-2650）
- `de442s`（约 31MB，1850-2150）

### 3.2 下载命令

```bash
lunar download get de442s --dir .
```

若当前网络环境无法直接访问 JPL NAIF 官方站，可改用镜像站 `https://naifproxy.lzray.cloud/` 手动下载对应 BSP 文件；镜像路径与官方目录结构保持一致，例如 `https://naifproxy.lzray.cloud/pub/naif/generic_kernels/spk/planets/de442s.bsp`。

## 4. CLI 入口与全局参数

### 4.1 主入口

```bash
lunar --help
lunar --version
lunar <command> [args...]
```

无参数启动会进入交互模式。

### 4.2 全局参数

- `--eclipse-method modern|legacy`
- `--lang zh|zht|en|ja|ko`

`--lang` 也接受常见别名：

- `zh-cn`/`cn`/`zh-hans`
- `zh-tw`/`zh-hk`/`zh-hant`/`tw`/`hk`/`hant`
- `en-us`/`us`
- `ja-jp`/`jp`
- `ko-kr`/`kr`

### 4.3 全局 BSP 覆盖参数

所有需要星历的命令都支持显式指定：

- `--bsp <path>`
- `--bsp=<path>`

示例：

```bash
lunar day 2025-06-01 --bsp ./de442.bsp
```

### 4.4 兼容语法

以下可用，等价于 `months`：

```bash
lunar <bsp> <years> [months options...]
```

## 5. 自动 BSP 选择机制

`entry` 会在命令分发前调用 `prep_cmd_args()` 自动补 `bsp`。

说明：

- 下文命令参考中写作 `[bsp]` 的命令，均表示该位置参数可以省略；省略时会走本节自动选择逻辑。
- `zodiac` 与 `sky` 当前不走这套自动补齐逻辑，仍需显式传入位置参数 `<bsp>`。

### 5.1 哪些命令需要 BSP

- `months calendar year event at convert day monthview export next range search eclipse festival almanac info`

不需要 BSP 的命令：

- `download config completion`

### 5.2 候选来源顺序

按以下顺序收集并去重（仅保留存在的文件）：

1. `def_bsp`
2. `bsp_list`
3. `bsp_dir` 目录下所有 `.bsp`
4. 当前工作目录下所有 `.bsp`

### 5.3 选择策略

- 若命令可推断时间区间，优先选择“完整覆盖该区间”的 BSP。
- 若无完整覆盖，选择与区间重叠最多的 BSP。
- 若无法推断区间，使用候选列表第一个。
- 若没有可用候选且 `LUNAR_ENABLE_SERIES_FALLBACK=ON`，自动切换到内置 VSOP87A + ELPMPP02。
- 若没有可用候选且 `LUNAR_ENABLE_SERIES_FALLBACK=OFF`，抛出错误并提示使用 `--bsp` 或 `lunar config set def_bsp`。

## 6. 配置文件 `lun_cfg.txt`

### 6.1 支持键

- `bsp_dir`
- `def_bsp`
- `bsp_list`
- `default_tz`
- `default_lang`
- `default_lunar_day_tz`
- `def_fmt`
- `hli_trad`
- `hli_year_boundary`
- `hli_month_boundary`
- `hli_leap_month_mode`
- `hli_day_boundary`
- `def_prety`

### 6.2 命令

```bash
lunar config show [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

### 6.3 细节

- `bsp_list` 支持逗号或分号分隔。
- `def_bsp` 设置后若不在 `bsp_list` 中，会自动追加。
- `default_lang` 仅允许 `zh|zht|en|ja|ko`。
- `default_lunar_day_tz` 用于覆盖农历判日日界时区；留空时会按语言默认值自动推断（当前 `ja/ko -> +09:00`，其余为 `+08:00`）。
- `def_fmt` 允许 `txt|json|csv|jsonl|ics`。
- `hli_trad`、`hli_year_boundary`、`hli_month_boundary`、`hli_leap_month_mode`、`hli_day_boundary` 会作为 `day` / `almanac` 的默认黄历规则。
- 上述 `hli_*` 键支持写回稳定英文 key；除 `hli_trad` 外，也支持用 `default` 恢复内建默认。
- `def_prety` 用 `0|1`。

## 7. 时间格式与时区规则

### 7.1 `parse_iso` 支持

- `YYYY-MM-DD`
- `YYYY-MM-DDZ`
- `YYYY-MM-DD+08:00` / `YYYY-MM-DD-05:00`
- `YYYY-MM-DDTHH:MM`
- `YYYY-MM-DDTHH:MM:SS`
- `YYYY-MM-DDTHH:MM:SS.sss`
- 以上均可带 `Z` 或 `±HH:MM`

如果输入不带时区，使用命令参数中的解析时区（如 `--input-tz`）或配置中的 `default_tz`。

### 7.2 `--tz` 与农历判日

- 多数命令里，`--tz` 只影响显示时区；`zodiac --year` 例外，它同时决定公历年裁剪窗口。
- 农历判日优先使用命令行 `--lunar-day-tz`；未显式提供时，读取配置 `default_lunar_day_tz`，再缺省时按语言自动推断（当前 `ja/ko -> +09:00`，其余为 `+08:00`）。

## 8. 输出格式

命令按各自支持范围可输出：

- `txt`
- `json`
- `jsonl`
- `csv`
- `ics`
- `geojson`（食相关）

多数 JSON 输出包含 `meta`，核心字段通常包括：

- `tool`
- `version`
- `schema`
- `ephem`
- `tz_display`
- `notes`

## 9. 命令参考

说明：

- 本节中写作 `[bsp]` 的命令可省略该位置参数；显式给出时优先使用显式值。

### 9.1 months

```bash
lunar months [bsp] <years>
  [--mode lunar|gregorian]
  [--format json|txt|csv] [--out <path>] [--tz +08:00|Z|-05:00]
  [--pretty 0|1] [--quiet] [--include-eclipses 0|1]
  [--output <json>] [--output-txt <txt>]   # deprecated
```

说明：

- `<years>` 支持：`2025`、`2024-2026`、`2024,2026,2030-2032`。
- `--include-eclipses 1` 时，`--format csv` 不可直接使用。
- `--output/--output-txt` 为旧接口；不能与 `--out` 同时使用。
- 多年计算时若非 `--quiet`，stderr 仅输出年份进度。

### 9.2 calendar

```bash
lunar calendar [bsp] [<years>]
  [--format json|txt|ics] [--out <path>] [--tz ...]
  [--include-months 0|1] [--include-eclipses 0|1] [--pretty 0|1] [--quiet]
```

说明：

- 省略 `<years>` 时默认使用 `2025`。
- `ics` 仅导出节气和月相事件。
- 多年计算时若非 `--quiet`，stderr 仅输出年份进度。

### 9.3 year

```bash
lunar year [bsp] <year>
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz ...]
  [--pretty 0|1] [--quiet]
```

### 9.4 event

```bash
lunar event [bsp] solar-term <code> <year>
lunar event [bsp] lunar-phase <new_moon|fst_qtr|full_moon|lst_qtr> --near <YYYY-MM-DD>
  [--format json|txt|ics] [--out <path>] [--tz ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]

# 也支持直接转发到 eclipse 命令：
lunar event [bsp] lunar-eclipse --near <YYYY-MM-DD>
  [--stage any|umb|total] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz ...] [--pretty 0|1] [--quiet]
lunar event [bsp] solar-eclipse --near <YYYY-MM-DD>
  [--stage any|central] [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1] [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--format json|txt|geojson] [--out <path>] [--tz ...] [--pretty 0|1] [--quiet]
```

说明：

- `solar-term` 的 `code` 使用 `J1..J12`、`Z1..Z12`。
- `lunar-phase` 必须提供 `--near`。
- `--eclipse 1` 用于在满月事件输出中附加月食细节。
- `lunar-eclipse` / `solar-eclipse` 分类会直接转发到 `eclipse` 命令，因此参数与校验规则和 `eclipse` 保持一致。

### 9.5 download

```bash
lunar download list
lunar download get <id> [--dir <path>] [--quiet]
```

若官方 NAIF 站点不可达，可改用镜像站 `https://naifproxy.lzray.cloud/` 手动获取对应 BSP 文件后再通过 `--bsp`、配置文件或工作目录进行加载。

### 9.6 at

```bash
lunar at [bsp] <time>
lunar at [bsp] --time <time>
lunar at [bsp] --stdin
lunar at [bsp] --file <path>
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--events 0|1] [--eot-lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--jobs N] [--meta-once 0|1]
```

说明：

- 批模式：`--stdin` 与 `--file` 互斥。
- 批模式下若同时给位置参数 `<time>`，该位置参数会被忽略。
- 单次模式下 `--format jsonl` 会自动回退为 `json`。
- `--jobs` 已接受，但当前执行器仍按顺序运行。
- `--lunar-day-tz` 控制农历日期映射所用的民用日边界；黄历规则参数与 `day` / `almanac` 共用同一套解析。
- 默认 `tz/format/pretty` 来自 `lun_cfg.txt`（`default_tz/def_fmt/def_prety`）。

### 9.7 convert

```bash
lunar convert [bsp] <dt_or_tm>
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --from-lunar <lunar_year> <month_no> <day> [--leap 0|1]
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]

lunar convert [bsp] --stdin
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--jobs N] [--meta-once 0|1]

lunar convert [bsp] --file <path>
  [--input-tz ...] [--tz ...]
  [--lunar-day-tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--jobs N] [--meta-once 0|1]
```

说明：

- `--from-lunar` 模式下不能再传位置参数 `<dt_or_tm>`。
- `--stdin` 与 `--file` 互斥。
- 单次模式下 `--format jsonl` 会自动回退为 `json`。
- `--lunar-day-tz` 同时影响公历转农历与农历转公历时所采用的民用日边界。
- 默认 `tz/format/pretty` 来自配置。

### 9.8 day

```bash
lunar day [bsp] <YYYY-MM-DD>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv|jsonl] [--out ...] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

说明：

- `--astro-lat` 与 `--astro-lon` 必须同时出现。
- `--astro-height` 只能在已给经纬度时使用。
- 逻辑实现走 `lunar::core::compute_day` + `lunar::core::format_day_output`。
- 默认 `tz/format/pretty` 来自配置。
- 还支持黄历规则补充参数：`--lunar-day-tz`、`--trad`、`--year-boundary`、`--month-boundary`、`--leap-month-mode`、`--day-boundary`。
- `--trad/--year-boundary/--month-boundary/--leap-month-mode/--day-boundary` 也接受常见 ASCII 别名。

### 9.9 monthview

```bash
lunar monthview [bsp] <YYYY-MM>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

说明：

- `--astro-lat` 与 `--astro-lon` 必须同时出现。
- 默认 `tz/format/pretty` 来自配置。
- 还支持 `--lunar-day-tz ...`，用于显式覆盖农历判日日界时区。

### 9.9.1 export

```bash
lunar export [bsp] <YYYY-MM>
lunar export [bsp] --from <YYYY-MM> --to <YYYY-MM>
lunar export [bsp] --from-year <year> --to-year <year>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|jsonl|csv|txt] [--out ...] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--jobs N] [--events 0|1] [--eclipse 0|1]
  [--scope basic|full] [--huangli off|folk|ziping|purple|xieji|all] [--lon <deg>]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

说明：

- 区间按 `--lunar-day-tz` 下的公历月闭区间导出。
- 每个导出日包含公历日期、农历日期、年/月/日干支，以及当日节气或月相事件的精确本地时间。
- 可选数据可追加食事件、天象事件与黄历内容；`--huangli all` 会输出全部已支持黄历流派。
- `--scope full` 会展开食事件、less 模式天象事件与全部黄历流派。
- JSON 与 JSONL 保留每日嵌套结构；CSV 是扁平摘要格式。
- `--jobs 1` 强制使用单线程逐日循环；更大的值会在启用内部线程时使用多线程逐日循环。

### 9.10 next

```bash
lunar next [bsp] --from <time> --count N
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz ...] [--format json|txt|csv|ics|jsonl]
  [--out ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

说明：

- `--from` 必填，`--count >= 1`。
- 默认 `kinds` 为四类并集；当前实现的基础事件池来自节气与月相，`--eclipse 1` 可为满月行追加月食细节字段。
- 默认 `tz/format/pretty` 来自配置。

### 9.11 range

```bash
lunar range [bsp] --from <time> --to <time>
  [--kinds solar_term,lunar_phase,lunar_eclipse,solar_eclipse]
  [--tz ...] [--format json|txt|csv|ics|jsonl]
  [--out ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

说明：

- `--from`、`--to` 必填，且 `--to >= --from`。
- `--kinds` 包含食类时，会加载对应食事件。
- 默认 `tz/format/pretty` 来自配置。

### 9.12 search

```bash
lunar search [bsp] <query>
  [--from <time>] [--count N] [--tz ...]
  [--format json|txt|csv|ics|jsonl] [--out ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

说明：

- 当前仅支持以 `next ...` 开头的 query。
- query 可以作为一个带引号参数传入（`"next full moon"`），也可以拆成多个
  CLI 词（`next full moon`）；受支持的事件名会统一处理空格、连字符和下划线。
- 内部会把 query 解析成受限的事件检索条件，而不是把整串文本原样转发给 `next`。
- 默认值：`from=当前 UTC 时刻`、`count=1`；`tz/format/pretty` 来自配置。

### 9.13 eclipse

```bash
lunar eclipse [bsp] --near <YYYY-MM-DD> [--kind lunar|solar]
  [--stage any|umb|total] (lunar)
  [--stage any|central]   (solar)
  [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1]
  [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--tz ...] [--format json|txt|geojson] [--out ...] [--pretty 0|1] [--quiet]

lunar eclipse [bsp] --visible-near <time> --point-lat <deg> --point-lon <deg>
  [--kind lunar|solar|both] [--visible-years <years>]
  [--stage any|umb|total|central] [--sample-min <minutes>] [--point-height <m>]
  [--tz ...] [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
```

说明：

- `--near` 与 `--visible-near` 二选一。
- `--kind` 默认 `lunar`；使用 `--visible-near` 且未指定 `--kind` 时，会同时搜索月食与日食。
- `--point-lat` 与 `--point-lon` 必须同时出现。
- `--point-height`、`--point-refine` 只能和 `--point-lat`、`--point-lon` 一起使用。
- `--visible-near` 用于查询给定地点距离目标时刻最近的可见食，必须提供 `--point-lat` 与 `--point-lon`。
- `--visible-near` 与 `--near` 互斥，也不支持 `--global-vis` 或 `--format geojson`。
- `--global` 是 `--global-vis` 的兼容别名。
- `--global-format`、`--grid-lat-step`、`--grid-lon-step` 需要 `--global-vis 1`，或直接使用 `--format geojson`。
- `--format geojson` 会强制开启全局可见性计算。
- 默认 `tz/format/pretty` 来自配置（`def_fmt` 非 `json|txt|geojson` 时回退到 `json`）。

### 9.14 festival

```bash
lunar festival [bsp] <year>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
```

默认 `tz/format/pretty` 来自配置。

补充：

- 还支持 `--lunar-day-tz ...`，用于显式覆盖农历判日日界时区。

### 9.15 almanac

```bash
lunar almanac [bsp] <YYYY-MM-DD>
  [--tz ...] [--lunar-day-tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet] [--lon <deg>]
  [--trad folk|ziping|purple|xieji]
  [--year-boundary lichun|lunar_new_year|dongzhi]
  [--month-boundary solar_term|lunar_first_day]
  [--leap-month-mode ignore|inherit_previous|split_midway|shift_to_next]
  [--day-boundary hour23|hour0]
```

默认 `tz/format/pretty` 来自配置。

补充：

- 还支持 `--lunar-day-tz`、`--trad`、`--year-boundary`、`--month-boundary`、`--leap-month-mode`、`--day-boundary`。
- `--trad xieji`、`--trad old_almanac` 等常见别名都可用；各类 boundary / mode 参数也接受稳定英文 key 与常见 ASCII 别名。

### 9.16 info

```bash
lunar info [bsp] [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
```

说明：

- 默认格式固定为 `txt`（不读取 `def_fmt`）。
- 输出包含文件存在性、大小、SPK 覆盖区间等信息。

### 9.17 config

```bash
lunar config show [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

### 9.18 completion

```bash
lunar completion bash|zsh|fish|powershell
```

脚本输出到 stdout，可自行重定向到文件。

### 9.19 zodiac

```bash
lunar zodiac <bsp> --time <time>
  [--input-tz ...] [--tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]

lunar zodiac <bsp> --year <year>
  [--tz ...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
```

说明：

- 这两个命令当前仍通过位置参数 `<bsp>` 指定星历，不走前文 `day/range/...` 那套自动补 BSP 语义。
- `--time` 与 `--year` 二选一。
- `--input-tz` 仅在 `--time` 模式下有效。
- `--time` 模式输出当前时刻所属太阳星座、黄经区间边界、已过时长与剩余时长。
- `--year` 模式输出该显示时区下公历年的 12 段星座区间摘要，以及每段在该年窗口内的截取时长。
- 太阳星座按地心太阳视黄经计算，已含光行时修正。

### 9.20 sky

```bash
lunar sky <bsp> <time> --lat <deg> --lon <deg>
lunar sky <bsp> --time <time> --lat <deg> --lon <deg>
  [--height <m>] [--input-tz ...] [--tz ...]
  [--mode all|pick] [--pick sun,moon,Spica,HR5056,...]
  [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
```

说明：

- 当前仍通过位置参数 `<bsp>` 指定星历。
- 输出为指定时刻、指定观测点的 topocentric apparent position。
- `--lat` 与 `--lon` 必须同时提供。
- `--mode pick` 时可用 `--pick` 指定输出子集，例如 `sun,moon,Spica`。
- 太阳系目标会排在星表恒星之前输出。

## 10. 交互模式

### 10.1 启动

直接运行 `lunar`（无参数）。

### 10.2 启动流程

- 读取 `lun_cfg.txt`
- 若 `def_bsp` 存在，会提示是否继续使用
- 可指定额外目录扫描 `.bsp`
- 如未找到可用星历，可进入下载界面
- 若启用 `LUNAR_ENABLE_SERIES_FALLBACK`，交互模式也可改用 `@series`

### 10.3 菜单

- `1` months
- `2` calendar
- `3` at
- `4` zodiac
- `5` convert
- `6` day
- `7` next
- `8` festival
- `9` info
- `10` monthview
- `11` range
- `12` search
- `13` eclipse
- `14` almanac
- `15` config
- `16` completion
- `17` sky
- `18` export
- `d` 切换/下载 BSP
- `l` 切换语言
- `h` 帮助
- `q` 退出

## 11. i18n（中简/中繁/英/日/韩）

### 11.1 语言来源

- 命令行：`--lang zh|zht|en|ja|ko`
- 配置默认：`default_lang`
- 交互模式：主菜单 `l` 可即时切换并写回 `default_lang`

优先级：命令行高于配置默认。

### 11.2 覆盖范围

- 交互界面文案（`src/i18n/catalog_interact.cpp`）
- 事件名称翻译（`src/i18n/catalog_event.cpp`）
- 星体/天象相关文案（`src/i18n/catalog_astro.cpp`）
- 黄历字段翻译（`src/i18n/catalog_almanac.cpp`）
- 部分 CLI 提示与注释

结构化 JSON 的键名保持稳定，不随语言改变。

## 12. C++ API（Library-First）

### 12.1 头文件

- `include/lunar/core.hpp`
- `include/lunar/models.hpp`
- `include/lunar/day_formatter.hpp`

### 12.2 典型调用

```cpp
#include "lunar/core.hpp"
#include "lunar/day_formatter.hpp"

lunar::core::DayComputeOptions opt;
opt.ephem = "./de442.bsp";
opt.date_text = "2025-06-01";
opt.tz = "+08:00";
opt.include_events = true;

DayResult result = lunar::core::compute_day(opt);
lunar::core::format_day_output(std::cout, result, "json", true);
```

`AtData`、`DayResult` 已公开在 `include/lunar/models.hpp`。

## 13. C API

头文件：`include/lunar/c_api.h`

### 13.1 通用接口

- `lunar_tool_ver`
- `lunar_last_error`
- `lunar_clear_error`
- `lunar_run`
- `lunar_root_batch`

### 13.2 核心计算接口

- `lunar_calc_eot`
- `lunar_core_day`

### 13.3 命令包装接口

已提供包装：

- `month cal year event dl at conv zodiac day mview export next range search eclipse fest alm info cfg comp`

补充：

- 现已额外导出 `zodiac` 包装：`lunar_cmd_zodiac`、`lunar_use_zodiac`。
- `export` 与 `eclipse` 也已提供独立命令包装。
- `sky` 目前仍建议通过 `lunar_run` 调用。

### 13.4 返回码

- `0`：成功
- `2`：参数错误（`std::invalid_argument`）
- `1`：其他错误

## 14. 仓库结构

```text
include/lunar/          公共头文件（CLI/C++ API/C API）
src/cli/                months/calendar/year/event/download 相关实现
src/query/              at/convert/day/monthview/next/... 相关实现
src/emscripten/         wasm 预加载脚本与 Worker 包装
src/i18n/               多语言目录与词条
src/interact.cpp        交互模式
src/global_context.cpp  全局配置与自动 BSP 选择
src/entry.cpp           CLI 顶层参数解析与命令分发
src/cli.cpp             cli 模块聚合编译入口
src/query.cpp           query 模块聚合编译入口
src/c_api.cpp           C API 导出实现
tests/                  GoogleTest / CTest 用例
vsop87a/                行星级数回退模型
elpmpp02/               月球级数回退模型
lun_cfg.txt             运行时配置文件
```

## 15. 致谢

- 感谢 [@ytliu0](https://github.com/ytliu0) 在中国历法与天文计算相关资料上的长期贡献。
- 感谢 JPL NAIF 公开发布 DE 系列 BSP 星历文件，为本项目提供高质量基础数据。
- 感谢 Hipparcos（HIP）星表与相关公开整理数据，为项目中的恒星条目与编号体系提供参考基础。

## 16. 快速示例

```bash
# 设置默认星历和语言
lunar config set def_bsp ./de442.bsp
lunar config set default_lang zh

# 新语法：不再手动输入 <bsp>
lunar day 2025-06-01 --format json

# 覆盖本次使用的星历
lunar day 2025-06-01 --bsp ./de440s.bsp --format txt

# 区间事件
lunar range --from 2025-01-01 --to 2025-12-31 --format json --kinds solar_term,lunar_phase

# 批量逐日导出
lunar export --from 2025-01 --to 2025-12 --scope full --format jsonl --out days.jsonl

# 太阳星座
lunar zodiac ./de442.bsp --time 2025-03-20T18:01:00+08:00 --format json

# 指定地点观星
lunar sky ./de442.bsp --time 2025-06-01T20:00 --input-tz +08:00 --lat 31.23 --lon 121.47 --mode pick --pick sun,moon,Spica

# 食计算
lunar eclipse --near 2025-09-07 --kind lunar --global-vis 1 --global-format geojson --format json

# 指定地点最近可见食
lunar eclipse --visible-near 2025-01-01T00:00:00+08:00 --point-lat 31.23 --point-lon 121.47 --kind both --format json
```

## 17. 输出字段代号说明

说明：

- 本节专门解释结构化输出里的缩写、代号、以及 `*_code` / `*_key` / `*_codes` / `*_mask_hex` 字段。
- `year`、`month`、`name`、`data`、`events`、`input` 这类语义直白字段不重复展开。
- 字段名以当前实现 `src/query/core/base.cpp`、`src/query/core/events.cpp`、`src/query/day_output.cpp`、`src/cli/output.cpp`、`src/query/cmd_day_mview.cpp`、`src/query/cmd_export.cpp`、`src/query/cmd_eclipse.cpp` 为准。

### 17.1 通用时间与采样字段

| 字段 | 主要出现处 | 说明 |
| --- | --- | --- |
| `jd_utc` | 通用 | UTC 儒略日。 |
| `jd_tdb` | 通用 | TDB 儒略日。 |
| `utc_iso` | 通用 | UTC ISO 8601 时间。 |
| `loc_iso` | 通用 | 按当前显示时区转换后的 ISO 8601 时间。 |
| `tz_display` | `meta` | 当前输出采用的显示时区。 |
| `input_tz` | `at.input` | 原始输入字符串的解析时区。 |
| `lunar_day_tz` | `at/day/almanac/monthview/export` | 农历判日时区。 |
| `smp_time` | `day.input` | `day` 命令在该日内取样的时刻，默认 `12:00:00`。 |
| `smp_jdutc` | `day.input` | `day` 取样点的 UTC 儒略日。 |
| `smp_uiso` | `day.data` / `day.csv` / `export.csv` | 取样点的 UTC ISO 时间。 |
| `smp_liso` | `day.data` / `day.csv` / `export.csv` | 取样点在显示时区下的 ISO 时间。 |
| `st_jdutc` / `ed_jdutc` | `months` | 农历月起止时刻的 UTC 儒略日。 |
| `st_utc` / `ed_utc` | `months` | 农历月起止时刻的 UTC ISO 时间。 |
| `st_loc` / `ed_loc` | `months` | 农历月起止时刻在显示时区下的 ISO 时间。 |
| `jd_start_utc` / `jd_end_utc` | `eclipse.global_vis` | 全局可见性采样窗口起止 UTC 儒略日。 |
| `utc_start_iso` / `utc_end_iso` | `eclipse.global_vis` | 全局可见性采样窗口起止 UTC ISO 时间。 |
| `loc_start_iso` / `loc_end_iso` | `eclipse.global_vis` | 全局可见性采样窗口起止本地 ISO 时间。 |

### 17.2 天文几何与月相字段

| 字段 | 主要出现处 | 说明 |
| --- | --- | --- |
| `lam_s` / `sun_lam` | `at.data` | 太阳视黄经，单位弧度。 |
| `lam_m` / `moon_lam` | `at.data` | 月亮视黄经，单位弧度。 |
| `elong` / `elongation_rad` | `at.data` | 日月黄经差，单位弧度。 |
| `elong_deg` / `elongation_deg` | `at.data` | 日月黄经差，单位度。 |
| `ill_frac` | `at/day` | 月面照亮比例，范围一般为 `0..1`。 |
| `ill_pct` | `at/day/monthview/export` | 月面照亮百分比。 |
| `phase_name` | `at/day` | 月相名称。 |
| `moon_dist_km` | `event/eclipse` | 地月距离，单位公里。 |
| `moon_xg` | `at/day` | 月亮所在星官摘要对象。 |
| `moon_xg_region` | `monthview` | 月亮所在星官区域名。 |
| `moon_xg_star` | `monthview` | 用于匹配该星官的参考恒星名。 |
| `sep_deg` / `moon_xg_sep_deg` | `moon_xg` / `monthview` | 月亮与参考恒星的角距离，单位度。 |
| `eot` | `at.data` | 均时差对象。 |
| `eot_lon_deg` | `at.input` | 计算均时差时采用的经度，单位度。 |
| `lon_deg` | `day.input` / `huangli` / `eot` | 黄历与真太阳时计算所用经度，单位度。 |
| `eot_minutes` | `eot` / `huangli` | 均时差，单位分钟。 |
| `eot_seconds` | `eot` | 均时差，单位秒。 |
| `true_solar_minutes` | `huangli` | 当地真太阳时折算后的分钟数。 |
| `ra_deg` | `sky/eclipse` | 赤经，单位度。 |
| `dec_deg` | `sky/eclipse` | 赤纬，单位度。 |
| `az_deg` | `sky` | 方位角，单位度。 |
| `alt_deg` | `sky` | 高度角，单位度。 |
| `mag_v` | `sky` | 视星等（V 波段）。 |
| `region` | `sky` | 星官或所属区域名称。 |
| `is_solar_system` | `sky` | 是否为太阳系目标。 |
| `is_juxing` | `sky` | 是否为聚星条目。 |
| `sun_lam_deg` | `zodiac` | 太阳视黄经，单位度。 |
| `sign_index` / `sign_order` | `zodiac` | 星座零基序号 / 从 1 开始的显示序号。 |
| `sign_code` / `sign_name` | `zodiac` | 稳定星座代号 / 当前语言名称。 |
| `term_code` | `zodiac` | 与该星座起点对应的节气代号。 |
| `start_lambda_deg` / `end_lambda_deg` | `zodiac` | 该星座黄经区间边界，单位度。 |
| `sign_offset_deg` | `zodiac.point` | 当前时刻在本星座区间内已推进的黄经偏移，单位度。 |
| `elapsed_sec` / `remain_sec` / `span_sec` | `zodiac.point` | 已过 / 剩余 / 总时长，单位秒。 |
| `interval_count` | `zodiac.year` | 年摘要中的区间数量，通常为 12。 |
| `in_year_start_loc_iso` / `in_year_end_loc_iso` | `zodiac.year` | 该星座区间在当前显示时区对应公历年窗口内的本地起止时间。 |
| `in_year_dur_sec` / `in_year_dur_days` | `zodiac.year` | 该区间落入该公历年窗口内的持续时间。 |
| `clipped_start` / `clipped_end` | `zodiac.year` | 区间起点 / 终点是否被年窗口裁剪。 |
| `sd_deg` | `eclipse` | 视半径角，单位度。 |
| `ehp_deg` | `eclipse` | 赤道地平视差，单位度。 |

### 17.3 农历、干支、八字字段

| 字段 | 主要出现处 | 说明 |
| --- | --- | --- |
| `lun_mno` | `lunar_date` | 农历月序号。 |
| `lun_mlab` / `lun_m_label` | `lunar_date` / `monthview.csv` | 农历月文字标签，如正月、闰二月。 |
| `lun_label` | `lunar_date` / `monthview` | 完整农历日期标签。 |
| `is_leap` | 通用 | 是否闰月，布尔或 `0/1`。 |
| `gz` | 黄历/干支 | 干支简称，字段中常见于 `m_gz`、`d_gz`、`h_gz`。 |
| `y_lun_gz` / `year_lunar` | `day.csv` / `huangli` | 以农历新年换算的年干支。 |
| `y_lchun_gz` / `year_lchun` | `day.csv` / `huangli` | 以立春换算的年干支。 |
| `y_rule_gz` / `year_rule` | `day.csv` / `huangli` | 按当前规则集真正采用的年干支。 |
| `m_gz` / `month` | `day.csv` / `huangli` | 月干支。 |
| `d_gz` / `day` | `day.csv` / `huangli` | 日干支。 |
| `h_gz` / `hour_clock` | `day.csv` / `huangli` | 按钟表时计算的时干支。 |
| `h_true_gz` / `hour_true_solar` | `day.csv` / `huangli` | 按真太阳时计算的时干支。 |
| `index` | `GzNode` 输出 | 60 甲子序号。 |
| `stem` | `GzNode` 输出 | 天干序号。 |
| `branch` | `GzNode` 输出 | 地支序号。 |
| `bazi_clock` | `huangli` | 按钟表时生成的八字串。 |
| `bazi_true` | `huangli` | 按真太阳时生成的八字串。 |

### 17.4 黄历、神煞、宜忌字段

| 字段 | 主要出现处 | 说明 |
| --- | --- | --- |
| `rule_profile` | `huangli` | 黄历规则档位名称。 |
| `year_boundary` | `huangli` | 年柱换年规则。 |
| `month_boundary` | `huangli` | 月柱换月规则。 |
| `leap_month_mode` | `huangli` | 闰月在月柱中的处理方式。 |
| `day_boundary` | `huangli` | 日柱换日规则。 |
| `jianchu` | `huangli` | 建除十二值。 |
| `duty_god` | `huangli` | 当日值神。 |
| `duty_is_yellow` | `huangli` | 值神是否属于黄道。 |
| `duty_tag` | `huangli` | 值神简类标签。 |
| `clash` | `huangli` | 当日冲支说明。 |
| `chong_sha` | `huangli` | 冲煞合并说明。 |
| `zodiac_day` | `huangli` | 值日生肖。 |
| `six_he` | `huangli` | 六合对应支。 |
| `three_he` | `huangli` | 三合组。 |
| `pengzu` | `huangli` | 彭祖百忌。 |
| `nayin` | `huangli` | 纳音。 |
| `wuxing_day` / `wx_day` | `huangli` / 结构体 | 当日五行。 |
| `fetal_god` | `huangli` | 胎神方位说明。 |
| `meridian` | `huangli` | 值日经络。 |
| `lucky_dir` | `huangli` | 喜神方位。 |
| `wealth_dir` | `huangli` | 财神方位。 |
| `mascot_dir` | `huangli` | 福神方位。 |
| `sun_noble_dir` | `huangli` | 阳贵神方位。 |
| `moon_noble_dir` | `huangli` | 阴贵神方位。 |
| `xiu28` | `huangli` | 二十八宿名。 |
| `xiu28_mod28` | `huangli` | 按模 28 规则得到的宿序文字。 |
| `xiu_star` | `huangli` | 二十八宿稳定标识。 |
| `good_gods` | `huangli` | 吉神名称数组。 |
| `bad_gods` | `huangli` | 凶煞名称数组。 |
| `yi` | `huangli` | 宜事项数组。 |
| `ji` | `huangli` | 忌事项数组。 |
| `yi_ji_level` | `huangli` | 宜忌强弱级别。 |
| `yi_ji_rule` | `huangli` | 宜忌判定规则说明。 |
| `hour_jx` | `huangli` | 时辰吉凶数组。 |
| `slot` | `hour_jx` | 时辰槽位，如子时、丑时。 |
| `slot_index` | `hour_jx` | 时辰槽位序号。 |
| `gz_index` | `hour_jx` | 该时辰干支的 60 甲子序号。 |
| `luck` | `hour_jx` | 时辰吉凶文字。 |
| `is_bad` | `hour_jx` | 该时辰是否凶。 |

### 17.5 事件、邻近事件与汇总字段

| 字段 | 主要出现处 | 说明 |
| --- | --- | --- |
| `kind` | `event` / `near_ev` / `eclipse` / `export` | 机器可判定的事件类别。 |
| `code` | `event` / `near_ev` | 该事件类别下的稳定代号。 |
| `st_prev` / `st_next` | `near_ev` | 前一个 / 后一个节气事件。 |
| `lp_prev` / `lp_next` | `near_ev` | 前一个 / 后一个月相事件。 |
| `ev_sum` | `monthview` | 当日事件名称汇总，使用 `|` 连接。 |
| `astro_ev_sum` | `monthview` | 当日天象事件名称汇总，使用 `|` 连接。 |
| `stage_window` | `eclipse.visibility` | 可见性统计所采用的阶段窗口，如 `any`、`umb`、`total`、`central`。 |
| `sample_count` | `eclipse.visibility` | 可见性采样次数。 |
| `first_visible` / `last_visible` | `eclipse.visibility` | 本地点或网格点最早 / 最晚可见时刻。 |
| `visible` | `eclipse.visibility` | 是否可见。 |
| `has_eclipse` | `solar.point_vis` | 该地点是否实际发生日食。 |
| `central` | `solar.point_vis` | 该地点是否处于中心食路径。 |
| `eclipses` | `export.day` | 归入该导出公历日的食事件。 |
| `astro_events` | `export.day` | 归入该导出公历日的天象事件。 |
| `huangli` | `export.day` | 所选流派或全部流派的黄历内容。 |

### 17.6 食相关字段

| 字段 | 主要出现处 | 说明 |
| --- | --- | --- |
| `pen_mag` | `lunar_eclipse` | 半影食分。 |
| `umb_mag` | `lunar_eclipse` | 本影食分。 |
| `mag` | `solar_eclipse` | 为兼容保留的地心视圆盘重叠口径食分；不是食甚地点的标准目录食分。 |
| `catalog_mag` | `solar_eclipse` | 标准全球食分：影轴命中地球椭球时取月/日视直径比；影轴未命中时取最近地表点的太阳直径遮蔽比例，包括偏食及非中心环食、全食。 |
| `obscuration` | `solar_eclipse` | 地心视圆盘重叠口径的太阳被遮蔽面积比例。 |
| `gamma` | `lunar_eclipse` / `solar_eclipse` | 食中心相对影轴的归一化偏距。 |
| `eps_deg` | `lunar_eclipse` | 月食几何角参数，单位度。 |
| `rp_re` | `eclipse` | 影区半径参数，单位地球半径。 |
| `ru_re` | `eclipse` | 另一组影区半径参数，单位地球半径。 |
| `opp_rp_re` / `opp_ru_re` | `lunar_eclipse` | 对冲时刻的影区半径参数，单位地球半径。 |
| `dur_pen_sec` | `lunar_eclipse` | 半影阶段时长，单位秒。 |
| `dur_umb_sec` | `lunar_eclipse` | 本影阶段时长，单位秒。 |
| `dur_tot_sec` | `lunar_eclipse` | 全食阶段时长，单位秒。 |
| `dt_max_sec` | `eclipse` | 最大食时刻的 Delta T（`TT-UT1`），单位秒。 |
| `sep_max_deg` | `solar_eclipse` | 最大食时日月中心角距，单位度。 |
| `sun_sd_max_deg` | `solar_eclipse` | 最大食时太阳视半径角，单位度。 |
| `moon_sd_max_deg` | `solar_eclipse` | 最大食时月亮视半径角，单位度。 |
| `besselian` | `solar_eclipse` | 日食贝塞尔参数对象，含基本平面坐标、影锥半径、锥角与时间多项式。 |
| `x` / `y` | `solar_eclipse.besselian` | 食甚时月影轴在基本平面上的坐标，单位地球半径。 |
| `d_deg` / `mu_deg` | `solar_eclipse.besselian` | 影轴赤纬与格林尼治时角，单位度。 |
| `l1` / `l2` | `solar_eclipse.besselian` | 半影 / 本影影锥在基本平面的半径，单位地球半径。 |
| `tan_f1` / `tan_f2` | `solar_eclipse.besselian` | 半影 / 本影影锥角正切值。 |
| `x_dot` / `y_dot` / `d_dot_deg` / `mu_dot_deg` / `l1_dot` / `l2_dot` | `solar_eclipse.besselian` | 对应贝塞尔参数对小时的导数。 |
| `coefficients` | `solar_eclipse.besselian` | 三次多项式系数数组 `[c0,c1,c2,c3]`，自 `epoch` 起按 TDB 小时计算。 |
| `sun_geo` / `moon_geo` | `lunar_eclipse` | 太阳 / 月亮几何参数对象。 |
| `lib` | `lunar_eclipse` | 月面天平动参数对象。 |
| `l_deg` / `b_deg` / `c_deg` | `lib` | 月面天平动角参数，单位度。 |
| `p1` / `u1` / `u2` / `u3` / `u4` / `p4` | `lunar_eclipse` | 月食各接触时刻节点对象。 |
| `opp` | `lunar_eclipse` | 望 / 对冲时刻节点对象。 |
| `max` | `eclipse` | 食甚节点对象。 |
| `c1` / `c2` / `c3` / `c4` | `solar_eclipse` / `solar.point_vis` | 日食初亏 / 食既 / 生光 / 复圆节点对象。 |
| `solar_eclipse_c1_loc_iso` / `solar_eclipse_c2_loc_iso` / `solar_eclipse_max_loc_iso` / `solar_eclipse_c3_loc_iso` / `solar_eclipse_c4_loc_iso` | `next/range/search.csv` | 日食接触节点在扁平 CSV 输出中的本地 ISO 时间列。 |
| `zen_lat_deg` / `zen_lon_deg` | `eclipse.node` | 对应节点的天顶点纬度 / 经度。 |
| `pa_deg` | `eclipse.node` | 对应节点的位置角。 |
| `axis_deg` | `eclipse.node` | 对应节点的轴角参数。 |
| `max_mag` | `solar.point_vis` / `solar.global_vis` | 本地点或网格点的最大食分。 |
| `max_obscuration` | `solar.point_vis` | 本地点的最大遮蔽面积比例。 |
| `max_sun_alt_deg` | `solar.point_vis` / `solar.global_vis` | 最大食时太阳高度角。 |
| `visible_target` | `eclipse.visible_near` | 最近可见食查询的目标时刻与距离元信息。 |
| `visible_delta_days` | `eclipse.visible_near` | 目标时刻与选中可见食之间的绝对间隔，单位日。 |

### 17.7 code / key / mask 字段

| 字段 | 对应文本字段 | 说明 |
| --- | --- | --- |
| `rule_profile_code` | `rule_profile` | 黄历规则档位的整数枚举值。 |
| `rule_profile_key` | `rule_profile` | 黄历规则档位的稳定英文键。 |
| `year_boundary_code` | `year_boundary` | 年柱换年规则的整数枚举值。 |
| `year_boundary_key` | `year_boundary` | 年柱换年规则的稳定英文键。 |
| `month_boundary_code` | `month_boundary` | 月柱换月规则的整数枚举值。 |
| `month_boundary_key` | `month_boundary` | 月柱换月规则的稳定英文键。 |
| `leap_month_mode_code` | `leap_month_mode` | 闰月处理规则的整数枚举值。 |
| `leap_month_mode_key` | `leap_month_mode` | 闰月处理规则的稳定英文键。 |
| `day_boundary_code` | `day_boundary` | 日柱换日规则的整数枚举值。 |
| `day_boundary_key` | `day_boundary` | 日柱换日规则的稳定英文键。 |
| `jianchu_code` | `jianchu` | 建除十二值的整数代号。 |
| `duty_god_code` | `duty_god` | 值神的整数代号。 |
| `duty_tag_code` | `duty_tag` | 值神大类标签代号。 |
| `clash_branch_code` | `clash` | 冲支所对应的地支代号。 |
| `sha_dir_code` | `chong_sha` | 煞方方向代号。 |
| `zodiac_day_code` | `zodiac_day` | 值日生肖代号。 |
| `six_he_branch_code` | `six_he` | 六合对应地支代号。 |
| `three_he_group_code` | `three_he` | 三合组代号。 |
| `nayin_code` | `nayin` | 纳音代号。 |
| `fetal_god_code` | `fetal_god` | 胎神方位代号。 |
| `meridian_code` | `meridian` | 经络代号。 |
| `lucky_dir_code` | `lucky_dir` | 喜神方位代号。 |
| `wealth_dir_code` | `wealth_dir` | 财神方位代号。 |
| `mascot_dir_code` | `mascot_dir` | 福神方位代号。 |
| `sun_noble_dir_code` | `sun_noble_dir` | 阳贵神方位代号。 |
| `moon_noble_dir_code` | `moon_noble_dir` | 阴贵神方位代号。 |
| `xiu28_code` | `xiu28` | 二十八宿代号。 |
| `xiu28_mod28_code` | `xiu28_mod28` | 模 28 宿序代号。 |
| `good_god_codes` | `good_gods` | 吉神名称数组对应的整数代号数组。 |
| `bad_god_codes` | `bad_gods` | 凶煞名称数组对应的整数代号数组。 |
| `yi_codes` | `yi` | 宜事项数组对应的整数代号数组。 |
| `ji_codes` | `ji` | 忌事项数组对应的整数代号数组。 |
| `good_god_mask_hex` | `good_gods` | 吉神位图，十六进制字符串。 |
| `bad_god_mask_hex` | `bad_gods` | 凶煞位图，十六进制字符串。 |
| `yi_mask_hex` | `yi` | 宜事项位图数组，十六进制字符串数组。 |
| `ji_mask_hex` | `ji` | 忌事项位图数组，十六进制字符串数组。 |
| `yi_ji_rule_code` | `yi_ji_rule` | 宜忌判定规则代号。 |

## 18. 主要函数与结构体变量说明

说明：

- 本节只列公共头文件和 README 当前直接涉及的主接口，不展开内部小工具函数。
- 接口声明以 `include/lunar/core.hpp`、`include/lunar/models.hpp`、`include/lunar/cli.hpp`、`include/lunar/cli_query.hpp`、`include/lunar/day_formatter.hpp`、`include/lunar/c_api.h` 为准。

### 18.1 主要函数

| 函数 | 位置 | 作用 |
| --- | --- | --- |
| `run_cli_args` | `include/lunar/entry.hpp` | CLI 总入口，接收参数数组后完成顶层分发。 |
| `cli_month` / `cli_cal` / `cli_year` / `cli_event` / `cli_dl` | `include/lunar/cli.hpp` | 旧式 CLI 参数结构体入口。 |
| `cli_at` / `cli_conv` | `include/lunar/cli_query.hpp` | `at` / `convert` 的旧式 CLI 参数结构体入口。 |
| `cmd_month` / `cmd_cal` / `cmd_year` / `cmd_event` / `cmd_dl` | `include/lunar/cli.hpp` | 直接接收字符串数组的命令入口。 |
| `cmd_at` / `cmd_conv` / `cmd_zodiac` / `cmd_sky` / `cmd_day` / `cmd_mview` / `cmd_export` / `cmd_next` / `cmd_range` / `cmd_search` / `cmd_eclipse` / `cmd_fest` / `cmd_alm` / `cmd_info` / `cmd_cfg` / `cmd_comp` | `include/lunar/cli_query.hpp` | 各子命令的直接入口。 |
| `lunar::calc_sky_pos` | `include/lunar/star.hpp` | 计算指定时刻、指定观测点的天空位置列表。 |
| `calc_solar_zodiac_at` / `calc_solar_zodiac_year` | `include/lunar/solar_zodiac.hpp` | 计算单时刻太阳星座或全年星座区间摘要。 |
| `lunar::core::compute_day` | `include/lunar/core.hpp` | 计算某一天的农历、黄历、月相、事件与可选天象信息。 |
| `lunar::core::format_day_output` | `include/lunar/day_formatter.hpp` | 把 `DayResult` 输出成 `json/txt/csv/jsonl`。 |
| `lunar::core::compute_ganzhi` | `include/lunar/core.hpp` | 计算单个时刻的年、月、日干支摘要。 |
| `lunar::core::compute_ganzhi_month` | `include/lunar/core.hpp` | 计算整月逐日干支摘要。 |
| `lunar_tool_ver` | `include/lunar/c_api.h` | 返回库 / 工具版本字符串。 |
| `lunar_last_error` / `lunar_clear_error` | `include/lunar/c_api.h` | 读取或清空 C API 最近一次错误信息。 |
| `lunar_run` | `include/lunar/c_api.h` | C 方式调用 CLI 总入口。 |
| `lunar_root_batch` | `include/lunar/c_api.h` | 批量根求解入口。 |
| `lunar_calc_eot` | `include/lunar/c_api.h` | 计算均时差并写入 `lunar_eot_result`。 |
| `lunar_core_day` | `include/lunar/c_api.h` | 计算单日核心摘要并写入 `lunar_day_summary`。 |
| `lunar_hli_rules_init` | `include/lunar/c_api.h` | 用默认值初始化 `lunar_hli_rules`。 |
| `lunar_core_ganzhi` | `include/lunar/c_api.h` | 计算单个时刻干支摘要并写入 `lunar_ganzhi_summary`。 |
| `lunar_core_ganzhi_month` | `include/lunar/c_api.h` | 计算整月干支摘要并写入 `lunar_ganzhi_month_summary`。 |

### 18.2 CLI 参数结构体

| 结构体 | 关键变量 | 说明 |
| --- | --- | --- |
| `MonthsArgs` | `ephem`、`years`、`mode`、`format`、`out`、`tz`、`pretty`、`quiet` | `months` 命令参数。 |
| `CalArgs` | `ephem`、`years_arg`、`has_years`、`format`、`inc_month`、`tz` | `calendar` 命令参数。 |
| `YearArgs` | `ephem`、`year`、`mode`、`format`、`tz` | `year` 命令参数。 |
| `EventArgs` | `ephem`、`category`、`code`、`year`、`near_date`、`format`、`tz` | `event` 命令参数。 |
| `DlArgs` | `action`、`id`、`dir`、`quiet` | 下载命令参数。 |
| `AtArgs` | `ephem`、`time_raw`、`input_tz`、`tz`、`lunar_day_tz`、`calc_eot`、`eot_lon_deg`、`hli_rules`、`jobs` | `at` 命令参数。 |
| `ConvArgs` | `ephem`、`in_value`、`from_lunar`、`lunar_year`、`lun_mno`、`lunar_day`、`leap`、`tz`、`lunar_day_tz` | `convert` 命令参数。 |

### 18.3 C++ 计算选项与结果结构体

| 结构体 | 关键变量 | 说明 |
| --- | --- | --- |
| `GanzhiComputeOptions` | `ephem`、`date_text`、`at_time`、`tz`、`lunar_day_tz`、`hli_rules` | 单个时刻干支计算输入。 |
| `GanzhiSummary` | `year`、`month`、`day`、`hli_rules` | 单个时刻干支计算结果。 |
| `GanzhiMonthComputeOptions` | `ephem`、`year`、`month`、`at_time`、`tz`、`lunar_day_tz`、`hli_rules` | 整月干支计算输入。 |
| `GanzhiMonthSummary` | `year`、`month`、`years`、`months`、`days`、`hli_rules` | 整月逐日干支结果。 |
| `DayComputeOptions` | `ephem`、`date_text`、`at_time`、`tz`、`lunar_day_tz`、`include_events`、`include_astro`、`astro_mode_text`、`astro_pick_csv`、`astro_lat_deg`、`astro_lon_deg`、`astro_height_m`、`has_astro_site`、`hli_lon_deg`、`hli_rules` | `compute_day` 的完整输入。 |
| `DayResult` | `ephem`、`date_text`、`at_time`、`tz`、`lunar_day_tz`、`hli_lon_deg`、`astro_obs`、`at_data`、`day_events`、`astro_events` | `compute_day` 的完整结果。 |

### 18.4 主要数据结构体

| 结构体 | 关键变量 | 说明 |
| --- | --- | --- |
| `LunDate` | `lunar_year`、`lun_mno`、`is_leap`、`lun_mlab`、`lunar_day`、`lun_label`、`cst_year`、`cst_month`、`cst_day`、`cstday_jd` | 农历日期对象，同时保留用于判日的民用日信息。 |
| `NearEvt` | `has`、`event` | 单个邻近事件槽位。 |
| `NearEvents` | `solar_prev`、`solar_next`、`phase_prev`、`phase_next` | 前后节气与前后月相。 |
| `AtData` | `time_raw`、`tz_in`、`display_tz`、`lunar_day_tz`、`jd_utc`、`jd_tdb`、`utc_iso`、`local_iso`、`lam_s`、`lam_m`、`elong`、`elong_deg`、`ill_frac`、`ill_pct`、`waxing`、`phase_name`、`lunar_date`、`near_ev`、`eot`、`moon_xg`、`hli` | `at/day/almanac` 的核心计算结果容器。 |
| `EventRec` | `kind`、`code`、`name`、`year`、`jd_tdb`、`jd_utc`、`utc_iso`、`loc_iso` | 单个事件记录。 |
| `MonthRec` | `label`、`month_no`、`is_leap`、`st_jdutc`、`ed_jdutc`、`st_utc`、`st_loc`、`ed_utc`、`ed_loc` | 单个农历月范围记录。 |
| `HliRuleSet` | `profile_code`、`year_boundary`、`month_boundary`、`leap_month_mode`、`day_boundary` | 黄历规则集合。 |
| `GzNode` | `stem`、`branch`、`text` | 单个干支节点。 |
| `HliHour` | `slot_index`、`gz_index`、`is_bad`、`slot`、`gz`、`luck` | 单个时辰吉凶结果。 |
| `HliData` | `y_lun`、`y_lchun`、`y_rule`、`m_gz`、`d_gz`、`h_gz`、`h_gz_true`、`bazi_clock`、`bazi_true`、`rule_profile_code`、`year_boundary_code`、`month_boundary_code`、`leap_month_mode_code`、`day_boundary_code`、`good_gods`、`bad_gods`、`yi`、`ji`、`hour_jx`、`eot_min`、`tst_min` | 黄历总结果对象。 |
| `HliInput` | `jd_utc`、`gy/gm/gd`、`hh/mm/ss`、`tz_off`、`lon_deg`、`lun_year`、`lun_month`、`lun_day`、`lun_leap`、`phase_name`、`moon_xg`、`rules` | 黄历计算输入对象。 |
| `EoTData` | `jd_utc`、`jd_tdb`、`lon_deg`、`lon_rad`、`apparent_solar_time_rad`、`mean_solar_time_rad`、`eot_rad`、`eot_minutes`、`eot_seconds` | 均时差计算结果。 |
| `AppLon` | `eph`、`frame_bias`、`prec_cache`、`r1n_cache`、`rot_cache` | 太阳 / 月亮视黄经与均时差的计算器对象。 |
| `MoonXg` | `region`、`star_id`、`star_name`、`sep_deg` | 月亮星官归属结果。 |
| `AstroObs` | `has_site`、`lat_deg`、`lon_deg`、`h_m` | 天象观测地点。 |
| `SkyPos` | `kind`、`code`、`name`、`region`、`mag_v`、`ra_deg`、`dec_deg`、`az_deg`、`alt_deg` | 天体天空位置输出记录。 |
| `SolarZodiacDef` | `index`、`code`、`term_code`、`start_lambda_rad`、`end_lambda_rad` | 单个太阳星座的固定黄经区间定义。 |
| `SolarZodiacPoint` | `sun_lam_deg`、`sign_index`、`sign_code`、`term_code`、`sign_start_jd_utc`、`sign_end_jd_utc`、`elapsed_sec`、`remain_sec` | 单时刻太阳星座结果。 |
| `SolarZodiacYearInterval` | `sign_index`、`sign_code`、`term_code`、`in_year_start_jd_utc`、`in_year_end_jd_utc`、`in_year_dur_sec`、`clipped_start`、`clipped_end` | 全年模式下单个星座区间摘要。 |
| `SolarZodiacYearSummary` | `year`、`tz_off`、`year_start_jd_utc`、`year_end_jd_utc`、`intervals` | 全年太阳星座汇总结果。 |
| `AstroEvt` | `kind`、`code`、`name`、`jd_utc`、`detail` | 天象事件记录。 |

### 18.5 食相关结构体

| 结构体 | 关键变量 | 说明 |
| --- | --- | --- |
| `EclipseGeoCoord` | `ra_deg`、`dec_deg`、`sd_deg`、`ehp_deg` | 食计算中的天体几何参数。 |
| `EclipseLibration` | `l_deg`、`b_deg`、`c_deg` | 月面天平动参数。 |
| `EclipsePointMeta` | `zen_lat_deg`、`zen_lon_deg`、`pa_deg`、`axis_deg` | 接触点几何元数据。 |
| `LunarEclipse` | `has`、`type`、`jd_tdb_p1/u1/u2/u3/u4/p4/opp/max`、`pen_mag`、`umb_mag`、`dur_pen_sec`、`dur_umb_sec`、`dur_tot_sec`、`gamma`、`eps_deg`、`sun_geo`、`moon_geo`、`lib` | 月食完整结果。 |
| `LunarEclipsePointVis` | `stage_window`、`lat_deg`、`lon_deg`、`height_m`、`visible`、`max_alt_deg`、`first_jd_utc`、`last_jd_utc`、`sample_count` | 月食单点可见性结果。 |
| `LunarEclipseGlobalVis` | `stage_window`、`jd_start_utc`、`jd_end_utc`、`lat_step_deg`、`lon_step_deg`、`sample_count`、`points` | 月食全局网格可见性结果。 |
| `SolarBesselianElements` | `x`、`y`、`d_deg`、`mu_deg`、`l1`、`l2`、`tan_f1`、`tan_f2`、`*_coeff` | 日食贝塞尔参数与三次时间多项式。 |
| `SolarEclipse` | `has`、`type`、`jd_tdb_c1/c2/c3/c4/max`、`mag`、`catalog_mag`、`obscuration`、`gamma`、`sep_max_deg`、`sun_sd_max_deg`、`moon_sd_max_deg`、`moon_dist_km`、`sun_dist_km`、`besselian` | 日食完整结果。 |
| `SolarEclipsePointVis` | `stage_window`、`lat_deg`、`lon_deg`、`height_m`、`has_eclipse`、`visible`、`central`、`max_mag`、`max_obscuration`、`max_sun_alt_deg`、`c1_jd_utc/c2_jd_utc/c3_jd_utc/c4_jd_utc/max_jd_utc` | 日食单点可见性结果。 |
| `SolarEclipseGlobalVis` | `stage_window`、`jd_start_utc`、`jd_end_utc`、`lat_step_deg`、`lon_step_deg`、`sample_count`、`points` | 日食全局网格可见性结果。 |

### 18.6 C API 结构体

| 结构体 | 关键变量 | 说明 |
| --- | --- | --- |
| `lunar_eot_result` | `jd_utc`、`jd_tdb`、`lon_deg`、`lon_rad`、`apparent_solar_time_rad`、`mean_solar_time_rad`、`eot_rad`、`eot_minutes`、`eot_seconds` | C API 的均时差结果。 |
| `lunar_day_summary` | `lunar_year`、`lun_mno`、`lunar_day`、`is_leap`、`ill_pct`、`phase_name`、`lun_label` | C API 的单日简表结果。 |
| `lunar_hli_rules` | `profile_code`、`year_boundary`、`month_boundary`、`leap_month_mode`、`day_boundary` | C API 的黄历规则集合。 |
| `lunar_ganzhi_node` | `index`、`stem`、`branch`、`text` | C API 的单个干支节点。 |
| `lunar_ganzhi_summary` | `year`、`month`、`day`、`rule_profile_code`、`year_boundary_code`、`month_boundary_code`、`leap_month_mode_code`、`day_boundary_code` | C API 的单时刻干支摘要。 |
| `lunar_ganzhi_month_summary` | `year`、`month`、`day_count`、`years[31]`、`months[31]`、`days[31]`、各类 `*_code` | C API 的整月干支摘要。 |
