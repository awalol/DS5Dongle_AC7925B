# AC7925B 兼容 nRF24L01 / 鼠标和手柄专有 2.4GHz 协议的调查

调查日期：2026-11-10，Asia/Shanghai。

## 结论及置信边界

**以本仓库的 AC7925B / WL83 SDK 和控制器库为依据，没有找到可直接实现 nRF24L01 / Enhanced ShockBurst（ESB）兼容收发的现成接口或实现。工程选型时应当将它视为“尚不支持直接兼容”，不能把 Wi-Fi Raw 或名字中带 24G 的 API 当成兼容证明。**

这不是“已经证明芯片硅片绝对做不到”。蓝牙基带确实存在修改广播 Access Address 和内部定频的能力；但是决定 nRF24 兼容性的任意包格式、白化、CRC、位序、调制参数及快速收发控制，仍缺乏足够资料和真机验证。是否存在未公开模式，需要杰理提供 WL83 专用资料或进一步做射频实验。

| 目标 | 调查结论 | 依据与限制 |
| --- | --- | --- |
| AC7925B 之间通过 Wi-Fi Raw 自定义数据通信 | SDK 有示例 | 使用 802.11 数据帧，不等于任意 GFSK 收发 |
| 用当前 SDK 直接兼容 nRF24 / ESB | 没有已证实路径 | 缺少完整 ESB 包格式和收发控制实现 |
| 修改 BLE 广播 Access Address | 库里存在真实实现 | 只是 BLE 包处理的一部分，不能推导为 ESB |
| 研究 AC7925B 内置射频模拟 ESB | 可以作为研究课题 | 需要厂商资料或硬件实验，不能保证成功 |
| 外接 nRF24L01+ 与已知、可配置的 nRF24 对端通信 | 有明确硬件路径 | 对端的频率、速率、地址、CRC、载荷和 ACK 配置必须匹配 |
| 外接可运行 Nordic ESB 的 nRF5 芯片 | 有厂商协议栈支持 | 仍须选定具体芯片及与 nRF24 兼容的模式，不可把所有 PHY 模式混用 |
| 兼容某个原厂鼠标/手柄 | 必须具体到型号、硬件版本和工作模式 | 对端不一定使用 ESB；即使使用 ESB，也还需要设备协议 |

## 1. Wi-Fi Raw 为什么解决不了 nRF24 兼容

本地 `sdk/apps/common/example/wifi/raw_txrx/raw_test.c` 的发送和接收明确使用 802.11 帧头：

- 示例定义 `head_802_11`，配置目标、源和 BSSID 地址。
- 使用监听模式和 `wifi_set_frame_cb()` 接收。
- 通过 `wifi_get_payload_ptr()` 和 `wifi_send_data()` 发数据。
- `wifi_connect.h` 的速率枚举与芯片规格书对应的是 Wi-Fi 速率。

AC7925B Datasheet V1.5 的 §3.8 将 802.11b 的 1Mbps 标为 DSSS，另有 CCK、OFDM 模式。nRF24L01+ 规格书 §2、§5 描述的是 GFSK。

因此，将 `WIFI_TXRATE_1M` 选成 1Mbps 或让频率接近 nRF24 所用频率，不会使 Wi-Fi 包变成 nRF24 包。Wi-Fi 监听也没有因此获得接收任意 GFSK 比特流的能力。

## 2. 对实际控制器库的取证

完整取证摘要在同目录 `ac7925b_proprietary_24g_evidence.txt`，包含库 SHA256、符号检索结果、相关 LLVM IR 和参考行号。

检查对象：

- 基础库 `sdk/cpu/wl83/liba/btctrler.a`。
- 当前 DS5 工程的派生库 `sdk/apps/ds5dongle/board/wl83/objs/btctrler_ds5_scanlog0.a`。
- `sdk/cpu/wl83/liba/` 顶层的全部 77 个归档库的相关符号。

基础控制器归档包含 175 个成员，成员名去重后 168 个。7 个名字各出现两次，因此额外用 `llvm-ar xN` 按出现次数提取，避免默认提取覆盖而漏查。174 个成员是可解析 LLVM bitcode；剩余 `version.z.o` 是版本数据，没有 `.text` 代码。版本字符串为 `BTCTRLER-$587dd82c2`。

