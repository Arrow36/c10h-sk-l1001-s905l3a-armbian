# 天翼智屏 C10H / SK-L1001 Linux 移植

创维 SK-L1001（天翼智屏 C10H，Amlogic S905L3A）的外置 Linux 启动和板级适配记录。主要开发基线是 Armbian，另在同一台设备上验证了社区 ARM fnOS。记录截至 2026-11-04，以下结果对应测试样机和所列内核。

- **Armbian**：U 盘启动、HDMI 控制台、USB 外设和有线 SSH 已验证。内屏能显示完整静态几何图，并在暖启动后自动进入横屏登录，普通画面仍偏色、抖动。
- **fnOS**：U 盘启动、有线 SSH、Web 管理页均已验证。HDMI 控制台可用。切换到内屏独占模式后，LCD 开机自动点亮，硬件彩条完整，普通画面仍有固定紫色区域、重复和闪动。
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
| LCD 普通画面 | 线性缓冲区的边框、十字与白块完整，仍偏色和抖动 | 线性缓冲区提交成功，左侧固定色块、右侧图形重复和闪动 |
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

## 内屏实验与排查进度

Armbian 原生 DRM 已完成面板初始化、ENCL 彩条和静态几何图测试。暖启动后可自动进入 20 像素横屏登录，仍需验收断电冷启动、普通图像色彩和抖动。原厂 Android 和 Linux 的硬件纯色、亮屏命令与关键寄存器已有对照。见[后续实测](docs/display-followup.md)。

fnOS 对照使用 DRM XR24 线性缓冲区（800×1280、pitch 3200）、fb0 全白、OSD1 短时强制白色和 ENCL 彩条。fb0 全白与 OSD1 强制白色都只使右侧变白，ENCL 彩条却完整。CPU 读回 fb0 基本全白，Canvas 地址与行跨度也和 DRM 配置相符。当前优先核对 OSD1 实际覆盖、Blend/VPP 与 ENCL 普通像素输入。DSI 错误状态抽查为零，Gamma 就绪超时仍存在，因果关系待验证。

浏览器滑杆通过 SSH 隧道改变测试图，测出可响应区域的 RGB 循环映射。逆置换可补偿测试图颜色，固定紫色区仍在。独立只读监测采集了 186 行状态，覆盖 138.449 秒。ENCL 状态持续变化，OSD/VPP 当前坐标读数固定，FIFO 慢速读数曾短暂变化，尚不足以判定欠载。见[飞牛实验](docs/display-fnos.md)与[浏览器诊断](docs/display-browser-diagnostics.md)。

下一轮先确认两套内核实际使用的模块来源和构建配置，再用同一输出模式、同一图案做对照。用非等距横带和竖带区分行重复、列重复和固定边界。普通画面通过几何、颜色和稳定性检查后，再验收移动画面、冷启动和触摸。无线方向先解决 PCIe 枚举、供电、复位及共享 PHY。

## 源码与文档索引

| 内容 | 入口 |
| --- | --- |
| 板级支持、启动、网络 | [硬件状态](docs/hardware.md) · [启动与回退](docs/boot.md) · [有线网络](docs/network.md) · [Wi-Fi](docs/wifi.md) |
| Armbian 显示 | [面板时序与实验](docs/display.md) · [后续实测](docs/display-followup.md) · [实验检查点](docs/checkpoint-2026-11-03.md) |
| fnOS 与浏览器工具 | [fnOS 实验](docs/display-fnos.md) · [浏览器诊断方法](docs/display-browser-diagnostics.md) |
| 构建与证据 | [构建说明](docs/build-display.md) · [镜像来源](docs/provenance.md) · [启动验证摘录](evidence/verified-boot.txt) |
| 面板、DSI 主机、ENCL 诊断 | [实验驱动](drivers/c10h-display/) · [设备树实验](dts/) |
| 图层测试 | [DRM 几何图探针](experiments/display/drm-geometry.c) |
| 启动工具 | [脚本示例](boot/aml_autoscript.cmd) · [U-Boot 脚本生成器](tools/make_boot_script.py) · [禁用 eMMC 的 DTB 工具](tools/disable_emmc.py) |
| 启动记录服务 | [记录脚本](scripts/c10h-boot-proof.sh) · [systemd 单元](systemd/c10h-boot-proof.service) |

内核模块需要针对实际运行内核使用匹配的 headers、配置与符号版本构建。DRM 几何图探针使用 libdrm，仅在已有活动的 800×1280 DSI 模式下提交主图层。参见[构建说明](docs/build-display.md)。

## 上游与许可

- [ophub/amlogic-s9xxx-armbian](https://github.com/ophub/amlogic-s9xxx-armbian)
- [ophub/fnnas](https://github.com/ophub/fnnas)
- [Calmact/e900v22c](https://github.com/Calmact/e900v22c)
- [C10H 原厂 Android 修改记录](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod)

维护者为 Arrow36。上游镜像、内核、设备树和固件分别遵循其许可。本仓库尚未指定统一许可证，实验驱动及上游派生文件保留原有 SPDX 和版权声明。公开资料以技术参数、源码和脱敏实验记录为主。
