# 源码与运行配置静态审计

基准：`13f3b88cddffd091862295e323c63cf1603bbabf`，2026-11-06。本轮只修改 Markdown，所有源码、寄存器值、DTS、启动文件和 systemd 单元保持原样。没有执行构建、加载模块、访问 MMIO 或操作设备；以下为静态发现，建议需另立代码变更并测试。

## 已确认的实现问题与限制

| 编号 / 影响 | 位置与依据 | 发现 | 建议（本轮未实施） |
| --- | --- | --- | --- |
| C1 / 高：失败可能显示成功 | [MMIO 工具 main](../tools/c10h-display-fix.c) 的两个 `mmap()` 分支及末尾 `return 0` | `/dev/mem` 打开失败返回 1；VPU/DSI 任一映射失败仅 `perror`，最终仍返回 0；若另一映射成功还会部分写入。`if (ptr && ptr != MAP_FAILED)` 额外把可能的零地址映射当失败，判定不等于只检查 MAP_FAILED | 传播失败、定义部分成功策略、保存 errno 并清理资源；故障注入验证每条路径。服务成功退出不能作为完成所有写入的证据 |
| C2 / 高：驱动成功日志与写入不一致 | [面板 c10h_prepare](../drivers/c10h-display/panel-c10h-jd9366tc.c) | 两次 `ioremap` 失败都跳过对应写入，但仍输出 `hardware MMIO fix complete` 并返回 0；前面 DCS/GPIO 错误则走 fail，错误语义不一致 | 单独报告映射/应用结果，确定失败时是否回滚；当前日志不证明完整配置生效 |
| C3 / 高：固定地址整值 MMIO 缺少协作 | [工具](../tools/c10h-display-fix.c)、[面板驱动](../drivers/c10h-display/panel-c10h-jd9366tc.c)、[Host 初始化](../drivers/c10h-display/c10h-dsi-host.c) | 工具无板型/模式检查、原值快照或回读；面板直接映射 VPU/DSI，未通过拥有这些资源的 DRM/Host 接口同步。工具、面板、Host 可重复修改相同寄存器，可能被后续初始化覆盖；实际先后需运行证据 | 按资源所有权和 DRM 生命周期设计修复，核对模式/版本、掩码与回读，单独验证升级/重设模式。这里只记录潜在影响，不断言现场已发生竞争 |
| C4 / 中：0x1DF5 的说明与定义冲突 | [工具注释](../tools/c10h-display-fix.c)、[面板注释](../drivers/c10h-display/panel-c10h-jd9366tc.c)、[meson_registers.h](../drivers/c10h-display/meson_registers.h) | 两处注释把 `0x1DF5` 写成 VPP OFIFO，头文件定义为 `VPP_OSD1_BLD_H_SCOPE`；真正 `VPP_OFIFO_SIZE` 是 `0x1D27`。因此“深度 1919/防欠载”的原因解释没有该定义支撑 | 先核对 S905L3A 手册及厂商代码；当前仅纠正文档并保留实验值，未改源码注释或把写入重定向到其他寄存器 |
| C5 / 中：自启就绪与服务状态不足 | [修复服务](../systemd/c10h-display-fix.service)、[旋转服务](../systemd/c10h-fbcon-rotate.service) | 修复服务只有模块加载排序，没有显示硬件就绪探测；Before=multi-user 也不保证所有登录/显示初始化之前应用。oneshot+RemainAfterExit 不自动持续监控、回读或跨 modeset 重写；旋转服务路径条件失败可能跳过 | 保存 enabled/status/journal、寄存器与实屏证据；研究合适的驱动生命周期，避免把 active/exited 当硬件验收 |
| C6 / 中：几何探针恢复失败仍可返回成功 | [drm-geometry main/apply/fail](../experiments/display/drm-geometry.c) | 正常信号退出尝试恢复旧 plane；恢复失败仅 perror，后续清理返回 0。fail() 直接 exit，不走统一恢复；崩溃/SIGKILL 不保证恢复。信号在条件检查与 pause 之间到达存在丢失唤醒窗口 | 统一退出/恢复错误传播；用无竞争的信号等待；验证恢复失败和超时路径，保留备用 SSH/启动回退 |
| C7 / 中：诊断的“可恢复”有范围 | [c10h-lcd-diagnostic.c](../drivers/c10h-display/c10h-lcd-diagnostic.c) 的 gamma_set/exit | Gamma 写表没有保存原表，超时后仅恢复控制寄存器，无法回滚已写入的表项；卸载恢复有限寄存器，DSI 重映射失败时跳过还原。硬件彩条写入不是纯只读 | 限定恢复承诺，保存适当状态并验证错误分支；不要依赖卸载恢复任意显示状态 |
| C8 / 中：Canvas 探针不是零写入 | [c10h-bootfb-info.c](../experiments/display/c10h-bootfb-info.c) | 为读取 LUT 向 `canvas+8` 写 RD_EN/索引，未与活动 Canvas 驱动锁协作；会读取固定地址，不验证当前板型或显示电源状态 | 文档明确有读选择写入及并发访问边界；不要作为普适无副作用工具自动加载 |
| C9 / 中：启动与 DTB 工具的范围限制 | [disable_emmc.py](../tools/disable_emmc.py)、[启动脚本](../boot/aml_autoscript.cmd)、[面板 overlay](../dts/c10h-panel-test.dtso) | DTB 工具依赖固定节点/原 status，用 assert 验证而非完整 FDT 格式校验；Python -O 会跳过断言。脚本固定 UUID/ext4/HDMI，不读取 uEnv。overlay 不禁用 eMMC/simplefb。脚本不 saveenv 不能限制原厂 update 逻辑 | 将工具限定于匹配基线；独立校验最终 DTB/启动脚本，不将其用于通用刷写或写保护保证 |

