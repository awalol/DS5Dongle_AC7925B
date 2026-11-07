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

## AC792 硬件 FFT 与 PI32V2 后端

默认启用以下两个独立选项：

| CMake 选项 | 默认值 | 功能 |
|---|---|---|
| `OPUS_JIELI_HW_FFT` | `ON` | 60 / 120 / 240 / 480 点复数硬件 FFT，启动校准及软件回退 |
| `OPUS_JIELI_PI32V2_ASM` | `ON` | 浮点单路/双路内积、对齐复制及清零的 PI32V2 `rep` 汇编循环 |

后端沿用杰理 FFT_EXT 的配置与共享驱动，把原版 FFT 的 bitrev 输入转成硬件要求的
自然顺序，动态完成 float → int → float 缩放。编码、解码的 MDCT 和其他经
`opus_fft_impl()` 的路径均经过同一个调度入口。零输入直接清零；未支持的长度、
NaN/Inf、无法安全缩放的输入和驱动超时均使用原版软件 FFT。

PI32V2 内核按原版浮点接口适配；厂商定点 SILK 的汇编没有直接搬入浮点实现。
复制/清零只在地址按 4 字节对齐且至少 16 字节时使用汇编，并处理尾部字节；
小块或未对齐地址使用 libc。`OPUS_MOVE` 保留 `memmove` 语义。
量化后的硬件 FFT 允许有数值差异，不保证输出码流与纯软件版逐字节相同。

移植代码在 `pi32v2/`；`cmake/OpusPi32v2.cmake` 在构建目录生成只重命名 FFT
软件函数定义的源文件副本。下载的上游源码保持原样；上游定义变化时配置会报错，
要求重新检查适配。

### 固件接入与启动自检

启用硬件 FFT 的固件必须编译 `sdk/audio/cpu/wl83/audio_accelerator/hw_fft.c`，
并提供 `sdk/include_lib/media/hw_fft_f32.h`。适配入口共用 SDK 原有 FFT/vector
锁、SRAM 工作区和硬件描述符。不要再添加一份独立驱动或另一把硬件锁。

应用包含 `pi32v2/opus_pi32v2.h`，在创建任何编解码任务前调用一次：

```c
opus_pi32v2_init();
log_info("opus acceleration backends: 0x%x", opus_pi32v2_backends());
```

初始化必须独立调用，避免关闭日志时被日志宏一并消除。初始化不是并发接口，
应在启动阶段调用；重复调用会返回已有结果。没有调用时保持软件路径。
当前 DS5 dongle 的 Makefile 和 `ds5_audio_init()` 已完成接入。

自检先验证实际 PI32V2 内核的长度边界、对齐、尾部及缓冲区哨兵，再按 FFT 长度
与原版软件结果对比，校准增益，检查随机输入、脉冲、直流、极小和 PCM 幅度。
每个 FFT 长度要求相对误差能量不超过 `1e-7`，未通过则单独禁用。
驱动等待设有轮询上限，失败时保持输入，以便执行软件回退。

日志/返回值是位掩码：

| 位 | 已通过启动自检的后端 |
|---|---|
| `0x001` | FFT 60 点 |
| `0x002` | FFT 120 点 |
| `0x004` | FFT 240 点 |
| `0x008` | FFT 480 点 |
| `0x100` | PI32V2 内积、复制、清零 |

全部通过时为 `0x10f`。当前 48 kHz 双声道、10 ms CELT 主变换使用 240 点 FFT，
关注 `0x004` 是否置位；短块还会使用 60 点 FFT。仅有 `0x100` 表示 FFT 未通过
或已在构建时关闭，`0` 表示全部走软件路径。自检通过说明对应测试向量满足要求，
整帧耗时、实际音频和 DS5 播放仍需板端验证。

接入原 SDK 驱动的共享工作区后，本次固件链接多占用约 16 KiB SRAM；
启动 FFT 自检额外申请约 7.5 KiB 临时堆空间，结束后释放，无逐帧堆分配。
本次链接 map 中剩余堆空间为 `0x1717c`（约 92.4 KiB），随固件配置变化。

### 关闭后端与验证

在本目录执行以下命令可恢复纯软件计算；应用的初始化接口仍可链接：

```powershell
cmake -S . -B build -G Ninja -DJIELI_TOOLCHAIN_ROOT=C:/JL/pi32 -DOPUS_JIELI_HW_FFT=OFF -DOPUS_JIELI_PI32V2_ASM=OFF
cmake --build build --parallel 4
```

恢复时把两个选项改回 `ON` 并重新构建。每次切换后均需重新链接固件。
也可单独关闭某个后端，对比硬件转换和锁等待开销后的整帧耗时。

在仓库根目录执行以下目标，只编译/链接 ELF，不执行 Makefile 的后处理下载流程：

```powershell
sdk/tools/utils/make.exe -C sdk/apps/ds5dongle/board/wl83 -f Makefile pre_build ../../../../cpu/wl83/tools/sdk.elf -j4
```

主机测试使用已有上游源码及普通 C 编译器；在本目录执行：

```powershell
cmake -S tests -B build-host-tests -G Ninja -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe "-DOPUS_UPSTREAM_SOURCE=$PWD/build/_deps/opus_upstream-src"
cmake --build build-host-tests --parallel 4
ctest --test-dir build-host-tests --output-on-failure -j4
```

四组测试覆盖正常硬件模型、超时、错误符号和仅禁用 240 点后端，检查增益校准、
异常输入、软件回退，以及 48 kHz 双声道 2.5 / 5 / 10 / 20 ms、complexity 0 / 5
的完整编解码。160 kbit/s CBR 的 10 ms 输出仍要求 200 字节；另检查 float API。
主机 FFT 用软件模拟量化和不同增益，汇编体替换为 C，因此这些测试不代表实际芯片
或 PI32V2 汇编已执行。实际内核检查在板端启动自检中完成。

2026-11-07 已通过两后端开启的交叉编译、固件 ELF 链接、纯软件配置交叉编译，
以及上述四组主机测试。尚未烧录或测量板端编码耗时，也未验证 DS5 实机播放。

工具链、OrbStack 命令、安装方法与配置说明见 [公共构建说明](../jieli/README.md)。
