# AC792 / WL83 静态库构建

`../wdl_jieli` 和 `../opus_jieli` 使用官方 2024-04-02 的
[pi32v2-r3 large-program demo](https://doc.zh-jieli.com/Tools/zh-cn/dev_tools/toolchains/linux_lib.html#id3)
的 Clang + LTO 归档方式。两个目录的 Makefile 是 CMake 的便捷入口，源文件列表由
CMake 管理；共用本目录的 `JieliCompilePi32v2.cmake`，无需维护两套编译参数。

## 编译

需要 CMake >= 3.24；Linux x64 使用 Make 和 `/opt/jieli/pi32v2` 工具链，
Windows 使用 Ninja 和 `C:/JL/pi32` 工具链（与 SDK Windows Makefile 默认路径一致）。
首次配置下载固定版本的上游源码，并校验 SHA-256；源码与生成物保存在已忽略的
`build/` 目录。已有 `lib/wdl`、`lib/opus` 工程不受影响。

Windows PowerShell 在 `opus_jieli` 或 `wdl_jieli` 目录执行：

```powershell
cmake -S . -B build -G Ninja -DJIELI_TOOLCHAIN_ROOT=C:/JL/pi32
cmake --build build --parallel 4
```

切换操作系统或工具链时，在配置命令中加 `--fresh` 重建 CMake 缓存。Windows 产物在 `build/lib/`。
工具链路径如有不同，修改 `JIELI_TOOLCHAIN_ROOT`。无需 WSL 或 GNU Make。
如果 MinGW 版 CMake 下载源码时报 `certificate signer not trusted`，
可指定已安装的 Git for Windows CA 文件后重新配置（按实际安装路径调整）：

```powershell
cmake -S . -B build -G Ninja -DJIELI_TOOLCHAIN_ROOT=C:/JL/pi32 "-DCMAKE_TLS_CAINFO=C:/Program Files/Git/mingw64/etc/ssl/certs/ca-bundle.crt"
```

安装到 SDK 使用 `cmake --build build --target install-sdk`，
会覆盖 SDK 对应的 `.a` 文件。

在 macOS 上通过 OrbStack 直接编译共享目录，无需同步副本：

```sh
orbctl run -m debian-x64 \
  -w /Users/awalol/Code/fw-AC792_SDK/sdk/lib/wdl_jieli \
  sh -lc 'set -eu; ulimit -n 8096; make JOBS=4'
orbctl run -m debian-x64 \
  -w /Users/awalol/Code/fw-AC792_SDK/sdk/lib/opus_jieli \
  sh -lc 'set -eu; ulimit -n 8096; make JOBS=4'
```

也可以在 Linux 的任一项目目录直接使用 CMake：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
```

输出分别为 `wdl_jieli/build/lib/libwdl_resampler.a` 和
`opus_jieli/build/lib/libopus.a`。默认编译只生成项目内的库。

如需将库复制到 SDK 的 `liba` 目录，在对应 Linux 项目目录执行 `make install-sdk`，
或 `cmake --build build --target install-sdk`。这会覆盖
`sdk/cpu/wl83/liba/` 中相应的 `.a`；库文件是已有的受版本控制文件，生成的二进制
不应混入源码提交。当前 `ac792n_ds5dongle` 默认直接读取两个项目的
`build/` 目录中的库和上游头文件，并将这两个库列为 ELF 的依赖；
构建完两个库后即可重新构建固件，无需执行 `install-sdk`。

Make 支持 `BUILD_DIR=build-debug`、`JOBS=4` 和
`CMAKE_ARGS='-DCMAKE_BUILD_TYPE=Debug'`。`make clean` 只清理当前构建的生成物，
保留已下载的源码。更换工具链时使用新的 build 目录；可通过
`-DJIELI_TOOLCHAIN_ROOT=/path/to/pi32v2` 指定工具链根目录。

## 与 AC792 的配置一致性

- 目标 `pi32v2`、CPU `r3`、`-mfprev1`，开启 `-flto` 与
  `-mllvm -pi32v2-large-program=true`；Release 使用 `-O2`。
- 使用 `lto-ar` 创建并索引归档，包括 CMake 的编译能力检测；不跳过编译器检测。
- 保持 SDK 的默认位域 ABI，不启用 AC710N demo 的 `-fdiscrete-bitfield-abi`。
- 对齐 SDK 的 section、emulated TLS、MS extensions 和 newlib 头文件设置。
- C++ 使用 `gnu++14`，禁用 exceptions 和 RTTI。
- WDL 只编译 `WDL/resample.cpp`，保留原工程的 `WDL_RESAMPLE_TYPE=float`
  和 `-fsigned-char`，禁用 x86 SSE。使用者包含 `resample.h` 时也必须定义
  `WDL_RESAMPLE_TYPE=float`；CMake target 将该宏作为 PUBLIC 属性传递。
- Opus 保留浮点实现和 float API，禁用 x86/ARM intrinsics、桌面程序与测试。
  不启用 fast-math；关闭仅凭编译选项检测无法确认固件运行库支持的
  stack protector 和 fortify，保留上游默认的轻量 hardening。

这些 `.a` 包含杰理 LLVM bitcode，最终必须由兼容版本的杰理 `lto-wrapper`
链接；固件仍需提供 SDK 的 libc/libm 和 C++ 运行库。静态库编译、链接验证不代表
已经验证板上音质、编码耗时或任务栈空间。
