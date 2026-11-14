# SDK 杰理 Opus 静态库优化分析

检查日期：2026-11-07。对象为当前工作区中的库，结论来自归档成员、符号表和 LLVM IR 静态检查，未进行板上耗时或音质测试。

> 本文记录的是移植前的静态检查快照；下文“当前工程”的库成员数、硬件依赖及建议均对应当时状态。
> 后续已按用户要求为原版浮点 Opus 接入硬件 FFT 和 PI32V2 后端，仍使用标准 Opus API，
> 没有改用厂商媒体编码库。实现、启动自检、构建开关及验证结果见
> [Opus 后端说明](../sdk/lib/opus_jieli/README.md)。原始二进制检查证据保留不变。

## 结论

杰理媒体目录下的 Opus 编码、解码库都已接入硬件 FFT。编码库使用定点 CELT/SILK，并含有 PI32V2 专用内联汇编。主要证据不是库文件大小或函数名称，而是 MDCT 实现内真实存在的硬件调用及其条件分支。

当前 DS5 dongle 使用的是 `sdk/lib/opus_jieli/build/lib/libopus.a`，没有使用这套杰理媒体编码库。因此，自带库中的硬件 FFT 优化不会自动作用于当前编码。

## 检查对象

| 对象 | 路径 | 成员数 | 识别结果 |
|---|---|---:|---|
| 杰理编码库 | `sdk/cpu/wl83/liba/media/lib_opus_enc.a` | 51 | 定点 CELT/SILK，厂商封装接口，MDCT 硬件 FFT |
| 杰理解码库 | `sdk/cpu/wl83/liba/media/lib_ogg_opus_dec.a` | 69 | 定点解码，逆 MDCT 硬件 FFT |
| SDK 根目录 libopus | `sdk/cpu/wl83/liba/libopus.a` | 137 | 标准 Opus API、浮点实现；未发现杰理 FFT 外部依赖 |
| 当前工程 Opus | `sdk/lib/opus_jieli/build/lib/libopus.a` | 137 | 当前源码构建的浮点版；未发现杰理 FFT 外部依赖 |

根目录 `libopus.a` 与当前构建库的成员列表相同，但文件内容和编译配置不同，不能仅凭文件名认定它是杰理优化版本。本文也不据此断言其来源。

编码库内嵌构建标记包含 `Dec 11 2025 11:06:04`、`PI32V2R3_OPUS_COMB_OGG_COEXIST`；解码库包含 `Mar 28 2025 14:40:08`、`PI32V2R3_ogg_opus_dec_v1_new_API_coexist`。这是构建标记，不是上游 Opus 版本号。

## 已确认的优化

### 1. 在 MDCT 内直接使用硬件 FFT

编码库 `mdct.o_pi32v2` 中的 `clt_mdct_forward_c()` 读取 `JL_HW_FFT_V`：

```c
/* 根据 LLVM IR 整理的逻辑示意，并非原始 C 源码。 */
if (JL_HW_FFT_V == 4) {
    /* 预旋转结果按连续顺序写入，不使用软件 FFT 的 bitrev 排列。 */
    hw_fft_run(fft_config_tab[shift], buffer, buffer);
} else if (JL_HW_FFT_V == 5) {
    /* 使用另一版本的配置接口和缓存配置。 */
    hw_fft_config(...);
    /* 经函数指针调用 _FFT_wrap_。 */
} else {
    /* 预旋转结果使用 st->bitrev[] 排列。 */
    opus_fft_impl(st, buffer);
}
```

FFT_EXT 分支确实把预旋转的数据排列和定点缩放一起改了；不是简单地将软件 FFT 函数名替换成硬件函数名。

编码、解码库中对应表的四个值相同：

| 配置值 | `(cfg >> 16) & 0xfff` | 对应复数 FFT 点数 |
|---|---:|---:|
| `0x01e03521` | 480 | 480 |
| `0x00f03411` | 240 | 240 |
| `0x00783301` | 120 | 120 |
| `0x003c3201` | 60 | 60 |

解码库 `mdct.o` 中的 `clt_mdct_backward_c()` 同样检查 `JL_HW_FFT_V == 4` 并调用 `hw_fft_run()`；其他版本走软件 FFT。逆 MDCT 内使用前向 FFT 是其变换分解的一部分，不能据函数名称判断为错误。

当前 WL83 的 `sdk/audio/cpu/wl83/audio_accelerator/hw_fft.c` 定义 `JL_HW_FFT_V = FFT_EXT`，公共定义中 `FFT_EXT == 4`。因此当前芯片对应的正是上述硬件分支，前提是将相应驱动和库接入构建。

### 2. 使用定点运算

厂商库 MDCT 的输入/输出数据及复数 FFT 分量为 `i32`，窗函数、旋转系数为 `i16`。数学函数 `celt_sqrt_opus_com()`、`celt_rsqrt_norm_opus_com()`、`celt_cos_norm_opus_com()`、`celt_rcp_opus_com()` 也使用整数接口和移位/乘法。

这与当前项目 `OPUS_FIXED_POINT=OFF` 的浮点实现不同。定点实现可直接衔接其硬件 FFT，不需要在 FFT 边界额外引入 float→int→float 转换。

定点本身是 Opus 支持的实现路线，不能将定点版中的全部算法都认作杰理独有修改，也不能仅凭定点选择推导速度提升比例。

### 3. 存在 PI32V2 专用内联汇编

