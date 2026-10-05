# Opus for AC792

基于 `lib_demo 2` 的 large-program 配置，构建 `libopus.a`。
上游固定为 Opus `03647f524a40b05a1898522e92033810b58103c7`，与 `../opus` 相同。
源码归档不含 Git 标签，版本字符串用 `0.0.0-g03647f524a40` 标识此快照。

在 Linux x64 中执行 `make`，输出 `build/lib/libopus.a`。
Windows PowerShell 下在本目录执行（需要 CMake >= 3.24、Ninja 和杰理工具链）：

```powershell
cmake -S . -B build -G Ninja -DJIELI_TOOLCHAIN_ROOT=C:/JL/pi32
cmake --build build --parallel 4
```

输出为 `build/lib/libopus.a`。切换操作系统或工具链时，在配置命令中加 `--fresh` 重建 CMake 缓存。
如果 CMake 下载源码时报证书错误，见公共构建说明中的 CA 配置。
需要安装到 SDK 时执行 `cmake --build build --target install-sdk`，
这会覆盖 `sdk/cpu/wl83/liba/libopus.a`。

公共头文件位于 `build/_deps/opus_upstream-src/include`；保留浮点编码实现以及
`opus_encode_float` / `opus_decode_float` API。

工具链、OrbStack 命令、安装方法与配置说明见 [公共构建说明](../jieli/README.md)。
