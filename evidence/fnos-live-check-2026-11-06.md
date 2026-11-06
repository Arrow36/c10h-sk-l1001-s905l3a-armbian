# fnOS 实机只读核查（2026-11-06）

记录时间：2026-11-06，Asia/Shanghai；电脑侧时间核对约为 19:55（UTC+08:00）。设备地址由使用者提供，本次经局域网 SSH 成功登录。账号与密码不写入报告。设备自身显示 2026-11-05，启动 service 的绝对时间还显示 2025-06-26，故以下以本次会话、`-b` 和相对开机时间为准，不据此虚构冷启动日期。

## 实测结果

| 项目 | 本次结果 | 边界 |
| --- | --- | --- |
| 系统 | `6.12.41-trim #219`，AArch64，Debian 12；ophub 元数据指向 fnnas | 当前 fnOS 应用精确版本号未独立查询，不由旧文直接补填 |
| USB 根文件系统 | `/dev/sda2` Btrfs，`/boot` 为 `/dev/sda1` vfat | 同时看到 `/vol1` Btrfs 挂载；未做存储读写/恢复/性能测试 |
| Ethernet | `carrier=1`、100 Mbps、full duplex；SSH 成功；sudo ping 电脑 3 次均响应 | 普通用户 ping 因 raw-socket 权限失败，sudo 后成功，不是链路故障；未测互联网/DNS或吞吐 |
| 管理页 | 电脑经局域网 GET 5666 返回 HTTP 200；80 最终跳转至 5666 后返回 200 | 仅首页可访问，未登录管理 API或验收 NAS 工作负载 |
| 内屏 | `DSI-1 enabled`、CRTC active；主图层 XR24/LINEAR、800×1280、pitch 3200、size 4096000 | 没有重新提交测试图或修改模式 |
| 实屏反馈 | 使用者在本轮明确确认“显示正常，无可见异常” | 支持当次画面完整、颜色正常、无可见折叠/抖动；不是量化色差或长时可靠性测量 |
| 方向与背光 | fbcon 当前 rotate=3；启动参数含 `fbcon=rotate:3`；背光 4/7、bl_power=0 | 原生 DRM 仍为 800×1280；只证明控制台方向 |
| 设备树 | DSI okay；HDMI、SD与 eMMC 节点 disabled；model 为借用的 E900V22C | 是当前运行树状态，不保证 U-Boot 阶段无写入 |
| 模块 | Panel/Host 为 live，vermagic 匹配；启动日志含 `hardware MMIO fix complete` | 支持运行驱动包含内嵌 MMIO 功能，但该日志仍受既有错误处理缺陷限制；未读回寄存器，源码精确映射未证明 |
| 修复服务 | 两服务 enabled、active/exited，ExecMainStatus=0；本次启动均有 Starting/Finished 日志 | 实际 unit 与仓库模板不同；旋转 unit 还存在下述参数拆分问题，退出 0 不代表完成预期写入 |
| 当前警告 | 当前采集的显示日志未见 Gamma timeout/underflow 匹配输出 | 不能由抽取日志无匹配推定所有显示问题已不存在 |
| Armbian | 未测试 | 本轮是 fnOS，不能升级 Armbian 内屏状态 |

## 实际配置与仓库的差异

设备上的两个 service 使用 `DefaultDependencies=no`、`WantedBy=sysinit.target`、`Before=basic.target sysinit.target shutdown.target`，仓库模板主要使用 `multi-user.target`。应把本机成功状态与本机 unit 一起保存，不能宣称仓库当前模板已经原样实测。

旋转 service 的实际配置行为已通过 systemd D-Bus 的结构化参数数组核对：

```text
argv[0] = /bin/sh
argv[1] = -c
argv[2] = echo
argv[3] = 3
argv[4] = >
argv[5] = /sys/class/graphics/fbcon/rotate_all
```

因此 `sh -c` 的命令字符串只有 `echo`，其余是位置参数，不执行预期的 `echo 3 > ...` 重定向。当前 rotate=3 与已有内核启动参数相符，不能把横屏效果归因于这个 service。本轮只记录问题，没有修复 unit 或重新运行它。仓库服务文件原有带引号的写法与设备不同，不应把此问题直接算作仓库模板同样有缺陷。