派生库中的 `RF.c.o`、`hw_bt_api.c.o`、`bb_le_adv.c.o`、`link_layer.c.o` 与基础库对应成员逐字节相同，下面的核心发现同样适用于这些派生成员。

### 2.1 `rf_set_24g_hackable_coded()` 只有保存参数

头文件确实声明了这个函数，注释称参数是 2.4G 配对码。反汇编为 LLVM IR 后，函数仅执行：

```llvm
store i32 %0, ptr @config_24g_hackable_coded
ret void
```

完整控制器 bitcode 中，该变量只有定义、上述写入和调试元数据三处引用，没有读取。对其他顶层库的符号检查也没有发现读取该变量的外部引用。

**可支持的结论：在本次检查的代码中，调用这个函数没有将参数送入收发路径。** 头文件的名字和注释不能证明它在 WL83 上生效。这里没有进行上电测试，仍保留不可见 ROM 或未公开机制的理论不确定性。

### 2.2 `rf_mdm_con_ble_sync_word()` 是空实现

`sdk/include_lib/btctrler/ble/hci_ll.h` 中的真实参数名是 `tws_esco`，本身也不应直接理解为任意同步字设置器。WL83 库中的函数体只有 `ret void`，没有寄存器写入。

它不能被用作当前芯片修改 ESB 同步字的现成途径。

### 2.3 真正能修改 Access Address 的是另一个接口

同一头文件声明：

```c
void ll_set_private_access_addr_pair_channel(uint32_t access_addr);
```

其实现调用 `bb_le_access_addr_set()`，保存到基带句柄的私有地址字段。`le_hw_advertising()` 随后会：

1. 向 `BDADDR0/BDADDR1` 写入默认广播 Access Address `0x8E89BED6`。
2. 私有地址不为零时，用它的低、高 16 位替换。
3. 向 `CRCWORD0/CRCWORD1` 写入 `0x5555/0x0055`，对应 BLE 广播 CRC 初值 `0x555555`。

**这证明广播 Access Address 有可修改路径。它没有证明可以选择 nRF24 的 3/4/5 字节地址格式、9-bit 控制字段、CRC 多项式、CRC 长度或关闭白化。** 也没有实测其对所有扫描路径是否对称生效。

### 2.4 定频：内部存在，公开声明未必有实现

`btcontroller_modules.h` 声明了 `ble_rf_vendor_fixed_channel()`，但本次检查的控制器库和全部 77 个顶层库中未发现它的定义。

实际存在内部函数 `__set_fixed_channel_index(hw_id, channel)`，被 DUT 测试代码调用，通过 `__set_ble_anchor_con()` 配置硬件。

这说明内部定频能力值得研究，但不应把生产测试内部函数直接当作稳定的应用层收发 API；其参数含义、硬件实体生命周期和协议调度需要一起处理。

### 2.5 `NONCONN_24G` 是什么

当前工程的 `bt_profile_cfg.h` 保留了该枚举和选择条件，未找到对应完整本地示例。

另外读取了杰理官方 AC63 SDK 的 `nonconn_24g/ble_24g_deal.c`，用于辨别术语。这个示例实际使用 BLE 广播、扫描、广播回应及 `ble_op_*`，再设置识别码和固定信道。

因此，“非连接 2.4G”可以指厂商在 BLE 机制上做的私有链路。**它不是“支持 nRF24L01”的同义词，而且 AC63 的示例不能直接证明 AC7925B 的硬件能力。**

## 3. 真正兼容 nRF24 需要哪些能力

依据 Nordic 编写的 nRF24L01+ Product Specification Revision 1.0：

- §5：GFSK 调制。nRF24L01+ 的速率包括 250kbps、1Mbps、2Mbps；不要把带 `+` 型号的全部能力泛化到原始 nRF24L01。
- §6.3：频率按 `2400 + RF_CH` MHz 配置。目标通道必须与对端一致；AC792 的 Wi-Fi 信道编号不能直接套用。
- §7.3：ESB 包由 1 字节前导、3–5 字节地址、9-bit 控制字段、0–32 字节载荷、1–2 字节 CRC 构成。
- 控制字段包括 6-bit 长度、2-bit PID 和 1-bit NO_ACK。
- §7.3.5：CRC 覆盖地址、控制字段和载荷。8-bit 和 16-bit CRC 的多项式及初值固定。
- §7.7、图 16：自动收发切换包含 130µs 时序；双向兼容要处理 ACK、重传和去重。
- §7.9：兼容旧 ShockBurst 的设置和包格式有所区别，不能把所有 Nordic 旧协议一律视为同一格式。

