# 创维 C10H / SK-L1001：fnOS 内置 LCD 适配完整记录（2026-11-04 更新版）

> 公开脱敏版。保留原章节、实验步骤和附录，登录账户、网络地址、用户目录及运行实例标识已替换。文中提及的原始附件仅作为历史材料引用。

> **整理日期**：2026-11-04；**最新补充**：本次测试截至全帧纯白检查结果<br>
> **设备**：Skyworth C10H / SK-L1001，Amlogic S905L3A<br>
> **系统**：非官方 ARM 版 fnOS 1.1.20（Debian 12，Linux `6.12.41-trim`，aarch64）<br>
> **参考项目**：[Arrow36 / c10h-sk-l1001-s905l3a-armbian](https://github.com/Arrow36/c10h-sk-l1001-s905l3a-armbian)<br>
> **文档性质**：基于本次对话中提供的命令、结果、屏幕照片、用户上传的 SSH 终端记录和前期对话要点做的**结构化整理**；不是逐字不漏的平台聊天记录。终端日志按原文附在末尾，照片收录于配套 ZIP。对 GitHub 研究结果的描述以**本次对话曾引用的资料**为准，不代表重新在线审计最新代码。

---

## 一、最新结论（含后续会话的新实验）

**已经完成（截止本轮）**：C10H 内屏自动点亮、800×1280@60Hz 模式、DSI Host / JD9366TC 模块绑定；DRM plane 提交成功。Canvas 读取配置与 DRM 报告的显存 DMA 地址、行跨度、高度一致。OSD/Blend 窗口与 800×1280 一致。**ENCL 硬件彩条完整且无重复**。此外，新增 Python framebuffer 滑杆工具完成了 RGB 通道循环补偿测试：标准颜色出现红→绿、绿→蓝、蓝→红；软件重排后，测试色恢复正确。

**仍未解决**：正常 framebuffer 图像严重分区；用户最近实拍明确指出**左侧大片紫色长期不变，右侧重复和闪动**。在“全屏纯白”实验中，左侧依旧紫色、右侧白色并有少量像素闪动。读取 `/dev/fb0` 共 4,096,000 字节，其中原脚本计得 1,023,872 个白色四字节序列、128 个非白色单位，首个异常位置 `x=96,y=288`。这强烈支持 CPU 可读取的 framebuffer 基本正确，但**不证明显示 DMA、缓存同步、OSD/VPP 或 DSI 传输正确**。

**重要限制**：上述数字来自原测试脚本的 `data.count(b'\xff\xff\xff\x00')` 计算（未显式逐像素步进），应将“128 个非白像素”视作**该脚本的计算结果**，需要时再用按 `i+=4` 的严格逐像素计数复核。此前多轮输出未找到可直接修改的确定性故障寄存器。**不能宣称已经修好屏幕**。

**当前待办（尚未执行）**：先确认测试进程退出、fb0 恢复，再决定是否只读检查 `OSD1_FIFO_CTRL_STAT` 对应的候选寄存器 `0x1A2B`（此前助手建议的物理地址 `0xFF9068AC`，需先核对准确寄存器定义），或转向内核 DMA/OSD 代码审查。**不要不经核实就直接写寄存器。**

> 阅读建议：如只需了解最新状态，请直接阅读 **第十二章「后续会话与最新实验」** 及最后的「可复制接续摘要」。第 2–11 章及附录 A 完整保留前一轮整理及当时的终端原始日志；其“最新”“下一步”字样代表前一轮时间点，不应覆盖本章的最新结论。

### 进展矩阵

| 功能/目标 | 已观测结果 | 结论等级 |
|---|---|---|
| USB 启动 fnOS | 能登录系统并 SSH | 已验证 |
| 加载自制 DSI host、JD9366TC panel 模块 | `lsmod`、sysfs 驱动绑定成功 | 已验证 |
| LCD 显示模式 | `800×1280@60 Hz`，76.107 MHz | 已验证 |
| HDMI 与 DSI 同时正常显示 | `possible clones` 不兼容，只有一条 CRTC | 当前未实现 |
| 禁用 HDMI，内屏自动获得 CRTC | `DSI-1 enabled`，DRM `active=1` | 已验证 |
| LCD 输出真实像素内容 | 控制台及测试图均产生画面 | 已验证但异常 |
| 完整、正确的图像 | 最新照片左侧大片紫色、右侧重复闪动；全白测试左侧不响应 | 未解决 |
| ENCL 硬件彩条 | 完整、无错位、无重复 | 已验证 |
| RGB 软件通道补偿 | 三通道循环反向排列后，测试色准确 | 已验证（仅测试图） |
| framebuffer 全白读取 | 4,096,000 字节；约 99.9875% 白色四字节序列 | 已观测，计数口径待严格复核 |
| 恢复到 HDMI 模式 | 有已保存的设备树及 U-Boot 脚本备份 | 尚未在文档整理后再次验证 |

---

## 二、硬件、操作系统与启动方式

### 2.1 硬件信息

| 项目 | 信息 |
|---|---|
| 主板/整机 | 创维 C10H / SK-L1001 |
| SoC | Amlogic S905L3A |
| 内屏面板 | `C_JD9366TC_Single_CSOT_10.1_BPA107LT1-1` |
| 内屏规格 | 10.1 英寸，800×1280，MIPI DSI 4 lanes，RGB888 |
| 内屏时序 | Pixel clock ≈ 76.107 MHz；`htotal=876`，`vtotal=1448` |
| 背光 | `pwm-backlight`，PWM_C / GPIOC_4（依据项目设备树资料） |
| 输出框架 | Linux DRM / Meson VPU，单 CRTC；HDMI 与 DSI 不具备当前驱动声明的兼容克隆能力 |

### 2.2 系统信息与磁盘

```text
Linux c10h 6.12.41-trim #219 SMP PREEMPT Thu Feb 12 01:38:06 UTC 2026 aarch64
fnOS: 1.1.20（非官方 ARM 移植）
Debian: 12
/boot: /dev/sda1, FAT32
/:     /dev/sda2, Btrfs
root UUID: ROOT_FS_UUID
```

- Android 保留在设备内置 eMMC；Linux 从 USB 盘运行。
- 当时 SSH：`ssh test-user@C10H_IP`（Windows PowerShell）。该 IP 为实验网络地址，今后可能改变。
- 一般重启可能进入 Android；此前使用 `adb connect ANDROID_IP:5555`，然后 `adb -s ANDROID_IP:5555 reboot update` 再次从 USB 引导。
- 系统曾报告 `/home/test-user` 不存在，仅是 SSH 登录时无法切换用户主目录；并非 LCD 故障。
- 部分 `journalctl` 行显示 `Jun 26`，与当次操作 `Nov 04` 不完全一致，做时间线判断应优先使用终端实际操作和 `dmesg` 相对启动时间，不直接据此推导异常重启日期。

### 2.3 本次任务目标的变化

1. **初始目标**：尝试启用内屏，同时尽可能保留 HDMI 正常显示。
2. **中期结论**：单 CRTC 且编码器克隆能力不足，无法在当前实现下直接保证 HDMI 与 DSI 同时显示。
3. **用户后来明确同意**：**可以关闭 HDMI**，因此改为内屏独占优先。
4. **最新目标**：不再重复 GitHub 已做的实验，直接对齐其最新已公开研究结果，在 fnOS `6.12.41-trim` 环境中修复图像错位/偏色。

---

## 三、使用的上游源码与模块

GitHub 仓库：https://github.com/Arrow36/c10h-sk-l1001-s905l3a-armbian

相关路径（依据本次对话）：

```text
drivers/c10h-display/c10h-dsi-host.c
drivers/c10h-display/panel-c10h-jd9366tc.c
drivers/c10h-display/c10h-lcd-diagnostic.c
dts/c10h-panel-test.dtso
experiments/display/drm-geometry.c
docs/display-followup.md
```

已在 `6.12.41-trim` 编译出以下模块，且 vermagic 匹配：

```text
c10h-dsi-host.ko
panel-c10h-jd9366tc.ko
c10h-lcd-diagnostic.ko
vermagic=6.12.41-trim SMP preempt mod_unload aarch64
```

相关内核配置已检查：`CONFIG_DRM_MIPI_DSI=y`、`CONFIG_DRM_DW_MIPI_DSI=y`、`CONFIG_DRM_MESON_DW_MIPI_DSI=y`、`CONFIG_DRM_MESON=y`、`CONFIG_PWM_MESON=y`、`CONFIG_BACKLIGHT_PWM=m`；构建目录 `/usr/src/linux-headers-6.12.41-trim`。

**当前实际使用的是适配目标内核后编译的 `.ko`，不是将 GitHub 为另一内核构建的二进制模块直接复制过来。** 仓库实验环境在此前对话中记为 Debian 13 / `6.12.114-ophub`，与当前 fnOS 不同；这些差异需要在排查 VPU/DRM 寄存器时保留。

---

## 四、设备树、内核模块和启动脚本：已完成的修改

### 4.1 设备树改动

基于原 e900v22c/USB 测试设备树和仓库的 `c10h-panel-test.dtso`，用 `dtc` + `fdtoverlay` 合并过实验 DTB。

早期测试版本：

```text
/boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-test.dtb
```

在保留 HDMI 的基础上，另外准备了 eMMC 禁用版：

```text
/boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb
```

前一阶段验证内容：

| 节点/功能 | 状态 |
|---|---|
| `/soc/bus@ffd00000/dsi@7000` | `okay` |
| DSI 面板 | `arrow36,c10h-jd9366tc` |
| `backlight-c10h` | `pwm-backlight` |
| HDMI | 早期 `okay`，后改为 `disabled` |
| SD 控制器 `/soc/mmc@ffe05000` | `disabled`（避免相关 GPIO/PWM 冲突） |
| eMMC `/soc/mmc@ffe07000` | 测试 DTB 中 `disabled`（保留 Android 安全性） |

**关键结果**：用户允许关闭 HDMI 后，修改了当前 USB 引导引用的 `meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb`，把 HDMI 设备树节点改为 `status = "disabled"`。重启后内屏自动启用。

当时使用的修改命令（**仅作为执行历史，请勿不检查路径就重复执行**）：

```bash
DTB=/boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb
BACKUP=/boot/c10h-lcd-stage/lcd-hdmi-working.dtb
TEST=/tmp/c10h-lcd-only.dtb
sudo cp "$DTB" "$BACKUP"
cp "$DTB" "$TEST"
fdtput -t s "$TEST" /soc/bus@ff600000/hdmi-tx@0 status disabled
fdtget -t s "$TEST" /soc/bus@ff600000/hdmi-tx@0 status
sudo install -m 0644 "$TEST" "$DTB"
sync
```

### 4.2 模块安装与自动加载

编译目录当时为：

```text
/tmp/c10h-lcd.0AY24B/repo/drivers/c10h-display/
```

编译方式：

```bash
make -j1 -C /lib/modules/6.12.41-trim/build \
  M=/tmp/c10h-lcd.0AY24B/repo/drivers/c10h-display modules
```

已安装两个主要模块至：

```text
/lib/modules/6.12.41-trim/extra/c10h-dsi-host.ko
/lib/modules/6.12.41-trim/extra/panel-c10h-jd9366tc.ko
```

并运行 `depmod -a`，在 `/etc/modules-load.d/c10h-lcd-test.conf` 设置自动加载。

```text
c10h-dsi-host
panel-c10h-jd9366tc
```

诊断模块 `c10h-lcd-diagnostic.ko` 存放在 `/boot/c10h-lcd-stage/`，仅用于诊断，不等同于持续启用的显示驱动。

### 4.3 启动脚本（**恢复系统时尤其重要**）

曾确认 `/boot/aml_autoscript` 与 `/boot/s905_autoscript` 是编译后的 U-Boot 脚本，**脚本内硬编码 DTB 路径和 bootargs，单改 `/boot/uEnv.txt` 不能改变本次实际启动行为**。

已重写两个脚本，使其引用以下 DTB：

```text
/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb
```

内核启动参数添加：

```text
initcall_blacklist=meson_dw_mipi_dsi_platform_driver_init
```

目的：屏蔽内核自带 Meson DW MIPI DSI platform driver 的初始化，转由自定义 `c10h_dsi_host` 驱动处理当前硬件；已验证在实际 `/proc/cmdline` 中出现。还保留了旧 HDMI 强制参数 `video=HDMI-A-1:1920x1080@60e`，但 HDMI 在设备树中禁用后，内屏已成功独占 CRTC。**这个旧启动参数值得后续清理，但不能把它当成已验证故障根因。**

### 4.4 已知备份与回退材料

```text
/boot/c10h-lcd-stage/aml_autoscript.original
/boot/c10h-lcd-stage/s905_autoscript.original
/boot/c10h-lcd-stage/uEnv.original.txt
/boot/c10h-lcd-stage/fnEnv.original.txt
/boot/c10h-lcd-stage/lcd-hdmi-working.dtb
```

还备份过初始已知可启动 DTB 和三个 `.ko`，具体文件名应在设备上执行 `ls -lah /boot/c10h-lcd-stage/` 再确认，不假定统一命名。

**回退原则：** 要恢复早期 HDMI 测试状态，可以恢复同名当前 DTB 的 HDMI 备份；若要回到原始 USB 引导，则**必须同时考虑恢复 `aml_autoscript` 和 `s905_autoscript`**，不应只复制 `uEnv.txt`。尽量使用 USB FAT32 `/boot` 分区在另一台电脑离线恢复，先备份当前文件再覆盖。此处记录的是恢复策略，未自动执行任何回退。

---

## 五、分阶段对话和实验过程

### 阶段 1：HDMI 与 LCD 共存测试（前期）

**用户诉求**：尽可能保留 HDMI。初始编译并载入 DSI host 和面板模块后：

```text
panel_c10h_jd9366tc    12288  0
c10h_dsi_host          12288  0
/sys/class/drm: card0-DSI-1、card0-HDMI-A-1、card0-Composite-1
/sys/class/backlight/backlight-c10h
```

面板设备实际绑定：

```text
/sys/bus/mipi-dsi/devices/ffd07000.dsi.0
    -> /sys/bus/mipi-dsi/drivers/panel-c10h-jd9366tc
```

当时 HDMI 正在使用 `meson_crtc`，DSI connector 为 `crtc=(null)`，屏幕黑。日志曾大量报告：

```text
[drm] kms: can't enable cloning when we probably wanted to.
```

**当时判断**：主显示管线只有一个 CRTC、HDMI 已占用，自动克隆未成功；但这并不足以推断屏幕驱动完全未工作。

### 阶段 2：准备 DRM 测试工具，解决软件包版本冲突

直接安装 `libdrm-tests` 失败：

```text
libdrm-tests : Depends: libdrm2 (= 2.4.114-1+b1)
but 2.4.123-1~bpo12+1 is to be installed
```

实际运行库版本：

```text
libdrm2 2.4.123-1~bpo12+1
```

通过下载匹配的 ARM64 Debian `.deb` 并 `dpkg-deb -x` 提取，不安装/降级系统库：

```bash
WORK=/tmp/c10h-modetest
mkdir -p "$WORK"
URL='https://mirrors.edge.kernel.org/debian/pool/main/libd/libdrm/libdrm-tests_2.4.123-1~bpo12%2B1_arm64.deb'
curl -fL --retry 2 "$URL" -o "$WORK/libdrm-tests.deb"
dpkg-deb -x "$WORK/libdrm-tests.deb" "$WORK/extracted"
BIN="$WORK/extracted/usr/bin/modetest"
ldd "$BIN"
```

结果：`libdrm.so.2` 和 `libc.so.6` 均能解析，`modetest` 正常工作。

### 阶段 3：`modetest` 明确发现内屏模式

```text
Connector 38: DSI-1, connected
Mode #0: 800x1280 60.00 Hz
hdisp=800 hss=840 hse=848 htot=876
vdisp=1280 vss=1420 vse=1428 vtot=1448
clock=76107 kHz
flags: nhsync, nvsync; preferred, driver

Encoder 34: HDMI, possible crtcs=0x1, possible clones=0x2
Encoder 37: DSI,  possible crtcs=0x1, possible clones=0x4
```

**结论**：面板模式已经发布。`/sys/class/drm/card0-DSI-1/modes` 之前为空并非永久性“驱动无模式”；该 sysfs 文件在早期状态下的输出不应替代完整 `modetest -c` 查询。

### 阶段 4：第一次手动点亮内屏（HDMI 尚开启）

当时执行过：

```bash
sudo timeout -s TERM -k 3s 12s \
  /tmp/c10h-modetest/extracted/usr/bin/modetest \
  -M meson -s 38@45:800x1280-60
```

输出：

```text
setting mode 800x1280-60.00Hz on connectors 38, crtc 45
```

**实拍 1**：左侧约 2/3 青蓝色，右侧约 1/3 是黑白噪点/竖条，说明 DSI 输出确实被打开，但内容严重异常。

这一回 SSH 会话断开，用户重新 SSH 成功。**不能据此断定内核崩溃**：终端前面曾使用 `set -e`，`timeout` 可能以非零状态结束并退出交互 Shell；只作为可能原因。

点屏后的关键日志：

```text
c10h-dsi-host ffd07000.dsi: C10H DSI requested=456642000 actual=531000000 pixel_target=76107000
c10h-dsi-host ffd07000.dsi: C10H DSI final bit=531000000 px=75857143 configured_hs=531000000
panel-c10h-jd9366tc ffd07000.dsi.0: C10H stock panel initialization complete
meson_encl_set_gamma_table: GAMMA WR_RDY timeout
meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
```

像素时钟偏差计算：`(76.107 - 75.857143)/76.107 ≈ 0.33%`，**偏差存在，但不能只凭它解释右侧重复、色块和噪点**。

### 阶段 5：诊断模块的第一次寄存器快照

`/boot/c10h-lcd-stage/c10h-lcd-diagnostic.ko` 存在、vermagic 匹配；`insmod`/`rmmod` 未出现在普通用户 `PATH`，需要检查 `/usr/sbin`、`/sbin`，而非直接假定未安装。

在 HDMI 恢复输出的状态下，以 `pattern=0` 加载诊断模块得到：

```text
c10h-lcd: ENCL_EN=0 MODE=8000 ADV=418 MAX_PX=875 MAX_LN=1447 GAMMA=31 RGBIN=3
c10h-lcd: HAVON=36..835 VAVON=28..1307
c10h-dsi-reg: PWR=0 COLOR=0 MODE=0 VIDMODE=0 PKTSIZE=0 CHUNKS=0 NULL=0
c10h-dsi-reg: HSA=0 HBP=0 HLINE=0 VSA=0 VBP=0 VFP=0 VACT=0 LANES=0 PHY=0 TOP=0
```

**正确解读**：这是 HDMI 运行、DSI 停用时的快照；DSI 全零与当时的显示状态一致，不能将其认定为“DSI 无法配置”或“面板故障”。诊断模块具有潜在寄存器访问风险，因此之后没有贸然在 HDMI 活跃时开启硬件彩条。

### 阶段 6：用户同意关闭 HDMI，内屏独占启动

用户明确表示“可以关闭 HDMI”。设备树 HDMI 节点改成 `disabled` 后重新从 USB 启动。验证结果：

```text
/sys/class/drm/card0-DSI-1/enabled: enabled
/sys/class/drm/card0-DSI-1/modes: 800x1280
DRM: active=1
DRM mode: "800x1280": 60 76107 800 840 848 876 1280 1420 1428 1448
```

新一次开机自动触发驱动/面板初始化：

```text
[   10.070265] c10h-dsi-host ffd07000.dsi: C10H DSI requested=456642000 actual=531000000 pixel_target=76107000
[   10.070317] c10h-dsi-host ffd07000.dsi: C10H DSI final bit=531000000 px=75857143 configured_hs=531000000
[   10.711909] panel-c10h-jd9366tc ffd07000.dsi.0: C10H stock panel initialization complete
[   10.722321] meson_encl_set_gamma_table: GAMMA WR_RDY timeout
[   13.075272] meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
```

**实拍 2**：开机屏幕出现 fnOS 控制台信息，但左侧大块青蓝色，右侧文字严重条纹/错位，**不是正确显示**。与 HDMI 仍开启阶段相比，已经实现“无需手动 `modetest` 开机启屏”。

### 阶段 7：复用仓库 `drm-geometry.c`，先解决编译依赖

系统有 `gcc`、`pkg-config`，但没有 `libdrm.pc`、`xf86drm.h` 和 `/usr/include/libdrm/drm.h`。直接提取匹配版本开发包：

```bash
WORK=/tmp/c10h-geometry
mkdir -p "$WORK"
curl -fL --retry 2 \
  'https://mirrors.edge.kernel.org/debian/pool/main/libd/libdrm/libdrm-dev_2.4.123-1~bpo12%2B1_arm64.deb' \
  -o "$WORK/libdrm-dev.deb"
dpkg-deb -x "$WORK/libdrm-dev.deb" "$WORK/dev"
curl -fL --retry 2 \
  'https://raw.githubusercontent.com/Arrow36/c10h-sk-l1001-s905l3a-armbian/main/experiments/display/drm-geometry.c' \
  -o "$WORK/drm-geometry.c"
gcc -O2 -std=gnu11 \
  -I"$WORK/dev/usr/include" \
  -I"$WORK/dev/usr/include/libdrm" \
  "$WORK/drm-geometry.c" \
  /lib/aarch64-linux-gnu/libdrm.so.2 \
  -o "$WORK/drm-geometry"
```

已成功生成：

```text
/tmp/c10h-geometry/drm-geometry
ELF 64-bit LSB pie executable, ARM aarch64
size: 72K
libdrm.so.2 => /lib/aarch64-linux-gnu/libdrm.so.2
```

### 阶段 8：运行 20 秒几何图测试（截至第一轮整理时）

测试由 `systemd-run` 作为临时服务启动，避免 SSH 交互环境影响自动结束：

```bash
sudo systemd-run \
  --unit=c10h-geometry-test \
  --collect \
  --property=RuntimeMaxSec=30s \
  --property=TimeoutStopSec=5s \
  /usr/bin/timeout --preserve-status \
  -s TERM -k 5s 20s \
  /tmp/c10h-geometry/drm-geometry
```

输出：

```text
Nov 04 14:05:19 c10h timeout[6030]: card=/dev/dri/card0 connector=35 crtc=42 plane=36 mode=800x1280 clock_khz=76107
Nov 04 14:05:19 c10h timeout[6030]: linear dumb: handle=1 framebuffer=45 pitch=3200 size=4096000 format=XRGB8888 modifier=LINEAR
Nov 04 14:05:19 c10h timeout[6030]: FB_ID: 43 -> 45
Nov 04 14:05:19 c10h timeout[6030]: CRTC_ID: 42 -> 42
Nov 04 14:05:19 c10h timeout[6030]: SRC_X: 0 -> 0
Nov 04 14:05:19 c10h timeout[6030]: SRC_Y: 0 -> 0
Nov 04 14:05:19 c10h timeout[6030]: SRC_W: 52428800 -> 52428800
Nov 04 14:05:19 c10h timeout[6030]: SRC_H: 83886080 -> 83886080
Nov 04 14:05:19 c10h timeout[6030]: CRTC_X: 0 -> 0
Nov 04 14:05:19 c10h timeout[6030]: CRTC_Y: 0 -> 0
Nov 04 14:05:19 c10h timeout[6030]: CRTC_W: 800 -> 800
Nov 04 14:05:19 c10h timeout[6030]: CRTC_H: 1280 -> 1280
Nov 04 14:05:19 c10h timeout[6030]: READY: plane-only commit; geometry border/cross/block at x80 y160. SIGUSR1 moves to x520.
Nov 04 14:05:39 c10h systemd[1]: c10h-geometry-test.service: Deactivated successfully.
```

**重要细节**：前期 HDMI 启用时 `connector=38`、`crtc=45`，此处 HDMI 已禁用后的 DRM 对象 ID **变化为** `connector=35`、`crtc=42`、`plane=36`。这些是每次初始化动态注册的 ID，**不能在跨次开机的自动化脚本里硬编码之前的 ID**。

**实拍 3**：左侧约 2/3 为青蓝色，右侧出现高密度重复白色横线、多个相同白块、被切断或重复的竖线。预期是黑底、单条白色外框与十字、单个白块。由此可确认：**即使使用仓库原版用户态 DRM dumb buffer、XRGB8888/LINEAR、plane-only commit，图像仍有严重的分区与内容重复**。

测试后：

```text
DSI-1: enabled
DRM active=1
DRM mode: 800x1280
Primary Plane: fb=43（从测试 fb=45 恢复）
```

**不要把“DRM commit 成功”误认为“屏幕显示内容正确”。**

### 阶段 9：讨论“为什么不能直接用 GitHub 最新成果”

用户多次明确希望**优先复用最新 GitHub 研究**，避免重复做已完成实验。对话中归纳的情况是：

- 已直接复用面板/DSI 驱动、设备树思路和 `drm-geometry.c`；
- 在此前核对的公开记录中，上游能看到完整边框和十字，但仍有偏色/抖动；用户当前实拍更接近早期“左侧青蓝 + 右侧重复”症状；
- 仓库此前对照过 Android 与 Linux VPP / HDR / OSD 的寄存器，出现值得核查的差异；
- 当前尚未在本次设备上验证这些地址/寄存器的准确映射或某个修复值的效果，**不要照抄寄存器值直接写入**。

此前对话提到的寄存器对照（作为**待验证线索**，不是可直接应用的补丁）：

| 寄存器地址 | 原厂 Android（仓库曾报告） | 实验 Linux（仓库曾报告） |
|---|---|---|
| `0x1D22`（VPP 行缓冲相关） | `0x08080808` | `0x404` |
| `0x1D5F`（VPP 矩阵控制相关） | `0x0` | `0x7800` |
| `0x38A0`（OSD1 HDR 控制相关） | `0x1C040` | `0xC040` |

这些寄存器描述与数值来自**此前对话对 GitHub 文档的概括**，实际使用前须再次对照上游当前代码、SoC 寄存器定义及本内核的适用性。不能断言其中某一项必定导致画面错位。

---

## 六、问题、错误及其处理记录

| 现象 | 已知原因/解释 | 当前处理 |
|---|---|---|
| `/sys/class/drm/DSI-1/modes` 初期为空 | 初期 DSI 未被选择；仅该文件不足以排除面板模式注册 | `modetest -c` 已查到 800×1280 |
| HDMI 正常、LCD 黑屏 | HDMI 使用唯一 CRTC，兼容克隆未实现 | 用户同意禁用 HDMI；LCD 自动接管 |
| `apt install libdrm-tests` 失败 | libdrm2 版本混用，系统选择 backports 2.4.123 | 只提取匹配 `.deb`，未变更运行库 |
| `libdrm.pc`、`xf86drm.h` 不存在 | 缺少 `libdrm-dev` | 匹配开发包下载提取、编译成功 |
| `insmod` / `rmmod` not found | 普通用户 PATH 可能不含 sbin | 需要用完整路径核查；不盲装 |
| DSI 初次点亮后 SSH 中断 | 不能排除命令 `set -e` 遇到 `timeout` 非零退出；也可能有其他原因 | 已重新 SSH；后续测试使用 systemd 单元 |
| `GAMMA WR_RDY/ADR_RDY timeout` | ENCL Gamma 编程未就绪，根因未确认 | 保留证据，不盲改 Gamma |
| `trim_wayland` / `trim_miniscreen` 失败 | `/usr/trim/bin/start_wayland.sh`、`start_miniscreen.sh` 不存在 | 与 DSI 驱动问题分开处理 |
| 重启后 Android 启动 | 该设备 USB 引导方式与 U-Boot/Android update 流程有关 | 通过 ADB reboot update 复现 |
| 当前青蓝+重复图案 | DRM 普通图像处理/时序链路存在问题，具体级段未确定 | 下一步关注 VPU、OSD/Canvas、VPP、ENCL、DSI 差异 |

---

## 七、可复用的只读检查命令

> 以下均用于**查看当前状态**，除特别注明外不会主动切换显示模式。日志的 DRM 对象编号会随启动变化，需以当次输出为准。

```bash
# 1) 内核、启动参数、驱动
uname -a
cat /proc/cmdline
lsmod | grep -Ei 'c10h|jd9366|dsi'

# 2) 内屏是否被启用
cat /sys/class/drm/card0-DSI-1/status
cat /sys/class/drm/card0-DSI-1/enabled
cat /sys/class/drm/card0-DSI-1/modes

# 3) 面板绑定
for d in /sys/bus/mipi-dsi/devices/*; do
  [ -d "$d" ] && echo "$d => $(readlink -f "$d/driver")"
done

# 4) DRM 完整状态
sudo cat /sys/kernel/debug/dri/0/state

# 5) 本机 modetest（若 /tmp 尚存在）
BIN=/tmp/c10h-modetest/extracted/usr/bin/modetest
[ -x "$BIN" ] && sudo "$BIN" -M meson -c
[ -x "$BIN" ] && sudo "$BIN" -M meson -e

# 6) 重要日志（去掉冗余 HDMI 报告）
sudo dmesg | grep -Ei \
  'C10H DSI|C10H stock|jd9366|GAMMA.*timeout|kms: can.t enable cloning|fail|error' \
  | tail -80

# 7) 几何图测试日志
sudo journalctl -u c10h-geometry-test --no-pager -n 60

# 8) 备份检查
ls -lah /boot/c10h-lcd-stage/
```

**注意**：`/tmp` 工作目录可能随重启/清理消失；真正持久的重要文件在 `/boot`、`/lib/modules`、`/etc/modules-load.d`。`/dev/dri/card0`、`connector` 和 `crtc` 序号也不保证永远一致。

---

## 八、前一轮整理时的下一步研究方向（现以第十二章为准）

1. **以当前成功启动的内屏独占设备树和已编译模块为基线**，先完整备份，不重新更换 DTB 或内核。
2. **同步确认 GitHub 当前代码版本/提交 ID**，而不是仅依赖此前聊天引用的“最新实验”表述；特别关注上游从早期“画面分区”发展到“完整几何图”的中间 commit、内核差异与必要补丁。
3. **对照 DRM 图像读取/处理链路**：OSD1 framebuffer、Canvas/stride、VPP line buffer、矩阵/HDR、ENCL active window、DSI packet/timing。优先审查版本差异与代码路径，而非随机写寄存器。
4. 可在经过评估、具备可靠回退和日志保存时做**单变量**实验，区分“普通 framebuffer 读取异常”和“后端扫描/传输异常”；此前 GitHub 自己做过的白块移动、Gamma、彩条等实验无需机械重复。
5. **输出目标**：800×1280 画面位置、颜色与几何形状均正常，稳定显示实际图像。当前尚未达到；`trim_wayland` 的桌面服务修复可放在图像硬件链路修好之后。

### 避免再次走弯路的约定

- 用户倾向直接复用 GitHub 已验证成果；不要重复已经完成、又没有新的区分度的实验。
- 禁用 HDMI 已获得用户许可；不必为了保留 HDMI 再绕回单 CRTC 克隆排查。
- 优先给出针对当前内核/系统的**可复现、可回退**修改，始终区分“观测事实”“推断”和“仓库曾声称”。
- 对 `modetest`、诊断模块和寄存器工具等可能改写硬件状态的操作应说明风险，并保存能从 USB FAT `/boot` 离线恢复的配置。

---

## 九、重要文件与目录索引

| 用途 | 路径 |
|---|---|
| 当前用于 LCD 启动的设备树 | `/boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb` |
| HDMI 仍启用时的备份 DTB | `/boot/c10h-lcd-stage/lcd-hdmi-working.dtb` |
| U-Boot 启动脚本 | `/boot/aml_autoscript`、`/boot/s905_autoscript` |
| 原始 U-Boot 脚本备份 | `/boot/c10h-lcd-stage/aml_autoscript.original`、`s905_autoscript.original` |
| 自动加载设置 | `/etc/modules-load.d/c10h-lcd-test.conf` |
| 自定义驱动 | `/lib/modules/6.12.41-trim/extra/` |
| 诊断模块备份 | `/boot/c10h-lcd-stage/c10h-lcd-diagnostic.ko` |
| DRM 工具（临时目录） | `/tmp/c10h-modetest/extracted/usr/bin/modetest` |
| 几何图源码和工具（临时目录） | `/tmp/c10h-geometry/drm-geometry.c`、`drm-geometry` |
| 内核显示调试状态 | `/sys/kernel/debug/dri/0/state` |
| 当前 LCD 连接器 | `/sys/class/drm/card0-DSI-1/` |
| 背光设备 | `/sys/class/backlight/backlight-c10h/` |

---

## 十、照片记录

配套 ZIP 的 `images/` 目录含本次对话已提供的三张关键实拍：

1. `01_HDMI未禁用_首次modetest点亮.jpg`：左侧青蓝、右侧黑白噪点/竖条。
2. `02_关闭HDMI_开机控制台异常.png`：开机自启 LCD，仍有青蓝色区域与右侧文字畸变。
3. `03_drm-geometry_边框白块重复.png`：几何图左侧青蓝、右侧多重横线/白块，证明图形提交后画面仍异常。

> 三张实拍都是拍摄屏幕的照片，可能包含镜头眩光、摩尔纹等摄影因素；判断系统性重复/错位主要依据肉眼可辨的图形结构与程序日志。

---

## 十一、第一轮接续摘要（历史版本；最新版本见第十二章）

> 创维 C10H / SK-L1001，Amlogic S905L3A，USB 启动的非官方 ARM fnOS 1.1.20（Debian 12 / Linux 6.12.41-trim）。使用 Arrow36 仓库 `c10h-dsi-host`、`panel-c10h-jd9366tc` 模块，在目标内核重新编译并绑定成功。修改当前 DTB，使 MIPI DSI/内屏启用、HDMI 禁用，eMMC 保持禁用。内屏开机自动被 DRM 启用，800×1280@60，目标像素时钟 76.107 MHz，实际 ≈75.857 MHz，DSI bit clock 531 MHz；面板日志显示 `C10H stock panel initialization complete`。DRM 原版 `drm-geometry.c` 编译运行成功（XRGB8888/LINEAR、pitch 3200、size 4096000；FB 43→45→43），但屏幕左侧仍大块青蓝，右侧出现重复白色横线/白块，控制台也畸变。`meson_encl_set_gamma_table` WR_RDY/ADR_RDY timeout。需要直接对比 GitHub 后期已解决图像分区的代码或配置与本机 Meson DRM/VPP/OSD/Canvas/ENCL/DSI 实现差异。不要重复早期彩条、Gamma 等已有实验，不要盲写寄存器；所有修改保留 USB `/boot` 回退能力。SSH 实验地址曾为 C10H_IP。

---

## 十二、后续会话与最新实验（2026-11-04，本次新增）

> 本章由原整理完成之后用户继续在对话中提供的 PowerShell/SSH 原始输出、实拍、上传 DTB，以及本次会话产生的 Python 测试脚本汇总。不是逐字聊天转录；**实际输出**与**解释/推测**严格分开。所有此前尚未执行的命令都标为“待执行”。

### 12.1 时间线与阶段性判断

| 顺序 | 动作 | 用户实测或观察 | 结论/意义 |
|---|---|---|---|
| 1 | 设备树状态、DRM 状态、备份检查 | DSI okay、HDMI disabled、SD/eMMC disabled、CRTC active=1、fb=43；`/boot/c10h-lcd-stage/` 备份存在 | 内屏独占启动状态成立 |
| 2 | DRM Primary Plane 详细状态 | XR24、800×1280、pitch 3200、dma_addr `DMA_ADDR`、未裁剪 | 用户态格式/画面描述正常，实际读取仍待验证 |
| 3 | 自定义诊断模块 `pattern=0` | ENCL_EN=1；DSI PWR=1、PKTSIZE=800、VACT=1280 等 | 输出硬件已启用，时序与驱动报告相符 |
| 4 | VPP / OSD / ENCL 6 项读取 | 关键三个 VPP/HDR 值与此前记载的上游 Linux 相同 | 不能把它们认作 fnOS 独有问题 |
| 5 | OSD1 寄存器与 Canvas 读取 | Canvas ID=0，物理地址/stride/高度与 fb0 完全一致 | 基本取图描述无明显错误 |
| 6 | Blend、VPP、VD1/VD2 读取 | Blend 窗口 800×1280，其他路由仍有不确定性 | 无可验证的单个故障位 |
| 7 | 内核包/源码调查 | `6.12.41-trim-219`、`Source: linux-upstream`、`ophub/fnnas`；未发现本地完整内核源码 | 无法证明当前闭源/定制补丁与公开源码一致 |
| 8 | Windows 公开源码 SHA-256 比对 | 6 个文件全是 IDENTICAL | 两个**公开参考版本**所取文件相同，不代表 fnOS 运行代码相同 |
| 9 | 解析用户上传的两个 DTB | 原 565 节点、现 570 节点；新增 5 个，无删除节点 | 不支持“设备树叠加时意外改坏 VPU 主节点”的假设 |
| 10 | ENCL `pattern=1` 彩条 | **完整彩条、无重复错位** | 后端可以正常呈现内部生成的测试图案 |
| 11 | 网页实时 framebuffer 滑杆 | 标准模式红→绿、绿→蓝、蓝→红；循环补偿后颜色准 | 确认测试画面存在固定通道循环错位，可在测试程序补偿 |
| 12 | 实拍、全屏纯白测试 | 左侧大片紫色不变；右侧纯白，少量像素闪动 | 左侧不会随这次 fb0 纯白写入改变，几何/空间问题未解决 |
| 13 | 读取 `/dev/fb0` 全帧 | 4,096,000 字节，按原脚本计数非白 128，DRM 仍 fb=43 | CPU 可见的 framebuffer 基本正确，问题位于后续路径的可能性提升 |

### 12.2 当前运行状态与 DRM 明细（用户命令输出）

````text
Linux: 6.12.41-trim #219, aarch64
fnOS: 1.1.20（非官方 ARM 移植）
启动参数包含：video=HDMI-A-1:1920x1080@60e
启动参数包含：initcall_blacklist=meson_dw_mipi_dsi_platform_driver_init
DSI status=okay; HDMI=disabled; SD=disabled; eMMC=disabled
Primary Plane: plane[36], fb=43
CRTC: crtc[42], active=1
Connector: DSI-1
format=XR24 little-endian (XRGB8888)
size=800x1280; pitch=3200; offset=0
DMA addr=DMA_ADDR
crtc-pos=800x1280+0+0
src-pos=800.000000x1280.000000+0.000000
rotation=1（DRM 的 rotate-0 标志位，不是旋转 90°）
````

保留一个容易误会的点：sysfs 显示 `fb=43` 和图层尺寸正确，是 DRM **软件状态**；并不能推出所有硬件像素扫描都正确。

### 12.3 内屏活动状态下的 ENCL / DSI 诊断（已执行）

命令：加载先前匹配 `6.12.41-trim` 的 `c10h-lcd-diagnostic.ko`，参数 `pattern=0`，读取并卸载。用户提供的输出：

````text
c10h-lcd: ENCL_EN=1 MODE=8000 ADV=418 MAX_PX=875 MAX_LN=1447 GAMMA=31 RGBIN=3
c10h-lcd: HAVON=36..835 VAVON=28..1307
c10h-dsi-reg: PWR=1 COLOR=5 MODE=0 VIDMODE=3f02 PKTSIZE=800 CHUNKS=0 NULL=0
c10h-dsi-reg: HSA=7 HBP=25 HLINE=764 VSA=8 VBP=20 VFP=140 VACT=1280 LANES=2003 PHY=1529 TOP=512400
````

理解：ENCL 水平活动区 800、垂直活动区 1280；DSI Host 已工作。`HLINE=764` 使用的不是 framebuffer 的“像素”单位，不应据此直接宣称时序错误。`meson_encl_set_gamma_table` 的 `WR_RDY/ADR_RDY timeout` 仍存在，原因未确认。

### 12.4 VPP / ENCL / OSD 寄存器快照（已执行；均为读取）

六个此前与 Android/上游 Linux 对照过的候选值（此处只陈述 **fnOS 实测**）：

| 寄存器编号 | fnOS 当前值 |
|---|---|
| `0x1CC1` | `0x00000402` |
| `0x1CC7` | `0x00000003` |
| `0x1D27` | `0xFFF01000` |
| `0x1D22` | `0x00000404` |
| `0x1D5F` | `0x00007800` |
| `0x38A0` | `0x0000C040` |

先前聊天援引的原厂 Android 报告值（**上游记录，不是本用户本轮实测 Android**）：`0x1D22=0x08080808`、`0x1D5F=0x0`、`0x38A0=0x1C040`。这些差异不能单独当作原因，也不应整体照搬写回 Linux。

OSD1 九项读数：

````text
0x1A10=0x05700001  0x1A13=0x04FF0000  0x1A14=0x00000000
0x1A1B=0x00008504  0x1A1C=0x031F0000  0x1A1D=0x04FF0000
0x1A1E=0x031F0000  0x1A2D=0x00007FC0  0x1A30=0x000FF000
````

解释：OSD1 已配置，来源与目的窗口的坐标对应 800×1280，用户观察到的左侧/右侧分区并不能通过这些表面窗口参数直接解释。

Canvas 硬件读取结果：

````text
OSD1 Canvas ID = 0
Canvas Address = DMA_ADDR
Canvas Pitch   = 3200
Canvas Height  = 1280
DRM: fb=43 pitch[0]=3200 dma_addr=DMA_ADDR
````

Canvas 读取需要向 Canvas 读取选择寄存器发出命令（不是纯 MMIO 只读），先前已执行并成功，无需机械重复。

### 12.5 Blend / VPP / VD1 路由状态（已执行）

````text
REG=0x39B0 VALUE=0x8F100001
REG=0x39B1 VALUE=0x031F0000
REG=0x39B2 VALUE=0x04FF0000
REG=0x39BB VALUE=0x05000320
REG=0x39BC VALUE=0x05000320
REG=0x1DF1 VALUE=0x03200500
REG=0x1DF5 VALUE=0x0000031F
REG=0x1DF6 VALUE=0x000004FF
REG=0x1DFD VALUE=0x00100300
REG=0x1DA5 VALUE=0x03200500
REG=0x1DC8 VALUE=0x00000000
REG=0x1D26 VALUE=0x00000482
REG=0x1A0E VALUE=0x00000000
````

追加 VD1/VD2 读取：

````text
0x1DFB=0x00000000
0x1DFC=0x00000000
0x1DFD=0x00100300
0x1D1C=0x0000077F
0x1D1D=0x00000437
0x1D26=0x00000482
````

在当前对话中曾讨论过 `VPP_MISC` 的 VD1 Postblend 位，及旧 `1920×1080` 窗口遗留值；**不能根据它们证明 VD1 实际干扰显示**。本轮没有对这些寄存器执行写操作。

### 12.6 内核版本、打包信息与公开源码对比

设备本地：

````text
uname -r: 6.12.41-trim
/proc/version: Linux version 6.12.41-trim ([构建账户已脱敏])
linux-image-6.12.41-trim: Version 6.12.41-trim-219, Source linux-upstream
linux-headers-6.12.41-trim: Version 6.12.41-trim-219, Source linux-upstream
Maintainer: [已脱敏]
apt-cache policy: 仅本地 dpkg 状态记录
内核头文件: /usr/src/linux-headers-6.12.41-trim
完整 Meson DRM C 源码: /usr/src 下未找到
/etc/ophub-release:
  BOARD=s905l3a
  KERNEL_REPO=ophub/fnnas
  KERNEL_TAGS=kernel_fnnas
  KERNEL_VERSION=6.12.41
  BUILDER_NAME=ophub
````

两份 C10H 外部模块位于：

````text
/lib/modules/6.12.41-trim/extra/c10h-dsi-host.ko
sha256: d90c5ed1bcca4595eb1c843a79e86e2765bedcac7af058c49a1bfe13ae0d378c
/lib/modules/6.12.41-trim/extra/panel-c10h-jd9366tc.ko
sha256: fb50a48ba01ff2884133a4a7b12048e207d25280bd10a934242fedd3ec1ad135
vermagic（两者）: 6.12.41-trim SMP preempt mod_unload aarch64
````

**网络障碍**：C10H 从 `raw.githubusercontent.com` 下载超时：`curl: (28) ... 12001 milliseconds`，HTTP `000`。之后改用 Windows PowerShell 下载到 `<WORKDIR>\C10H-srcdiff`。

用户使用 SHA-256 逐文件比对以下两个**公开仓库路径**：

- Linux 官方参考：`https://raw.githubusercontent.com/gregkh/linux/v6.12.41/drivers/gpu/drm/meson/`
- 另一参考：`https://raw.githubusercontent.com/unifreq/linux-6.12.y/main/drivers/gpu/drm/meson/`

| 源码 | 用户 PowerShell 结果 |
|---|---|
| `meson_crtc.c` | `IDENTICAL` |
| `meson_plane.c` | `IDENTICAL` |
| `meson_viu.c` | `IDENTICAL` |
| `meson_vpp.c` | `IDENTICAL` |
| `meson_dw_mipi_dsi.c` | `IDENTICAL` |
| `meson_encoder_dsi.c` | `IDENTICAL` |

**限制：这些结果只代表下载的这两组公开源码文件一致；不能得出 fnOS `6.12.41-trim` 的运行时内核代码、厂商补丁或 Arrow36 实验所用 `6.12.114-ophub` 必然相同的结论。**

### 12.7 用户上传两个 DTB 的本地验证

用户上传：`base-original.dtb`、`current.dtb`。本次整理时重新使用 Python 解析 FDT 二进制结构，不修改文件，得到：

| 项目 | 原始 DTB | 当前 DTB |
|---|---:|---:|
| 文件大小 | 79,447 字节 | 80,260 字节 |
| 节点数量 | 565 | 570 |
| 新增节点 | — | 5 |
| 删除节点 | — | 0 |

新增的 5 个节点分别为：

````text
/backlight-c10h
/soc/bus@ffd00000/dsi@7000/panel@0
/soc/bus@ffd00000/dsi@7000/panel@0/port
/soc/bus@ffd00000/dsi@7000/panel@0/port/endpoint
/soc/bus@ffd00000/dsi@7000/ports/port@1/endpoint
````

设备节点 `status` 的已验证变更：

````text
DSI: disabled -> okay
HDMI: okay -> disabled
pwm@1a000: disabled -> okay
/soc/bus@ff600000/phy@44000: disabled -> okay
/soc/bus@ff600000/bus@3c000/system-controller@0/phy: disabled -> okay
/soc/mmc@ffe05000: okay -> disabled
/soc/mmc@ffe07000: okay -> disabled
````

此次仅统计 FDT 节点数量和 `status` 改动；不能凭此单独保证所有 pinctrl、时钟、GPIO 或 phandle 在所有内核环境下均正确。当前没有证据证明重制 DTB 是解决画面重复的必要步骤。

### 12.8 ENCL 硬件彩条测试（成功，但不是显示修复）

在当前内屏独占状态下使用诊断模块 `pattern=1`，以限时 `systemd-run` 单元临时激活。用户反馈：

> **「完整彩条，没有重复、错位。」**

**意义**：ENCL 内部测试图案能够经过后段输出链路完整显示，屏幕/DSI 基础传输功能存在。**限制**：这不能证明普通像素流在 OSD/VPP/ENCL/DSI 上的每个环节都正确。

注意：诊断模块在卸载时尝试恢复先前 ENCL 寄存器状态，但在内核死锁、系统重启等极端情况下无法保证自动恢复。不要未经核对源码和恢复条件便重复使用。

### 12.9 Windows 浏览器实时滑杆与 RGB 通道补偿

此前对话中创建了三份 Python 诊断工具（文件名见下）。程序运行在 C10H，Windows 通过 SSH `-L 8765:127.0.0.1:8765` 浏览器访问 `http://127.0.0.1:8765`。测试工具通过 `/dev/fb0` 写入图案，可调整色块位置、RGB 值和背光；**不是**可任意调 VPU/DSI 寄存器的工具。

````text
c10h_live_slider.py
c10h_live_slider_rgb_test.py
c10h_fullscreen_color_probe.py
````

用户观察的 RGB 映射（标准颜色模式）：

| 指定颜色 | 屏幕显示 |
|---|---|
| 红色 | 绿色 |
| 绿色 | 蓝色 |
| 蓝色 | 红色 |

Python 写入方式经源文件核对：

````python
# 小端 XRGB8888 原始模式
pixel = bytes((b, g, r, 0))
# 仅用于诊断的循环补偿模式
pixel = bytes((r, b, g, 0))
````

用户反馈 **「循环错位补偿后颜色准了」**。而 `/dev/fb0` 经 `FBIOGET_VSCREENINFO` 报告：

````text
fb0 channel offsets R=16:8 G=8:8 B=0:8
````

说明软件可通过逆置换让这套测试图颜色变对，但**没有找到具体导致颜色通道循环错位的硬件寄存器/函数，也没有完成全系统颜色修复**。

启动 SSH 端口转发时曾出现：`channel 3: open failed: connect failed: Connection refused`，其后浏览器 GET `/` 返回 200，说明网页服务之后已正常工作。`/home/test-user` 不存在的提示也与显示问题无关。

### 12.10 实拍：左侧大片紫色，右侧重复闪动

用户上传了本轮最新实拍（当前对话中的图片文件；此纯 Markdown 文件不内嵌照片），并描述：

> **「左侧大片一直是紫色，不是补偿后变的。右侧还是重复的闪动。」**

使用全屏颜色版测试程序，用户进一步观察：

> **「左侧没变，右侧变成纯白。」**
>
> **「右侧有几个像素点在闪动。」**

**直接推论**：仅 RGB 通道交换无法解释左侧在纯白模式下维持紫色（理论上白色 R/G/B 相等）；右侧屏幕能对 framebuffer 更新作出响应；左侧则没有随该测试改变。需要审查像素从内存经 DMA/OSD/VPP/ENCL/DSI 的真实扫描路径。右侧少数闪点也可能受 Linux 控制台并发写 fb0 的影响，尚无证据证明存在物理坏点。

### 12.11 九点抽样和整个 fb0 的读取结果

用户在全屏纯白模式执行 Python `os.pread`：

九个采样点 `(x=50,400,750; y=100,640,1100)` 均得到：

````text
pixel=ff ff ff 00 WHITE
````

全帧读取与计数的**原始结果**：

````text
===== 全帧检查 =====
读取字节: 4096000
总像素: 1024000
纯白像素: 1023872
非纯白像素: 128
第一个异常位置: x= 96 y= 288 值= 00000000
===== 当前 DRM 扫描图层 =====
fb=43
format=XR24 little-endian (0x34325258)
pitch[0]=3200
dma_addr=DMA_ADDR
crtc-pos=800x1280+0+0
````

原脚本的判断机制：

````python
white = bytes([255, 255, 255, 0])
correct = data.count(white)
total = 800 * 1280
````

结论：原脚本报告纯白比例 `1,023,872 / 1,024,000 = 99.9875%`。对这份报告最稳妥的解释是 **CPU 可见的 `/dev/fb0` 内容几乎都是白色**。对“128 个异常像素”的数量/布局仍可严格按每 4 字节一像素复核；少量非白也可能来自控制台字符或光标并发绘制，**不应直接当作坏显存证据**。此结果不直接证明 DMA 看到了同样的数据（缓存一致性、扫描读取与后处理仍未验证）。

### 12.12 还没做的事和应当避免的重复工作

**最后一条提出但未执行的检查**：此前助手建议只读 `0x1A2B`（候选物理地址 `0xFF9068AC`）以了解 OSD FIFO 状态：

````bash
# 尚未执行；先核对 SoC 寄存器定义与访问安全性
# sudo busybox devmem 0xFF9068AC 32
````

这只能得到 FIFO 某个配置值，未必有足够的上游对照信息，**不是已找到的修复方案**。

后续推荐优先级：

1. **保留当前可启动环境与回退**：原厂 Android 仍在 eMMC，fnOS 从 USB 启动；原始 U-Boot 脚本和 DTB 备份存在。做风险实验前确认 `/boot` 的离线恢复材料，以及停止/恢复 `/dev/fb0` 测试程序。
2. **先查“同一测试条件”的对照数据**：如果能获得 Arrow36 的完整图像阶段的关键 VPU/OSD FIFO/DMA/DSI 状态，比继续猜寄存器有效；或经 ADB 在原厂 Android 采集只读驱动资料，但应先确认权限和接口。
3. **重点区分 CPU 写入、显示 DMA 实际读取、OSD FIFO/Blend、ENCL/DSI 像素流**，优先选择单变量、可恢复且能区分假设的测试。
4. **RGB 通道置换单独处理**：软件补偿验证有效；若要永久修复，应定位 framebuffer 格式、OSD/VPP 色彩处理或 DSI 映射的原因，不能直接认定一个寄存器。
5. **不要把移植成熟度夸大**：Arrow36 已能显示完整几何图/可辨终端但还存在偏色与抖动；fnOS 当前仍严重分区。公开版本比较不能代替实际运行内核补丁对比。
6. **不要重复已经做完的几何图、内屏彩条、Canvas、纯白、六份公开源码比对，除非提出的新实验确实需要作为对照**。不随意写 OSD/VPP/DSI 寄存器，也不优先更换内核、刷 eMMC 或重做 DTB。

### 12.13 重要材料与路径

| 材料 | 文件/路径 | 状态 |
|---|---|---|
| 当前运行 DTB | `/boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb` | 已上传副本为 `current.dtb` |
| 原始 DTB | `/boot/c10h-lcd-stage/base-original.dtb` | 已上传副本为 `base-original.dtb` |
| 两个 U-Boot 脚本备份 | `/boot/c10h-lcd-stage/aml_autoscript.original`、`s905_autoscript.original` | 设备上的备份目录已列出 |
| 自制 DSI/面板模块 | `/lib/modules/6.12.41-trim/extra/` | 已安装 |
| 诊断模块 | `/boot/c10h-lcd-stage/c10h-lcd-diagnostic.ko` | 已用于 pattern=0、pattern=1 |
| Windows 对比目录 | `<WORKDIR>\C10H-srcdiff` | 已完成 6 文件 SHA 比对 |
| Python 滑杆 | `c10h_live_slider.py` | 历史会话生成 |
| RGB 补偿滑杆 | `c10h_live_slider_rgb_test.py` | 历史会话生成，颜色补偿有效 |
| 全屏颜色测试 | `c10h_fullscreen_color_probe.py` | 历史会话生成，全白观察已完成 |
| 实拍 | 当次会话上传照片 | 左侧紫色不变，右侧重复/闪动 |

### 12.14 供下一次会话直接粘贴的**最新接续摘要**

> C10H / SK-L1001 (S905L3A) 10.1 英寸 JD9366TC，800×1280@60，MIPI DSI 4-lane RGB888；非官方 ARM fnOS 1.1.20 / Debian 12 / Linux `6.12.41-trim`，USB 启动、保留 Android eMMC。已移植并编译 Arrow36 的 C10H DSI Host + 面板驱动，HDMI 禁用，内屏可在开机时自动点亮。DRM 主 plane fb=43，XR24 / pitch 3200，DMA 地址 `DMA_ADDR`；Canvas ID 0 的地址/pitch/高度与其一致，OSD/Blend 窗口 800×1280。ENCL `pattern=1` 硬件彩条完整、无错位；普通 framebuffer 图像严重分区、重复、闪动。网页 Python 滑杆发现颜色通道循环错位：标准色红→绿、绿→蓝、蓝→红；逆置换 `(R,B,G,X)` 后测试图颜色准确；fb0 位偏移仍报告 R16/G8/B0。最新实拍：屏幕左侧大片紫色**不随全屏纯白改变**；右侧变白且有少量闪点。全白情况下读取 `/dev/fb0` 4,096,000 字节，脚本报告 1,023,872 个纯白、128 非白（该计数使用 `data.count`，建议严格逐像素复核），DRM 仍为 fb=43、pitch=3200、dma `DMA_ADDR`。已取得 ENCL/DSI、OSD、Blend、VPP、Canvas 多批寄存器；无确定修复值。Windows 对比公开 Linux v6.12.41 与 unifreq 6.12.y 的 6 个 Meson 驱动 C 文件均 IDENTICAL，但**不代表飞牛 `6.12.41-trim #219` 或 Arrow36 `6.12.114-ophub` 的实际源码一致**。用户上传当前/原始 DTB，经 FDT 解析为 570/565 个节点，主要修改是启用 DSI/PHY/PWM、禁用 HDMI/SD/eMMC、添加面板与背光节点。**最新待办**：停止/确认恢复测试程序，审查 OSD DMA/FIFO 或获取同测试条件的 Android/Arrow36 对照数据；前助手建议的 `0x1A2B / 0xFF9068AC` FIFO 只读检查尚未执行，先复核寄存器定义。不重复无区分度的测试、不盲写寄存器，不假定已经成功修复。`

---

## 附录 A：用户上传的 SSH 终端日志原文

以下为本次对话附带的 `粘贴的文本 (1).txt` 内容，包含从前期启动/模块检查到首次内屏点亮后的完整终端输出。为保证可追溯性，保留重复日志；不代表其中每条命令都应再次运行。

```text
Windows PowerShell
Copyright (C) Microsoft Corporation. All rights reserved.

PS <USERDIR>> ssh test-user@C10H_IP
[交互式凭据提示已脱敏]
Linux c10h 6.12.41-trim #219 SMP PREEMPT Thu Feb 12 01:38:06 UTC 2026 aarch64

The programs included with the Debian GNU/Linux system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Debian GNU/Linux comes with ABSOLUTELY NO WARRANTY, to the extent
permitted by applicable law.
[登录时间与来源已脱敏]
Could not chdir to home directory /home/test-user: No such file or directory
test-user@device:/$ echo "===== 1. 实际DSI状态 ====="
tr -d '\0' < \
/sys/firmware/devicetree/base/soc/bus@ffd00000/dsi@7000/status
echo

echo "===== 2. 实际启动参数 ====="
cat /proc/cmdline

echo "===== 3. C10H驱动 ====="
lsmod | grep -Ei 'c10h|jd9366|dsi'

echo "===== 4. 显示连接器 ====="
ls /sys/class/drm/

echo "===== 5. 背光设备 ====="
ls /sys/class/backlight/

echo "===== 6. 显示驱动日志 ====="
sudo dmesg | grep -Ei \
'c10h|jd9366|dsi|panel|backlight|drm|initcall|probe|fail|error' \
| tail -120

echo "===== 7. 模块自动加载日志 ====="
sudo journalctl -b -u systemd-modules-load --no-pager -n 30
===== 1. 实际DSI状态 =====
okay
===== 2. 实际启动参数 =====
root=UUID=ROOT_FS_UUID rootflags=compress=zstd:1 rw rootwait rootfstype=btrfs console=ttyAML0,115200n8 console=tty0 no_console_suspend consoleblank=0 fsck.fix=yes fsck.repair=yes net.ifnames=0 max_loop=128 cgroup_enable=cpuset cgroup_memory=1 cgroup_enable=memory swapaccount=1 video=HDMI-A-1:1920x1080@60e plymouth.enable=0 initcall_blacklist=meson_dw_mipi_dsi_platform_driver_init
===== 3. C10H驱动 =====
panel_c10h_jd9366tc    12288  0
c10h_dsi_host          12288  0
===== 4. 显示连接器 =====
card0  card0-Composite-1  card0-DSI-1  card0-HDMI-A-1  card1  renderD128  version
===== 5. 背光设备 =====
backlight-c10h
===== 6. 显示驱动日志 =====
[交互式凭据提示已脱敏]
[    8.455877] [drm] forcing HDMI-A-1 connector on
[    8.502350] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.509443] [drm] forcing HDMI-A-1 connector on
[    8.556363] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.563301] [drm] forcing HDMI-A-1 connector on
[    8.608279] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.615243] [drm] forcing HDMI-A-1 connector on
[    8.660064] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.667167] [drm] forcing HDMI-A-1 connector on
[    8.712264] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.719493] [drm] forcing HDMI-A-1 connector on
[    8.764040] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.771183] [drm] forcing HDMI-A-1 connector on
[    8.816191] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.823212] [drm] forcing HDMI-A-1 connector on
[    8.868091] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.875243] [drm] forcing HDMI-A-1 connector on
[    8.924032] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.931187] [drm] forcing HDMI-A-1 connector on
[    8.996119] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.017415] [drm] forcing HDMI-A-1 connector on
[    9.098853] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.121058] [drm] forcing HDMI-A-1 connector on
[    9.202757] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.224534] [drm] forcing HDMI-A-1 connector on
[    9.306494] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.328798] [drm] forcing HDMI-A-1 connector on
[    9.398946] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.421216] [drm] forcing HDMI-A-1 connector on
[    9.510420] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.532548] [drm] forcing HDMI-A-1 connector on
[    9.604077] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.626315] [drm] forcing HDMI-A-1 connector on
[    9.702489] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.709626] [drm] forcing HDMI-A-1 connector on
[    9.791888] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.813609] [drm] forcing HDMI-A-1 connector on
[    9.895106] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.916481] [drm] forcing HDMI-A-1 connector on
[    9.983409] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.990607] [drm] forcing HDMI-A-1 connector on
[   10.032096] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.039277] [drm] forcing HDMI-A-1 connector on
[   10.081959] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.089220] [drm] forcing HDMI-A-1 connector on
[   10.111753] systemd[1]: Created slice system-modprobe.slice - Slice /system/modprobe.
[   10.151912] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.159077] [drm] forcing HDMI-A-1 connector on
[   10.233154] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.250582] [drm] forcing HDMI-A-1 connector on
[   10.310459] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.323208] [drm] forcing HDMI-A-1 connector on
[   10.370421] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.385347] [drm] forcing HDMI-A-1 connector on
[   10.428256] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.435481] [drm] forcing HDMI-A-1 connector on
[   10.497654] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.513690] [drm] forcing HDMI-A-1 connector on
[   10.559282] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.566473] [drm] forcing HDMI-A-1 connector on
[   10.644927] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.652090] [drm] forcing HDMI-A-1 connector on
[   10.733633] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.746951] [drm] forcing HDMI-A-1 connector on
[   10.826179] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.833416] [drm] forcing HDMI-A-1 connector on
[   10.874707] systemd[1]: Starting modprobe@configfs.service - Load Kernel Module configfs...
[   10.895974] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.896049] [drm] forcing HDMI-A-1 connector on
[   10.936109] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.940787] systemd[1]: Starting modprobe@dm_mod.service - Load Kernel Module dm_mod...
[   10.943214] [drm] forcing HDMI-A-1 connector on
[   10.961152] systemd[1]: Starting modprobe@drm.service - Load Kernel Module drm...
[   10.968236] systemd[1]: Starting modprobe@efi_pstore.service - Load Kernel Module efi_pstore...
[   10.976702] systemd[1]: Starting modprobe@fuse.service - Load Kernel Module fuse...
[   10.985244] systemd[1]: Starting modprobe@loop.service - Load Kernel Module loop...
[   11.018628] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.025638] [drm] forcing HDMI-A-1 connector on
[   11.096091] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.101561] [drm] forcing HDMI-A-1 connector on
[   11.165705] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.172789] [drm] forcing HDMI-A-1 connector on
[   11.235410] systemd[1]: modprobe@configfs.service: Deactivated successfully.
[   11.242145] systemd[1]: Finished modprobe@configfs.service - Load Kernel Module configfs.
[   11.255637] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.265232] [drm] forcing HDMI-A-1 connector on
[   11.266131] systemd[1]: modprobe@dm_mod.service: Deactivated successfully.
[   11.276560] systemd[1]: Finished modprobe@dm_mod.service - Load Kernel Module dm_mod.
[   11.289119] systemd[1]: modprobe@drm.service: Deactivated successfully.
[   11.293533] c10h_dsi_host: loading out-of-tree module taints kernel.
[   11.294885] systemd[1]: Finished modprobe@drm.service - Load Kernel Module drm.
[   11.308949] systemd[1]: modprobe@efi_pstore.service: Deactivated successfully.
[   11.315326] systemd[1]: Finished modprobe@efi_pstore.service - Load Kernel Module efi_pstore.
[   11.327682] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[   11.343080] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[   11.344468] systemd[1]: modprobe@fuse.service: Deactivated successfully.
[   11.357372] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.359473] systemd[1]: Finished modprobe@fuse.service - Load Kernel Module fuse.
[   11.371743] [drm] forcing HDMI-A-1 connector on
[   11.389532] systemd[1]: modprobe@loop.service: Deactivated successfully.
[   11.399351] systemd[1]: Finished modprobe@loop.service - Load Kernel Module loop.
[   11.411590] systemd[1]: trim_wayland.service: Failed with result 'exit-code'.
[   11.428331] systemd[1]: trim_miniscreen.service: Failed with result 'exit-code'.
[   11.461049] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.463066] [drm] forcing HDMI-A-1 connector on
[   11.472387] [drm] Initialized meson 1.0.0 for ff900000.vpu on minor 0
[   11.731805] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   12.060980] meson-drm ff900000.vpu: [drm] fb0: mesondrmfb frame buffer device
[   12.339817] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   12.582080] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   12.846469] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   14.664023] [drm] Initialized panfrost 1.2.0 for ffe40000.gpu on minor 1
[   14.966363] audit: type=1400 audit(1750949926.824:2): apparmor="STATUS" operation="profile_load" profile="unconfined" name="nvidia_modprobe" pid=2248 comm="apparmor_parser"
[   14.983251] audit: type=1400 audit(1750949926.824:3): apparmor="STATUS" operation="profile_load" profile="unconfined" name="nvidia_modprobe//kmod" pid=2248 comm="apparmor_parser"
[   24.800896] platform sdio-pwrseq: deferred probe pending: platform: supplier wifi32k not ready
[   24.808204] platform wifi32k: deferred probe pending: (reason unknown)
[   41.094570] meson-drm ff900000.vpu: [drm] User-defined mode not supported: "1920x1080": 60 148500 1920 2008 2052 2200 1080 1084 1089 1125 0x68 0x5
[   41.094625] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   41.111963] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   61.755653] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
===== 7. 模块自动加载日志 =====
Jun 26 22:58:43 c10h systemd-modules-load[1894]: Inserted module 'zram'
Jun 26 22:58:43 c10h systemd-modules-load[1894]: Inserted module 'c10h_dsi_host'
Jun 26 22:58:43 c10h systemd-modules-load[1894]: Inserted module 'panel_c10h_jd9366tc'
Jun 26 22:58:43 c10h systemd-modules-load[1894]: Failed to find module 'msr'
Jun 26 22:58:43 c10h systemd-modules-load[1894]: Inserted module 'tcp_bbr'
Jun 26 22:58:43 c10h systemd-modules-load[1894]: Inserted module 'nft_fullcone'
Jun 26 22:58:43 c10h systemd-modules-load[1894]: Inserted module 'zfs'
Jun 26 22:58:43 c10h systemd[1]: Finished systemd-modules-load.service - Load Kernel Modules.
test-user@device:/$ echo "===== 1. 两个屏幕的连接与启用状态 ====="
for OUT in DSI-1 HDMI-A-1; do
    P="/sys/class/drm/card0-$OUT"
    echo "--- $OUT ---"
    for F in status enabled modes; do
        if [ -r "$P/$F" ]; then
            echo "[$F]"
            cat "$P/$F"
        fi
    done