修复工具与旋转服务按 systemd 记录的进程持续时间分别约为 50.975 ms、56.542 ms；这不是纯 MMIO 算法耗时，也不是开机总成本。不能据此确认或反驳旧记录中另一测量条件下的 4 ms。Panel 初始化日志约在开机 13.064 s，fb0 注册约 13.187 s，两个 service 随后执行；两条修复路径本次均存在，无法隔离各自贡献。

## 版本指纹与复现边界

下方保存当前磁盘模块、工具、DTB和两个二进制脚本 SHA-256。当前模块哈希与 11 月 4 日记录中的旧哈希不同，只说明文件字节不同，不能单独证明精确源码提交。模块在 sysfs 为 live，但本次没有取得可比较的已加载 build-id 内容，未证明磁盘文件自加载以来从未替换；精确源码/构建映射仍待补齐。

本次未读取 `/dev/mem`、未运行修复工具、未改寄存器、未写 framebuffer、未加载/卸载模块、未改 service/DTB/启动参数、未重启或刷写。没有编译当前仓库源码，也没有进行冷启动、长时负载或 Armbian 测试。

采集时 uptime 约 1 小时，表示设备本轮已运行到该时间点，不表示本次进行了 1 小时连续观察。实屏正常由使用者现场确认，SSH 输出不能代替这一目视证据。

## 脱敏采集输出

UUID、MAC及存储卷实例名已替换；仅去除输出行末空格，其他内容保留。以下为命令 stdout/stderr，非推测补写；设备绝对时间按原样保留。首次非提权 DRM 通配符未读到输出，随后用固定路径 `sudo cat /sys/kernel/debug/dri/0/state` 成功取得，下方为成功采集。

### system

返回码：`0`。

```text
Linux c10h 6.12.41-trim #219 SMP PREEMPT Thu Feb 12 01:38:06 UTC 2026 aarch64 GNU/Linux
PRETTY_NAME="Debian GNU/Linux 12 (bookworm)"
NAME="Debian GNU/Linux"
VERSION_ID="12"
VERSION="12 (bookworm)"
VERSION_CODENAME=bookworm
ID=debian
HOME_URL="https://www.debian.org/"
SUPPORT_URL="https://www.debian.org/support"
BUG_REPORT_URL="https://bugs.debian.org/"

FILE /etc/ophub-release
PLATFORM='amlogic'
VERSION_CODEID=''
VERSION_CODENAME=''
MODEL_ID='304'
MODEL_NAME='[IDL],E900V22C-D,CM311-1a-CH,IP112H,B863AV3.1'
SOC='s905l3a'
FDTFILE='meson-g12a-s905l3a-e900v22c.dtb'
FAMILY='meson-g12a'
BOARD='s905l3a'
KERNEL_REPO='ophub/fnnas'
KERNEL_TAGS='kernel_fnnas'
KERNEL_VERSION='6.12.41'
KERNEL_BACKUP='yes'
BOOT_CONF='uEnv.txt'
ROOTFS_TYPE='btrfs'
DISK_TYPE='usb'
AMPART_STATUS='no'
MLUBOOT_STATUS='no'
MAINLINE_UBOOT=''
BOOTLOADER_IMG=''
FNNAS_WEBSITE='fnnas.com'
REBUILD_REPOSITORY='github.com/ophub/fnnas'
CONTRIBUTORS='Calmact,KryptonLee,yu6zhou,orientzane,AboutCXJ'
BUILDER_NAME=[REDACTED]
PACKAGED_DATE='2026-09-23'
UBOOT_OVERLOAD='u-boot-e900v22c.bin'
uid=1000(test-user) gid=1001(Users) groups=1001(Users),1000(Administrators)
 01:44:17 up  1:00,  2 users,  load average: 0.10, 0.09, 0.09
root=UUID=[UUID_REDACTED] rootflags=compress=zstd:1 rw rootwait rootfstype=btrfs console=ttyAML0,115200n8 console=tty0 no_console_suspend consoleblank=0 fsck.fix=yes fsck.repair=yes net.ifnames=0 max_loop=128 cgroup_enable=cpuset cgroup_memory=1 cgroup_enable=memory swapaccount=1 video=HDMI-A-1:1920x1080@60e plymouth.enable=0 initcall_blacklist=meson_dw_mipi_dsi_platform_driver_init fbcon=rotate:3
```

