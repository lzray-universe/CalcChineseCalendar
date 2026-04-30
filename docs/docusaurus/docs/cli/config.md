---
title: config
description: CLI 配置读写的参数与输出。
---

# config

查看和修改 `lun_cfg.txt` 配置。

## 语法

```bash
lunar config show [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

## 参数

- `show`：输出当前配置。
- `set`：写入单个配置项。
- `<key>`：配置键。
- `<value>`：配置值。
- `--format`：`show` 输出格式，支持 `json`、`txt`。
- `--out`：`show` 输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。

## 配置键

- `def_bsp`：默认星历路径或 `@series`。
- `bsp_dir`：自动扫描 `.bsp` 的目录。
- `bsp_list`：候选 BSP 列表，逗号或分号分隔。
- `default_tz`：默认解析和显示时区。
- `default_lang`：默认语言，支持 `zh`、`zht`、`en`、`ja`、`ko`。
- `default_lunar_day_tz`：默认农历判日时区；可用 `default`、`auto`、`inherit` 清空。
- `def_fmt`：默认输出格式。
- `hli_trad`：默认黄历流派。
- `hli_year_boundary`：默认黄历年界。
- `hli_month_boundary`：默认黄历月界。
- `hli_leap_month_mode`：默认闰月处理方式。
- `hli_day_boundary`：默认黄历日界。
- `def_prety`：默认 JSON pretty 开关。

## 输出

- `show json`：结构化配置对象。
- `show txt`：`key=value` 形式。
- `set`：成功时更新配置文件；错误时输出非法键或非法值原因。

## 示例

```bash
lunar config show --format json
lunar config set default_tz +08:00
lunar config set def_bsp ./de442.bsp
```
