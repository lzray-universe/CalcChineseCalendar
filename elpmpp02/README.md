# ELPMPP02

保留内容： 

- `elpmpp02.hpp`
- `elpmpp02.cpp`
- `elpmpp02_data_*.cpp`
- `elpmpp02_data_decl.hpp`

调用：

```cpp
elpmpp02::StateVector state;
elpmpp02::Evaluate(elpmpp02::CorrectionSet::DE405,jd_tdb,state);
```

参数：

- `correction`：`LLR`、`DE405`、`DE406`
- `jd_tdb`：TDB/TT 风格儒略日输入
- `state.position_km`：输出位置，单位 km
- `state.velocity_km_per_day`：输出速度，单位 km/day