### storage

返回码：`0`。

```text
/dev/sda2 btrfs  rw,noatime,compress=zstd:1,space_cache=v2,subvolid=5,subvol=/
/dev/sda1 vfat
NAME                                              TYPE    SIZE FSTYPE            MOUNTPOINTS
sda                                               disk   29.8G
├─sda1                                            part    511M vfat              /boot
├─sda2                                            part   15.5G btrfs             /
└─sda3                                            part   13.8G linux_raid_member
  └─md127                                         raid1  13.8G LVM2_member
    └─[VOLUME_REDACTED] lvm    13.8G btrfs             /vol1
zram0                                             disk  972.7M                   [SWAP]
               total        used        free      shared  buff/cache   available
Mem:           1.9Gi       755Mi        52Mi        35Mi       1.2Gi       1.2Gi
Swap:          972Mi       512Ki       972Mi
```

### ethernet

返回码：`0`。

```text
carrier=1
speed=100
duplex=full
operstate=up
eth0             UP             192.168.137.2/24
default via 192.168.137.1 dev eth0 proto static metric 100
172.17.0.0/16 dev docker0 proto kernel scope link src 172.17.0.1 linkdown
192.168.137.0/24 dev eth0 proto kernel scope link src 192.168.137.2 metric 100
PING 192.168.137.1 (192.168.137.1) 56(84) bytes of data.
64 bytes from 192.168.137.1: icmp_seq=1 ttl=128 time=1.12 ms
64 bytes from 192.168.137.1: icmp_seq=2 ttl=128 time=1.14 ms
64 bytes from 192.168.137.1: icmp_seq=3 ttl=128 time=1.17 ms

--- 192.168.137.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2003ms
rtt min/avg/max/mdev = 1.119/1.143/1.169/0.020 ms
```

标准错误（保留失败项）：

```text
ping: socktype: SOCK_RAW
ping: socket: Operation not permitted
ping: => missing cap_net_raw+p capability or setuid?
```

### display_sysfs

返回码：`0`。

```text

FILE /sys/class/drm/card0-Composite-1/status
unknown

FILE /sys/class/drm/card0-DSI-1/status
connected

FILE /sys/class/drm/card0-Composite-1/enabled
disabled

FILE /sys/class/drm/card0-DSI-1/enabled
enabled

FILE /sys/class/drm/card0-Composite-1/modes
720x576i
720x480i

FILE /sys/class/drm/card0-DSI-1/modes
800x1280

FILE /sys/class/graphics/fb0/name
mesondrmfb

FILE /sys/class/graphics/fb0/virtual_size
800,1280

FILE /sys/class/graphics/fb0/bits_per_pixel
32

FILE /sys/class/graphics/fbcon/rotate
3

FILE /sys/class/backlight/backlight-c10h/brightness
4

FILE /sys/class/backlight/backlight-c10h/actual_brightness
4

FILE /sys/class/backlight/backlight-c10h/max_brightness
7

FILE /sys/class/backlight/backlight-c10h/bl_power
0
0 mesondrmfb
total 0
drwxr-xr-x  2 root root        100 Nov  5 00:44 by-path
crw-rw----+ 1 root video  226,   0 Jun 26  2025 card0
crw-rw----+ 1 root video  226,   1 Nov  5 00:44 card1
crw-rw----+ 1 root render 226, 128 Nov  5 00:44 renderD128
```

### drm_state

返回码：`0`。

