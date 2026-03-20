# VSOP87A

保留内容：

- `vsop87a.hpp`
- `vsop87a.cpp`
- `vsop87a_*.cpp`

调用：

```cpp
double xyz[3]={0.0,0.0,0.0};
double vxyz[3]={0.0,0.0,0.0};
vsop87a::EvaluateXYZ(vsop87a::Body::Earth,jd_tdb,xyz,vxyz);
```

参数：

- `body`：行星或地月质心
- `jd_tdb`：TDB 儒略日
- `xyz`：输出位置，单位 au
- `vxyz`：输出速度，单位 au/day
