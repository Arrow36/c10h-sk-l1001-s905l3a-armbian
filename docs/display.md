# C10H 内置屏幕适配概览

状态整理日期：2026-11-06（Asia/Shanghai）。本页先列当前记录结论，再保留早期参数与实验过程。

## 当前结果（仅 fnOS）

设备使用者已确认 fnOS 内屏显示正常。**Armbian 内屏尚未测试，不能沿用 fnOS 的成功结论。** 下表均指 fnOS；后文历史资料中涉及 Armbian 的显示叙述保留作溯源，不作为已实测状态。

| 项目 | 记录结论 | 边界 |
| --- | --- | --- |
| 面板初始化与背光 | 已验证 JD9366TC 初始化、PWM 背光 | 限指定面板和配置 |
| ENCL 硬件彩条 | 当次完整、无可见抖动 | 绕过 VPP/OSD/Canvas，不能验收普通图像通路 |
| 普通图像几何 | fnOS 实测画面完整 | 冷启动与模式重设待复测 |
| 黑偏绿、白偏紫 | fnOS 实测颜色正常，无明显偏色 | 仅目视反馈，未量化色彩准确度 |
| 高频抽动 | fnOS 联合调整后实测无可见抖动 | 未独立确定每项配置的贡献或完成长时测试 |
| 横屏终端 | fnOS 已实现 1280×800 横屏控制台 | FbTerm 仅为历史参考；Armbian 内屏尚未测试 |
| 自启 | 已有 MMIO 工具、驱动内写入与服务文件 | 源码存在或服务成功不等于实际显示验收 |

具体寄存器值、两条写入路径及验证边界见 [11 月 6 日记录](research/fnos-display-resolution-record-2026-11-06.md)。以下历史实验中的异常和临时配置保留作对照，不代表它们仍是最新状态。

## 原厂参数

- SoC：S905L3A；原厂 Android 内核 4.9，早期 Armbian 实验为 Linux 6.12.114-ophub ARM64。
- 面板：`C_JD9366TC_Single_CSOT_10.1_BPA107LT1-1`，原厂选择 `lcd_8`；MIPI DSI 4 lanes、RGB888、burst video。
- 原生 800×1280；横放对应 1280×800。未使用 1280×720。横屏终端另由 fbcon 或 FbTerm 处理，不改变物理扫描方向。
- 水平：有效 800、前肩 40、同步 8、后肩 28、总计 876。
- 垂直：有效 1280、前肩 140、同步 8、后肩 20、总计 1448。
- 原厂实际像素时钟记录约 76.104 MHz；早期实验 DRM 请求 76.107 MHz，驱动报告实际 75.857143 MHz、位时钟 531 MHz；当前面板源码声明为 75.857 MHz。此处实际值来自时钟框架，未独立用外部仪器测量。
- 背光 PWM_C / GPIOC_4，25 kHz、正常极性；主线 GPIO 索引 C2=43、A11=60、C1=42，PWM_C 的 C4=45。
- 初始化字节来自本机原厂 lcd_extern/extern_9；不能仅凭 JD9366 相似名称替换为另一块屏幕的初始化表。

## 早期测试路径（历史过程）

### 保留原厂初始化状态

禁用原生 VPU/DSI，让启动程序保持面板输出。曾得到完整稳定硬件彩条；手工接入 simplefb、改 Canvas/图层窗口的普通图像仍异常，未完成接管。

Canvas 读取必须使用选择读取后的 RDATAL/RDATAH（偏移 0x0c/0x10）；偏移 0/4 是写入暂存数据，早期将其当真实取图地址的结论已作废。修正后读到原厂 OSD1 Canvas64：地址 0x7f400000、stride3840、Canvas 高1600、图层1280×800、RGB24。这个地址不是通用可用内存；不应直接从用户态写入。

### 原生 DRM 接管

从已验证的基础 DTB 重新生成干净配置，禁用两处基础 simplefb、HDMI、借用 SD 卡节点及 eMMC。由原生 meson DRM/PHY、实验 DSI 主机和 C10H 面板驱动拥有显示硬件；未同时加载旧的 OSD/HDR/VE 取图补丁。

先验证同一 ENCL 彩条发生器，用户确认稳定；再用 DRM 分配的线性 dumb buffer，仅做原子图层提交，不传 ALLOW_MODESET，不写假定物理地址。已记录：

```text
connector DSI-1: 35; CRTC: 42; primary plane: 36
mode/source/destination: 800x1280, x=0, y=0
format: XRGB8888, LINEAR; pitch: 3200; size: 4096000
original framebuffer: 43; test framebuffer: 45
```