done

echo "===== 2. 内屏背光状态 ====="
B=/sys/class/backlight/backlight-c10h

for F in brightness actual_brightness max_brightness bl_power; do
    if [ -r "$B/$F" ]; then
        echo -n "$F = "
        cat "$B/$F"
    fi
done

echo "===== 3. DRM显示通路状态 ====="
if sudo test -r /sys/kernel/debug/dri/0/state; then
    sudo cat /sys/kernel/debug/dri/0/state |
      grep -E 'crtc\[|connector\[|active=|mode='
else
    echo "DRM debugfs状态不可读取"
fi

echo "===== 4. 飞牛显示服务错误 ====="
sudo journalctl -b \
  -u trim_wayland.service \
  -u trim_miniscreen.service \
  --no-pager -n 50
===== 1. 两个屏幕的连接与启用状态 =====
--- DSI-1 ---
[status]
connected
[enabled]
disabled
[modes]
--- HDMI-A-1 ---
[status]
connected
[enabled]
enabled
[modes]
1920x1080
1024x768
800x600
800x600
848x480
===== 2. 内屏背光状态 =====
brightness = 4
actual_brightness = 4
max_brightness = 7
bl_power = 0
===== 3. DRM显示通路状态 =====
crtc[45]: meson_crtc
        active=1
        self_refresh_active=0
