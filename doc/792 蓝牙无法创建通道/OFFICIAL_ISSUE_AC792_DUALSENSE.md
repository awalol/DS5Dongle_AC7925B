# Issue 标题

AC792N 连接 Sony DualSense 时第二次 `LMP_AU_RAND` 被拒绝，导致 HID L2CAP 通道超时

## 问题描述

在 AC792N 上以 Classic Bluetooth 主机方式连接 Sony DualSense Wireless Controller 时，SSP 配对、认证和首次开启加密均能成功，但控制器在加密完成后收到对端再次发来的 `LMP_AU_RAND` 时，没有返回 `SRES`，而是在预编译蓝牙控制器库中进入 `fix lmp_rx_au_not accept` 分支。

此后 HID Control 通道（PSM `0x0011`）一直无法建立，最终因 supervision timeout 断开。相同的应用层 HID/L2CAP 连接逻辑在 AC701 平台上可以正常完成 HID Control 和 HID Interrupt 通道创建。

## 测试环境

- 芯片/平台：WL83（AC792N / AC7925B）
- SDK 分支：`release/AC792N_SDK_V3`
- SDK 标签：`AC792N_SDK_BETA_V3.1.8_2026-09-24`
- BT controller 版本：`BTCTRLER-$587dd82c2`
- 目标设备：Sony DualSense Wireless Controller
- 连接方式：Bluetooth Classic，设备端作为主机主动连接手柄
- HID PSM：Control `0x0011`，Interrupt `0x0013`
- 涉及的预编译库：`sdk/cpu/wl83/liba/btctrler.a`
- 原始库 SHA-256：`f5c1058ac1cc543800bc36b76529acf796e62275f6e8cbea4512a11e9eaf3ec4`

## 复现步骤

1. 使用上述 SDK 编译并烧录 AC792N 固件。
2. 注册 HID Control（PSM `0x0011`）和 HID Interrupt（PSM `0x0013`）L2CAP 服务。
3. 将 DualSense 置于配对模式，由 AC792N 搜索并连接手柄。
4. 接受 SSP user confirmation，等待认证和加密完成。
5. 在 `BT_STATUS_ENCRY_COMPLETE` 后创建 HID Control L2CAP 通道。
6. 观察控制器日志及 L2CAP channel-opened 事件。

## 实际结果

首次认证和加密完成后，DualSense 再次发送 `LMP_AU_RAND`。AC792N 进入 `fix lmp_rx_au_not accept` 分支，没有继续发送 `SRES`：

```text
[00:00:20.159][Info]: [LMP]LMP_SRES
[00:00:20.162][Info]: [LMP]sres  match
[00:00:20.166][Info]: [LMP]LMP_AU_RAND
[00:00:20.170][Info]: [LMP]lmp_rx_au_rand=790,36
[00:00:20.174][Info]: [LMP]send_sres_now : 4, 36
...
[00:00:20.386]HCI_EVENT_ENCRYPTION_CHANGE
[00:00:20.394]<BT-log> : HCI_EVENT_VENDOR_ENCRY_COMPLETE
[00:00:20.400][Info]: [DS5_BT]BT_STATUS_ENCRY_COMPLETE
[00:00:20.405][Info]: [LMP]LMP_AU_RAND
[00:00:20.408][Info]: [LMP]lmp_rx_au_rand=3,0
[00:00:20.417][Info]: [LMP]fix lmp_rx_au_not accept
[00:00:20.422][L2C-info] :L2CAP_STATE_WILL_SEND_CONNECTION_REQUEST
[00:00:20.428][Info]: [DS5_BT]opening HID Control, waiting for channel-opened event
```

约 62 秒后链路以 reason `0x08`（Connection Timeout）断开，HID Control 通道返回失败状态 `0x50`：

```text
[00:01:22.675][Info]: [LMP]lmp_super_timeout 48
[00:01:22.680][Info]: [LMP]do_detach : 48, 8
[00:01:22.688]<BT-log> :HCI_EVENT_DISCONNECTION_COMPLETE:0030,08
[00:01:22.724][L2C-info] :L2CAP_EVENT_CHANNEL_OPENED status 0x50 ... psm 0x11 ...
[00:01:22.748](error): <Error>: [DS5_BT]Open failed psm=0x0011 status=0x50
```

## 期望结果

收到第二次 `LMP_AU_RAND` 后，控制器应按协议完成认证响应（发送 `SRES`）及后续加密状态切换，使 HID Control 和 HID Interrupt L2CAP 通道能够正常建立，而不是拒绝认证并等待链路超时。

作为对比，AC701 的成功日志中同一阶段会继续执行以下流程：

```text
LMP_AU_RAND
send_sres_now
LMP_PAUSE_ENCRYPTION_REQ
LMP_TX_STOP_ENCRYPTION_REQ
LMP_STOP_ENCRYPTION_REQ
LMP_RESUME_ENCRYPTIONG_REQ
LMP_START_ENCRYPTION_REQ
HID Control opened
HID Interrupt opened
```

## 初步定位

应用层收到的 `BT_STATUS_ENCRY_COMPLETE` 只是控制器上报事件；应用层没有公开接口可以处理 `AU_RAND`、生成 `SRES`，或安全修改 LMP 内部状态。因此延迟 HID 建链、改变 SDP 顺序等应用层调整无法解决该问题。

进一步分析 `btctrler.a` 中 `lmp.c.o` 的 LLVM bitcode 后，问题可定位到 `lmp_rx_au_rand` 的拒绝分支。该分支在日志所示内部状态（`lmp_rx_au_rand=3,0`）下发送 `LMP_NOT_ACCEPTED` 并提前返回，未进入后续的 SRES 和加密处理。

为了验证定位，我们仅在测试环境中绕过该拒绝分支并重新生成 `btctrler.a`。修改后的固件能够正常连接同一只 DualSense，HID Control/Interrupt 通道均能建立并持续收到 HID 数据。这说明问题位于预编译 controller 库的 LMP 状态处理，而不是应用层 HID PSM、SDP 或 L2CAP 配置。

此修改仅用于定位问题，不建议作为正式产品修复，因为无条件绕过该保护分支可能影响其他认证场景。

## 希望官方协助

请协助确认：

1. `lmp_rx_au_rand` 在已开启加密后收到新的 `AU_RAND` 时进入上述拒绝分支是否为已知问题；
2. `lmp_rx_au_rand=3,0` 所表示的内部 flags/state 是否符合预期；
3. 是否可以提供修复后的官方 `btctrler.a` 或新版 SDK；
4. 如果当前行为依赖某项 controller 配置，请提供对应的公开配置方法；
5. 正式修复应保留哪些状态检查，以便在支持 DualSense 二次认证的同时不影响其他设备。

## 附件

附件包：`792 蓝牙无法创建通道.zip`

其中包含：

- `7925.log`：AC792N 完整失败日志；
- `701.log`：AC701 正常连接的对比日志；
- `792/`、`701/`：两平台使用的应用层连接代码；
- `AC792_DualSense_btctrler_patch_report.md`：详细定位记录；
- `AC792_btctrler_reproduction.md`：测试补丁复现说明；
- `patch_btctrler_lmp_rx_au_rand.sh`：仅用于证明根因的测试脚本。

如有需要，可以进一步提供完整工程、固件、串口日志或协助抓取空口数据。
