# AC792 DualSense 连接问题修复记录

## 问题现象

AC792 连接 DualSense 手柄时，SSP 配对和第一次加密可以成功，但收到第二次 `LMP_AU_RAND` 后进入：

```text
fix lmp_rx_au_not accept
```

随后没有发送 `SRES`，也没有完成后续的暂停、停止、恢复加密流程。HID Control 通道最终超时并断开。

## 问题定位

问题位于预编译控制器库：

```text
sdk/cpu/wl83/liba/btctrler.a
```

从库中提取 `lmp.c.o` 的 LLVM 4.0.1 bitcode，并转换为 LLVM IR 后，定位到函数：

```text
lmp_rx_au_rand
```

该函数在特定连接状态下会打印 `fix lmp_rx_au_not accept`，发送 `LMP_NOT_ACCEPTED` 后直接返回，没有继续执行认证处理，因此中断了 DualSense 的第二次认证流程。

这里的“特定连接状态”不是泛指“连接不稳定”，而是控制器内部 `lmp_connection` 的状态组合。原始 LLVM IR 中的判断等价于：

```c
if ((conn->core.flags & 0x00401001) != 0 &&
    conn->core.state != 0x24) {
    send_lmp_not_accepted();
    return;
}
```

失败日志中的 `lmp_rx_au_rand=3,0` 表示该次 `AU_RAND` 到达时，控制器记录的 flags 为 `0x00000003`、状态字节为 `0x00`。因此 flags 不满足 `flags & 0x00401001 == 0`，状态也不是允许继续处理的 `0x24`，命中了拒绝分支。正常分支随后才会分配或复用加密上下文、保存随机数，并调用 SRES/后续加密处理函数。

## 修复方式

对 `lmp_rx_au_rand` 做二进制级修改，绕过错误的拒绝分支，直接进入后续认证处理，保留原有的 `SRES`、暂停加密、停止加密、恢复加密和再次启动加密流程。

## 为什么应用层不能可靠解决

应用层收到的 `BT_STATUS_ENCRY_COMPLETE` 只是控制器向上层报告的事件，应用层看不到或不能安全修改 `lmp_connection` 内部的 flags、状态字节、加密上下文和 LMP 状态机。`SRES`、`LMP_NOT_ACCEPTED`、`LMP_PAUSE_ENCRYPTION_REQ`、`LMP_STOP_ENCRYPTION_REQ` 和 `LMP_RESUME_ENCRYPTION_REQ` 都由预编译控制器库内部生成，应用层没有公开接口可以在收到 `AU_RAND` 后重新驱动这些步骤。

应用层可以延迟 HID/L2CAP 请求或调整 SDP 顺序，但这只能改变上层事务的时间，不能修复控制器已经对 `AU_RAND` 发送 `LMP_NOT_ACCEPTED` 后的内部状态。实际日志也显示，失败发生在 HID 通道超时之前，根因是控制器未发送 SRES；因此必须修复控制器库或更换使用不同控制器实现的芯片。

修改后的库：

```text
sdk/cpu/wl83/liba/btctrler.a
```

原始库备份：

```text
sdk/cpu/wl83/liba/btctrler.a.before-lto-patch
```

## 工具链与构建

使用 `debian-x64` 上的 Jieli 私有工具链：

```text
/opt/jieli
```

重新生成 `lmp.c.o` 时保留工程原有 LTO 参数，包括：

```text
-flto
-Oz
-g
-fprefer-gnu-section
-mllvm -pi32v2-large-program=true
```

完整 clean 构建命令：

```sh
make clean
make ac792n_ds5dongle
```

## 构建结果

- clean 成功；
- 全量编译成功；
- 链接成功；
- 固件后处理成功；
- `jl_isd.fw` 生成成功；
- `jl_isd.ufw` 生成成功。

固件输出目录：

```text
sdk/cpu/wl83/tools/
```

## 验证结果

补丁固件刷入后，DualSense 已能够正常连接，说明修改后的 `btctrler.a` 已实际参与最终链接，二次认证流程恢复，HID 连接问题得到解决。

## 结论

问题根因是 AC792 控制器库中 `lmp_rx_au_rand` 的错误拒绝分支，而不是应用层 HID、SDP 或 PSM 配置错误。通过修改 `btctrler.a` 并使用正确的 Jieli LTO 参数重新构建，问题已解决。
