---
sidebar_position: 4
title: CLI 概览
description: 按命令族梳理 lunar 的主要入口。
---

# CLI 概览


## 日历与日期视图

- `months`：枚举农历月
- `calendar`、`year`：年度视角的节气与月相信息
- `day`、`monthview`：单日和单月视图
- `convert`：公历与农历互转

```bash
lunar monthview @series 2025-06
lunar convert @series solar 2025-06-01
```

## 事件与搜索

- `event`：求单个事件的时刻
- `next`、`range`、`search`：做事件检索、区间扫描和查询表达式搜索
- `at`：给定时刻查看综合天象/历法数据

```bash
lunar event @series full_moon 2025
lunar search @series next solar term --from 2025-06-01T00:00:00+08:00
```

## 天文扩展

- `zodiac`：太阳星座
- `sky`：观测点天空位置
- `eclipse`：日食、月食相关计算

```bash
lunar zodiac @series 2025-09-07
lunar sky @series 2025-06-01T20:00:00+08:00 --lat 31.23 --lon 121.47
```

## 黄历与节日

- `festival`：传统节日
- `almanac`：黄历摘要与宜忌信息

```bash
lunar almanac @series 2025-06-01
```

## 工具型命令

- `download`：列出和下载可用星历
- `info`：查看版本、能力和运行信息
- `config`：管理配置
- `completion`：生成 shell 补全