connector[33]: Composite-1
connector[35]: HDMI-A-1
connector[38]: DSI-1
===== 4. 飞牛显示服务错误 =====
Jun 26 22:58:46 c10h systemd[1]: trim_wayland.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Jun 26 22:58:46 c10h systemd[1]: trim_wayland.service: Failed with result 'exit-code'.
Jun 26 22:58:46 c10h systemd[1]: trim_miniscreen.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Jun 26 22:58:46 c10h systemd[1]: trim_miniscreen.service: Failed with result 'exit-code'.
Nov 04 13:12:00 c10h systemd[1]: trim_miniscreen.service: Scheduled restart job, restart counter is at 2.
Nov 04 13:12:00 c10h systemd[1]: trim_wayland.service: Scheduled restart job, restart counter is at 2.
Nov 04 13:12:00 c10h systemd[1]: Stopped trim_miniscreen.service - Trim Miniscreen Service.
Nov 04 13:12:00 c10h systemd[1]: Stopped trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:00 c10h systemd[1]: Started trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:00 c10h systemd[1]: Started trim_miniscreen.service - Trim Miniscreen Service.
Nov 04 13:12:00 c10h sh[2553]: sh: 0: cannot open /usr/trim/bin/start_wayland.sh: No such file
Nov 04 13:12:00 c10h sh[2556]: sh: 0: cannot open /usr/trim/bin/start_miniscreen.sh: No such file
Nov 04 13:12:00 c10h systemd[1]: trim_wayland.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Nov 04 13:12:00 c10h systemd[1]: trim_wayland.service: Failed with result 'exit-code'.
Nov 04 13:12:00 c10h systemd[1]: trim_miniscreen.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Nov 04 13:12:00 c10h systemd[1]: trim_miniscreen.service: Failed with result 'exit-code'.
Nov 04 13:12:03 c10h systemd[1]: trim_wayland.service: Scheduled restart job, restart counter is at 3.
Nov 04 13:12:03 c10h systemd[1]: trim_miniscreen.service: Scheduled restart job, restart counter is at 3.
Nov 04 13:12:03 c10h systemd[1]: Stopped trim_miniscreen.service - Trim Miniscreen Service.
Nov 04 13:12:03 c10h systemd[1]: Stopped trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:03 c10h systemd[1]: Started trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:03 c10h sh[2770]: sh: 0: cannot open /usr/trim/bin/start_wayland.sh: No such file
Nov 04 13:12:03 c10h systemd[1]: Started trim_miniscreen.service - Trim Miniscreen Service.
Nov 04 13:12:03 c10h sh[2774]: sh: 0: cannot open /usr/trim/bin/start_miniscreen.sh: No such file
Nov 04 13:12:03 c10h systemd[1]: trim_wayland.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Nov 04 13:12:03 c10h systemd[1]: trim_wayland.service: Failed with result 'exit-code'.
Nov 04 13:12:03 c10h systemd[1]: trim_miniscreen.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Nov 04 13:12:03 c10h systemd[1]: trim_miniscreen.service: Failed with result 'exit-code'.
Nov 04 13:12:06 c10h systemd[1]: trim_wayland.service: Scheduled restart job, restart counter is at 4.
Nov 04 13:12:06 c10h systemd[1]: trim_miniscreen.service: Scheduled restart job, restart counter is at 4.
Nov 04 13:12:06 c10h systemd[1]: Stopped trim_miniscreen.service - Trim Miniscreen Service.
Nov 04 13:12:06 c10h systemd[1]: Stopped trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:06 c10h systemd[1]: Started trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:06 c10h sh[2816]: sh: 0: cannot open /usr/trim/bin/start_wayland.sh: No such file
Nov 04 13:12:06 c10h systemd[1]: Started trim_miniscreen.service - Trim Miniscreen Service.
Nov 04 13:12:06 c10h sh[2817]: sh: 0: cannot open /usr/trim/bin/start_miniscreen.sh: No such file
Nov 04 13:12:06 c10h systemd[1]: trim_wayland.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Nov 04 13:12:06 c10h systemd[1]: trim_wayland.service: Failed with result 'exit-code'.
Nov 04 13:12:06 c10h systemd[1]: trim_miniscreen.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
Nov 04 13:12:06 c10h systemd[1]: trim_miniscreen.service: Failed with result 'exit-code'.
Nov 04 13:12:09 c10h systemd[1]: trim_miniscreen.service: Scheduled restart job, restart counter is at 5.
Nov 04 13:12:09 c10h systemd[1]: trim_wayland.service: Scheduled restart job, restart counter is at 5.
Nov 04 13:12:09 c10h systemd[1]: Stopped trim_miniscreen.service - Trim Miniscreen Service.
Nov 04 13:12:09 c10h systemd[1]: Stopped trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:09 c10h systemd[1]: trim_wayland.service: Start request repeated too quickly.
Nov 04 13:12:09 c10h systemd[1]: trim_wayland.service: Failed with result 'exit-code'.
Nov 04 13:12:09 c10h systemd[1]: Failed to start trim_wayland.service - Trim Wayland Service.
Nov 04 13:12:09 c10h systemd[1]: trim_miniscreen.service: Start request repeated too quickly.
Nov 04 13:12:09 c10h systemd[1]: trim_miniscreen.service: Failed with result 'exit-code'.
Nov 04 13:12:09 c10h systemd[1]: Failed to start trim_miniscreen.service - Trim Miniscreen Service.
test-user@device:/$ echo "===== 1. 完整DRM状态 ====="
sudo cat /sys/kernel/debug/dri/0/state