这些 ID 只对应当次启动，测试程序会动态查找，不应在其他启动中硬编码。静态图仍抖动，说明不是测试动画刷新造成的。成功提交不等于可见结果正确。

## 早期实验与限制（11 月 6 日调整前）

| 实验 | 可见结果/结论 |
| --- | --- |
| DSI 使用实际位时钟配置 PHY | 改善早期画面；主机请求值与实际值均保留日志 |
| 保留路径：路由从 HDMI 编码器改到 LCD | 原先条纹变成大色块，但未完成普通图像接管 |
| 保留路径：HDR 遗留开关旁路 | 右侧背景由青色变黑；几何仍重复，不能作为完整修复 |
| OSD FIFO、VE 尺寸、输出高度修正 | 对当时异常未见明显改善 |
| 同步极性、Gamma、行周期等早期试验 | 未形成稳定图像；不同启动状态的结果不能混用 |
| 当前 DRM：硬件彩条 | 完整稳定，第一阶段通过 |
| 当前 DRM：晚加载线性 Gamma | 重载成功，但用户反馈颜色和抖动无明显改善 |
| 当前 DRM：HLINE 764→767 | 依据实际频率差异及原厂快照的单项测试；用户反馈无明显改善 |

早期 HLINE=767 是单项测试值；11 月 6 日配置又将其纳入联合调整，不能把前后两次结果等同。启动期有 Gamma WR/ADR 就绪超时；后加载成功不代表根因已消除。自动重启没有明确崩溃栈，不能归因于某一读取或驱动；一组状态采集未留下成功完成证据，后续改为拆开采集并先同步基本日志。

自动彩条脚本最初因 Windows CRLF shebang 失败（203/EXEC），已改为 LF 并验证服务成功执行；不能把原先“无彩条”当成硬件测试失败。

## 源码与来源

- `drivers/c10h-display/`：实验面板、原厂初始化表、实际位时钟修正的 DSI 主机、可恢复 ENCL 诊断模块及匹配头文件。内核源码保留 GPL/SPDX 和上游版权声明。
- `dts/c10h-panel-test.dtso`：当前面板连接、GPIO/PWM、禁用 HDMI 与冲突 SD 节点的实验 overlay。基线必须先禁用 eMMC，不能把 overlay 单独当完整板级支持。
- `experiments/display/drm-geometry.c`：用户态原生 DRM 几何图测试；需要 libdrm 开发文件。SIGUSR1 移动白块；SIGTERM 恢复原图层配置。程序持有 DRM master，运行期间不要并发运行其他显示测试。
- `experiments/display/c10h-bootfb-info.c`：寄存器白名单快照，Canvas 选择读取会写入 RD 命令，不是纯粹零写入程序。仅供调查，不自动加载。
- `scripts/c10h-drm-stage1-bars.sh`、对应 systemd unit：指定启动参数时，先保存基本日志再显示硬件彩条；路径对应本次测试环境，不能直接当通用安装脚本。
- `boot/*.cmd.example`：实验启动模板，ROOT_UUID 已改为占位符；不能覆盖原来的已验证脚本后直接使用。

