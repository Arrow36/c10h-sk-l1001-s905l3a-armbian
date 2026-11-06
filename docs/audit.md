# 文档真实性审计与修改说明

## 范围与基准

基准提交：`13f3b88cddffd091862295e323c63cf1603bbabf`；审计日期：2026-11-06（Asia/Shanghai）。本轮从干净工作树开始，先只读核查全部 48 个受版本控制文本文件（344,444 字节）、10 个提交及重要差异，再进行文档修改。GitHub 网页曾显示旧 README，以 fetch 得到的完整提交为准。

本轮仅编辑/新增 `.md`，不修改代码、源码注释、DTS、Shell、service、启动文件、二进制或其他运行文件，不移动/删除文件，不提交、不合并、不推送。旧研究正文、失败材料和 `evidence/*.txt` 保留；11 月 6 日研究文档仅末尾追加审计补注。此前 `f377466` 曾改两处源码注释属于既有历史，不是本轮修改。

## 结论与评价方法

保留两项独立事实：**fnOS 内屏已实测显示正常；Armbian 内屏尚未测试。** 设备使用者后续确认已在 `f377466` 文档中留存。A–D 等级与每条核心结论的依据见 [证据索引](../evidence/README.md)，其中 B 表示缺少完整公开复核材料，不能理解成否认现场成果。

已实现源码、构建记录、执行输出、功能目视验收和机制推断分别评价。例如：`3f3997e` 证明驱动内 MMIO 已实现，不能证明当前设备加载该版；11 月 4 日输出证明当时模块执行，但不能验收之后的源码。

## README 核心技术结论逐项检查

| 结论类别 | 核查结果 | 依据/处理 |
| --- | --- | --- |
| 实物型号、平台与借用 DTB | 保留 SK-L1001 / p291_iptv；E900V22C 为 DTB 字符串 | 原厂研究页面、E1/E3；不增加兼容型号 |
| Armbian 版本、根盘、启动 | 有内核/根挂载/多用户日志，镜像名称为历史来源记录 | E1、provenance；未重新下载镜像复算 |
| fnOS 版本、Btrfs、USB/SSH | 历史终端、bootargs 与环境记录相符 | E3；与 Armbian 分列 |
| 一次性引导、不 saveenv | 脚本文本可核对，最终二进制与原厂 U-Boot 全流程缺失 | E7；限定为脚本内容，不作整机无写入保证 |
| eMMC 保留/禁用 | DTB 工具和 fnOS 设备状态有依据，但 overlay 本身不禁 eMMC | E3/E7；区分基线、overlay 和最终 DTB |
| USB hub/键盘 | 初始提交/历史反馈支持，缺独立 USB/input 输出 | F3，明确证据层级，不扩大到 fnOS |
| Ethernet、SSH、DNS/HTTPS | Armbian 有记录，fnOS 有 SSH 会话；100 Mbps/自动联网不能混为吞吐/可靠性 | E1/E3/E9；两系统分别表述 |
| fnOS Web | 摘要报告可访问，缺 HTTP/截图原件 | F5；不延伸到 NAS 应用可用 |
| HDMI、单 CRTC 与 DSI 独占 | fnOS 早期 modetest/debugfs 和画面描述支持当前配置未实现克隆 | E3；不宣称硬件永远不能双显 |
| Panel、DSI、背光与时序 | 源码/overlay 和 fnOS 记录可核对；原厂原始资料未公开 | E3/E6/E7；物理 800×1280 与终端横屏分开 |
| fnOS 几何/色彩/抖动正常 | 保留现场成功结论，缺成套最终复核材料 | F6/F7；不使用 100% 色准/永久稳定 |
| Armbian 内屏 | 与旧归属矛盾 | F10；明确尚未测试，旧输出不删不重归属 |
| MMIO 地址、默认值与根因 | 写入值存在；默认状态非通用，0x1DF5 命名冲突 | F11/F12、代码 C4；配置表移到 display.md 并校正解释 |
| 自启持久化 | 实现与部署记录存在；当前版本/冷启动日志缺失 | F8/F9、C1/C2/C5；不推定驱动已替代工具 |
| 其他外设、量化性能和恢复 | 未有完整可用性/量化证据 | F14–F17；标待验证，不由节点注册推出可用 |
| 仓库范围、构建与许可 | 无镜像/预编译模块/统一 LICENSE；各文件保留 SPDX | 全文件清单、Makefile；不替上游重新授权 |

