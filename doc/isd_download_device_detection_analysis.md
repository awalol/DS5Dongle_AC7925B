# isd_download 在本机无法识别设备的原因

检查日期：2026-11-08（Asia/Shanghai）。

## 后续补充：CLion 构建仍报告 OFFLINE

用户将下载工具改为 Medium 后，又遇到 CLion 构建的 POST-BUILD 阶段报告设备离线。再次检查确认：

- 项目内 `isd_download.exe` 已是显式 Medium，直接运行 `-check-dev-online wl83` 返回 ONLINE。
- CLion 当前进程是 Medium。
- CLion 配置使用的 `sdk/tools/utils/make.exe` 仍继承 Low 标签。
- Low 的 make 启动后续命令时，子进程继续处于 Low；下载工具文件上的 Medium 标签不能让它从 Low 父进程自动提升到 Medium。

因此，CLion 构建场景还需要处理 **make.exe 的 Low 标签**。前面的“只设置下载工具”建议只覆盖从 Medium 终端直接启动的场景，没有覆盖 Low make 启动的构建链。

实际构建调用关系为：

```text
CLion（Medium）
  -> sdk/tools/utils/make.exe（Low）
     -> 递归 make（Low，同一 EXE）
        -> cmd / download.bat（Low）
           -> isd_download.exe（Low，尽管 EXE 文件已标为 Medium）
```

在临时目录创建了只执行完整性级别读取和在线检查的 Makefile，没有运行真实构建或下载固件：

| 启动方式 | 探针进程完整性 | 项目内下载工具检查 | 外部下载工具检查 |
| --- | --- | --- | --- |
| 直接从普通终端启动 | Medium | ONLINE，退出码 0 | ONLINE，退出码 0 |
| SDK 原路径 make 启动 | Low | OFFLINE，退出码 -11 | OFFLINE，退出码 -11 |
| 临时目录中相同 make 副本启动 | Medium | ONLINE，退出码 0 | ONLINE，退出码 0 |

原路径 make 和临时目录副本的 SHA-256 均为：

```text
CD6734001D62DAA472B4CD1284F685213027A35F458E86C04EA3320A3A455225
```

这也说明，只将烧录脚本里的工具路径换成 `C:\jltools\isd_download.exe`，不能解决由 Low make 启动时的问题。

建议在保持下载工具为 Medium 的同时，对实际使用的 make 单独设置 Medium：

```powershell
icacls.exe 'C:\Users\awalol\Code\DS5Dongle_AC7925B\sdk\tools\utils\make.exe' /setintegritylevel M
```

然后停止旧构建，再启动一次新的构建。本次诊断没有修改原路径 make 的权限，也没有执行真实的 SDRAM 下载或固件烧录。

## 结论

项目内 `sdk/cpu/wl83/tools/isd_download.exe` 继承了 `sdk` 目录的 **Low Mandatory Level（低完整性级别）**。从该文件启动的进程处于 Low 完整性级别，申请以读写方式打开升级设备卷时，被 Windows 拒绝。程序随后将设备判定为离线。

已经用**二进制完全一致、位于其他目录的 isd_download.exe** 验证：在本机、同一台设备、普通用户权限下，可以正常打开设备并报告 ONLINE。因此本次故障已经定位到文件完整性标签。

首次诊断没有修改原始 EXE、目录 ACL、系统安全设置或烧录脚本，没有触发固件升级或擦除。后续用户已将下载工具文件标签设置为 Medium，见上面的 CLion 补充。

## 环境和设备

- 系统：Windows 11 专业工作站版，`10.0.26300`，64 位。
- 项目内下载工具：`isd_download 4.2.81`。
- 图形升级工具：`Firmware Update 1.4.50`。
- USB：`VID_4C4A&PID_2942`。
- USB 产品描述：`WL83UBOOT1.00`。
- 驱动：Microsoft `usbstor.inf`，服务 `USBSTOR`，版本 `10.0.26100.9444`。
- 在线时的磁盘：`WL83 UBOOT1.00 USB Device`，`\\.\PHYSICALDRIVE2`，卷盘符 `E:`。

磁盘编号、盘符和实例路径是检查时的值，重新插拔后可能变化。

## 实测证据

### 1. 原路径与外部路径是同一个程序

比较了：

```text
C:\Users\awalol\Code\DS5Dongle_AC7925B\sdk\cpu\wl83\tools\isd_download.exe
C:\jltools\isd_download.exe
```

两者 SHA-256 均为：

```text
9a4d5364e63d1affc937ecb8f7f683749053fe9c50477ccbfbe194618d69c10a
```

两者的 Authenticode 签名均有效，签名者为珠海杰理。

### 2. 相同调用，结果不同

