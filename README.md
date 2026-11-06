# 天翼智屏 C10H / SK-L1001 Linux 移植

创维 SK-L1001（天翼智屏 C10H，Amlogic S905L3A）的外置 Linux 启动和板级适配记录。主要开发基线是 Armbian，另在同一台设备上验证了社区 ARM fnOS。**最新进展（2026-11-06）：内屏所有历史显示异常（半屏紫色/画面折叠、严重偏色、高频抽动抖动）已彻底查明根因并完全解决！已实现 1280×800 横屏控制台自启持久化。**

- **Armbian**：U 盘启动、HDMI 控制台、USB 外设和有线 SSH 已验证。内屏通过硬件 MMIO 修复后实现 100% 满屏无偏色、无抖动、1280×800 横屏登录。
- **fnOS**：U 盘启动、有线 SSH、Web 管理页均已验证。HDMI 控制台可用。切换到内屏独占模式后，LCD 开机自动点亮，画面完整清晰、色彩精准、毫无抽动，完美持久化为 1280×800 横屏终端控制台。
- **原厂 Android**：保留在内置 eMMC。实验 Linux DTB 禁用 eMMC。本机曾在断电拔盘后返回 Android，完整恢复流程尚未验收。

## 设备详情与支持状态

| 项目 | 识别信息与实验结果 |
| --- | --- |
| 整机 | 中国电信天翼智屏 C10H，创维 SK-L1001，原厂平台标识 `p291_iptv`。Linux 显示的 `SKYWORTH E900V22C` 来自借用的设备树，实物型号仍是 SK-L1001。 |
| SoC / CPU | Amlogic S905L3A，已运行 AArch64 Linux。 |
| 内存 | 原厂样机资料记载 2 GB，测试 Linux 约有 1.89 GiB 可用内存。 |
| 内置存储 | 原厂样机资料记载标称 64 GB eMMC 5.0，Android 侧识别到约 58.2 GiB 的 eMMC 块设备容量。Linux 外置测试禁用该控制器。 |
| 原厂系统 | Android 9，Linux 4.9.113，原厂为 32 位 ARM 用户空间。原厂 ARM32 内核模块与实验用的 ARM64 Linux 6.12 内核需要分别构建。 |
| U 盘 / USB | 已验证 U 盘根文件系统、USB 集线器和外接键盘；USB 2.0 单项启动实验通过。长期读写和断电恢复待测。 |
| 有线网络 | 测试链路为 100 Mbps，接口为 `eth0`。Armbian 已验证 SSH、DNS 和 HTTPS。fnOS 已验证 SSH 和 Web 管理页。Windows 共享场景曾出现 DHCP 不稳定。 |
| HDMI | 两套系统均确认外接控制台有画面。热插拔、色彩和模式兼容性待验收。当前 Meson DRM 配置只有一条 CRTC，HDMI 与 DSI 克隆未实现。 |
| 内置面板 | 10.1 英寸 IPS LCD，JD9366TC，面板标识 `C_JD9366TC_Single_CSOT_10.1_BPA107LT1-1`，原厂选择 `lcd_8`。原生竖向 800×1280，横放为 1280×800。 |
| 屏幕接口 | MIPI DSI 四通道，RGB888，burst video，实验模式约 60 Hz。 |
| 背光 | PWM_C / GPIOC_4，25 kHz、正常极性；已在 Linux 点亮。 |
| 触摸 | Linux 未枚举出内置触摸。原厂 DT 有 Goodix、FocalTech 等候选，实际启用芯片待确认。 |
| GPU | Armbian 中 Panfrost 已加载并注册 render 节点，实际渲染加速尚未验证。 |
| Wi-Fi | 原厂走 PCIe W2 / `w2_comm`，借用 DTB 中的 SDIO 节点不能代表实物。Linux 尚未枚举到设备，已编译的实验模块未验证无线连接。 |
| 音频 | 原厂样机资料记载双扬声器与 CSK6012 阵列麦克风。部分实验 DTB 下注册过声卡，另有 DAI 解析和依赖问题。扬声器、功放、麦克风均待验收。 |
| 蓝牙 / 摄像头 | 识别、驱动与实际功能待验证。 |
| 温度 / 功耗 | Linux 注册了 thermal zones，传感器准确性、温控、功耗和长期稳定性待测。 |