## 需结合完整内核进一步核查的候选问题

| 编号 | 位置 | 静态线索与限制 |
| --- | --- | --- |
| C10 / 候选 | [DSI Host dw_mipi_dsi_phy_init](../drivers/c10h-display/c10h-dsi-host.c) | `clk_rate_exclusive_get()` 成功后，像素时钟设置/使能或 `phy_configure()` 失败分支没有本地 exclusive_put/恢复；正常 power_off 才 put。需结合目标 DW-MIPI-DSI 调用方错误清理确认是否失衡，不能仅凭局部函数宣称运行时已泄漏 |
| C11 / 候选 | 同文件 host_attach/phy_init、get_lane_mbps | format switch 无 default，未知格式可能进入未定义处理；当前 Panel 固定 RGB888，未证明在本机正常路径可触发。`phy_mipi_dphy_get_default_config()` 首次调用未检查返回值，后续 helper 有检查；建议结合目标内核 API 和错误传播审查 |

## 其他配置与构建核对

- `drivers/c10h-display/Makefile` 构建 Panel、Host、ENCL 三个模块；`experiments/display/Makefile` 只构建 bootfb 内核探针。libdrm 几何图和 MMIO 用户程序需独立编译，没有 DKMS/通用安装器。
- 面板源码 `.clock=75857`，早期记录 `76107`；`fbcon/rotate_all=3` 只旋转控制台，不改变 800×1280 模式；不能混用不同提交的参数和运行日志。
- `c10h-drm-stage1-bars.service` 和脚本依赖 `/root/c10h-screen-work/` 及指定 `.ko`，仓库位置不等于安装位置；脚本等待 5 秒也不是显示就绪保证。
- 取证脚本确实写 `/boot/c10h-linux-proof`，不能称“完全不写磁盘”；旧摘录保留其 CRLF 错误，仓库 LF 设置只说明当前格式，不能补造修复后的成功日志。
- Wi-Fi PCIe overlay 已标 FAILED/UNVALIDATED，修改共享 USB PHY 与 GPIO；保留其失败归档，不作为可安装无线方案。

## 审计参考与边界

寄存器命名与 Linux [v6.12.41 头文件](https://github.com/gregkh/linux/blob/v6.12.41/drivers/gpu/drm/meson/meson_registers.h)交叉核对；服务顺序语义参考 [systemd 官方单元说明](https://github.com/systemd/systemd/blob/v257/man/systemd.unit.xml)。未审计完整上游内核、原厂 U-Boot、未公开固件和运行时调用链，因此本报告不作“所有代码无其他问题”的承诺，也不以静态问题否定 fnOS 已确认的正常画面。

## 现场配置补充（fnOS，本轮后续）

[实机核查](../evidence/fnos-live-check-2026-11-06.md)确认设备上的修复与旋转服务均 enabled、active/exited，并有本次启动执行日志；实际 unit 采用更早的 sysinit 排序，与仓库模板不同。

**现场问题 C12：旋转 service 参数拆分错误。** D-Bus 返回 argv 为 `/bin/sh`, `-c`, `echo`, `3`, `>`, `/sys/class/graphics/fbcon/rotate_all`；`sh -c` 仅执行 `echo`，未执行预期的重定向写入。它返回 0 不能证明写入成功；当前 rotate=3 另有启动参数 `fbcon=rotate:3`。此问题属于设备实际配置，仓库模板的命令有引号，不能混同。本轮未修改或重启服务。

Panel live 模块的本次启动日志含 `hardware MMIO fix complete`，补充了驱动内路径执行证据；考虑 C2 的失败处理，仍不能单靠该日志保证所有映射/写入成功。实际模块、工具和 DTB 哈希已保存在实机记录，尚未取得与仓库源码的精确构建映射。

### 现场问题 C12 的处理状态

经使用者明确要求，已停用并删除实机旧旋转服务；核验其 LoadState=not-found、ActiveState=inactive，控制台 rotate=3、启动参数与显示修复服务保留。没有修改仓库运行文件；原 service 模板保留作历史参考。C12 是清理前现场发现，不再列作本机当前启用服务问题。
