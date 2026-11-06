# 证据索引与结论评价

审计基准 `13f3b88`（2026-11-06）。本轮只读核对仓库及公开参考，未新增设备实验。A–D 表示公开材料对某条结论的支撑程度，不是对设备使用者反馈的否定，也不是产品成熟度评级。

- **A**：有明确系统、操作与结果的实测输出，足以支撑该限定结论。
- **B**：有源码/研究记录及实验反馈，缺少完整可独立复核材料。
- **C**：合理技术分析或待验证方案，尚不足以认定功能或因果关系。
- **D**：证据不足或存在未解决的材料冲突。

## 已有材料

| 编号 | 材料 | 性质与限制 |
| --- | --- | --- |
| E1 | [verified-boot.txt](verified-boot.txt) | Armbian 内核/根挂载/multi-user/sshd 的日志摘录，末尾含 SSH、互联网和 HDMI 的后续汇总；不是完整串口日志 |
| E2 | [display-2026-11-03.txt](display-2026-11-03.txt) | 脱敏显示摘要，非逐条原始输出；标称 6.12.114-ophub 与设备使用者“Armbian 内屏未测”的后续确认冲突，归属待核对 |
| E3 | [fnOS 完整记录](../docs/research/fnos-lcd-full-record-2026-11-04.md) | 指定 fnOS 内核、模块构建/绑定、DRM 状态和附录终端原文；照片与 DTB 原件缺失，部分绝对日期受设备时钟影响 |
| E4 | [11 月 4 日详细纪要](../docs/research/fnos-lcd-experiment-log-2026-11-04.md) | 单变量试验、CSV 与配置统计；原 CSV/完整配置不在仓库，不能重算聚合值 |
| E5 | [11 月 6 日修复记录](../docs/research/fnos-display-resolution-record-2026-11-06.md) | 正常显示、横屏和部署的事后整理；设备使用者后续明确确认 fnOS 显示正常、Armbian 未测，已记入基准提交 `f377466` |
| E6 | [独立 MMIO 工具](../tools/c10h-display-fix.c) · [面板驱动](../drivers/c10h-display/panel-c10h-jd9366tc.c) · [DSI Host](../drivers/c10h-display/c10h-dsi-host.c) | 实现可审阅；缺最终加载二进制与当前源码对应关系，不能直接证明运行 |
| E7 | [面板 overlay](../dts/c10h-panel-test.dtso) · [启动脚本](../boot/aml_autoscript.cmd) · [DTB 工具](../tools/disable_emmc.py) | 配置目标可查，不等于最终设备 DTB/脚本内容；overlay 本身不禁用 eMMC |
| E8 | [修复服务](../systemd/c10h-display-fix.service) · [旋转服务](../systemd/c10h-fbcon-rotate.service) | 服务模板可查，不等于已安装/启用/执行及硬件验收；排序不表示硬件就绪 |
| E9 | [镜像记录](../docs/provenance.md) · [网络记录](../docs/network.md) · [Wi-Fi 记录](../docs/wifi.md) | 历史实验汇总；缺镜像文件、完整下载校验记录及 Wi-Fi 构建产物 |

## 核心结论与依据

| 编号 / 结论 | 系统与阶段 | 等级 | 可支持范围及缺口 |
| --- | --- | --- | --- |
| F1：ARM64 Linux 启动、USB ext4 根挂载、multi-user | Armbian `6.12.114-ophub` | A | E1 有日志；不证明冷启动成功率 |
| F2：SSH、HDMI 控制台、一次 DNS/HTTPS | Armbian | B | E1 后续验证汇总 + E9 网络细节；sshd 监听本身不等于客户端登录，后续汇总才提供登录反馈 |
| F3：USB 集线器/键盘成功使用 | Armbian 历史阶段 | B | 初始提交 `9b53b5d` 与硬件文档有反馈，未附 USB/input 独立输出 |
| F4：fnOS 启动、SSH、目标内核、DSI/Panel 已执行 | fnOS `6.12.41-trim #219`，11 月 4 日 | A | E3 有 SSH 欢迎信息、bootargs、模块/驱动绑定、初始化及 DRM 命令输出；只针对当时版本 |
| F5：fnOS Web 管理页可访问 | fnOS | B | [历史摘要](../docs/display-fnos.md)有反馈，缺 HTTP/界面原件；不能验收 NAS 应用 |
| F6：内屏普通画面完整、颜色正常，无可见折叠/抖动 | fnOS，11 月 6 日及后续确认 | B | E5 + 使用者明确确认，保留成功结论；缺最终二进制、成套前后快照/照片与长时测试 |
| F7：1280×800 横屏终端；物理模式仍 800×1280 | fnOS | B | E5、E8 与 fbcon 机制相符；不证明其他 framebuffer 应用、桌面或触摸旋转 |
| F8：开机 MMIO/横屏部署方案 | fnOS | B | E5 记载部署，E8 为实现；缺本机当前 service 安装/启用/status/journal 及冷启动批次日志 |
| F9：面板 prepare 内 MMIO 已实现 | 两系统共享源码，`3f3997e` 以后 | A（仅代码存在） | E6 与提交差异；“本机已加载此版且不需用户态修复”为 D，缺版本指纹 |
| F10：Armbian 内屏已验证 | Armbian | D | E2/早期资料与后续确认冲突；当前明确标记未测试，不改写 E2 来源 |
| F11：FIFO/DDR 欠载是唯一根因、每项写入分别有效 | fnOS 分析 | C | E4 未确诊欠载；联合配置有效不构成每项独立因果证明；`0x1DF5` 命名又有冲突 |
| F12：`0x1DF5` 是输出 FIFO 容量，深度为 1919 | 文档解释 | D | E6 头文件和 Linux v6.12.41 将其定义为 BLD_H_SCOPE；VPP_OFIFO_SIZE 为 `0x1D27`，撤下原解释 |
| F13：eMMC 禁用与不保存环境的一次性脚本 | 配置实现 | A（仅代码目标） | E7 可查；最终 DTB/二进制缺失，不保证 U-Boot update 全路径无写入 |
| F14：拔盘返回 Android | 早期单机实验 | B | [启动记录](../docs/boot.md)有反馈；完整恢复、Bootloader 无改动及免拆救砖为 D |
| F15：Panfrost 注册 | fnOS 早期日志 | A（仅初始化/节点） | E3 附录有日志；实际加速为 D。Armbian 注册仅见历史汇总，待补输出 |
| F16：触摸/无线/音频/蓝牙/摄像头/温控可用 | 两系统分别 | D | 未见完整功能验收；Wi-Fi 失败 overlay 和仅编译/加载记录不能证明连接 |
| F17：100% 色彩准确、永久无抖动、4 ms 开机成本 | 正常显示之外的量化承诺 | D | 缺色度测量、长时/批次数据、计时条件，不能由目视反馈或单次程序耗时推导 |