Nordic 官方 ESB 源码提供了进一步可检查的细节：

- 动态载荷模式配置 `s0len=0`、`s1len=3`、长度字段通常为 6 位。
- 关闭白化：`whiteen=false`。
- 使用 MSB 优先的包配置：`big_endian=true`。
- 16-bit CRC 采用 `0xFFFF` 初值、`0x11021` 多项式；8-bit CRC 采用 `0xFF` 初值、`0x107` 多项式，并覆盖地址。
- 官方文档明确描述 nRF5 的 ESB 库与旧 nRF24L 系列互通，但同时要求匹配相应的兼容配置。

这些能力是收发兼容的整体条件。只改一个 32-bit 识别码，甚至只改空中比特率，都不足以替换整套协议。

AC7925B 规格书列出了 BLE 1Mbps、2Mbps 和编码 PHY，但没有公开列出通用 Nordic ESB 模式。BLE 的编码 PHY 不等于 nRF24L01+ 的 250kbps 模式。规格书没有列出某能力也不能单独证明硅片不具备该能力。

## 4. 鼠标 / 手柄专有协议的额外条件

在型号未知时，不能假定它使用 nRF24 或 ESB，也不能仅按品牌推断协议。

即使已确认物理层是 ESB，仍需要识别：

- 频率集合、跳频顺序及丢链后的扫描方式。
- 地址、配对流程及保存的设备身份。
- 数据载荷、序号、校验、ACK 载荷和上报时序。
- 是否有加密、认证或绑定状态。
- 接收器对 USB 主机呈现的 HID 或其他接口。

以上是要逐项确认的调查条件，并非声称所有设备都有全部机制。能接收一个包、能被接收器识别、能稳定实现全部按键/震动/音频，是不同的验证阶段。

因此，没有具体型号前，不能对“某个鼠标或手柄”给出确定兼容清单，也不能承诺外接 nRF24L01+ 后即可兼容任意 2.4G 外设。

## 5. 可实施路线

### 路线 A：保留 AC7925B，外接对应射频器件

如果目标已确认使用可兼容的 nRF24 / ESB，可让 AC7925B 通过 SPI 控制 nRF24L01+。自动 ACK、CRC 等由专用射频器件完成，主芯片负责 USB、载荷处理及业务逻辑。

现有工程已经有 SPI 平台设备注册框架，SDK 也有 SPI master 示例。但当前板子的可用引脚、引出情况、供电和天线布局尚未检查，本文不指定接线。

若目标需要更灵活的包配置或跳频控制，可评估具有合适 RADIO 能力、能运行兼容 ESB 的具体 nRF5 器件，并通过 SPI/UART 与 AC7925B 通信。Nordic 官方 ESB 示例和驱动是这条路线的依据；实际型号和模式仍需核对。

### 路线 B：继续研究 AC7925B 内置射频

优先向杰理确认以下 WL83 专用问题，而不是只问“支持不支持私有 2.4G”：

1. 是否提供与 nRF24L01 / ESB 空中兼容的模式及真机例程？
2. 是否可以完全关闭 BLE 白化、固定包头和自动 CRC；是否能接收并导出校验失败的包或原始解调比特？
3. 是否可以配置前导长度、同步字/地址长度、MSB/LSB 位序、9-bit 包控制字段及非整字节对齐？
4. 是否可以配置 8/16-bit CRC 多项式、初值、覆盖范围？
5. 1Mbps / 2Mbps GFSK 的调制参数和接收滤波能否匹配目标设备？是否有独立的非编码 250kbps 模式？
6. 是否可由硬件或确定时延机制完成 ESB 要求的 ACK 切换？
7. `rf_set_24g_hackable_coded()` 为何在该库中只有变量写入？WL83 应使用哪个替代接口？
8. 是否提供内部 `bb_le_frame.h`、`RF_ble.h`、MODEM 寄存器的有效位定义及独立收发调度方法？

