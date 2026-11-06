# U 盘启动与测试边界

## 已测试路径

原厂 Android 中执行 `adb reboot update`，原厂 U-Boot 的 update 路径最终可加载 USB 上的 `aml_autoscript`，由脚本执行 `booti`。

**这条入口包含原厂 USB/SD 烧录和恢复逻辑，不能理解为所有设备都无副作用的通用 U 盘启动命令。** 本机在用户知悉风险后执行并成功启动过 Linux。只针对这台已测试设备记录，不建议其他版本直接照搬。

正常 bootcmd 仍以内部 Android 为先。此项目没有配置每次上电自动从 USB 启动。

## 文件与构建

基线 DTB 必须与所用内核匹配。下面命令只在电脑上生成文件，不会写入物理磁盘：

```sh
python3 tools/disable_emmc.py meson-g12a-s905l3a-e900v22c.dtb meson-g12a-s905l3a-c10h-usb-test.dtb
python3 tools/make_boot_script.py boot/aml_autoscript.cmd aml_autoscript
```

已测试的 BOOT 分区文件布局：

```text
/aml_autoscript
/s905_autoscript            # 与 aml_autoscript 相同的测试容器
/zImage                    # 实际为 ARM64 Image，保留上游文件名
/uInitrd
/dtb/amlogic/meson-g12a-s905l3a-c10h-usb-test.dtb
```

启动脚本中的根 UUID 来自该上游镜像，换镜像必须改为实际根 UUID。它不导入 uEnv.txt；修改 uEnv.txt 不会自动修改这个脚本的内核参数。文本修改后必须重新生成两个二进制容器。

本次介质把 boot.scr、boot-emmc.scr、emmc_autoscript、u-boot.usb、u-boot.sd、boot.ini、boot-emmc.ini 等其他入口改名为 `.unused`；未使用 u-boot.ext 跳转。原版 ophub aml_autoscript 包含修改 bootcmd、saveenv 和 reboot 的逻辑，不能与本仓库脚本混淆。

## 修改与已知问题

- 测试 DTB 仅将 `/soc/mmc@ffe07000` 的 status 改为 disabled。
- 脚本自身不保存 U-Boot 环境，不写 eMMC；Linux DTB 禁用 eMMC 不等于保护原厂 U-Boot 的写入行为。
- 失败返回后停留在循环，避免脚本直接返回后继续原厂恢复路径。
- `systemd.mask=armbian-resize-filesystem.service` 未阻止其他首次启动逻辑扩容；实际根分区和 ext4 已扩到约 29 GiB。
- 启动记录脚本最初因 CRLF 报错，已改为 LF，语法及写后内容已验证；修复后是否执行应继续看实际记录。
- Linux 首次启动后，Android 4.9 无法以读写方式挂载该 ext4，提示 unsupported optional features (10000)；只读挂载可用。未强行修改文件系统特性。

## 回退证据

一次测试中，用户断电、拔 U 盘后正常返回 Android。测试前后 bootcmd 相同，但原厂环境原始哈希并非完全相同：重启原因和 bootargs 中的 reboot_mode_android 字段有变化。

本地已有 boot、dtbo、recovery、vbmeta、bootloader 及测试前 env 的校验备份。它们不等于完整原厂固件；恢复流程和免拆救砖能力尚未实测。备份不在公开仓库中。

## 按系统核对与审计边界

以上布局及 `boot/aml_autoscript.cmd` 对应 Armbian 的 ext4/固定 UUID/HDMI 参数；不是 fnOS 的可直接复用启动脚本。fnOS 历史终端输出为 Btrfs，bootargs 还包含 `rootflags=compress=zstd:1` 和自定义主机的 initcall blacklist，见 [fnOS 完整记录](research/fnos-lcd-full-record-2026-11-04.md)。实际已安装 fnOS DTB、二进制脚本和最终模块不在仓库。

“仅修改 eMMC 节点”只指 [disable_emmc.py](../tools/disable_emmc.py) 的目标，不表示整个 LCD 实验 DTB 只改了这一项。[面板 overlay](../dts/c10h-panel-test.dtso)还启用 PWM/DSI、添加面板及 graph、禁用 HDMI/SD；它本身不禁用 eMMC，也不禁用基础 simplefb，必须核对最终 DTB，不能只检查 overlay 文本。

[脚本文本](../boot/aml_autoscript.cmd)没有保存环境或写入 eMMC 的命令，失败后循环；但本次未取得原厂 U-Boot 完整二进制/源码、全程串口日志或完整恢复备份，不能证明整个 update 路径无写入副作用。现有证据也不支持 SD 启动、eMMC 安装、免拆救砖或所有冷启动自动 USB 引导。详见 [证据缺口](audit.md#仍缺乏证据的结论)。

原版 ophub 脚本行为属于当时镜像的记录，不能推定所有当前上游版本相同。本文的原厂备份是历史设备本地材料，本次未访问这些备份或进行恢复测试。