## 缺失或无法独立核对的原始材料

原始 Android DTB/固件/发送日志、S905L3A 专用寄存器手册、最终 Linux DTB/二进制启动脚本、11 月 6 日成功配置的模块与构建映射、冷启动 service 日志、照片/录像、四份 Python 工具、原始 `c10h-display-monitor.csv`、完整内核配置、两份比较用 DTB、原厂分区备份与完整恢复记录均未随当前仓库提供。

这些材料未公开可能有脱敏或体积原因，本次不把缺失解释为实验未发生。研究记录引用的 ZIP、图片名、设备绝对路径不能当作仓库已有文件链接。校验值只作历史指纹，不声称本轮已重新计算镜像或设备文件哈希。

## 上游核对与访问局限

已查阅 [Linux v6.12.41 寄存器定义](https://github.com/gregkh/linux/blob/v6.12.41/drivers/gpu/drm/meson/meson_registers.h)、[meson_viu.c](https://github.com/gregkh/linux/blob/v6.12.41/drivers/gpu/drm/meson/meson_viu.c)、[kbuild 文档](https://docs.kernel.org/kbuild/modules.html)、[fbcon 文档](https://docs.kernel.org/fb/fbcon.html)、[systemd v257 单元说明](https://github.com/systemd/systemd/blob/v257/man/systemd.unit.xml)和 [原厂研究项目页面](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod)。参考文档只解释接口/源码，不证明设备运行结果。

freedesktop 的在线 man 页面本轮访问超时，改查官方仓库单元说明；本地直接下载 raw 文件也遇到 TLS 超时，源码核对使用浏览工具返回的内容，未取得上游完整校验快照。镜像发布资产未重新下载，全部外链没有逐条验证有效性。GitHub 网页有时返回旧 README，审计基准以实际 `git fetch` 得到的完整提交及工作树为准。

## 本轮后续实机补证

维护者提供设备连接信息后，已进行 [fnOS 实机只读核查](fnos-live-check-2026-11-06.md)。该记录补充当前内核、DRM 状态、DT节点、背光、模块/文件哈希、实际 unit、service 本次启动日志，以及管理页 HTTP 200；使用者同时确认实屏正常。

此前表格是静态审计时的证据盘点，补证后按以下范围更新：F5 首页可访问、F6 当次显示正常、F7 当前横屏状态可评为 **A（限本次现场）**；F8 两服务已启用/执行有直接日志，**执行结果为 A，旋转服务完成预期写入为 D（实际 argv 错误）**。F9 有当前 live 模块和内嵌 MMIO 完成日志，支持功能路径已执行，但完整寄存器写入、精确源码/构建映射和单路径贡献仍未证明。F10 Armbian 未测保持不变。

设备实际 unit 与仓库模板不同；当前横屏另有 `fbcon=rotate:3`。原先关于未提供当前运行输出的缺口已被部分填补，冷启动/长时测试、源码对应与寄存器回读缺口仍存在。未因 service 退出 0 而越过既有错误处理限制。

### 旧旋转服务清理后的当前状态

使用者随后明确要求移除实机旧旋转服务，已执行并核验 `not-found/inactive`，rotate 仍为 3、`fbcon=rotate:3` 保留、显示修复服务 active。详见 [实机记录末节](fnos-live-check-2026-11-06.md#后续操作移除旧旋转服务)。上文关于旧服务的状态与问题均是清理前记录，不作为当前部署状态。