分别执行各路径的 EXE，并传入 `-check-dev-online wl83`：

| 程序位置 | 输出 | 退出码 |
| --- | --- | --- |
| 项目内 `sdk/cpu/wl83/tools` | `Device "wl83" status: OFFLINE` | `-11`（无符号显示为 `4294967285`） |
| `C:\jltools` | `Device "wl83" status: ONLINE` | `0` |
| 临时目录中的相同文件副本 | `Device "wl83" status: ONLINE` | `0` |

使用 `--console`，只执行 `open E:`、`close`、`quit`：

```text
项目内：
ERROR: Failed to open device(\\?\E:), error code:5

C:\jltools：
Open device(\\?\E:) successful
Close device(\\?\E:) successful
```

此外，在项目原来的 `tools` 工作目录下调用 `C:\jltools\isd_download.exe`，同样返回 ONLINE。因此导致差异的是 EXE 所在文件的标签，而不是当前工作目录。

### 3. 系统调用与进程令牌

对项目内下载工具和图形升级工具分别抓取运行时调用：

| 项目 | 项目内 isd_download | 图形升级工具 |
| --- | --- | --- |
| 进程完整性 RID | `4096 / 0x1000`，Low | `8192 / 0x2000`，Medium |
| 打开的路径 | `\\?\E:` | `\\?\E:` |
| 请求访问权限 | `0xC0000000`，读和写 | `0xC0000000`，读和写 |
| 共享模式 | `3`，共享读和写 | `3`，共享读和写 |
| 创建方式 | `OPEN_EXISTING` | `OPEN_EXISTING` |
| CreateFile 标志 | `0` | `0` |
| NtCreateFile 结果 | `0xC0000022`，拒绝访问 | `0`，成功 |
| CreateFile 结果 | 无效句柄，Win32 错误 `5` | 有效句柄 |

图形升级工具随后成功执行 `IOCTL_STORAGE_QUERY_PROPERTY`（`0x2D1400`），返回的描述包含 `WL83` 和 `UBOOT1.00`。

两个工具的 USB/SCSI 层均存在 `jl_usb_scsi_windows`，相关打开设备代码使用相同的访问参数。关键差异出现在 Windows 为进程授予设备访问权限时。

### 4. 文件与目录标签

只读执行 `icacls`，项目内 EXE 显示：

```text
Mandatory Label\Low Mandatory Level:(I)(NW)
```

其中 `(I)` 表示继承。沿父目录检查，`sdk` 目录显式带有：

```text
Mandatory Label\Low Mandatory Level:(OI)(CI)(NW)
```

`sdk/cpu`、`sdk/cpu/wl83` 和 `sdk/cpu/wl83/tools` 均继承该标签。

`C:\jltools\isd_download.exe` 和图形升级工具未显示这个 Low 标签。

相关目录 ACL 还包含 `CodexSandboxUsers` 和沙箱 SID 的权限条目。这是本机观察到的关联线索；本次没有审计历史，不能断言 Low 标签具体由哪个进程、在什么时间设置。

## 为什么 Low 文件会影响设备识别

Windows 的 Mandatory Integrity Control（MIC）除了检查普通文件权限，还会检查完整性级别。进程启动时，其完整性级别受可执行文件标签约束。

本次原路径的 EXE 启动后确实处于 Low。下载工具以读写权限申请打开设备卷，该请求被系统拒绝；程序对打开失败的设备继续筛选，最终报告 OFFLINE。

因此，“系统里设备在线”和“下载工具报告设备离线”可以同时成立：Windows 已经完成 USB 枚举，但工具没有取得所需的设备句柄。

微软机制说明：

```text
https://learn.microsoft.com/en-us/windows/win32/secauthz/mandatory-integrity-control
```

## 已验证的使用方法

保留 SDK 的工作目录，使配置、固件和相对路径按原方式解析，调用外部目录中已经存在的同版工具：

```powershell
Set-Location 'C:\Users\awalol\Code\DS5Dongle_AC7925B\sdk\cpu\wl83\tools'
& 'C:\jltools\isd_download.exe' -check-dev-online wl83
```

本机实测：

```text
Device "wl83" status: ONLINE
```

后续若调整烧录脚本，可以把调用位置的 `isd_download.exe` 改为 `"C:\jltools\isd_download.exe"`，保留原来的配置和烧录参数。本次仅提供分析，未修改脚本。

## Windows 10 对比的边界

尚未读取 Windows 10 机器上对应文件的完整性标签，也没有审计本机标签的变更历史。已完成的同机、同二进制对照足以解释当前 Windows 11 机器的故障，但不能据此声称 Windows 11 普遍不支持这个版本的工具，或断言 Windows 10 上一定没有同类配置问题。