```text
plane[36]: meson_primary_plane
	crtc=meson_crtc
	fb=43
		allocated by = [fbcon]
		refcount=2
		format=XR24 little-endian (0x34325258)
		modifier=0x0
		size=800x1280
		layers:
			size[0]=800x1280
			pitch[0]=3200
			offset[0]=0
			obj[0]:
				name=0
				refcount=3
				start=00100000
				size=4096000
				imported=no
				dma_addr=0x0000000062900000
				vaddr=0000000093c4bd5e
	crtc-pos=800x1280+0+0
	src-pos=800.000000x1280.000000+0.000000+0.000000
	rotation=1
	normalized-zpos=1
	color-encoding=ITU-R BT.601 YCbCr
	color-range=YCbCr limited range
	color_mgmt_changed=0
plane[39]: meson_overlay_plane
	crtc=(null)
	fb=0
	crtc-pos=0x0+0+0
	src-pos=0.000000x0.000000+0.000000+0.000000
	rotation=1
	normalized-zpos=0
	color-encoding=ITU-R BT.601 YCbCr
	color-range=YCbCr limited range
	color_mgmt_changed=0
crtc[42]: meson_crtc
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
	mode: "800x1280": 60 75857 800 840 848 876 1280 1420 1428 1448 0x48 0xa
connector[33]: Composite-1
	crtc=(null)
	self_refresh_aware=0
	max_requested_bpc=0
	colorspace=Default
connector[35]: DSI-1
	crtc=meson_crtc
	self_refresh_aware=0
	max_requested_bpc=0
	colorspace=Default
```

### services

返回码：`0`。

```text
Result=success
ExecMainStartTimestamp=Thu 2025-06-26 22:58:43 CST
ExecMainExitTimestamp=Thu 2025-06-26 22:58:43 CST
ExecMainCode=1
ExecMainStatus=0
Id=c10h-display-fix.service
Before=basic.target sysinit.target shutdown.target
After=systemd-modules-load.service system.slice systemd-journald.socket
LoadState=loaded
ActiveState=active
SubState=exited
FragmentPath=/etc/systemd/system/c10h-display-fix.service
UnitFileState=enabled
ConditionResult=yes

Result=success
ExecMainStartTimestamp=Thu 2025-06-26 22:58:43 CST
ExecMainExitTimestamp=Thu 2025-06-26 22:58:43 CST
ExecMainCode=1
ExecMainStatus=0
Id=c10h-fbcon-rotate.service
Before=sysinit.target shutdown.target basic.target
After=systemd-modules-load.service system.slice systemd-journald.socket
LoadState=loaded
ActiveState=active
SubState=exited
FragmentPath=/etc/systemd/system/c10h-fbcon-rotate.service
UnitFileState=enabled
ConditionResult=yes
# /etc/systemd/system/c10h-display-fix.service
[Unit]
Description=C10H Display Early Hardware Register Fix
DefaultDependencies=no
After=systemd-modules-load.service
Before=basic.target sysinit.target shutdown.target
Conflicts=shutdown.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/c10h-display-fix
RemainAfterExit=yes

[Install]
WantedBy=sysinit.target

# /etc/systemd/system/c10h-fbcon-rotate.service
[Unit]
Description=Rotate C10H framebuffer consoles to landscape
DefaultDependencies=no
After=systemd-modules-load.service
Before=basic.target sysinit.target shutdown.target
Conflicts=shutdown.target
ConditionPathExists=/sys/class/graphics/fbcon/rotate_all

[Service]
Type=oneshot
ExecStart=/bin/sh -c echo 3 > /sys/class/graphics/fbcon/rotate_all
RemainAfterExit=yes

[Install]
WantedBy=sysinit.target
[   13.233029] c10h systemd[1]: Starting c10h-display-fix.service - C10H Display Early Hardware Register Fix...
[   13.239354] c10h systemd[1]: Starting c10h-fbcon-rotate.service - Rotate C10H framebuffer consoles to landscape...
[   13.314756] c10h systemd[1]: Finished c10h-display-fix.service - C10H Display Early Hardware Register Fix.
[   13.315749] c10h systemd[1]: Finished c10h-fbcon-rotate.service - Rotate C10H framebuffer consoles to landscape.
```

### modules

返回码：`0`。