主要参考：[unifreq/linux-6.12.y 的 meson 驱动](https://github.com/unifreq/linux-6.12.y/tree/main/drivers/gpu/drm/meson)、[Khadas 4.9 厂商 LCD 驱动](https://github.com/khadas/linux/tree/khadas-vims-4.9.y/drivers/amlogic/media/vout/lcd)、本机原厂设备树和运行时参数。通用内核头文件不是为本机独立重写的驱动。新增实验驱动保留各文件许可，不因此把原厂固件重新授权。

内核模块必须用目标内核匹配的 headers、配置、Module.symvers 构建。早期 6.12.114-ophub 实验在电脑 WSL 交叉编译，GCC14 构建模块、运行内核 GCC15.3；当时用户态小程序在设备 GCC14 编译并通过 -Wall -Wextra -Werror。这些历史检查不等于当前源码在所有目标内核的构建验收。仓库不提供预编译模块或可保证直接启动的镜像。

构建步骤见[源码构建说明](build-display.md)。

## 恢复与后续

见[后续实测](display-followup.md)和[历史检查点](checkpoint-2026-11-03.md)。其中关闭自动彩条、FbTerm 横屏及偏色/抖动待定位等描述属于早期状态。11 月 6 日后的配置与限制以本文概览及寄存器调整记录为准；冷启动和长期运行证据仍待补齐。

## 寄存器配置与命名核对

以下为当前工具与面板驱动中实际存在的写入值，证明实现内容，不单独证明执行成功。fnOS 显示正常的现场结论保留，Armbian 内屏仍未测试。详细问题见 [源码审计](code-audit.md)。

| 模块 / 索引或偏移 | 当前写入值 | 按仓库头文件核对的含义 | 证据边界 |
| --- | --- | --- | --- |
| VPU `0x38A0` | `0x0001C040` | `OSD1_HDR2_CTRL`，包含 `REG_ONLY_MAT` bit16 | 写后反馈颜色正常；不能由单个控制值还原全部矩阵系数 |
| VPU `0x1DF1` | `0x05000320` | `VPP_OSD1_IN_SIZE` | 记录关联 800×1280 输入尺寸与几何恢复；不证明所有启动路径 |
| VPU `0x1A2B` | `0x82820501` | `VIU_OSD1_FIFO_CTRL_STAT`；按宏可解出 burst 96、hold lines 8 | 联合修复的一部分；不能据此确诊 DDR 欠载 |
| VPU `0x1DF5` | `0x0000077F` | **`VPP_OSD1_BLD_H_SCOPE`**，不是头文件的 `VPP_OFIFO_SIZE` | 原文称“1919 深度 FIFO”与命名冲突，撤下该解释；保留原写入值和正常显示观察 |
| VPU `0x1D22` | `0x08080808` | `VPP_HOLD_LINES` | 早期 `0x404→0x808` 与后来完整写入值、组合测试不同，不能合并为同一实验 |
| DSI `0x3C8` | `0x00512430` | `MIPI_DSI_TOP_CNTL`，HS/VS invert 位为 bit5/bit4 | Host 也在 PHY 初始化中设置这两位；单项贡献待验证 |
| DSI `0x014` | `0x00000000` | DPI 极性控制（历史记录命名） | 索引口径为字节偏移；字段的 SoC 适用性需结合主控定义 |
| DSI `0x050` | `0x000002FF` | HLINE 767（历史记录命名） | 不是 767 个 framebuffer 像素；早期单项试验无明显改善 |

VPU 基址为 `0xFF900000`，地址按索引乘 4；DSI 基址为 `0xFFD07000`，偏移直接相加。源码可见 [独立工具](../tools/c10h-display-fix.c)、[面板写入](../drivers/c10h-display/panel-c10h-jd9366tc.c)、[寄存器定义](../drivers/c10h-display/meson_registers.h)和 [DSI TOP 定义](../drivers/c10h-display/meson_dw_mipi_dsi.h)。

同一头文件将真正名为 `VPP_OFIFO_SIZE` 的寄存器定义在 `0x1D27`；[11 月 4 日汇总](display-followup.md)记录其值为 `0xFFF01000`。Linux [v6.12.41 头文件](https://github.com/gregkh/linux/blob/v6.12.41/drivers/gpu/drm/meson/meson_registers.h)也作相同命名。这里纠正文档解释，不改动任何地址、写入值或驱动行为；S905L3A 专用完整手册仍是证据缺口。

### 上游代码与当前运行内核的区别

查阅的 Linux [v6.12.41 `meson_viu_init()`](https://github.com/gregkh/linux/blob/v6.12.41/drivers/gpu/drm/meson/meson_viu.c)在 G12A 分支中通过掩码清除 HDR2 的相关位，并配置 OSD1 FIFO；不能简化为“所有主线内核始终整值写入 `0xC040`”。公开参考源码也不能替代 `6.12.41-trim #219` 实际源码、补丁和运行时寄存器证据。

fbcon 旋转只影响控制台，且要求内核编译相关支持；不改变底层 framebuffer 驱动模式或其他直接绘图程序的方向，参见 [Linux fbcon 文档](https://docs.kernel.org/fb/fbcon.html)。

## 当前 fnOS 实机补证

[本轮只读核查](../evidence/fnos-live-check-2026-11-06.md)取得 DSI enabled、活动 DRM 800×1280/XR24、pitch 3200、背光及 rotate=3，使用者确认显示正常。已取得含内嵌 MMIO 信息的当前驱动日志、模块/工具/DTB哈希及 service 本次启动日志。实际 unit 与仓库不同，旋转服务 argv 错误；当前方向另有 `fbcon=rotate:3`，不能以服务返回成功归因横屏。未做寄存器回读、冷启动或 Armbian 测试。

### 当前控制台旋转配置

经使用者要求，实机旧旋转服务已停用并删除；当前由启动参数 `fbcon=rotate:3` 保持控制台方向，核验 rotate=3，显示修复服务 active。旧 unit 的执行日志和问题仅作清理前历史记录，见 [操作记录](../evidence/fnos-live-check-2026-11-06.md#后续操作移除旧旋转服务)。未进行重启复测。
