---
title: config
description: Parameters and output for CLI config reads and writes.
---

# config

Reads and updates `lun_cfg.txt`.

## Syntax

```bash
lunar config show [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
lunar config set <key> <value>
```

## Parameters

- `show`: print current config.
- `set`: write one config value.
- `<key>`: config key.
- `<value>`: config value.
- `--format`: output format for `show`, `json` or `txt`.
- `--out`: output path for `show`; omitted means stdout.
- `--pretty`: pretty-print JSON.
- `--quiet`: suppress file-write notices.

## Config keys

- `def_bsp`: default ephemeris path or `@series`.
- `bsp_dir`: directory scanned for `.bsp` files.
- `bsp_list`: candidate BSP list, comma- or semicolon-separated.
- `default_tz`: default parsing and display timezone.
- `default_lang`: default language, `zh`, `zht`, `en`, `ja`, or `ko`.
- `default_lunar_day_tz`: default lunar-day timezone; clear with `default`, `auto`, or `inherit`.
- `def_fmt`: default output format.
- `hli_trad`: default Huangli profile.
- `hli_year_boundary`: default Huangli year boundary.
- `hli_month_boundary`: default Huangli month boundary.
- `hli_leap_month_mode`: default leap-month handling.
- `hli_day_boundary`: default Huangli day boundary.
- `def_prety`: default JSON pretty flag.

## Output

- `show json`: structured config object.
- `show txt`: `key=value` text.
- `set`: updates the config file on success; errors report invalid keys or values.

## Examples

```bash
lunar config show --format json
lunar config set default_tz +08:00
lunar config set def_bsp ./de442.bsp
```