```text
panfrost               77824  0
gpu_sched              40960  1 panfrost
drm_shmem_helper       24576  1 panfrost
panel_c10h_jd9366tc    12288  0
c10h_dsi_host          12288  0
phy_meson_g12a_mipi_dphy_analog    12288  1
phy_meson_axg_mipi_dphy    12288  1
pwm_bl                 16384  0

MODULE c10h_dsi_host
filename:       /lib/modules/6.12.41-trim/extra/c10h-dsi-host.ko
description:    Amlogic Meson MIPI-DSI DRM driver
depends:
name:           c10h_dsi_host
vermagic:       6.12.41-trim SMP preempt mod_unload aarch64
d51340fa87de35a899d6b33d1cd904951cc10822c5f97d78df614459c9489a39  /lib/modules/6.12.41-trim/extra/c10h-dsi-host.ko

MODULE panel_c10h_jd9366tc
filename:       /lib/modules/6.12.41-trim/extra/panel-c10h-jd9366tc.ko
description:    Experimental C10H stock JD9366TC panel
depends:
name:           panel_c10h_jd9366tc
vermagic:       6.12.41-trim SMP preempt mod_unload aarch64
6429e7408a4181392b96fb48e050a62a06a3e2da9e91012f10e03507614d8076  /lib/modules/6.12.41-trim/extra/panel-c10h-jd9366tc.ko
-rwxr-xr-x 1 root root 70608 Nov  4 23:48 /usr/local/bin/c10h-display-fix
713476fe25880ee9c572ca3fc2212c6ed4ee755b37df95cc65f750e9c34f8108  /usr/local/bin/c10h-display-fix
-rw-r--r-- 1 root root 34 Nov  4 12:50 /etc/modules-load.d/c10h-lcd-test.conf
cf7a371ddc1dd74f1098e9fbb34b1d98dc649f6e45d855bea3b31a6e467aaee8  /etc/modules-load.d/c10h-lcd-test.conf
```

### display_logs

返回码：`0`。

```text
[    0.000000] Kernel command line: root=UUID=[UUID_REDACTED] rootflags=compress=zstd:1 rw rootwait rootfstype=btrfs console=ttyAML0,115200n8 console=tty0 no_console_suspend consoleblank=0 fsck.fix=yes fsck.repair=yes net.ifnames=0 max_loop=128 cgroup_enable=cpuset cgroup_memory=1 cgroup_enable=memory swapaccount=1 video=HDMI-A-1:1920x1080@60e plymouth.enable=0 initcall_blacklist=meson_dw_mipi_dsi_platform_driver_init fbcon=rotate:3
[    0.000000] blacklisting initcall meson_dw_mipi_dsi_platform_driver_init
[    0.043937] /soc/vpu@ff900000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.044022] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.044052] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/vpu@ff900000
[    0.044090] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.053228] /soc/vpu@ff900000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.053451] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.053486] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/vpu@ff900000
[    0.053653] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.054438] /soc/vpu@ff900000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    0.054514] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[    0.054548] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/vpu@ff900000
[    0.054750] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[    2.885813] initcall meson_dw_mipi_dsi_platform_driver_init blacklisted
[    3.168328] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    3.281902] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    3.326021] dwc2 ff400000.usb: EPs: 7, dedicated fifos, 712 entries in SPRAM
[    3.468881] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    3.474409] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    3.481110] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    3.898459] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.120232] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.134843] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.149449] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.158321] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.578377] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.677338] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.728718] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    4.748675] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    5.256867] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    6.325717] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[    9.993045] systemd[1]: Hostname set to <c10h>.
[   11.607390] systemd[1]: Listening on dm-event.socket - Device-mapper event daemon FIFOs.
[   12.380071] c10h_dsi_host: loading out-of-tree module taints kernel.
[   12.386901] /soc/bus@ffd00000/dsi@7000: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000/panel@0
[   12.391548] /soc/bus@ffd00000/dsi@7000/panel@0: Fixed dependency cycle(s) with /soc/bus@ffd00000/dsi@7000
[   12.411757] meson-drm ff900000.vpu: HDMI transceiver device is disabled
[   12.427869] c10h-dsi-host ffd07000.dsi: C10H DSI requested=455142000 actual=531000000 pixel_target=75857000
[   12.427927] c10h-dsi-host ffd07000.dsi: C10H DSI final bit=531000000 px=75857143 configured_hs=531000000
[   13.064383] panel-c10h-jd9366tc ffd07000.dsi.0: C10H stock panel initialization & hardware MMIO fix complete
[   13.186917] meson-drm ff900000.vpu: [drm] fb0: mesondrmfb frame buffer device
[   15.473864] panfrost ffe40000.gpu: clock rate = 24000000
[   15.484548] panfrost ffe40000.gpu: mali-g31 id 0x7093 major 0x0 minor 0x0 status 0x0
[   15.495331] panfrost ffe40000.gpu: features: 00000000,000027f7, issues: 00000000,00000400
[   15.507241] panfrost ffe40000.gpu: Features: L2:0x07100206 Shader:0x00000000 Tiler:0x00000209 Mem:0x1 MMU:0x00002821 AS:0xff JS:0x7
[   15.523329] panfrost ffe40000.gpu: shader_present=0x1 l2_present=0x1
[   15.552635] [drm] Initialized panfrost 1.2.0 for ffe40000.gpu on minor 1
```

