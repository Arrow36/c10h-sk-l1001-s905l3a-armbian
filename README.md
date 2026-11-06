# 天翼智屏 C10H / SK-L1001 Linux 适配记录

创维 SK-L1001（中国电信天翼智屏 C10H，Amlogic S905L3A）的外置 Linux 启动、板级适配与显示实验资料。主要开发基线为 Armbian，同一台设备也测试了社区 ARM fnOS。

**项目处于实验阶段。** 已有 U 盘启动、有线调试和内屏控制台的阶段性结果；尚未完成整机功能、长期运行及完整恢复流程验收。本仓库提供源码、配置示例和实验记录，没有可直接安装的完整系统镜像。

状态整理日期：2026-11-06（Asia/Shanghai），并按设备使用者补充确认修正系统范围：**fnOS 内屏已实测显示正常，画面完整、颜色正常、无可见抖动；Armbian 内屏尚未测试。** 以下验证结论仅对应指定设备、系统与配置。

## 1. 当前进展与限制

| 范围 | 已有记录 | 验证边界 |
| --- | --- | --- |
| Armbian | U 盘启动、HDMI 控制台、USB 集线器、外接键盘、有线 SSH；有过 DNS/HTTPS 成功记录 | 自动联网曾间歇失败；长期读写和断电恢复待测 |
| 社区 ARM fnOS | U 盘启动、有线 SSH、Web 管理页、HDMI 控制台 | Web 可访问不等于存储管理、应用与 NAS 工作负载已验收 |
| fnOS 内屏与背光 | 已实测显示正常：画面完整、颜色正常、无可见抖动 | 结论限本机 fnOS 配置；长期运行、量化色彩与其他设备兼容性另行验证 |
| Armbian 内屏 | 尚未测试 | 不引用 fnOS 显示结果作为 Armbian 的验证结论 |
| 横屏终端 | fnOS 已实现 1280×800 横屏控制台；历史资料另提及 FbTerm 路径 | 面板仍按 800×1280 扫描；历史方案不作为 Armbian 内屏已测试的依据 |
| 原厂 Android | 保留在内置 eMMC；曾断电拔盘返回 Android | Linux DTB 禁用 eMMC 仅在内核接管后生效，完整恢复流程未验收 |
| 其他外设 | GPU 已注册节点，部分配置注册过声卡 | GPU 实际加速、触摸、Wi-Fi、音频、蓝牙、摄像头均未完成可用性验证 |

硬件规格和逐项状态见 [硬件支持表](docs/hardware.md)。Linux 的 `SKYWORTH E900V22C` 型号字符串来自借用的设备树，实物仍是 SK-L1001。

## 2. 阅读顺序