完整检查编码库的 49 个 LLVM bitcode 成员后，发现 16 个成员中包含合计 59 处内联汇编指令块。统计包含编译后重复/内联的指令块，不代表 59 个独立算法。其余两个归档成员为原生 ELF，未计入该统计。

可见示例：

| 成员 | 可见指令/用途 |
|---|---|
| `burg_modified_FIX` | `rep` 循环、16 位装载、乘加累积 |
| `LTP_analysis_filter_FIX` | 多项乘加、移位、饱和、滤波 |
| `noise_shape_analysis_FIX` | 能量累计、滤波、缩放循环 |
| `find_LPC_FIX` | 预测系数递推中的乘法与移位 |
| `NSQ_del_dec`、`encode_frame_FIX` | 批量定点缩放与饱和 |
| `entenc` | `clz` 指令 |
| `opus_demo` | `rep` 批量复制、清零 |

这些汇编优化有相当部分属于 SILK。当前 dongle 使用 CELT 模式，不能把整库中 SILK 的优化都算作当前音频配置能获得的收益。当前场景最直接相关的证据仍是 CELT MDCT 硬件 FFT 和定点计算。

### 4. 未发现逐项调用独立硬件数学函数

从编码库全部 bitcode 成员的直接调用及外部符号看，没有发现 `math_sqrt()`、`math_log()`、`math_exp()`、`root_float()` 或 SDK `vector_real_*` 等接口调用。

`mathops` 内的平方根、倒平方根、余弦、倒数仍可见整数近似计算。因此，目前可确认的是硬件 FFT、定点实现和 CPU 汇编优化，不能宣称它把所有数学计算都交给硬件。这个结论不排除编译器在最终生成机器码时选择专用指令。

部分代码/常量使用 `.opus_comb_enc.text.cache.L2.*`、`.ogg_opus_dec.text.cache.L2.*` 分段。段名说明存在专门的链接布局入口，但实际落在哪种存储器，必须结合最终链接脚本和 map 文件判断。

## 对 DS5 dongle 的适配意义

当前 `sdk/apps/ds5dongle/common/ds5_audio.c` 配置：

- 48 kHz，双声道。
- 每声道 480 样本，即 10 ms。
- CELT，complexity 0。
- 160000 bit/s，CBR。
- 要求每个输出包恰好 200 字节。

厂商库的 `opus_stdemo.o_pi32v2` 导出 `get_opus_stenc_ops()`，其操作表包含查询缓冲区大小、open、run 和 `opus_strun_wb()` 四个入口。静态检查确认：

- 参数结构包含 `sr`、`br`、`nch`、`format_mode`、`complexity`、`frame_ms`。
- open 接受 48000 采样率、双声道及 10 ms 帧。
- open 接受的 complexity 范围为 0..4，不能直接沿用标准 API 0..10 的语义。
- 码率按帧时长计算目标字节数，最大为 512；按当前 160000 bit/s、10 ms 参数推导，目标为 200 字节。
- `opus_strun_wb()` 接受分开的左右声道 PCM 指针。当前应用传给标准 `opus_encode()` 的 PCM 是交错格式，适配时要处理布局。
- 该同步入口可直接写入调用方提供的输出缓冲区；`format_mode == 1` 时可见额外的 8 字节封装，选择 raw 路径时需要避开这类封装。

因此，从静态实现看，这套库有适配当前参数的基础；实际输出是否被 DS5 正确接受、200 字节包是否稳定、音质与耗时是否满足要求，仍需运行验证。

### 不能直接替换链接路径

厂商编码库没有导出标准 `opus_encoder_create()`、`opus_encode()`、`opus_encode_float()`。尽管它也有 `opus_encoder_init()`、`opus_encoder_ctl()` 等同名符号，其内部上下文和部分函数签名与当前标准 API 不同。例如厂商库的 `opus_encoder_init()` 接收其 SILK 上下文和参数指针，不能用标准 Opus 头文件调用。

当前 SDK 对外头文件搜索未找到上述 `OPUS_ENC_PARA` 和 `OPUS_STENC_OPS` 的公开定义；结构和接口信息来自库中保留的调试类型及实现。正式接入宜取得匹配头文件，或在充分验证 ABI 后提供独立适配层。

两套库存在同名全局符号，混合链接也需要明确隔离，不能假设标准版与厂商版能自然共存。当前 dongle 的 Makefile 默认只链接自编译的标准版 Opus。

## 建议

下一步优先验证厂商 CELT 编码入口是否能直接满足当前音频协议；如果可以，应先测这套已经接好硬件 FFT 的实现，再决定是否维护自编译 Opus 的硬件后端。

对比应使用相同的 48 kHz 双声道、10 ms、160 kbit/s 原始包参数，包含左右声道布局转换、驱动等待和内存搬运开销，测量平均及最坏编码耗时，并验证解码音频及 DS5 实机播放。不能仅凭静态库体积或 FFT 单项耗时给出整帧提速结论。

## 证据与复现

同目录 `opus_vendor_optimization_evidence.txt` 保存了库 SHA-256、归档概况、去除内部可解析定义后的外部符号、关键 IR 摘录及汇编统计。

检查工具：当前本机 `C:/JL/pi32/bin/llvm-ar.exe`、`llvm-nm.exe` 和 `clang.exe`。从归档中提取 LLVM bitcode 后用 `clang --target=pi32v2 -mcpu=r3 -mfprev1 -S -emit-llvm -x ir` 转为可读 IR。这是静态检查，不是反编译获得原始源码，也未执行库中的目标代码。