LLVM 调试信息能恢复部分结构体字段，如 `FORMAT`、`OPTCNTL`、`CRCWORD0/1`、`TXPTR`、`RXPTR`、`FRQ_TBL`；但字段名不等于位定义，更不能证明存在任意包格式模式。

如果厂商资料暂不可得，建议先用可控的 nRF24 对端做最小实验：

1. 固定频率、1Mbps、固定已知地址和载荷，先关闭自动 ACK，建立对端的正常通信基线。
2. 用能解调目标信号的设备检查实际空中比特流；频谱上“有峰”不能当作兼容证据。
3. 分别验证 AC792→nRF24 和 nRF24→AC792，确认收到正确载荷并校验 CRC。
4. 两个方向都成功后再验证自动 ACK、重传和长时间误包率。
5. 最后才加入目标设备的配对、跳频和业务协议。

实验可能失败在调制/同步、包格式、CRC 或调度的任一层。即使某种特殊构造能单向发送部分兼容包，也不等于通用双向 ESB 已经实现。当前没有执行射频实验，也没有烧录修改过的固件。

## 6. 来源与复现

本地来源：

- `doc/硬件资料/datasheet/AC7925B Datasheet V1.5.pdf`：功能页、§3.7–3.8。
- `sdk/include_lib/btctrler/btcontroller_modules.h`：24G API 与定频声明。
- `sdk/include_lib/btctrler/ble/hci_ll.h`：私有 Access Address API。
- `sdk/apps/common/example/wifi/raw_txrx/raw_test.c`：Wi-Fi Raw 示例。
- 本文第 2 节所列静态库及同目录取证摘要。

外部来源均在 2026-11-10 获取，采用厂商资料、厂商维护的代码或厂商编写资料的镜像：

| 来源 | 位置与用途 |
| --- | --- |
| Nordic nRF24L01+ Product Specification Rev. 1.0，SparkFun 托管的 Nordic 原文 | `https://cdn.sparkfun.com/assets/3/d/8/5/1/nRF24L01P_Product_Specification_1_0.pdf`；核对 §5–7，已目视核对第 28 页包格式图、第 43 页时序图 |
| Nordic 官方 ESB 文档 | `https://github.com/nrfconnect/sdk-nrf/blob/main/doc/nrf/protocols/esb/index.rst` |
| Nordic 官方 ESB 实现 | `https://github.com/nrfconnect/sdk-nrf/blob/main/subsys/esb/esb.c`；包格式、白化、位序、CRC |
| Nordic 官方 ESB 配置 | `https://github.com/nrfconnect/sdk-nrf/blob/main/subsys/esb/Kconfig` |
| 杰理官方 AC63 非连接 24G 示例 | `https://github.com/Jieli-Tech/fw-AC63_BT_SDK/blob/master/apps/spp_and_le/examples/nonconn_24g/ble_24g_deal.c`；仅用于辨别该厂商术语，不作为 WL83 能力证明 |
| 杰理 AC792 在线文档 | `https://doc.zh-jieli.com/AC792/zh-cn/wifi_video_master/index.html`；已读取文档入口及搜索索引，未据此找到 ESB 兼容承诺 |

原始下载、全部解码 IR、引用检索和 PDF 页图保留在已被 Git 忽略的 `tmp/rf_compat_20261110/`。它们是本地调查缓存，不属于固件输入。

复现关键命令：

```powershell
llvm-ar t sdk/cpu/wl83/liba/btctrler.a
llvm-nm --print-file-name sdk/cpu/wl83/liba/btctrler.a
# 在单独临时目录提取，勿覆盖工程成员。
llvm-ar x <absolute-path-to-btctrler.a> RF.c.o hw_bt_api.c.o
llvm-dis RF.c.o -o RF.c.o.ll
llvm-dis hw_bt_api.c.o -o hw_bt_api.c.o.ll
# 对有重名成员的归档，使用 xN 和对应的出现次数分别提取。
```

本次只增加调查文档和取证摘要，没有修改固件源文件、库、构建参数或已连接设备。
