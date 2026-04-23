---
sidebar_position: 5
title: 语言绑定
description: C++、C、Python 与 npm 的接入方式。
---

# 语言绑定


## C++ API

如果在 C++ 项目里，最直接的方式是复用头文件和核心接口。

- 适合性能敏感或本地工具型项目
- 可直接接入 `lunar::core` 相关能力
- 保持与仓库内部实现最一致的抽象层

## C API

如果你需要给其他语言或运行时做桥接，C API 是更稳的 ABI 边界。

- 适合动态库调用
- 适合跨语言封装
- 也是 Python / npm 发布物设计的重要基础

## Python 包

Python 发布物名为 `calcchinesecalendar`。

```python
import calcchinesecalendar as ccc

print(ccc.version())
result = ccc.day("@series", "2025-06-01")
print(result["data"]["phase_name"])
```

特点：

- 提供高层 helper，如 `day`、`search`、`almanac`
- 提供底层核心接口，如 `core_day`、`ganzhi`
- 也支持 `Lunar` 客户端封装共享默认配置

## npm 包

npm 发布物同样叫 `calcchinesecalendar`，以 ESM 导向的 WebAssembly 形态提供。

```js
import { coreDay, createLunar } from "calcchinesecalendar";

const data = await coreDay({ ephem: "@series", date: "2025-06-01" });
const lunar = await createLunar();
console.log(data.data.phase_name);
console.log(lunar.ganzhi({ ephem: "@series", date: "2025-09-07" }));
```

特点：

- 适合 Node.js 与其他 ESM 环境
- 支持 wasm 文件系统写入 BSP 后再做查询
- 提供高层 helper 和低层 `run` / `runJson`

## 选型建议

| 场景 | 建议入口 |
| --- | --- |
| 本地原生应用 | C++ API |
| 跨语言桥接 | C API |
| 数据分析脚本、服务端任务 | Python |
| Node.js / Web 前后端共用 | npm wasm 包 |