| 目的 | 入口 |
| --- | --- |
| 了解文档层次与当前状态来源 | [文档导航](docs/README.md) |
| 评估能否用于自己的设备 | [硬件支持表](docs/hardware.md) → [启动边界与回退](docs/boot.md) |
| 准备外置系统实验 | [镜像来源](docs/provenance.md) → [启动记录](docs/boot.md) → [有线网络](docs/network.md) |
| 研究内屏适配 | [显示概览](docs/display.md) → [11 月 6 日寄存器调整记录](docs/research/fnos-display-resolution-record-2026-11-06.md) → [构建说明](docs/build-display.md) |
| 追溯早期故障与尝试 | [历史实验索引](docs/README.md#历史实验与研究归档) |
| 继续验证与补充证据 | [验证计划](docs/validation.md) |

## 3. 测试环境


| 项目 | Armbian | fnOS |
| --- | --- | --- |
| 基础镜像 | ophub Armbian 26.11.0 S905L3A Server | ophub 社区 ARM fnOS 1.1.20 S905L3A |
| 用户空间 | Debian 13 Trixie，ARM64 | Debian 12，ARM64 |
| 运行内核 | `6.12.114-ophub` | `6.12.41-trim #219` |
| 根文件系统 | U 盘 `/dev/sda2`，ext4 | U 盘 `/dev/sda2`，Btrfs |
| 板级基线 | 内核配套 E900V22C DTB，加入 C10H 实验修改 | 内核配套 E900V22C DTB，加入 C10H 实验修改 |
| HDMI 与 LCD | HDMI 控制台已测试；内屏尚未测试 | 初次启动使用 HDMI，LCD 实验中禁用 HDMI，由 DSI 独占 CRTC |
| LCD 普通画面 | 尚未测试 | 已实测显示正常：画面完整、颜色正常、无可见抖动 |
| LCD 硬件彩条 | 尚未测试 | 当次观察完整，无可见重复或错位 |

Armbian 镜像文件名和校验值见 [镜像来源](docs/provenance.md)。fnOS 实验使用 `fnnas_amlogic_s905l3a_k6.12.41_2026.09.23.img.gz`，压缩包 SHA-256 为 `94652b7556fa8c1bd78298003fd6cf0db31d03fa8ecd681e997b74c77b367a87`，下载后已核对。写盘前替换了测试 DTB 与一次性启动脚本。验证范围限于外置试启动，仓库没有完整预制系统镜像。 [fnOS 发布页](https://github.com/ophub/fnnas/releases/tag/fnnas_amlogic_693)

## 4. 外置启动的基本顺序

这是准备顺序，具体文件与适用条件以 [启动文档](docs/boot.md) 为准。

1. 核对镜像来源和 SHA-256，确认目标 U 盘可清空。整盘写入会覆盖原分区及数据。
2. 以目标内核配套 DTB 为基线生成禁用 eMMC 的测试副本；内屏实验还需面板、背光、DSI 与显示路由配置。
3. 核对启动脚本中的根 UUID、文件系统、内核、initramfs 和 DTB 路径，再生成配套的 `aml_autoscript`、`s905_autoscript`。当前脚本不导入 `uEnv.txt`。
4. 先确认 HDMI 与有线 SSH，再尝试 LCD 独占。保存可用的 HDMI DTB 和启动脚本，以便离线恢复 U 盘启动分区。
5. 本机曾经由 Android 的 `adb reboot update` 进入原厂 U-Boot 更新路径并启动 U 盘。该路径包含烧录/恢复逻辑，不能据此推定其他设备也可直接使用。

仓库脚本不含 `saveenv` 或 eMMC 写入命令；这不能约束 Linux 启动前的原厂 U-Boot 行为。本机曾拔盘回到 Android，但已有分区备份不等于完整固件恢复能力。

## 5. 内屏适配摘要

面板为 JD9366TC，四通道 MIPI DSI、RGB888、原生 800×1280。以下调整已在 fnOS 上实测获得正常显示；Armbian 内屏尚未测试：

| 现象 | 相关调整 | 结论边界 |
| --- | --- | --- |
| 固定紫色区、画面折叠/重复 | `VPU[0x1DF1] = 0x05000320`，对应 800×1280 输入尺寸 | 支持排查输入尺寸与启动遗留状态，冷启动与重设模式仍需复测 |
| 黑偏绿、白偏紫 | `VPU[0x38A0] = 0x0001C040` | 记录反馈颜色恢复正常；未进行色度计或色差测量 |
| 高频垂直抽动 | 联合调整 FIFO、保持行数与 DSI 配置 | 当次反馈抽动消失；各项贡献及欠载机制尚无完整单变量证据 |
| 横屏控制台 | fnOS 使用 fbcon 旋转 | Armbian 内屏未测；历史 FbTerm 配置仅作参考 |

当前源码同时包含面板 `prepare()` 中的 MMIO 写入和独立 `/dev/mem` 工具。两者的时序、适用性及服务状态见 [寄存器调整记录](docs/research/fnos-display-resolution-record-2026-11-06.md)。不要把服务执行成功直接当作屏幕验收通过。

## 6. 仓库结构

```text
boot/                    U-Boot 脚本及实验模板
docs/                    专题说明、当前状态与验证计划
docs/research/           按日期保存的研究过程
drivers/c10h-display/    实验面板、DSI 主机及 ENCL 诊断模块
dts/                     面板与 PCIe/Wi-Fi 实验 overlay
evidence/                脱敏启动与显示日志摘录
experiments/display/     DRM 几何图、保留 framebuffer 等探针
scripts/                 启动取证与彩条脚本
systemd/                 配套服务示例
tools/                   DTB、启动容器生成工具及 MMIO 程序
```

`dts/c10h-wifi-pcie-test.dtso` 归档的是未验证成功的实验方向。`boot/*.cmd.example` 和 systemd 服务中的路径、参数均需对照实际环境。内核模块需要匹配目标内核的 headers、配置和符号版本；仓库没有 DKMS 或一键安装方案。

## 7. 上游、参考与许可


- [ophub/amlogic-s9xxx-armbian](https://github.com/ophub/amlogic-s9xxx-armbian)
- [ophub/fnnas](https://github.com/ophub/fnnas)
- [Calmact/e900v22c](https://github.com/Calmact/e900v22c)
- [C10H 原厂 Android 修改记录](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod)
- [zanjie1999/rk3566-guomeiyun-armbian](https://github.com/zanjie1999/rk3566-guomeiyun-armbian)：提供了通过 U-Boot 开机 Logo 初始化显示通路的参考思路。

维护者为 Arrow36。上游镜像、内核、设备树和固件分别遵循其许可。本仓库尚未指定统一许可证，实验驱动及上游派生文件保留原有 SPDX 和版权声明。公开资料以技术参数、源码和脱敏实验记录为主。