## 材料冲突与处理

| 冲突 | 判断与处理 |
| --- | --- |
| E2 与旧 display-followup/checkpoint 标为 6.12.114-ophub，后续明确 Armbian 内屏未测 | 无法从现有数据确定旧显示试验的真实系统/设备归属。保留旧文和摘要，仅以当前确认陈述“未测试”；不捏造 fnOS 归属，也不宣称旧日志是伪造 |
| 11 月 4 日“普通画面未修复”与 11 月 6 日“显示正常” | 为不同实验阶段；前者保留失败过程，后者及后续使用者确认用于当前 fnOS 状态 |
| 源码 .clock=75857 与旧模式 76107 | `fb4d30c` 修改了声明值；旧日志的请求/时钟框架报告/当前源码不等同，不回改历史数字 |
| 0x1DF5 被写成 VPP_OFIFO_SIZE | 仓库与 Linux v6.12.41 头文件均指向 BLD_H_SCOPE；输出 FIFO 名称对应 0x1D27。纠正说明，不改寄存器值，不因机制有误否定实际正常显示 |
| 保持行数单项无效与联合配置有效 | 早期测试值 0x808 与完整值 0x08080808 不同，其他寄存器/启动状态也不同；不能作同一条件互相证伪 |
| 简化 YCbCr 模型与“100% 色准” | 近似公式未核对硬件系数/范围；只支撑分析线索，保留“目视颜色正常” |
| systemd 已部署与驱动新增内写入 | 代码演变顺序可查，未取得成功运行的最终模块指纹；保留两条路径，不臆断哪条当前生效 |
| journal 的 Jun 26、Nov 04 与编译日期 | 使用文档记录日期和 dmesg 相对时间，不把不一致的绝对时间用于虚构实验首次日期 |

## 提交历史与日期口径

以下按 Git 提交日期（+08:00）排列，仅说明材料何时进入仓库。

| 日期 | 提交 | 可确认的变更与限制 |
| --- | --- | --- |
| 11-03 | `9b53b5d` | 初始 Armbian USB bring-up、证据和启动工具；README 所列实验区间为 11-02 至 11-03 |
| 11-03 | `f03bc3b` | 显示模块、overlay、DRM/Canvas 探针、显示摘要与暂停检查点；显示实验系统归属有冲突 |
| 11-04 | `87ba345` | FbTerm/原厂对照文档；不作为 Armbian 内屏验证证据 |
| 11-04 | `1bc8688` | fnOS 内屏及浏览器诊断摘要 |
| 11-04 | `6ca0e28` | README 硬件汇总；新增文字不等于新增独立实测 |
| 11-04 | `715821d` | 发布三份长篇研究记录及原始终端附录，不等于全部实验当日首次完成 |
| 11-06 | `fb4d30c` | 发布修复研究、MMIO 工具/服务；Panel 时钟 76107→75857，Host 新增 HS/VS invert 位 |
| 11-06 | `3f3997e` | Panel prepare 新增同组 VPU/DSI 写入；提交中的 0 ms/完全消除说法没有独立运行证据 |
| 11-06 | `f377466` | 整理文档、明确 fnOS 正常/Armbian 未测，同时仅改两处既有源码注释 |
| 11-06 | `13f3b88` | README 新增寄存器对照表，本轮以其为基准审查命名与过强解释 |

完整首次实验日期不能由此确定；原厂参数提取没有独立时间证据，因此 README 不编造具体里程碑日期。

## 重大修改的原文、原因与依据

