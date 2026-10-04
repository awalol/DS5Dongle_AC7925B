# Opus for AC792

基于 `lib_demo 2` 的 large-program 配置，构建 `libopus.a`。
上游固定为 Opus `03647f524a40b05a1898522e92033810b58103c7`，与 `../opus` 相同。
源码归档不含 Git 标签，版本字符串用 `0.0.0-g03647f524a40` 标识此快照。

在 Linux x64 中执行 `make`，输出 `build/lib/libopus.a`。
公共头文件位于 `build/_deps/opus_upstream-src/include`；保留浮点编码实现以及
`opus_encode_float` / `opus_decode_float` API。

工具链、OrbStack 命令、安装方法与配置说明见 [公共构建说明](../jieli/README.md)。