### kernel_features

返回码：`0`。

```text

FILE /boot/config-6.12.41-trim
CONFIG_DEVMEM=y
CONFIG_DRM_DW_MIPI_DSI=y
CONFIG_DRM_MESON=y
CONFIG_DRM_MESON_DW_MIPI_DSI=y
CONFIG_BACKLIGHT_PWM=m
CONFIG_FRAMEBUFFER_CONSOLE=y
CONFIG_FRAMEBUFFER_CONSOLE_ROTATION=y
CONFIG_PWM_MESON=y
CONFIG_STRICT_DEVMEM=y
/usr/bin/cc
/usr/bin/dtc
/usr/bin/busybox
lrwxrwxrwx 1 root root 35 Sep 23 17:52 /lib/modules/6.12.41-trim/build -> /usr/src/linux-headers-6.12.41-trim
```

### device_tree

返回码：`0`。

```text

FILE /sys/firmware/devicetree/base/model
SKYWORTH E900V22C

FILE /sys/firmware/devicetree/base/compatible
skyworth,e900v22c
amlogic,g12a

FILE /sys/firmware/devicetree/base/soc/mmc@ffe07000/status
disabled

FILE /sys/firmware/devicetree/base/soc/mmc@ffe05000/status
disabled

FILE /sys/firmware/devicetree/base/soc/bus@ffd00000/dsi@7000/status
okay

FILE /sys/firmware/devicetree/base/soc/bus@ff600000/hdmi-tx@0/status
disabled
```

### web_listeners

返回码：`0`。

```text
State  Recv-Q Send-Q Local Address:Port Peer Address:PortProcess
LISTEN 0      511          0.0.0.0:5666      0.0.0.0:*
LISTEN 0      511          0.0.0.0:443       0.0.0.0:*
LISTEN 0      511          0.0.0.0:80        0.0.0.0:*
LISTEN 0      128          0.0.0.0:22        0.0.0.0:*
LISTEN 0      511             [::]:5666         [::]:*
LISTEN 0      511             [::]:443          [::]:*
LISTEN 0      511             [::]:80           [::]:*
LISTEN 0      128             [::]:22           [::]:*
```

### loaded_module_identity

返回码：`0`。

```text

MODULE c10h_dsi_host
initstate=live
taint=O

MODULE panel_c10h_jd9366tc
initstate=live
taint=O
-rwxr-xr-x 1 root root 80260 Nov  4 13:54 /boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb
-rwxr-xr-x 1 root root 80252 Nov  4 12:43 /boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-test.dtb
-rwxr-xr-x 1 root root 79451 Nov  4 20:02 /boot/dtb/amlogic/meson-g12a-s905l3a-c10h-usb-test.dtb
47c90ad6db6393eda195a63173b0025c229b6dab1e14f973ae977f66ce703483  /boot/aml_autoscript
47c90ad6db6393eda195a63173b0025c229b6dab1e14f973ae977f66ce703483  /boot/s905_autoscript
c24b8ef59394f8f5c15c41542082d52daeb48eadc26cadf58be28f8134993b13  /boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb
772c3c8afdd4b3f6220f3706c867766f623f2d3f50aeabb809555053636cee4e  /boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-test.dtb
47f1e6fdf4b2e80b2d31a65fed0eaf1d96f99e3234a6bce7cd054736b6b46450  /boot/dtb/amlogic/meson-g12a-s905l3a-c10h-usb-test.dtb
```

