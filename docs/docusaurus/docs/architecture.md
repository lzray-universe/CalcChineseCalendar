---
sidebar_position: 6
title: 架构与仓库结构
description: 从源码目录理解项目分层。
---

# 架构与仓库结构

## 目录分层

| 路径 | 作用 |
| --- | --- |
| `include/lunar/` | 对外头文件与公开接口 |
| `src/` | 核心实现、CLI、查询子系统、i18n、格式化 |
| `src/query/` | `at`、`convert`、`day`、`monthview` 等查询命令实现 |
| `tests/` | gtest/ctest 测试 |calcchinesecalendar
| `publish/python/` | Python 包打包与扩展入口 |
| `publish/npm/` | npm 包打包与 wasm 封装 |
| `vsop87a/`、`elpmpp02/` | 内置级数模型数据与实现 |

## 能力分层

1. 星历与天文计算底层
2. 历法、黄历、事件求解等业务抽象
3. CLI 与格式化输出
4. Python / npm / 动态库等发布封装

项目既能作为命令行工具使用，也能作为内嵌库对外暴露。

