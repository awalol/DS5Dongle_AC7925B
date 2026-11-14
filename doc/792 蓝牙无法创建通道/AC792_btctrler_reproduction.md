# AC792 `btctrler.a` 补丁复现方法

之前的修复不是传统文本 patch。`btctrler.a` 的成员是 LLVM 4.0.1 bitcode，不能直接对归档做普通源码 diff。实际修复过程是：提取 `lmp.c.o`，转换为 LLVM IR，修改 `lmp_rx_au_rand` 的一个分支，再用 Jieli pi32v2 编译器生成成员并重建归档索引。

## 原始条件

需要在 `debian-x64` 使用 Jieli 私有工具链：

```text
/opt/jieli/common/bin/clang       LLVM 4.0.1
/opt/jieli/common/bin/llvm-ar
/opt/jieli/pi32v2/bin/clang
```

原始库应使用未修改版本，例如：

```text
sdk/cpu/wl83/liba/btctrler.a.before-lto-patch
```

## 补丁内容

原始 IR 中的判断为：

```llvm
%32 = and i32 %31, 4196481
%33 = icmp eq i32 %32, 0
br i1 %33, label %47, label %34
```

`4196481` 即 `0x00401001`。分支 `%34 -> %38` 会打印 `fix lmp_rx_au_not accept`，发送 `LMP_NOT_ACCEPTED` 后返回；正常认证处理从 `%47` 开始。

补丁把分支改成：

```llvm
br i1 true, label %47, label %47
```

也就是无论内部 flags 判断结果如何，都继续进入后面的 SRES 和加密处理。

## 一键复现

脚本位于：

```text
doc/report/patch_btctrler_lmp_rx_au_rand.sh
```

在虚拟机中执行：

```sh
cd /mnt/mac/Users/awalol/Code/fw-AC792_SDK
sh doc/report/patch_btctrler_lmp_rx_au_rand.sh \
  sdk/cpu/wl83/liba/btctrler.a.before-lto-patch \
  sdk/cpu/wl83/liba/btctrler.a.patched
```

脚本使用与工程一致的 LTO 参数重新生成 `lmp.c.o`，然后用 `llvm-ar r` 替换归档成员，并用 `llvm-ar s` 重建符号索引。

## 构建验证

将生成的 `btctrler.a.patched` 复制为 SDK 使用的 `btctrler.a` 后，在 `sdk` 目录执行：

```sh
make -C apps/ds5dongle/board/wl83 -f Makefile clean
make -C apps/ds5dongle/board/wl83 -f Makefile
```

此前使用该方法生成的库已经通过 clean 编译、链接和固件生成，并在真机上验证 DualSense 可以正常连接。
