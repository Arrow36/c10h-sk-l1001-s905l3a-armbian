# 硬件支持状态

状态日期：2026-11-06；依据仓库实验记录整理，未新增实机验收。

“驱动已加载 / 设备已注册”不代表设备的所有功能已经实测。当前 Linux 的 model 字符串仍为 SKYWORTH E900V22C，这是借用的 DTB 名称，不是实物型号。

| 设备 | 现状与证据 | 后续工作 |
| --- | --- | --- |
| CPU / 内存 | ARM64 Linux 已运行；约 1.89 GiB 可用内存 | 稳定性与功耗测试 |
| USB / 存储 | U 盘根文件系统、集线器、外接键盘已使用 | 长时读写与断电恢复测试 |
| Ethernet | eth0，100 Mbps 链路、SSH 和互联网已验证 | 保持可用的调试连接 |
| HDMI | 外接显示器可见控制台；meson DRM 已注册 | 分辨率、色彩、热插拔；日志仍有显示模式警告 |
| GPU | panfrost 已加载，renderD128 已注册 | 未验证实际渲染加速 |
| 内置 LCD / 背光 | JD9366TC；fnOS 内屏已实测显示正常，画面完整、颜色正常、无可见抖动；Armbian 内屏尚未测试 | Armbian 需首次实测；fnOS 长期运行、模式重设及量化色彩另行验证 |
| 内置触摸 | Linux input 列表未见触摸设备 | 原厂 DT 包含 Goodix/FocalTech 等候选节点，必须确认实际启用的芯片 |
| Wi-Fi | Linux lspci 未枚举设备；现 DTB 仍有 SDIO wifi32k deferred probe | 原厂是 PCIe W2 / w2_comm；先适配 PCIe、供电和复位，再移植驱动与固件 |
| 音频 | 不同测试 DTB 下曾注册声卡，也出现 axg-sound-card 无法解析 DAI、依赖未就绪 | 尚未验证扬声器、功放和麦克风，不能标记为可用 |
| 蓝牙 / 摄像头 | 未完成识别和验证 | 对照原厂 DT、驱动和固件 |
| 温度 | 已注册 thermal zones | 进一步核对传感器和温控 |
| eMMC | 测试 DTB 主动禁用 | 不应解释成内核不支持 eMMC |

原厂 Android 4.9 的 32 位 `.ko` 不能直接加载到当前 6.12 ARM64 内核。应逐项核对主线驱动、设备树绑定、固件及厂商驱动源码兼容性。

## 设备识别与标称规格

| 项目 | 记录 |
| --- | --- |
| 整机/平台 | 中国电信天翼智屏 C10H，创维 SK-L1001；原厂平台 `p291_iptv` |
| SoC | Amlogic S905L3A；实验 Linux 为 AArch64 |
| 内存 | 标称 2 GB；测试 Linux 约 1.89 GiB 可用 |
| 内置存储 | 标称 64 GB eMMC 5.0；Android 侧识别约 58.2 GiB |
| 原厂系统 | Android 9、Linux 4.9.113、ARM32 用户空间 |
| 内屏 | 10.1 英寸 IPS，JD9366TC；`C_JD9366TC_Single_CSOT_10.1_BPA107LT1-1`，原厂选择 `lcd_8` |
| 背光 | PWM_C / GPIOC_4，25 kHz、正常极性 |
| 音频资料 | 原厂样机资料记载双扬声器、CSK6012 阵列麦克风；Linux 可用性未验收 |

标称规格来自 [原厂 Android 样机记录](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod)。面板参数见 [显示概览](display.md)，系统版本见 [项目首页](../README.md)。