### systemd_monotonic

返回码：`0`。

```text
ExecMainStartTimestampMonotonic=13232334
ExecMainExitTimestampMonotonic=13283309
Id=c10h-display-fix.service

ExecMainStartTimestampMonotonic=13238895
ExecMainExitTimestampMonotonic=13295437
Id=c10h-fbcon-rotate.service
3628.28 13875.05
2026-11-05T01:44:20+08:00
```

### rotation_argv

返回码：`0`。

```text
a(sasbttttuii) 1 "/bin/sh" 6 "/bin/sh" "-c" "echo" "3" ">" "/sys/class/graphics/fbcon/rotate_all" false 1750949923445919 13238895 1750949923502460 13295437 1582 1 0
ExecStart={ path=/bin/sh ; argv[]=/bin/sh -c echo 3 > /sys/class/graphics/fbcon/rotate_all ; ignore_errors=no ; start_time=[Thu 2025-06-26 22:58:43 CST] ; stop_time=[Thu 2025-06-26 22:58:43 CST] ; pid=1582 ; code=exited ; status=0 }
3
```

### loaded_build_ids

返回码：`0`。

```text

MODULE c10h_dsi_host
total 0

Displaying notes found in: .note.gnu.property
  Owner                Data size 	Description
  GNU                  0x00000010	NT_GNU_PROPERTY_TYPE_0
      Properties: AArch64 feature: PAC

Displaying notes found in: .note.gnu.build-id
  Owner                Data size 	Description
  GNU                  0x00000014	NT_GNU_BUILD_ID (unique build ID bitstring)
    Build ID: 35d45a0e680a33779b07909f9606971f9b42b01a

Displaying notes found in: .note.Linux
  Owner                Data size 	Description
  Linux                0x00000004	func
   description data: 00 00 00 00
  Linux                0x00000001	OPEN
   description data: 00

MODULE panel_c10h_jd9366tc
total 0

Displaying notes found in: .note.gnu.property
  Owner                Data size 	Description
  GNU                  0x00000010	NT_GNU_PROPERTY_TYPE_0
      Properties: AArch64 feature: PAC

Displaying notes found in: .note.gnu.build-id
  Owner                Data size 	Description
  GNU                  0x00000014	NT_GNU_BUILD_ID (unique build ID bitstring)
    Build ID: 5c94561a5fc892ac9923a3a321518c7f1a7680ef

Displaying notes found in: .note.Linux
  Owner                Data size 	Description
  Linux                0x00000004	func
   description data: 00 00 00 00
  Linux                0x00000001	OPEN
   description data: 00
```

### 电脑侧 HTTP 访问

```json
[
  {
    "port": 5666,
    "status": 200,
    "content_type": "text/html",
    "bytes_read": 2099,
    "final_url": "http://192.168.137.2:5666/"
  },
  {
    "port": 80,
    "status": 200,
    "content_type": "text/html",
    "bytes_read": 2099,
    "final_url": "http://192.168.137.2:5666/"
  }
]
```

## 后续操作：移除旧旋转服务

本节发生在以上只读核查之后。经设备使用者明确要求，执行 `systemctl disable --now c10h-fbcon-rotate.service`，删除 `/etc/systemd/system/c10h-fbcon-rotate.service` 并 `daemon-reload`。禁用时清除了 sysinit.target 与 multi-user.target 两处启用链接。当前状态核验输出：

```text
LoadState=not-found
ActiveState=inactive
FragmentPath=
UnitFileState=

/sys/class/graphics/fbcon/rotate:
3

/proc/cmdline 中的旋转参数:
fbcon=rotate:3

c10h-display-fix.service:
active
```

未修改启动参数、显示修复服务、驱动、DTB或寄存器，未重启。仓库原有旋转 service 模板保留为历史实现参考，已不属于本机当前部署。上文旧服务状态及参数数组是清理前的现场记录。
