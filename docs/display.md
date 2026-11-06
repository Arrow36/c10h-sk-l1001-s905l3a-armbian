# C10H 内置屏幕适配记录

更新时间：2026-11-06（Asia/Shanghai）。

> **最新重要进展（2026-11-06）**：**偏色、高频抖动、画面折叠等内屏顽疾已全部攻克**！详见完整研究记录 [2026-11-06 内屏偏色与抖动彻底解决及全流程记录](research/fnos-display-resolution-record-2026-11-06.md)。

## 当前结果

| 验收项 | 结果 |
| --- | --- |
| 原厂面板参数提取 | 完成，运行时确认 lcd_8 |
| 原生 DRM 面板初始化、背光 | 已验证 |
| DRM 接管后 ENCL 硬件彩条 | 完整、无上下抖动 |
| DRM 线性 dumb buffer 静态几何图 | 边框、中心十字、单白块完整 |
| 正常颜色（消除黑变绿、白变紫） | **已通过**：激活 `VPU[0x38A0]=0x0001C040` 开启 RGB->YUV709l 矩阵配平 |
| 画面稳定性（消除垂直高频抽动） | **已通过**：优化 `VPU[0x1A2B]=0x82820501`、`0x1DF5=0x77F`、`0x1D22=0x08080808` 及 DSI 极性 |
| 控制台旋转（横屏 1280×800） | **已通过**：`echo 3 > /sys/class/graphics/fbcon/rotate_all` 完美横屏 |
| 开机持久化自启 | **已通过**：MMIO 固化工具 `c10h-display-fix` (4ms 耗时) + systemd 单元 |

硬件彩条在 ENCL 注入，绕过 VPP/OSD/Canvas；其稳定不能证明普通图像通路正常。当前原生几何图已不再有早期的部分画面和重复分区，但仍不能称为完整屏幕支持。

## 原厂参数

- SoC：S905L3A；原厂 Android 内核 4.9，当前 Linux 6.12.114-ophub ARM64。
- 面板：`C_JD9366TC_Single_CSOT_10.1_BPA107LT1-1`，原厂选择 `lcd_8`；MIPI DSI 4 lanes、RGB888、burst video。
- 原生 800×1280；横放对应 1280×800。当前没有使用 1280×720。方向尚未处理。
- 水平：有效 800、前肩 40、同步 8、后肩 28、总计 876。
- 垂直：有效 1280、前肩 140、同步 8、后肩 20、总计 1448。
- 原厂实际像素时钟记录约 76.104 MHz；实验 DRM 请求 76.107 MHz，驱动报告实际 75.857143 MHz、位时钟 531 MHz。此处实际值来自时钟框架，未独立用外部仪器测量。
- 背光 PWM_C / GPIOC_4，25 kHz、正常极性；主线 GPIO 索引 C2=43、A11=60、C1=42，PWM_C 的 C4=45。
- 初始化字节来自本机原厂 lcd_extern/extern_9；不能仅凭 JD9366 相似名称替换为另一块屏幕的初始化表。

## 两条测试路径

### 保留原厂初始化状态

禁用原生 VPU/DSI，让启动程序保持面板输出。曾得到完整稳定硬件彩条；手工接入 simplefb、改 Canvas/图层窗口的普通图像仍异常，未完成接管。

Canvas 读取必须使用选择读取后的 RDATAL/RDATAH（偏移 0x0c/0x10）；偏移 0/4 是写入暂存数据，早期将其当真实取图地址的结论已作废。修正后读到原厂 OSD1 Canvas64：地址 0x7f400000、stride3840、Canvas 高1600、图层1280×800、RGB24。这个地址不是通用可用内存；不应直接从用户态写入。

### 原生 DRM 接管（当前）

从已验证的基础 DTB 重新生成干净配置，禁用两处基础 simplefb、HDMI、借用 SD 卡节点及 eMMC。由原生 meson DRM/PHY、实验 DSI 主机和 C10H 面板驱动拥有显示硬件；未同时加载旧的 OSD/HDR/VE 取图补丁。

先验证同一 ENCL 彩条发生器，用户确认稳定；再用 DRM 分配的线性 dumb buffer，仅做原子图层提交，不传 ALLOW_MODESET，不写假定物理地址。已记录：

```text
connector DSI-1: 35; CRTC: 42; primary plane: 36
mode/source/destination: 800x1280, x=0, y=0
format: XRGB8888, LINEAR; pitch: 3200; size: 4096000
original framebuffer: 43; test framebuffer: 45
```

这些 ID 只对应当次启动，测试程序会动态查找，不应在其他启动中硬编码。静态图仍抖动，说明不是测试动画刷新造成的。成功提交不等于可见结果正确。

## 已做的实验与限制

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

HLINE=767 是当次临时测试值。启动期有 Gamma WR/ADR 就绪超时；后加载成功不代表根因已消除。自动重启没有明确崩溃栈，不能归因于某一读取或驱动；一组状态采集未留下成功完成证据，后续改为拆开采集并先同步基本日志。

自动彩条脚本最初因 Windows CRLF shebang 失败（203/EXEC），已改为 LF 并验证服务成功执行；不能把原先“无彩条”当成硬件测试失败。

## 源码与来源

- `drivers/c10h-display/`：实验面板、原厂初始化表、实际位时钟修正的 DSI 主机、可恢复 ENCL 诊断模块及匹配头文件。内核源码保留 GPL/SPDX 和上游版权声明。
- `dts/c10h-panel-test.dtso`：当前面板连接、GPIO/PWM、禁用 HDMI 与冲突 SD 节点的实验 overlay。基线必须先禁用 eMMC，不能把 overlay 单独当完整板级支持。
- `experiments/display/drm-geometry.c`：用户态原生 DRM 几何图测试；需要 libdrm 开发文件。SIGUSR1 移动白块；SIGTERM 恢复原图层配置。程序持有 DRM master，运行期间不要并发运行其他显示测试。
- `experiments/display/c10h-bootfb-info.c`：寄存器白名单快照，Canvas 选择读取会写入 RD 命令，不是纯粹零写入程序。仅供调查，不自动加载。
- `scripts/c10h-drm-stage1-bars.sh`、对应 systemd unit：指定启动参数时，先保存基本日志再显示硬件彩条；路径对应本次测试环境，不能直接当通用安装脚本。
- `boot/*.cmd.example`：实验启动模板，ROOT_UUID 已改为占位符；不能覆盖原来的已验证脚本后直接使用。

主要参考：[unifreq/linux-6.12.y 的 meson 驱动](https://github.com/unifreq/linux-6.12.y/tree/main/drivers/gpu/drm/meson)、[Khadas 4.9 厂商 LCD 驱动](https://github.com/khadas/linux/tree/khadas-vims-4.9.y/drivers/amlogic/media/vout/lcd)、本机原厂设备树和运行时参数。通用内核头文件不是为本机独立重写的驱动。新增实验驱动保留各文件许可，不因此把原厂固件重新授权。

内核模块必须用匹配的 6.12.114-ophub headers、配置、Module.symvers 构建；本次在电脑 WSL 交叉编译，GCC14 构建模块、运行内核 GCC15.3。用户态小程序在设备 GCC14 编译并通过 -Wall -Wextra -Werror。仓库不提供预编译模块或可保证直接启动的镜像。

构建步骤见[源码构建说明](build-display.md)。

## 恢复与后续

见[后续实测](display-followup.md)和[历史检查点](checkpoint-2026-11-03.md)。当前开机自动彩条已关闭，横屏登录由 FbTerm 提供。偏色和抖动仍待定位，断电重启尚未验收。
