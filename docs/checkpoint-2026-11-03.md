# 2026-11-03 暂停检查点

> **系统范围更正（设备使用者确认）**：fnOS 内屏已实测显示正常；Armbian 内屏尚未测试。下文涉及 Armbian 显示效果或 FbTerm 的历史叙述仅保留作溯源，不作为已测试结论。

> **历史归档**：以下“当前”“最新”“下一步”和路径均指记录日期的实验环境。后续阶段性结果与验证边界见 [11 月 6 日记录](research/fnos-display-resolution-record-2026-11-06.md)。保留历史日志和现场反馈原意。

调试已按使用者要求暂停，本次提交只同步研究进展，不表示驱动已可日用。

## 暂停时设备状态

- U 盘原生 DRM 测试系统，6.12.114-ophub / Debian Trixie。内置 eMMC 在测试 DTB 中禁用；没有写入内部 Android 或保存 U-Boot 环境。
- 仅面板、实际位时钟 DSI 主机、ENCL 诊断模块加载；旧的手工 OSD/HDR/FIFO/VE 补丁已卸载。
- 原生 `drm-geometry` 用户态程序仍持有显示缓冲区；画面边框完整，但底色不正确且仍抖动。没有为此程序配置开机自启。
- ENCL pattern=0（普通图像），HLINE=767（临时无效实验值），线性 Gamma 曾晚加载。未继续改动或重启设备。
- 专用 DRM 启动带 `c10h.drm_stage1=1`，自动彩条 unit 已修正并启用；下次同样启动会先自动显示彩条。
- 有线 SSH 可用；DHCP、启动联网和 DNS 仍有间歇性问题。

## 本次已验证的退出/回退方式

以下路径只对应当前测试设备，操作前先确认模块和文件实际存在；不应在其他板上照搬。

1. 临时回到已验证的硬件彩条：向 `/sys/module/c10h_lcd_diagnostic/parameters/pattern` 写入 `1`。这仅验收后半段输出。
2. 结束原生缓冲区探针：向 `/root/c10h-screen-work/drm-stage1/geometry.pid` 中记录的仍存活探针进程发送 SIGTERM。程序会恢复自己保存的原 framebuffer/plane 属性。避免直接杀死不匹配的 PID。
3. 本次 fbcon 已解绑；路径保存在 `/boot/c10h-drm-stage1/vtconsole-path.txt`。只有需要回到控制台时再把对应 bind 写回 1。控制台仍可能抖动。
4. ENCL 诊断模块会在卸载时恢复自己保存的测试相关寄存器及 HLINE 原值；Gamma 表不是完整快照，卸载不会还原整张 Gamma 表。程序仍运行时不要在线拆卸 DRM 主机或面板驱动。

## USB 启动回退

原来的四份已知可启动文件保存在 U 盘 FAT 的 `/boot/c10h-known-good/`（Linux 路径）；另有 `/root/c10h-screen-work/drm-stage1/before-boot/` 保存进入本次测试前的四份脚本和模块加载配置。

基线分为：

- 初始已知可用 HDMI/有线 Linux：二进制启动脚本 SHA-256 `84df31d551aa90b161e10896d986e6261a47f59d3a236c9544faad440b4062f4`。
- 保留原厂屏幕状态的实验 V1：脚本 SHA-256 `78684a4394813917e40b13a5c5a7269903308abb06503fba981ca2cb63c803cc`，不是普通 LCD 图像已正常的版本。
- 当前自动 DRM 彩条测试：脚本 SHA-256 `c072df2b935a0d9dad33312a0dc014487f848251769c4eab641b8c050495d504`。

回退时应核对 U 盘身份、DTB 与脚本配套，将 `aml_autoscript`、`aml_autoscript.cmd`、`s905_autoscript`、`s905_autoscript.cmd` 四份一起恢复并同步。正常断电再通电曾能进入 Android，再经 root ADB 恢复 FAT 文件；这是已用过的恢复路径，不是完整救砖保证。

原始快照、启动日志、构建产物和原厂分区备份保留在本地，未上传公共仓库。仓库中的 `.example` 使用 UUID 占位符，不对应上述实际脚本哈希。
