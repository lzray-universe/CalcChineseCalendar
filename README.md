# CalcChineseCalendar（lunar）

CalcChineseCalendar 是一个优先使用 JPL DE BSP 星历、在缺失 BSP 时可自动回退到仓库内置 VSOP87A + ELPMPP02 的历法与天文计算工具，提供：

- 命令行工具 `lunar`
- 动态库 `lunar_dll`（导出 C API）
- 面向 C++ 的可复用计算接口（`lunar::core`）

[![Lines of Code](https://img.shields.io/endpoint?url=https%3A%2F%2Ftokei.kojix2.net%2Fbadge%2Fgithub%2Flzray-universe%2FCalcChineseCalendar%2Flines)](https://tokei.kojix2.net/github/lzray-universe/CalcChineseCalendar)

[![License](https://img.shields.io/github/license/lzray-universe/CalcChineseCalendar)](LICENSE)

## 1. 功能概览

- 农历月枚举：`months`
- 年度节气/月相日历：`calendar`、`year`
- 单事件求解：`event`
- 星历下载：`download`
- 时刻查询与批处理：`at`
- 公历/农历互转与批处理：`convert`
- 单日/单月视图：`day`、`monthview`
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

### 2.6 可选构建开关

- `-DLUNAR_ENABLE_SERIES_FALLBACK=ON|OFF`
  - `ON`（默认）：未找到 BSP 时自动切换到内置 VSOP87A + ELPMPP02
  - `OFF`：保持旧行为，必须提供可用 BSP
- `-DLUNAR_BUILD_TESTS=ON|OFF`
  - `ON`（默认）：构建 gtest/ctest 测试目标
  - `OFF`：不构建测试目标

### 2.7 运行测试

```bash
cmake -S . -B build
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

### 5.1 哪些命令需要 BSP

- `months calendar year event at convert day monthview next range search eclipse festival almanac info`

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
- `def_fmt`
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
- `def_fmt` 允许 `txt|json|csv|jsonl|ics`。
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

- `--tz` 只影响显示时区。
- 农历判日固定按 UTC+8 民用日边界执行。

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

### 9.1 months

```bash
lunar months <bsp> <years>
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
lunar calendar <bsp> [<years>]
  [--format json|txt|ics] [--out <path>] [--tz ...]
  [--include-months 0|1] [--include-eclipses 0|1] [--pretty 0|1] [--quiet]
```

说明：

- 省略 `<years>` 时默认使用 `2025`。
- `ics` 仅导出节气和月相事件。
- 多年计算时若非 `--quiet`，stderr 仅输出年份进度。

### 9.3 year

```bash
lunar year <bsp> <year>
  [--mode lunar|gregorian]
  [--format json|txt|ics] [--out <path>] [--tz ...]
  [--pretty 0|1] [--quiet]
```

### 9.4 event

```bash
lunar event <bsp> solar-term <code> <year>
lunar event <bsp> lunar-phase <new_moon|fst_qtr|full_moon|lst_qtr> --near <YYYY-MM-DD>
  [--format json|txt|ics] [--out <path>] [--tz ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]

# 也支持直接转发到 eclipse 命令：
lunar event <bsp> lunar-eclipse --near <YYYY-MM-DD> ...
lunar event <bsp> solar-eclipse --near <YYYY-MM-DD> ...
```

说明：

- `solar-term` 的 `code` 使用 `J1..J12`、`Z1..Z12`。
- `lunar-phase` 必须提供 `--near`。
- `--eclipse 1` 用于在满月事件输出中附加月食细节。

### 9.5 download

```bash
lunar download list
lunar download get <id> [--dir <path>] [--quiet]
```

### 9.6 at

```bash
lunar at <bsp> <time>
lunar at <bsp> --time <time>
lunar at <bsp> --stdin
lunar at <bsp> --file <path>
  [--input-tz ...] [--tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--events 0|1] [--eot-lon <deg>] [--jobs N] [--meta-once 0|1]
```

说明：

- 批模式：`--stdin` 与 `--file` 互斥。
- 批模式下若同时给位置参数 `<time>`，该位置参数会被忽略。
- 单次模式下 `--format jsonl` 会自动回退为 `json`。
- `--jobs` 已接受，但当前执行器仍按顺序运行。
- 默认 `tz/format/pretty` 来自 `lun_cfg.txt`（`default_tz/def_fmt/def_prety`）。

### 9.7 convert

```bash
lunar convert <bsp> <dt_or_tm>
  [--input-tz ...] [--tz ...]
  [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
  [--stdin|--file <path>] [--jobs N] [--meta-once 0|1]

lunar convert <bsp> --from-lunar <lunar_year> <month_no> <day> [--leap 0|1]
  [--tz ...] [--format json|txt|jsonl] [--out <path>] [--pretty 0|1] [--quiet]
```

说明：

- `--from-lunar` 模式下不能再传位置参数 `<dt_or_tm>`。
- `--stdin` 与 `--file` 互斥。
- 单次模式下 `--format jsonl` 会自动回退为 `json`。
- 默认 `tz/format/pretty` 来自配置。

### 9.8 day

```bash
lunar day <bsp> <YYYY-MM-DD>
  [--tz ...] [--format json|txt|csv|jsonl] [--out ...] [--pretty 0|1] [--quiet]
  [--at HH:MM[:SS]] [--events 0|1] [--lon <deg>]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

说明：

- `--astro-lat` 与 `--astro-lon` 必须同时出现。
- `--astro-height` 只能在已给经纬度时使用。
- 逻辑实现走 `lunar::core::compute_day` + `lunar::core::format_day_output`。
- 默认 `tz/format/pretty` 来自配置。

### 9.9 monthview

```bash
lunar monthview <bsp> <YYYY-MM>
  [--tz ...] [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
  [--astro 0|1] [--astro-mode less|all|pick] [--astro-pick id,en,zh,...]
  [--astro-lat deg --astro-lon deg [--astro-height m]]
```

说明：

- `--astro-lat` 与 `--astro-lon` 必须同时出现。
- 默认 `tz/format/pretty` 来自配置。

### 9.10 next

```bash
lunar next <bsp> --from <time> --count N
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
lunar range <bsp> --from <time> --to <time>
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
lunar search <bsp> <query>
  [--from <time>] [--count N] [--tz ...]
  [--format json|txt|csv|ics|jsonl] [--out ...] [--pretty 0|1] [--quiet] [--eclipse 0|1]
```

说明：

- 当前仅支持以 `next ...` 开头的 query。
- 内部实现是转成 `next` 参数再调用 `cmd_next`。
- 默认值：`from=2025-01-01T00:00:00+08:00`、`count=1`、`format=txt`、`tz=+08:00`、`pretty=1`。

### 9.13 eclipse

```bash
lunar eclipse <bsp> --near <YYYY-MM-DD> [--kind lunar|solar]
  [--stage any|umb|total] (lunar)
  [--stage any|central]   (solar)
  [--sample-min <minutes>]
  [--point-lat <deg> --point-lon <deg> [--point-height <m>]] [--point-refine 0|1]
  [--global-vis 0|1] [--global 0|1]
  [--global-format json|geojson]
  [--grid-lat-step <deg>] [--grid-lon-step <deg>]
  [--tz ...] [--format json|txt|geojson] [--out ...] [--pretty 0|1] [--quiet]
```

说明：

- `--near` 必填。
- `--kind` 默认 `lunar`。
- `--point-lat` 与 `--point-lon` 必须同时出现。
- `--format geojson` 会强制开启全局可见性计算。
- 默认 `tz/format/pretty` 来自配置（`def_fmt` 非 `json|txt|geojson` 时回退到 `json`）。

### 9.14 festival

```bash
lunar festival <bsp> <year>
  [--tz ...] [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet]
```

默认 `tz/format/pretty` 来自配置。

### 9.15 almanac

```bash
lunar almanac <bsp> <YYYY-MM-DD>
  [--tz ...] [--format json|txt|csv] [--out ...] [--pretty 0|1] [--quiet] [--lon <deg>]
```

默认 `tz/format/pretty` 来自配置。

### 9.16 info

```bash
lunar info <bsp> [--format json|txt] [--out ...] [--pretty 0|1] [--quiet]
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

## 10. 交互模式

### 10.1 启动

直接运行 `lunar`（无参数）。

### 10.2 启动流程

- 读取 `lun_cfg.txt`
- 若 `def_bsp` 存在，会提示是否继续使用
- 可指定额外目录扫描 `.bsp`
- 如未找到可用星历，可进入下载界面

### 10.3 菜单

- `1` months
- `2` calendar
- `3` at
- `4` convert
- `5` day
- `6` next
- `7` festival
- `8` info
- `9` monthview
- `10` range
- `11` search
- `12` eclipse
- `13` almanac
- `14` config
- `15` completion
- `d` 切换/下载 BSP
- `h` 帮助
- `q` 退出

## 11. i18n（中简/中繁/英/日/韩）

### 11.1 语言来源

- 命令行：`--lang zh|zht|en|ja|ko`
- 配置默认：`default_lang`

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

- `month cal year event dl at conv day mview next range search fest alm info test cfg comp`

说明：`eclipse` 当前未单独导出 `lunar_cmd_eclipse`，可通过 `lunar_run` 调用。

### 13.4 返回码

- `0`：成功
- `2`：参数错误（`std::invalid_argument`）
- `1`：其他错误

## 14. 仓库结构

```text
include/lunar/          公共头文件（CLI/C++ API/C API）
src/cli/                months/calendar/year/event/download 相关实现
src/query/              at/convert/day/monthview/next/... 相关实现
src/i18n/               多语言目录与词条
src/interact.cpp        交互模式
src/global_context.cpp  全局配置与自动 BSP 选择
src/entry.cpp           CLI 顶层参数解析与命令分发
src/cli.cpp             cli 模块聚合编译入口
src/query.cpp           query 模块聚合编译入口
src/c_api.cpp           C API 导出实现
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

# 食计算
lunar eclipse --near 2025-09-07 --kind lunar --global-vis 1 --global-format geojson --format json
```
