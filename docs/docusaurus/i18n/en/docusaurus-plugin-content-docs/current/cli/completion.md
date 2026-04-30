---
title: completion
description: Shell completion script generation.
---

# completion

Generates shell completion scripts.

## Syntax

```bash
lunar completion bash|zsh|fish|powershell
```

## Parameters

- `bash`: generate a Bash completion script.
- `zsh`: generate a Zsh completion script.
- `fish`: generate a Fish completion script.
- `powershell`: generate a PowerShell completion script.

## Output

The script is written to stdout. The command does not install it automatically; redirect and load it according to the target shell.

## Examples

```bash
lunar completion bash > lunar-completion.bash
lunar completion powershell > lunar-completion.ps1
```
