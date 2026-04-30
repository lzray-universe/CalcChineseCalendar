---
title: download
description: BSP 下载命令的参数与输出。
---

# download

列出和下载内置下载表中的 BSP 文件。

## 语法

```bash
lunar download list
lunar download get <id> [--dir <path>] [--quiet]
```

## 参数

- `list`：列出可下载项。
- `get`：下载指定项。
- `<id>`：下载项 ID，例如 `de442s`。
- `--dir`：保存目录；省略时使用当前目录。
- `--quiet`：抑制进度和提示。

## 输出

- `list`：输出可下载 BSP 的 ID、说明和来源。
- `get`：下载文件到目标目录，并输出保存路径或错误信息。

## 示例

```bash
lunar download list
lunar download get de442s --dir ./ephem
```
