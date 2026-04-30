---
sidebar_position: 1
title: 项目概览
description: CalcChineseCalendar 文档站点总览。
---

# CalcChineseCalendar 文档概览

CalcChineseCalendar 是一个偏工程化的历法与天文计算项目。它优先使用 JPL DE BSP 星历，在缺失 BSP 时自动回退到仓库内置的 VSOP87A + ELPMPP02 级数模型，兼顾结果质量、可移植性和部署灵活性。

目前仓库提供四类主要能力：

- 命令行工具 `lunar`
- C++ 复用接口与 C API
- Python 包 `calcchinesecalendar`
- npm 包 `calcchinesecalendar`

## 适用场景

- 生成农历、黄历、月相、节气等传统历法数据
- 查询指定时间的天象或事件
- 做节日、食象、星座、观测方位等功能型接口
- 在 C++、Python、Node.js 或 WebAssembly 环境中复用统一计算核心

## 核心特点

| 能力方向 | 说明 |
| --- | --- |
| 星历策略 | 优先 BSP，缺失时自动切回内置级数模型 |
| 交互方式 | 既支持 CLI，也支持嵌入式 API |
| 跨平台 | Windows、Linux、macOS、WebAssembly |
| 多语言 | 支持中简、中繁、英文、日文、韩文输出 |
| 发布形态 | 原生库、Python 扩展、npm wasm 包 |

## 文档导览

- [快速开始](./quick-start.md)：先把项目跑起来并执行几个典型命令
- [运行时与星历](./runtime-and-ephemeris.md)：理解 BSP、配置文件和时区规则
- [命令行使用方法](./cli-overview)：查看完整命令语法、参数和示例
- [语言绑定](./bindings.md)：了解 C++ / C / Python / npm 的接入方式
- [架构与仓库结构](./architecture.md)：查看模块边界与源码分布

