# WDL resampler for AC792

基于 `lib_demo 2` 的 large-program 配置，构建 `libwdl_resampler.a`。
上游固定为 WDL `8338e8193e4c1523c9eb0ec99d1815e14d062375`，与 `../wdl` 相同。

在 Linux x64 中执行 `make`，输出 `build/lib/libwdl_resampler.a`。
头文件位于 `build/_deps/wdl_upstream-src/WDL`；调用侧必须定义
`WDL_RESAMPLE_TYPE=float` 并使用一致的 AC792 C++ 编译参数。

DS5 的 C 音频逻辑通过 `apps/ds5dongle/include/wdl_bridge.h` 调用。
`wdl_bridge.cpp` 单独包含 WDL 头文件，提供不透明句柄的创建、销毁、
输入缓冲区准备与重采样接口；使用输入驱动的线性插值，不启用低通或 sinc 滤波。
实例在音频初始化时显式构造，初始化失败时释放，不依赖全局 C++ 对象构造。

工具链、OrbStack 命令、安装方法与配置说明见 [公共构建说明](../jieli/README.md)。