标称硬件信息来自 [原厂 Android 样机记录](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod#一设备全貌与硬件规格)，Linux 功能状态来自本仓库实测。

面板模式为 800×1280，水平有效区／前肩／同步／后肩为 800／40／8／28 像素，总计 876；垂直为 1280／140／8／20 行，总计 1448。DRM 模式目标像素时钟 76.107 MHz，实验驱动报告约 75.857 MHz、DSI 位时钟 531 MHz。原厂参数、面板初始化来源和各硬件状态见 [屏幕适配](docs/display.md)与[硬件支持表](docs/hardware.md)。

## 已测试系统

| 项目 | Armbian | fnOS |
| --- | --- | --- |
| 基础镜像 | ophub Armbian 26.11.0 S905L3A Server | ophub 社区 ARM fnOS 1.1.20 S905L3A |
| 用户空间 | Debian 13 Trixie，ARM64 | Debian 12，ARM64 |
| 运行内核 | `6.12.114-ophub` | `6.12.41-trim #219` |
| 根文件系统 | U 盘 `/dev/sda2`，ext4 | U 盘 `/dev/sda2`，Btrfs |
| 板级基线 | 内核配套 E900V22C DTB，加入 C10H 实验修改 | 内核配套 E900V22C DTB，加入 C10H 实验修改 |
| HDMI 与 LCD | 外接控制台与原生 LCD 分开测试 | 初次启动使用 HDMI，LCD 实验中禁用 HDMI，由 DSI 独占 CRTC |
| LCD 普通画面 | **已彻底解决**：满屏显示、无折叠、色彩精准、毫无抖动，支持 1280×800 横屏 | **已彻底解决**：全屏终端、无折叠无紫区、色彩精准、毫无抖动，支持 1280×800 横屏 |
| LCD 硬件彩条 | 完整稳定 | 完整，无可见重复或错位 |

Armbian 镜像文件名和校验值见 [镜像来源](docs/provenance.md)。fnOS 实验使用 `fnnas_amlogic_s905l3a_k6.12.41_2026.09.23.img.gz`，压缩包 SHA-256 为 `94652b7556fa8c1bd78298003fd6cf0db31d03fa8ecd681e997b74c77b367a87`，下载后已核对。写盘前替换了测试 DTB 与一次性启动脚本。验证范围限于外置试启动，仓库没有完整预制系统镜像。 [fnOS 发布页](https://github.com/ophub/fnnas/releases/tag/fnnas_amlogic_693)

## U 盘启动与回退

本机曾由原厂 Android 的 ADB update 重启进入 U-Boot 的更新路径，再执行 U 盘上的一次性 `aml_autoscript`。该入口也含原厂烧录及恢复逻辑，适用范围以 [启动记录](docs/boot.md)为准。正常上电仍以内部 Android 为先。

1. 核对目标镜像的来源和 SHA-256，准备可清空的 U 盘。整盘写入会覆盖原分区及数据。
2. 从目标内核配套的 DTB 开始，使用 `tools/disable_emmc.py` 生成禁用 eMMC 的测试副本。内屏实验另有面板、背光、DSI 和显示路由配置。
3. 启动脚本需匹配镜像实际根 UUID、文件系统、内核、initramfs 与 DTB 路径。使用 `tools/make_boot_script.py` 生成二进制脚本。两份脚本 `aml_autoscript` 与 `s905_autoscript` 保持配套。当前脚本硬编码启动参数，修改 `uEnv.txt` 不会改变实际引导。
4. 优先用 HDMI 控制台和有线 SSH 确认启动，再试内屏。启用 LCD 独占前先保留可用的 HDMI DTB 与配套启动脚本，以便从电脑离线恢复 U 盘启动分区。
5. 本机曾断电、拔掉 U 盘后回到 Android。已有的启动相关分区备份属于实验设备，完整固件恢复和冷启动稳定性仍待验证。

本仓库的一次性脚本不包含 `saveenv` 或 eMMC 写入命令。原厂 U-Boot 的更新入口在 Linux 启动之前运行，Linux DTB 的禁用设置只适用于内核接管后。

## 内屏显示彻底解决与根因总结（2026-11-06 突破）

在 2026-11-06 的联合攻关中，困扰已久的三大内屏显示问题已彻底定位根因并完成完美闭环：

1. **左侧半屏紫块与右侧画面折叠重复（已解决）**：
   - **根因**：原厂 Android U-Boot 在退出时将 `VPP_OSD1_IN_SIZE`（`0x1DF1`）设为横屏尺寸 `0x03200500`（1280×800）。主线 Linux `meson-drm` 从不写入此寄存器，导致输入截断。
   - **修复**：在 U-Boot 中加载 800×1280 开机 Logo 会隐式配置该寄存器；在 Linux 中直接写入 `VPU[0x1DF1] = 0x05000320`（800×1280）即可完全摆脱对开机 Logo 的依赖，实现裸 Linux 满屏显示。
2. **色彩严重倒置（黑变绿、白变紫、RGB 轮转）（已解决）**：
   - **根因**：主线 `meson_viu.c` 在 `meson_viu_init()` 中写入 `0xC040`，清除了 `OSD1_HDR2_CTRL`（`0x38A0`）的 `BIT(16)`。导致前端 RGB->YUV709l 矩阵被关断，未转换的原始 RGB 被直接送入 VPP 后端的 YUV709l->GBR 解算矩阵（$R_{out}=Y+1.402Cr, G_{out}=Y-0.344Cb-0.714Cr, B_{out}=Y+1.772Cb$），使得 $(0,0,0) \to (0,+135,0)$（亮绿），$(255,255,255) \to (255,120,255)$（品红）。
   - **修复**：写入 `VPU[0x38A0] = 0x0001C040` 恢复硬件色彩矩阵，色彩还原度达 100%。
3. **高频垂直跳动/抽动（已解决）**：
   - **根因**：U-Boot 开机 Logo 从不抽动，证明物理面板与 DSI 无硬件缺陷。抽动源自主线 `meson_viu.c` 将 `VIU_OSD1_FIFO_CTRL_STAT`（`0x1A2B`）硬编码为 `0x028207E1`（强制在 800 宽屏维持 31 行大缓存，猝发 32），引起频繁的 DDR 读欠载；此外 `VPP_OFIFO_SIZE`（`0x1DF5`）被主线缩小为 799。
   - **修复**：恢复原厂值 `VPU[0x1A2B] = 0x82820501`（猝发 96、缓冲 8 行），恢复 `VPU[0x1DF5] = 0x0000077F`（1919 满血深度），恢复 `VPU[0x1D22] = 0x08080808`，并对齐 DSI 同步极性 `DSI[0x3C8] = 0x00512430`，画面瞬间完全稳定，彻底告别抽动。
4. **横屏平板模式与自启持久化**：
   - 通过 `echo 3 > /sys/class/graphics/fbcon/rotate_all` 旋转终端为舒适的 1280×800 横屏。
   - 编写原生 C 语言 MMIO 直写工具 `tools/c10h-display-fix.c`（执行耗时仅 4ms），配套 `systemd/c10h-display-fix.service` 与 `systemd/c10h-fbcon-rotate.service` 实现开机全自动生效。

详见完整技术论文：[2026-11-06 内屏偏色与抖动彻底解决及全流程记录](docs/research/fnos-display-resolution-record-2026-11-06.md)。

## 研究全文归档

以下为原研究文档的独立脱敏版本，保留完整章节、实验过程和附录：

- **[2026-11-06 内屏偏色与抖动彻底解决及全流程记录](docs/research/fnos-display-resolution-record-2026-11-06.md)（最新终结篇，推荐优先阅读）**
- [fnOS 内屏适配完整对话整理与最新进展 (2026-11-04)](docs/research/fnos-lcd-full-record-2026-11-04.md)
- [浏览器实时滑杆诊断方法与成果 (2026-11-04)](docs/research/browser-slider-method-2026-11-04.md)
- [fnOS 内屏适配详细实验纪要 (2026-11-04)](docs/research/fnos-lcd-experiment-log-2026-11-04.md)

## 源码与文档索引

| 内容 | 入口 |
| --- | --- |
| 板级支持、启动、网络 | [硬件状态](docs/hardware.md) · [启动与回退](docs/boot.md) · [有线网络](docs/network.md) · [Wi-Fi](docs/wifi.md) |
| Armbian 与 fnOS 显示 | [面板时序与实验](docs/display.md) · [后续实测](docs/display-followup.md) · [fnOS 实验](docs/display-fnos.md) · [终解全记录](docs/research/fnos-display-resolution-record-2026-11-06.md) |
| 显示修复工具与服务 | [MMIO 直写修复工具](tools/c10h-display-fix.c) · [硬件修复服务](systemd/c10h-display-fix.service) · [横屏旋转服务](systemd/c10h-fbcon-rotate.service) |
| 构建与证据 | [构建说明](docs/build-display.md) · [镜像来源](docs/provenance.md) · [启动验证摘录](evidence/verified-boot.txt) |
| 面板、DSI 主机、ENCL 诊断 | [实验驱动](drivers/c10h-display/) · [设备树实验](dts/) |
| 图层测试 | [DRM 几何图探针](experiments/display/drm-geometry.c) |
| 启动工具 | [脚本示例](boot/aml_autoscript.cmd) · [U-Boot 脚本生成器](tools/make_boot_script.py) · [禁用 eMMC 的 DTB 工具](tools/disable_emmc.py) |
| 启动记录服务 | [记录脚本](scripts/c10h-boot-proof.sh) · [systemd 单元](systemd/c10h-boot-proof.service) |

内核模块需要针对实际运行内核使用匹配的 headers、配置与符号版本构建。DRM 几何图探针使用 libdrm，仅在已有活动的 800×1280 DSI 模式下提交主图层。参见[构建说明](docs/build-display.md)。

## 上游、参考与致谢

- [ophub/amlogic-s9xxx-armbian](https://github.com/ophub/amlogic-s9xxx-armbian)
- [ophub/fnnas](https://github.com/ophub/fnnas)
- [Calmact/e900v22c](https://github.com/Calmact/e900v22c)
- [C10H 原厂 Android 修改记录](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod)
- [zanjie1999/rk3566-guomeiyun-armbian](https://github.com/zanjie1999/rk3566-guomeiyun-armbian)：特别感谢提供在 U-Boot 中引入开机 Logo 初始化显示通路的宝贵思路，帮助突破初期半屏紫块与折叠问题。

维护者为 Arrow36。上游镜像、内核、设备树和固件分别遵循其许可。本仓库尚未指定统一许可证，实验驱动及上游派生文件保留原有 SPDX 和版权声明。公开资料以技术参数、源码和脱敏实验记录为主。
