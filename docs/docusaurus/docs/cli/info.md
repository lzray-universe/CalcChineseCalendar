---
title: info
description: 版本、配置和星历信息的参数与输出。
---

# info

查看工具版本、配置、星历文件状态和覆盖区间。

## 语法

```bash
lunar info [bsp] [--format json|txt] [--out <path>] [--pretty 0|1] [--quiet]
```

## 参数

- `[bsp]`：可选星历。省略时自动选择。
- `--format`：输出格式，支持 `json`、`txt`。默认固定为 `txt`。
- `--out`：输出文件路径；省略时输出到 stdout。
- `--pretty`：JSON 是否格式化。
- `--quiet`：抑制写文件提示。

## 输出

- `json`：包含工具版本、配置值、星历路径、文件存在性、大小、SPK 对象和覆盖区间。
- `txt`：同类信息的键值文本。

## 示例

```bash
lunar info
lunar info ./de442.bsp --format json --out info.json
```
