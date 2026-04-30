---
title: completion
description: Shell 补全脚本生成命令。
---

# completion

生成 shell 补全脚本。

## 语法

```bash
lunar completion bash|zsh|fish|powershell
```

## 参数

- `bash`：生成 Bash 补全脚本。
- `zsh`：生成 Zsh 补全脚本。
- `fish`：生成 Fish 补全脚本。
- `powershell`：生成 PowerShell 补全脚本。

## 输出

补全脚本写入 stdout。命令不会自动安装脚本，需要按目标 shell 的要求自行重定向和加载。

## 示例

```bash
lunar completion bash > lunar-completion.bash
lunar completion powershell > lunar-completion.ps1
```
