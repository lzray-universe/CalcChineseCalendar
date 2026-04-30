---
sidebar_position: 2
title: 快速开始
description: 从构建到首次查询的最短路径。
---

# 快速开始

这一页只覆盖最短上手路径，目标是先把可执行程序跑通，再逐步深入。

## 构建要求

- CMake 3.20+
- 支持 C++20 的编译器
- 运行时建议准备 `.bsp` 星历文件，但没有它也能先跑通基础流程
- `download get` 依赖系统中的 `curl` 或 `wget`

## Windows 构建

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

## Linux / macOS 构建

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 先确认程序可运行

```bash
lunar --version
lunar info
```

如果你还没准备 BSP 文件，许多命令仍然可以通过 `@series` 使用内置模型。

## 下载并配置 BSP

```bash
lunar download list
lunar download get de442s
```

常见做法是把下载好的 `.bsp` 放在项目根目录，或写进 `lun_cfg.txt` 作为默认星历。

## 几个高频命令

```bash
lunar day @series 2025-06-01
lunar monthview @series 2025-06
lunar search @series next full moon --from 2025-06-01T00:00:00+08:00
lunar zodiac @series 2025-09-07
```

## 推荐的阅读顺序

1. 先看 [运行时与星历](./runtime-and-ephemeris.md)，避免在 BSP 选择上踩坑。
2. 再看 [CLI 概览](./cli-overview.md)，按命令族理解功能边界。
3. 如果要嵌入到程序里，再看 [语言绑定](./bindings.md)。