echo "===== 2. DSI设备绑定 ====="
ls -l /sys/bus/mipi-dsi/devices/ 2>&1

for d in /sys/bus/mipi-dsi/devices/*; do
    [ -d "$d" ] || continue
    echo "--- $d ---"
    readlink -f "$d/driver" 2>/dev/null \
      || echo "未绑定驱动"
done

echo "===== 3. 面板初始化日志 ====="
sudo dmesg | grep -Ei \
'initialization complete|DCS command|failed to get|panel|mipi|dsi' \
| tail -60

echo "===== 4. DRM工具 ====="
command -v modetest || echo "modetest未安装"
command -v drm_info || echo "drm_info未安装"
===== 1. 完整DRM状态 =====
plane[39]: meson_primary_plane
        crtc=meson_crtc
        fb=47
                allocated by = [fbcon]
                refcount=2
                format=XR24 little-endian (0x34325258)
                modifier=0x0
                size=1920x1080
                layers:
                        size[0]=1920x1080
                        pitch[0]=7680
                        offset[0]=0
                        obj[0]:
                                name=0
                                refcount=3
                                start=00100000
                                size=8294400
                                imported=no
                                dma_addr=DMA_ADDR
                                vaddr=0000000092bb1dc7
        crtc-pos=1920x1080+0+0
        src-pos=1920.000000x1080.000000+0.000000+0.000000
        rotation=1
        normalized-zpos=1
        color-encoding=ITU-R BT.601 YCbCr
        color-range=YCbCr limited range
        color_mgmt_changed=0
plane[42]: meson_overlay_plane
        crtc=(null)
        fb=0
        crtc-pos=0x0+0+0
        src-pos=0.000000x0.000000+0.000000+0.000000
        rotation=1
        normalized-zpos=0
        color-encoding=ITU-R BT.601 YCbCr
        color-range=YCbCr limited range
        color_mgmt_changed=0
crtc[45]: meson_crtc
        enable=1
        active=1
        self_refresh_active=0
        planes_changed=1
        mode_changed=0
        active_changed=0
        connectors_changed=0
        color_mgmt_changed=0
        plane_mask=1
        connector_mask=2
        encoder_mask=2
        mode: "1920x1080": 60 172780 1920 2040 2248 2576 1080 1081 1084 1118 0x20 0x6
connector[33]: Composite-1
        crtc=(null)
        self_refresh_aware=0
        max_requested_bpc=0
        colorspace=Default
connector[35]: HDMI-A-1
        crtc=meson_crtc
        self_refresh_aware=0
        max_requested_bpc=0
        colorspace=Default
        broadcast_rgb=Automatic
        is_limited_range=n
        output_bpc=0
        output_format=RGB
        tmds_char_rate=0
connector[38]: DSI-1
        crtc=(null)
        self_refresh_aware=0
        max_requested_bpc=0
        colorspace=Default
===== 2. DSI设备绑定 =====
total 0
lrwxrwxrwx 1 root root 0 Nov  4 13:12 ffd07000.dsi.0 -> ../../../devices/platform/soc/ffd00000.bus/ffd07000.dsi/ffd07000.dsi.0
--- /sys/bus/mipi-dsi/devices/ffd07000.dsi.0 ---
/sys/bus/mipi-dsi/drivers/panel-c10h-jd9366tc
===== 3. 面板初始化日志 =====
[    0.000000] Kernel command line: root=UUID=ROOT_FS_UUID rootflags=compress=zstd:1 rw rootwait rootfstype=btrfs console=ttyAML0,115200n8 console=tty0 no_console_suspend consoleblank=0 fsck.fix=yes fsck.repair=yes net.ifnames=0 max_loop=128 cgroup_enable=cpuset cgroup_memory=1 cgroup_enable=memory swapaccount=1 video=HDMI-A-1:1920x1080@60e plymouth.enable=0 initcall_blacklist=meson_dw_mipi_dsi_platform_driver_init
[    0.000000] blacklisting initcall meson_dw_mipi_dsi_platform_driver_init
[    0.044117] /soc/vpu@ff900000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.044216] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.044254] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/vpu@ff900000
[    0.044299] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.053880] /soc/vpu@ff900000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.054154] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.054195] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/vpu@ff900000
[    0.054360] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.055198] /soc/vpu@ff900000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.055268] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.055310] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/vpu@ff900000
[    0.055522] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    2.953459] initcall meson_dw_mipi_dsi_platform_driver_init blacklisted
[   11.293533] c10h_dsi_host: loading out-of-tree module taints kernel.
[   11.327682] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[   11.343080] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
===== 4. DRM工具 =====
modetest未安装
drm_info未安装
test-user@device:/$ echo "===== 1. 安装DRM测试工具 ====="
if ! command -v modetest >/dev/null; then
    sudo apt-get install -y libdrm-tests
fi

echo "===== 2. 查询显示连接器 ====="
sudo modetest -M meson -c

echo "===== 3. 查询显示编码器 ====="
sudo modetest -M meson -e

echo "===== 4. 查询CRTC和显示平面 ====="
sudo modetest -M meson -p

echo "===== 5. 再次检查内屏显示模式 ====="
cat /sys/class/drm/card0-DSI-1/modes
===== 1. 安装DRM测试工具 =====
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
Some packages could not be installed. This may mean that you have
requested an impossible situation or if you are using the unstable
distribution that some required packages have not yet been created
or been moved out of Incoming.
The following information may help to resolve the situation:

The following packages have unmet dependencies:
 libdrm-tests : Depends: libdrm2 (= 2.4.114-1+b1) but 2.4.123-1~bpo12+1 is to be installed
E: Unable to correct problems, you have held broken packages.
===== 2. 查询显示连接器 =====
sudo: modetest: command not found
===== 3. 查询显示编码器 =====
sudo: modetest: command not found
===== 4. 查询CRTC和显示平面 =====
sudo: modetest: command not found
===== 5. 再次检查内屏显示模式 =====
test-user@device:/$ echo "===== 1. 确认当前libdrm版本 ====="
dpkg-query -W -f='${Package} ${Version}\n' libdrm2

echo "===== 2. 下载匹配版本的工具 ====="
WORK=/tmp/c10h-modetest
mkdir -p "$WORK"
cd "$WORK" || exit 1

apt download 'libdrm-tests=2.4.123-1~bpo12+1'

echo "===== 3. 提取工具，不安装软件包 ====="
if ls libdrm-tests_*_arm64.deb >/dev/null 2>&1; then
    dpkg-deb -x libdrm-tests_*_arm64.deb extracted
else
    echo "下载失败，请提供 apt-cache policy libdrm-tests 的输出"
fi

echo "===== 4. 检查运行依赖 ====="
if [ -x "$WORK/extracted/usr/bin/modetest" ]; then
    ldd "$WORK/extracted/usr/bin/modetest"

    echo "===== 5. 查询DRM连接器与模式 ====="
    sudo "$WORK/extracted/usr/bin/modetest" -M meson -c

    echo "===== 6. 查询编码器 ====="
    sudo "$WORK/extracted/usr/bin/modetest" -M meson -e
fi
===== 1. 确认当前libdrm版本 =====
libdrm2 2.4.123-1~bpo12+1
===== 2. 下载匹配版本的工具 =====
E: Version '2.4.123-1~bpo12+1' for 'libdrm-tests' was not found
===== 3. 提取工具，不安装软件包 =====
下载失败，请提供 apt-cache policy libdrm-tests 的输出
===== 4. 检查运行依赖 =====
test-user@device:/tmp/c10h-modetest$
test-user@device:/tmp/c10h-modetest$ set -e

WORK=/tmp/c10h-modetest
mkdir -p "$WORK"

URL='https://mirrors.edge.kernel.org/debian/pool/main/libd/libdrm/libdrm-tests_2.4.123-1~bpo12%2B1_arm64.deb'
DEB="$WORK/libdrm-tests.deb"

echo "===== 1. 下载官方ARM64软件包 ====="
if command -v curl >/dev/null; then
    curl -fL --retry 2 "$URL" -o "$DEB"
else
    wget -O "$DEB" "$URL"
fi

echo "===== 2. 验证版本 ====="
dpkg-deb -f "$DEB" Package Version Architecture

echo "===== 3. 提取工具（不安装） ====="
dpkg-deb -x "$DEB" "$WORK/extracted"

BIN="$WORK/extracted/usr/bin/modetest"
test -x "$BIN"

echo "===== 4. 检查运行依赖 ====="
ldd "$BIN"

echo "===== 5. 检查内屏连接器 ====="
sudo "$BIN" -M meson -c

echo "===== 6. 检查显示编码器 ====="
sudo "$BIN" -M meson -e
===== 1. 下载官方ARM64软件包 =====
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100 69240  100 69240    0     0  25674      0  0:00:02  0:00:02 --:--:-- 25672
===== 2. 验证版本 =====
Package: libdrm-tests
Version: 2.4.123-1~bpo12+1
Architecture: arm64
===== 3. 提取工具（不安装） =====
===== 4. 检查运行依赖 =====
        linux-vdso.so.1 (0x0000ffff8156c000)
        libdrm.so.2 => /lib/aarch64-linux-gnu/libdrm.so.2 (0x0000ffff814b0000)
        libc.so.6 => /lib/aarch64-linux-gnu/libc.so.6 (0x0000ffff81300000)
        /lib/ld-linux-aarch64.so.1 (0x0000ffff81530000)
===== 5. 检查内屏连接器 =====
Connectors:
id      encoder status          name            size (mm)       modes   encoders
33      0       unknown Composite-1     0x0             2       32
  modes:
        index name refresh (Hz) hdisp hss hse htot vdisp vss vse vtot
  #0 720x576i 50.00 720 732 795 864 576 580 586 625 13500 flags: interlace; type: driver
  #1 720x480i 59.94 720 739 801 858 480 488 494 525 13500 flags: interlace; type: driver
  props:
        1 EDID:
                flags: immutable blob
                blobs:

                value:
        2 DPMS:
                flags: enum
                enums: On=0 Standby=1 Suspend=2 Off=3
                value: 0
        5 link-status:
                flags: enum
                enums: Good=0 Bad=1
                value: 0
        6 non-desktop:
                flags: immutable range
                values: 0 1
                value: 0
        4 TILE:
                flags: immutable blob
                blobs:

                value:
35      34      connected       HDMI-A-1        0x0             5       34
  modes:
        index name refresh (Hz) hdisp hss hse htot vdisp vss vse vtot
  #0 1920x1080 59.99 1920 2040 2248 2576 1080 1081 1084 1118 172780 flags: nhsync, pvsync; type: userdef
  #1 1024x768 60.00 1024 1048 1184 1344 768 771 777 806 65000 flags: nhsync, nvsync; type: driver
  #2 800x600 60.32 800 840 968 1056 600 601 605 628 40000 flags: phsync, pvsync; type: driver
  #3 800x600 56.25 800 824 896 1024 600 601 603 625 36000 flags: phsync, pvsync; type: driver
  #4 848x480 60.00 848 864 976 1088 480 486 494 517 33750 flags: phsync, pvsync; type: driver
  props:
        1 EDID:
                flags: immutable blob
                blobs:

                value:
        2 DPMS:
                flags: enum
                enums: On=0 Standby=1 Suspend=2 Off=3
                value: 0
        5 link-status:
                flags: enum
                enums: Good=0 Bad=1
                value: 0
        6 non-desktop:
                flags: immutable range
                values: 0 1
                value: 0
        4 TILE:
                flags: immutable blob
                blobs:

                value:
        7 HDR_OUTPUT_METADATA:
                flags: blob
                blobs:

                value:
        36 max bpc:
                flags: range
                values: 8 8
                value: 0
38      0       connected       DSI-1           0x0             1       37
  modes:
        index name refresh (Hz) hdisp hss hse htot vdisp vss vse vtot
  #0 800x1280 60.00 800 840 848 876 1280 1420 1428 1448 76107 flags: nhsync, nvsync; type: preferred, driver
  props:
        1 EDID:
                flags: immutable blob
                blobs:

                value:
        2 DPMS:
                flags: enum
                enums: On=0 Standby=1 Suspend=2 Off=3
                value: 0
        5 link-status:
                flags: enum
                enums: Good=0 Bad=1
                value: 0
        6 non-desktop:
                flags: immutable range
                values: 0 1
                value: 0
        4 TILE:
                flags: immutable blob
                blobs:

                value:

===== 6. 检查显示编码器 =====
Encoders:
id      crtc    type    possible crtcs  possible clones
32      0       TVDAC   0x00000001      0x00000001
34      45      TMDS    0x00000001      0x00000002
37      0       DSI     0x00000001      0x00000004

test-user@device:/tmp/c10h-modetest$ BIN=/tmp/c10h-modetest/extracted/usr/bin/modetest

test -x "$BIN" || {
    echo "modetest文件不存在"
    exit 1
}

echo "===== 记录当前显示状态 ====="
sudo cat /sys/kernel/debug/dri/0/state \
  > /tmp/c10h-drm-before.txt

echo "===== 临时测试内屏（最长12秒） ====="
sudo timeout -s TERM -k 3s 12s \
  "$BIN" -M meson -s 38@45:800x1280-60

echo "===== 测试后的显示状态 ====="
sudo cat /sys/kernel/debug/dri/0/state \
  | grep -E 'crtc=|active=|mode:'

echo "===== 内屏初始化日志 ====="
sudo dmesg | grep -Ei \
'c10h|jd9366|dsi|panel|clock|phy|fail|error' \
| tail -80
===== 记录当前显示状态 =====
===== 临时测试内屏（最长12秒） =====
setting mode 800x1280-60.00Hz on connectors 38, crtc 45
Connection to C10H_IP closed.
PS <USERDIR>> ssh test-user@C10H_IP
[交互式凭据提示已脱敏]
Linux c10h 6.12.41-trim #219 SMP PREEMPT Thu Feb 12 01:38:06 UTC 2026 aarch64

The programs included with the Debian GNU/Linux system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Debian GNU/Linux comes with ABSOLUTELY NO WARRANTY, to the extent
permitted by applicable law.
[登录时间与来源已脱敏]
Could not chdir to home directory /home/test-user: No such file or directory
test-user@device:/$ echo "===== DSI实际时钟 ====="
sudo dmesg | grep -Ei \
'C10H DSI|pixel_target|configured_hs|bit.clock|lane|phy'

echo "===== 面板初始化 ====="
sudo dmesg | grep -Ei \
'C10H stock|jd9366|panel|initialization complete'

echo "===== DRM模式设置错误 ====="
sudo dmesg | grep -Ei \
'meson-drm|kms|venc|encl|fifo|gamma|fail|error' \
| tail -60

echo "===== 当前显示状态 ====="
sudo cat /sys/kernel/debug/dri/0/state \
| grep -E 'crtc=|active=|mode:|fb='
===== DSI实际时钟 =====
[交互式凭据提示已脱敏]
[    0.000000] Booting Linux on physical CPU 0x0000000000 [0x411fd050]
[    0.000000] arch_timer: cp15 timer(s) running at 24.00MHz (phys).
[    3.263484] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    3.423484] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    3.667815] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    3.716829] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    3.775881] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.099556] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.483535] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.549357] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.635541] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.731506] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.779513] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.828630] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.885315] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    4.952226] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.080582] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.132558] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.188044] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.243892] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.307507] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.363950] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.440680] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.491543] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.544340] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.595576] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.648338] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.699895] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.751892] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.804646] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.855501] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.908697] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    5.959515] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.011513] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.067516] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.123520] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.179523] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.235519] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.287550] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.347511] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.403512] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.463611] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.511756] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.567565] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.624730] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.680320] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.735522] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.791572] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.911517] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    6.963529] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.016301] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.071540] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.123556] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.180313] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.231516] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.288583] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.375592] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.427746] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.483564] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.535911] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.587506] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.643574] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.700757] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.755586] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.807985] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.867559] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.923598] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    7.975530] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.027545] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.079554] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.131546] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.195534] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.259513] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.339518] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.436235] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.487592] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.543551] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.595591] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.647536] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.699830] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.751534] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.803564] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.855592] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.911549] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    8.959524] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.063509] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.163501] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.271492] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.371525] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.463530] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.591613] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.675528] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.755500] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.859495] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[    9.963495] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.019634] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.067516] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.127546] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.199503] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.279488] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.351834] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.415819] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.476623] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.543540] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.595522] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.695508] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.813783] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.871562] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.923566] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   10.983569] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   11.071561] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   11.147555] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   11.219543] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   11.315538] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   11.423586] meson-dw-hdmi ff600000.hdmi-tx: Detected HDMI TX controller v2.01a with HDCP (meson_dw_hdmi_phy)
[   18.500141] meson8b-dwmac ff3f0000.ethernet eth0: PHY [mdio_mux-0.1:08] driver [Meson G12A Internal PHY] (irq=21)
[   18.527461] meson8b-dwmac ff3f0000.ethernet eth0: configuring for phy/rmii link mode
[ 1189.100652] c10h-dsi-host ffd07000.dsi: C10H DSI requested=456642000 actual=531000000 pixel_target=76107000
[ 1189.101441] c10h-dsi-host ffd07000.dsi: C10H DSI final bit=531000000 px=75857143 configured_hs=531000000
===== 面板初始化 =====
[    0.044216] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.044299] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.054154] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.054360] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.055268] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.055522] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[   11.327682] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[   11.343080] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[ 1189.737245] panel-c10h-jd9366tc ffd07000.dsi.0: C10H stock panel initialization complete
===== DRM模式设置错误 =====
[    8.362524] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.448767] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.502350] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.556363] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.608279] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.660064] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.712264] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.764040] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.816191] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.868091] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.924032] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    8.996119] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.098853] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.202757] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.306494] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.398946] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.510420] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.604077] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.702489] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.791888] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.895106] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[    9.983409] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.032096] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.081959] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.151912] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.233154] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.310459] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.344979] systemd[1]: Listening on dm-event.socket - Device-mapper event daemon FIFOs.
[   10.370421] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.428256] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.497654] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.559282] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.644927] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.733633] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.826179] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.895974] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   10.936109] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.018628] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.096091] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.165705] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.255637] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.357372] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.411590] systemd[1]: trim_wayland.service: Failed with result 'exit-code'.
[   11.428331] systemd[1]: trim_miniscreen.service: Failed with result 'exit-code'.
[   11.461049] meson-drm ff900000.vpu: bound ff600000.hdmi-tx (ops meson_dw_hdmi_ops)
[   11.731805] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   12.060980] meson-drm ff900000.vpu: [drm] fb0: mesondrmfb frame buffer device
[   12.339817] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   12.582080] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   12.846469] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   41.094570] meson-drm ff900000.vpu: [drm] User-defined mode not supported: "1920x1080": 60 148500 1920 2008 2052 2200 1080 1084 1089 1125 0x68 0x5
[   41.094625] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   41.111963] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[   61.755653] meson-drm ff900000.vpu: [drm] kms: can't enable cloning when we probably wanted to.
[ 1189.747675] meson_encl_set_gamma_table: GAMMA WR_RDY timeout
[ 1192.061141] meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
[ 1192.071180] meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
[ 1194.645088] meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
[ 1194.655121] meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
[ 1197.229203] meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
===== 当前显示状态 =====
        crtc=meson_crtc
        fb=47
        crtc=(null)
        fb=0
        active=1
        self_refresh_active=0
        mode: "1920x1080": 60 172780 1920 2040 2248 2576 1080 1081 1084 1118 0x20 0x6
        crtc=(null)
        crtc=meson_crtc
        crtc=(null)
test-user@device:/$

```

---

*整理说明：此文件是在既有完整整理上追加后续对话、实测与文件验证。旧章节保留历史现场的时序，遇到“最新”冲突时以第十二章为准。它不是已经交付“LCD 正常显示”的完成报告。*