| 基准原文/原状 | 本轮处理 | 原因与证据 |
| --- | --- | --- |
| README：“彻底解决偏色”“彻底消除每秒高频垂直抽动”“恢复满血 1919 深度 FIFO” | 首页保留 fnOS 实测正常；详细值集中到 display.md；纠正 0x1DF5 名称 | F6/F11/F12；源码/头文件与目视反馈分开 |
| README 的“原厂 / 主线默认 / 最终修复”长表 | 改成专题表的“当前实现值与命名核对”，记录历史状态并非通用默认 | E3/E5/E6；原厂完整快照和运行内核源码缺失 |
| hardware.md 共用一列 Ethernet/GPU/USB/音频 | 按 Armbian/fnOS 分列，标明仅汇总、节点注册与实测范围 | E1/E3/E9；杜绝跨系统推广 |
| display-fnos.md：“同一台…Armbian…可显示完整边框与十字” | 说明与后续确认冲突，指向审计；旧研究正文原样保留 | F10，无法确定旧试验来源 |
| build-display.md：“本次使用…编译”“SIGTERM 恢复原图层” | 改为历史构建记录、尝试恢复；增加本轮未构建声明 | C6、Makefile 与 E3；不能冒充本轮测试 |
| boot.md：“测试 DTB 仅将…eMMC…disabled”易被当全套 LCD 改动 | 追加工具/overlay/最终 DTB 的边界，区分 ext4 与 Btrfs | E7；不更改脚本或 DTB |
| 11 月 6 日研究表的 OFIFO 命名 | 仅末尾追加审计更正，不删原表/历史段落 | C4、保留可追溯性 |
| 索引分散、证据层级不明 | 新增 research/README、evidence/README、code-audit 与本报告 | 便于定位历史、输出、实现和缺口；不机械拆散研究文件 |

## 仍缺乏证据的结论

1. Armbian 内屏实际测试与成功状态；旧 Armbian 显示记录的精确来源。
2. fnOS 正常显示对应的最终 DTB、二进制脚本、内核配置、模块哈希与源码提交；特别是是否执行 `3f3997e` 后的驱动内写入。
3. 当前两个修复/旋转服务的启用、实际执行时序、逐次冷启动与暖启动结果；约 4 ms 的计时条件及其与总开机时长的关系。
4. 原始 Android 初始化日志、完整 DTB/固件与 S905L3A 专用手册；152 项匹配统计和所有位含义的独立核算。
5. RGB/YUV 矩阵完整系数、色差指标、单寄存器贡献、FIFO/DDR 欠载的直接因果证据；不能由联合修复成功推定每项机制成立。
6. 原始 CSV、四份 Python 工具、照片/录像、完整内核配置与两份比较 DTB；现有汇总不能作本轮复算。
7. 两系统各自的外设功能、长期读写/负载、温控/功耗、fnOS NAS 工作负载、其他批次兼容性。
8. 原厂 update 全路径无副作用、完整固件恢复/救砖、SD 启动、eMMC 安装与内核升级后的兼容性。
9. 历史镜像哈希/资产当前可下载性、全部外链与旧附件可访问性；未重新下载资产，外部页面检查不等于二进制认证。

## 文件盘点

目录结构保留。下表是编辑前基准的全部受版本控制文件，行数用于定位规模，不等于每个宏或每条历史解释已经被实机证实。源码及配置作静态审阅，外部原厂材料的缺口如上。

