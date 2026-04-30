---
title: download
description: Parameters and output for BSP download commands.
---

# download

Lists and downloads BSP files from the built-in download table.

## Syntax

```bash
lunar download list
lunar download get <id> [--dir <path>] [--quiet]
```

## Parameters

- `list`: list available downloads.
- `get`: download an item.
- `<id>`: download ID, such as `de442s`.
- `--dir`: save directory; defaults to the current directory.
- `--quiet`: suppress progress and notices.

## Output

- `list`: available BSP IDs, descriptions, and sources.
- `get`: saves the file and prints the path, or prints an error.

## Examples

```bash
lunar download list
lunar download get de442s --dir ./ephem
```
