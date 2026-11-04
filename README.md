# C10H / SK-L1001 / S905L3A Armbian

天翼智屏 C10H（创维 SK-L1001，Amlogic S905L3A）的 Armbian 移植记录。

**已验证：U 盘启动 ARM64 Linux、HDMI、有线 SSH；内置 LCD 已能由原生 DRM 输出完整几何图，硬件彩条稳定。普通图像仍抖动且颜色异常，屏幕适配尚未完成；Wi-Fi、触摸等仍未完成。**

2026-11-04：U 盘启动后可自动进入 20 像素横屏登录。偏色和抖动仍在，已完成原厂 Android 硬件纯色、亮屏命令和关键寄存器对照。见[后续实测](docs/display-followup.md)和[屏幕适配记录](docs/display.md)。

当前借用 E900V22C 设备树，只做了外置试启动所需的修改，还不是完整的 C10H 主板支持。

## 当前系统

| 项目 | 已测试配置 |
| --- | --- |
| 基础镜像 | ophub Armbian 26.11.0，S905L3A server |
| 用户空间 | Debian 13 Trixie，ARM64 / aarch64 |
| 内核 | 6.12.114-ophub |
| 根文件系统 | U 盘 `/dev/sda2`，ext4 |
| 设备树基线 | `meson-g12a-s905l3a-e900v22c.dtb` |
| 内置 eMMC | 在测试 DTB 中主动禁用 |
| 测试日期 | 2026-11-02 至 2026-11-03（Asia/Shanghai） |

## 已完成的验证

- 原厂 Android root ADB 可用；启动相关分区已在本地备份并校验。
- 用一次性 `aml_autoscript` 从 U 盘加载内核、initramfs 和 DTB；脚本没有 `saveenv` 或 eMMC 写入命令。
- Linux 进入 systemd 多用户系统，SSH 端口可用，并已成功登录。
- HDMI 外接显示器有控制台输出。
- 内置屏：原生 DRM 的硬件彩条完整、稳定；800×1280 线性 dumb buffer 能输出完整边框、十字与白块，但仍有抖动和颜色错误。
- USB 存储、USB 集线器、外接键盘、有线 Ethernet 已验证。
- 电脑 Wi-Fi → Windows Internet Connection Sharing → USB 网卡 → C10H 网口，已验证 DNS 与 HTTPS。
- 一次试验中断电、拔 U 盘后成功返回 Android。此结果不构成完整救砖验证。

## 仓库内容

- [硬件支持状态](docs/hardware.md)
- [启动过程与限制](docs/boot.md)
- [网络配置](docs/network.md)
- [屏幕适配、源码及待解决问题](docs/display.md)
- [暂停时检查点与恢复方法](docs/checkpoint-2026-11-03.md)
- [Wi-Fi 实验结果](docs/wifi.md)
- [镜像来源与校验记录](docs/provenance.md)
- [脱敏验证摘录](evidence/verified-boot.txt)
- `boot/aml_autoscript.cmd`：本次验证使用的启动脚本文本。
- `tools/make_boot_script.py`：生成带 CRC 的 U-Boot legacy script 容器。
- `tools/disable_emmc.py`：对基线 DTB 中指定 eMMC 节点做最小修改。
- `scripts/c10h-boot-proof.sh`、`systemd/c10h-boot-proof.service`：启动记录服务。

本仓库不包含完整系统镜像、原厂分区备份、私钥、登录密码或个人设备环境转储。

## 下一阶段

1. 对照原厂设备树，制作 C10H 专用 DTS；保留已有有线 SSH 调试路径。
2. 先解决原生 DRM 普通图像的抖动与颜色，再验收移动图像、方向、控制台及断电重启。
3. 内置触摸尚未适配。
4. 启用正确 PCIe 配置，再处理原厂 W2 Wi-Fi 的驱动和固件。
5. 验证扬声器、麦克风、蓝牙、摄像头与 GPU 加速。
6. 外置系统稳定后再单独评估 eMMC 访问和安装。

## 上游项目

- [ophub/amlogic-s9xxx-armbian](https://github.com/ophub/amlogic-s9xxx-armbian)
- [Calmact/e900v22c](https://github.com/Calmact/e900v22c)
- [C10H 原厂 Android 修改记录](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod)

维护者：Arrow36。上游镜像、内核、设备树和固件分别遵循其自身许可；本项目尚未指定统一许可证；本次新增内核驱动及上游派生文件保留各自的 SPDX、版权和许可声明，见屏幕源码说明。