| 文件 | 基准行数 | 作用 |
| --- | --- | --- |
| [.gitattributes](../.gitattributes) | 9 | 文本 LF 规则；本轮只读 |
| [.gitignore](../.gitignore) | 25 | 产物与敏感文件忽略规则；本轮只读 |
| [README.md](../README.md) | 114 | 当前项目入口与核心结论 |
| [boot/aml_autoscript.cmd](../boot/aml_autoscript.cmd) | 14 | Armbian 固定 UUID/ext4/HDMI 一次性引导 |
| [boot/drm-stage1-bars.cmd.example](../boot/drm-stage1-bars.cmd.example) | 14 | 原生 DRM 彩条启动模板，UUID 占位符 |
| [boot/retained-fb.cmd.example](../boot/retained-fb.cmd.example) | 14 | 保留固件显示状态的启动模板 |
| [docs/README.md](../docs/README.md) | 44 | 文档导航；文档/历史记录 |
| [docs/boot.md](../docs/boot.md) | 47 | U 盘启动与测试边界；文档/历史记录 |
| [docs/build-display.md](../docs/build-display.md) | 50 | 屏幕实验源码构建说明；文档/历史记录 |
| [docs/checkpoint-2026-11-03.md](../docs/checkpoint-2026-11-03.md) | 39 | 2026-11-03 暂停检查点；文档/历史记录 |
| [docs/display-browser-diagnostics.md](../docs/display-browser-diagnostics.md) | 29 | C10H 浏览器显示诊断方法；文档/历史记录 |
| [docs/display-fnos.md](../docs/display-fnos.md) | 39 | C10H 上的 fnOS 内屏实验；文档/历史记录 |
| [docs/display-followup.md](../docs/display-followup.md) | 43 | C10H 内置屏后续实测；文档/历史记录 |
| [docs/display.md](../docs/display.md) | 89 | C10H 内置屏幕适配概览；文档/历史记录 |
| [docs/hardware.md](../docs/hardware.md) | 37 | 硬件支持状态；文档/历史记录 |
| [docs/network.md](../docs/network.md) | 35 | Windows 共享网络与 SSH；文档/历史记录 |
| [docs/provenance.md](../docs/provenance.md) | 24 | 测试镜像来源；文档/历史记录 |
| [docs/research/browser-slider-method-2026-11-04.md](../docs/research/browser-slider-method-2026-11-04.md) | 211 | 创维 C10H 内屏适配：用浏览器实时控制加速底层显示故障研究；文档/历史记录 |
| [docs/research/fnos-display-resolution-record-2026-11-06.md](../docs/research/fnos-display-resolution-record-2026-11-06.md) | 86 | C10H 内屏寄存器调整与阶段性结果（2026-11-06）；文档/历史记录 |
| [docs/research/fnos-lcd-experiment-log-2026-11-04.md](../docs/research/fnos-lcd-experiment-log-2026-11-04.md) | 576 | 创维 C10H / SK-L1001：fnOS 内屏适配本轮详细实验纪要；文档/历史记录 |
| [docs/research/fnos-lcd-full-record-2026-11-04.md](../docs/research/fnos-lcd-full-record-2026-11-04.md) | 1841 | 创维 C10H / SK-L1001：fnOS 内置 LCD 适配完整记录（2026-11-04 更新版）；文档/历史记录 |
| [docs/validation.md](../docs/validation.md) | 38 | 后续验证计划；文档/历史记录 |
| [docs/wifi.md](../docs/wifi.md) | 9 | Wi-Fi 实验记录；文档/历史记录 |
| [drivers/c10h-display/Makefile](../drivers/c10h-display/Makefile) | 3 | Panel/Host/ENCL 三模块 Kbuild |
| [drivers/c10h-display/c10h-dsi-host.c](../drivers/c10h-display/c10h-dsi-host.c) | 376 | 实验 Meson Host 与实际位时钟/极性配置 |
| [drivers/c10h-display/c10h-lcd-diagnostic.c](../drivers/c10h-display/c10h-lcd-diagnostic.c) | 234 | ENCL 测试/Gamma/DSI 临时修改及有限恢复 |
| [drivers/c10h-display/c10h-panel-init.h](../drivers/c10h-display/c10h-panel-init.h) | 44 | 原厂提取初始化数组；缺原始提取文件 |
| [drivers/c10h-display/meson_dw_mipi_dsi.h](../drivers/c10h-display/meson_dw_mipi_dsi.h) | 160 | DSI TOP 寄存器与同步位定义 |
| [drivers/c10h-display/meson_registers.h](../drivers/c10h-display/meson_registers.h) | 1910 | VPU 寄存器定义；核对地址命名 |
| [drivers/c10h-display/panel-c10h-jd9366tc.c](../drivers/c10h-display/panel-c10h-jd9366tc.c) | 201 | Panel 时序、DCS/GPIO 与直接 MMIO |
| [dts/c10h-panel-test.dtso](../dts/c10h-panel-test.dtso) | 45 | 面板、背光、graph、HDMI/SD 路由 overlay |
| [dts/c10h-wifi-pcie-test.dtso](../dts/c10h-wifi-pcie-test.dtso) | 37 | 失败/未验证 PCIe W2 与共享 PHY 实验 |
| [evidence/display-2026-11-03.txt](../evidence/display-2026-11-03.txt) | 17 | 显示摘要，系统归属存在冲突 |
| [evidence/verified-boot.txt](../evidence/verified-boot.txt) | 16 | 原始启动摘录与后续验证摘要 |
| [experiments/display/Makefile](../experiments/display/Makefile) | 1 | 仅 bootfb 内核模块 Kbuild |
| [experiments/display/c10h-bootfb-info.c](../experiments/display/c10h-bootfb-info.c) | 67 | Canvas 读选择与硬件状态快照 |
| [experiments/display/c10h-retained-fb.dtso](../experiments/display/c10h-retained-fb.dtso) | 27 | 固定地址 simplefb/reserved-memory 历史实验 |
| [experiments/display/drm-geometry.c](../experiments/display/drm-geometry.c) | 107 | 固定模式的 DRM plane-only 用户探针 |
| [scripts/c10h-boot-proof.sh](../scripts/c10h-boot-proof.sh) | 16 | 用户空间启动证据写入 /boot |
| [scripts/c10h-drm-stage1-bars.sh](../scripts/c10h-drm-stage1-bars.sh) | 17 | 保存日志后 insmod 彩条模块 |
| [systemd/c10h-boot-proof.service](../systemd/c10h-boot-proof.service) | 12 | 启动取证单元示例 |
| [systemd/c10h-display-fix.service](../systemd/c10h-display-fix.service) | 12 | MMIO oneshot 开机单元示例 |
| [systemd/c10h-drm-stage1-bars.service](../systemd/c10h-drm-stage1-bars.service) | 12 | 按 kernel cmdline 启动彩条 |
| [systemd/c10h-fbcon-rotate.service](../systemd/c10h-fbcon-rotate.service) | 12 | fbcon 旋转单元示例 |
| [systemd/c10h-network-profile-owner.service](../systemd/c10h-network-profile-owner.service) | 12 | 仅恢复 NetworkManager profile 属主 |
| [tools/c10h-display-fix.c](../tools/c10h-display-fix.c) | 78 | 独立 /dev/mem 实验写入程序 |
| [tools/disable_emmc.py](../tools/disable_emmc.py) | 33 | 限定基线 DTB 的固定 eMMC 节点修改 |
| [tools/make_boot_script.py](../tools/make_boot_script.py) | 17 | legacy script 容器、头/载荷 CRC 和时间戳生成 |

## 检查范围

编辑前 57 处 Markdown 本地文件链接（含 1 处内部锚点）检查无断链。最终相对路径与标题锚点、非 Markdown 文件逐字节不变、旧研究正文/失败证据保留及补丁可应用性由交付检查说明记录。全部外链未逐个核验，历史命令块中的设备路径/下载链接仅保留为原始材料。

本轮未构建、未运行设备测试，不能把文档检查通过称为硬件验证通过。最终修改清单、Git Diff 摘要和自检结果随交付包提供；所有工作仅保留本地，等待维护者审核。

## 后续现场核查对审计结论的补充

静态审计后，经维护者授权 SSH 连接实际 fnOS。新增 [实机证据](../evidence/fnos-live-check-2026-11-06.md)，补齐当前运行环境、DRM/DT状态、文件指纹、服务日志与实际 unit；使用者现场确认显示正常，电脑访问管理页返回 HTTP 200。此前“本轮未连接设备”描述指最初静态审计阶段；本节之后应与实机记录共同阅读。

当前执行证据已不再完全缺失，但并未编译仓库源码、冷启动或进行长时试验。发现现场旋转服务 argv 错误，服务成功不能解释当前横屏；设备实际 unit 与仓库模板不同。原静态审计材料和问题表保留，避免把新增证据倒填为早期已知事实。Armbian 内屏仍未测。所有源码、service、DTB、启动文件及寄存器值仍未修改，也未提交/推送。

## 维护者后续授权与当前部署

维护者在完成审阅与实机核查后，明确授权将本次 Markdown 更新发布到 GitHub，并要求删除实机旧旋转服务。现场已完成停用、删除和 daemon-reload；核验服务 not-found/inactive、rotate=3、启动参数保留、显示修复服务 active。清理不涉及仓库代码或运行文件；仓库 service 模板仍作历史参考。先前“等待审核/未修改设备”的描述分别属于静态审计与只读核查阶段，后续操作独立记录，未倒改历史输出。
