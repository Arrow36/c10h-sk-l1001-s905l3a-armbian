# 创维 C10H / SK-L1001：fnOS 内屏适配本轮详细实验纪要

> 公开脱敏版。保留原章节、实验步骤和附录，登录账户、网络地址、用户目录及运行实例标识已替换。文中提及的原始附件仅作为历史材料引用。

> **整理日期**：2026-11-04（截至本次对话停止处）<br>
> **性质**：对本轮聊天的**结构化、详尽技术归档**，不是逐字聊天导出，也不是屏幕修复完成报告。<br>
> **主题**：在已实现内屏自动点亮的基础上，定位 Linux 普通图像左侧固定紫色、右侧重复与闪动的原因。<br>
> **相关材料**：①《C10H_fnOS_内屏适配_完整对话与最新进展_2026-11-04.md》；②《C10H_浏览器实时滑杆诊断_方法与成果_2026-11-04.md》；③ `c10h-display-monitor.csv`；④ `c10h-fnos-config.txt`。<br>
> **事实分级**：**[实测]** 用户发来的终端输出、肉眼观察、上传文件；**[上游记录]** 前期引用的公开仓库或厂商资料；**[推断]** 基于现象的解释；**[待核实]** 尚无实机或独立来源充分验证。不能把推断当成已经证明的根因。

---

## 0. 给接续研究者的摘要（先读）

**机器**：创维 C10H / SK-L1001，Amlogic S905L3A；10.1 英寸 JD9366TC 面板，800×1280、MIPI DSI 4 lane RGB888。USB 启动非官方 ARM 移植版 fnOS 1.1.20、Debian 12、Linux `6.12.41-trim #219`；原厂 Android 位于 eMMC，当前设备树禁用 eMMC 与 HDMI，保留 USB 启动。采用 Arrow36 项目的 `c10h-dsi-host.ko` / `panel-c10h-jd9366tc.ko`，针对本机内核编译后安装。

**已实现**：内屏自动点亮、DRM 模式 800×1280@60Hz、Host/Panel 绑定、标准 DRM XRGB8888 线性 framebuffer 可提交、Canvas 地址与 framebuffer 地址及 stride 对齐；ENCL **内部硬件彩条能完整显示，无重复、无错位**。

**核心故障仍在**：普通 fb0 写入全白时，**左侧大片紫色不变**，右侧变白且可能闪动；随后短时使能 OSD1 内部强制白色，**左侧依旧紫色，右侧变白**。这使“仅是用户态像素值写错”或“仅是 RGB 顺序错”不足以解释空间分区。颜色通道还有独立可观察的 R→G、G→B、B→R 置换，实验脚本逆置换可补偿，但没有全系统修复。

**本轮新增且已确认的排查结果**：

- OSD1 FIFO 初始化值 `0x028207E1` 与之前对照的主线驱动预期一致；后续只读采样曾短暂出现 `0x029207E1`，暂不能证明欠载。
- `VIU_OSD1_MALI_UNPACK_CTRL=0x00101234`，未发现 AFBC 错误开启的明确证据。
- OSD1 单色控制原值与恢复值均为 `0x00000000`；6 秒强制白色仍只改变右侧。
- ENCL 状态读数不断变化；OSD Blend 当前 XY 持续 `0x00040000`，VPP Postblend 当前 XY 持续 `0x00000000`。这些只读寄存器固定**不等于**能证明模块停摆，可能与当前硬件路径或寄存器实现有关。
- 临时关闭 `VPP_MISC` 中旧 VD1 Postblend 位：**画面无变化**，恢复 `0x00000482`；临时把 `VPP_HOLD_LINES` 从 `0x0404` 改 `0x0808`：**画面无变化**，恢复 `0x0404`。
- DSI PHY 状态有动态变化，但控制器 `INT_ST0=0`、`INT_ST1=0`；掩码 `INT_MSK0=0`、`INT_MSK1=0x0003E000`，未报告 DSI 视频载荷欠载，不代表所有像素都正确。
- simplefb HDMI、CVBS 两个 DT 节点均 `disabled`；`/proc/fb` 仅有 `0 mesondrmfb`，绑定 `meson-drm`；没有发现 simplefb 接管冲突。
- 在 Windows 以 SHA-256 比较公开 Linux `v6.12.41` 与 `v6.12.114` 的 **10 个 Meson 显示驱动文件，全部 `SAME`**。这不证明 fnOS 定制内核、Arrow36 运行内核的二进制实现相同。
- fnOS 内核配置已上传。配置包括 `CONFIG_DRM_MESON=y`、`CONFIG_MESON_CANVAS=y`、`CONFIG_DRM_MIPI_DSI=y`、`CONFIG_DMA_CMA=y`、`CONFIG_CMA_SIZE_MBYTES=128`；与当时查看的 ophub 公开配置有 CMA 128/256 MB、`DRM_FBDEV_OVERALLOC` 100/300 等区别，**没有证据表明这些区别直接造成紫屏**。
- 用户对原始 DTB 与当前 DTB 做了 `dtc` 转 DTS 的完整 diff，总计 **131 行**；本轮补齐最后 31 行后，差异均属于预期的面板、背光、DSI/PHY、PWM 与 HDMI/SD/eMMC 及 symbols 变化，**没有发现意外改写 VPU 主节点的证据**。
- 原来 `/tmp/c10h-lcd.0AY24B/repo` 编译源码目录已不存在，在 `/tmp`、`/usr/src` 未找到 `c10h-dsi-host.c` 与 `panel-c10h-jd9366tc.c`。**最后提出但尚未执行的任务**：读取当前安装两个 `.ko` 的 modinfo、SHA-256，检查 `/boot/c10h-lcd-stage/` 是否保留构建来源／源码补丁；之后再判断是否需要对照上游重新构建。**尚未修复内屏**。

**优先级建议**：先做实际模块来源/二进制归属确认，再收集能与 Arrow36 *同测试条件* 对比的运行配置或内核构建证据；停止对未知 OSD/VPP/DSI 寄存器进行猜测性写操作。暂不刷 eMMC、不升级内核、不修改当前已启动的 DTB。

---

## 1. 环境、版本和既有基础

| 项目 | 已知状态 |
|---|---|
| 设备 | 创维 C10H / SK-L1001，S905L3A |
| 面板 | `C_JD9366TC_Single_CSOT_10.1_BPA107LT1-1`；800×1280，RGB888，4-lane DSI |
| 系统 | fnOS 1.1.20（非官方 ARM 移植）；Debian 12 |
| 内核 | `6.12.41-trim #219 SMP PREEMPT`，aarch64；构建标识 `[构建账户已脱敏]` |
| 启动 | USB 介质，`/boot` FAT32，`/` Btrfs；eMMC 上保留原厂 Android |
| 当前图形 | 单 CRTC；HDMI DT 节点 disabled，DSI-1 使用该 CRTC |
| 内屏 DRM | 800×1280@60 Hz，模式像素时钟 76.107 MHz |
| DSI Host | 打印目标 PLL 456642000、实际 bit clock 531000000 Hz、估算 pixel clock 75857143 Hz |
| 主 framebuffer | `/dev/fb0`，`mesondrmfb`，XRGB8888 (`XR24`)，800×1280，pitch 3200，大小 4,096,000 字节 |
| DRM 主 plane / Canvas | 先前记录 plane[36] / fb=43；DMA 与 Canvas 地址均为 `DMA_ADDR`，stride 3200，高 1280。DRM 对象 ID 会随重启变化，不可硬编码 |
| 已安装外部模块 | `/lib/modules/6.12.41-trim/extra/c10h-dsi-host.ko`、`panel-c10h-jd9366tc.ko` |
| 参考项目 | https://github.com/Arrow36/c10h-sk-l1001-s905l3a-armbian |
| 原实验 SSH 地址 | `test-user@C10H_IP`，仅为当时的实验网络地址，可能变化 |

### 1.1 初始问题与已经做过的试验（来自此前两份整理）

1. 最初希望 HDMI 与 LCD 同时显示，后来用户明确接受关闭 HDMI；单 CRTC、不能直接兼容克隆的问题已绕过，**不再把 HDMI 共存当成本次目标**。
2. HDMI disabled 后，LCD 开机自动点亮，显示控制台/DRM 测试画面；但左侧大色块、右侧重复白线和白块，不能正常使用。
3. `drm-geometry` 使用 DRM linear dumb buffer、`XRGB8888`、pitch 3200、size 4096000、plane-only commit；结果依旧分区，说明问题不只发生在 Python 画图程序。
4. ENCL 内部硬件彩条测试成功，整屏色条完整，无空间重复。这说明至少内部测试图到面板的链路可以工作，**并不代表普通 ENCL 像素输入的处理也正常**。
5. `/dev/fb0` RGB 标准写法 `bytes((b,g,r,0))` 在实屏观察为红→绿、绿→蓝、蓝→红；测试脚本改用 `bytes((r,b,g,0))` 逆补偿后颜色正确，**仅是测试程序补偿**。
6. 全帧白图时左侧固定紫色、右侧变白；CPU 读取到 4,096,000 字节，原脚本得到 1,023,872 个“纯白四字节序列”、128 个非白。原脚本以 `data.count()` 计数，未按四字节对齐迭代；这个数量不能当作严格的异常像素计数。即便 CPU 看到白图，也不能直接保证 DMA/后处理所见相同。
7. 内核在开机阶段报告 `meson_encl_set_gamma_table` 的 `WR_RDY/ADR_RDY timeout`；Gamma 可能有独立初始化问题，但既有数据不能用它完整解释画面空间分区。
8. 早期读过 OSD、Blend、VPP、Canvas、ENCL、DSI 多个寄存器，地址/pitch/窗口尺寸表面正确；不意味着真实扫描像素路径已经证实正确。

---

## 2. 本轮详细实验时间线（按对话推进顺序）

> 同一项实验的多次读数只列关键值；凡涉及寄存器**写入**的步骤均注明“历史实验，已恢复，不建议直接重跑”。本轮对话里没有证据表明任何一次实验已经将显示永久修复。

### 2.1 OSD1 FIFO 初始化配置与测试进程

**目的**：确认 framebuffer/滑杆测试进程已结束；检查 `VIU_OSD1_FIFO_CTRL_STAT` 是否与内核代码的预期一致。

**[实测]**

```text
===== 当前测试进程 =====
（无输出）
===== OSD1 FIFO 配置 =====
0x028207E1
```

读取候选 MMIO 地址 `0xFF9068AC`（寄存器编号 `0x1A2B`）。此前对照的 Linux 6.12 初始化值为 `0x028207E1`。**判定**：初始化配置符合该预期值，不能据此宣布 FIFO 没有运行时欠载；无需直接修改。

### 2.2 AFBC / FIFO 运行态抽查

**目的**：排查是否错误使用 AFBC 解码，查看 VPP FIFO 的短时读数变化。

**[实测]**

```text
OSD1 MALI_UNPACK_CTRL, 0xFF9068BC: 0x00101234
VPP_FIFO_STATUS, 0xFF9074A0，连续5次：
0x0003D000
0x0003CFFE
0x00039000
0x0003D000
0x0003D000
```

前期代码对照认为 `0xFF9068BC` 为 `VIU_OSD1_MALI_UNPACK_CTRL`，其最高位 bit31 未开启；之前 AFBC 路径选择也读到 0。**[推断]** 没有发现 AFBC 被错误启用的直接证据。VPP FIFO 读数存在变化，但缺少 S905L3A 专用字段说明，**不能据此判断欠载**。

### 2.3 OSD1 内部强制单色：最关键的空间隔离实验

**目的**：在不依赖正常 framebuffer 像素数据的情况下，判断左侧紫色是否响应 OSD1 内部纯白图像。

**[实测]**

```text
VIU_OSD1_DIMM_CTRL（候选地址 0xFF906B7C）初始值：0x00000000
短时尝试开启 OSD1 纯白：0x7FFFFFFF
肉眼观察：左侧仍然紫色，只有右侧变白
测试后值：0x00000000
```

测试方案约 6 秒主动恢复，另安排独立的 12 秒保险；**用户明确报告左侧没有变化**。虽然当时依据同系列的 DIMM 控制寄存器定义解释了 bit30 等字段，但**在 S905L3A 上该功能的每一位意义仍需结合实际源码核实**。

**重要结论**：先前 CPU/fb0 全白与现在 OSD1 内部纯白，均只能改变右侧。该现象大幅降低“只是用户态色值顺序错误”的解释力；优先检查 OSD1 的真实覆盖范围、混合与后续 ENCL 输入，而不是继续 RGB 置换。注意：这不自动证明 DSI/PHY 完全无关。

### 2.4 Blend / Postblend 坐标和背景寄存器

**目的**：判断 OSD 混合器和 VPP 的实时扫描状态；寻找固定紫色是否可能是背景颜色。

**[实测]**

```text
0xFF90E6FC（OSD_BLEND_CURRENT_XY）：8 次均为 0x00040000
0xFF907494（VPP_POSTBLEND_CURRENT_XY）：8 次均为 0x00000000
背景相关读取：
0xFF90E6E4=0x00000000
0xFF90E6E8=0x00000000
0xFF90E5A0=0x00008080
0xFF90E5A4=0x00000000
```

早期历史还读到 Blend 窗口覆盖水平 0–799 / 垂直 0–1279，`VPP_OSD_SC_CTRL0=0`。**[推断]** 配置看似完整，但“当前 XY”不变无法直接证明该模块停滞：寄存器是否在本 SoC、本路径上更新尚需确认。背景寄存器数值也没有给出可直接判定紫色来源的依据。

### 2.5 ENCL 动态状态与前级扫描状态对照

**目的**：将后级扫描计数与 OSD/VPP 状态同期读取，区分“整个时序停了”与“前级状态未更新”。

**[实测]**

```text
VIU 到显示编码器路由（0xFF909C68）：0x00000000

ENCL_INFO_READ（0xFF909C7C）连续12次：
0x44A401F6, 0x622002FA, 0x80A700F8, 0x844300EC,
0xA49101A0, 0xC54401BC, 0xE33F0104, 0x01EA00EA,
0x22F70122, 0x42B20204, 0x62820052, 0x8090008C

同时 OSD_BLEND_RO_CURRENT_XY：始终 0x00040000
同时 VPP_POSTBLEND_CURRENT_XY：始终 0x00000000
```

**结论边界**：ENCL 状态寄存器有变化，支持 ENCL 显示时序在运行；路由 0 与之前对照的 VIU1→ENCL 选择一致。OSD/VPP 状态固定仍需判断是否因为该 SoC 没有通过这两项寄存器报告实际进度，**不能认定其硬件必然卡死**。

### 2.6 VD1 旧 Postblend 位：短时单变量测试

**动机**：历史寄存器 `VPP_MISC=0x00000482` 中存在旧 VD1 Postblend bit10（`0x400`），但此前 `VD1_BLEND_SRC_CTRL=0`。这可能是无害遗留，也可能影响画面；实验仅变更这一位。

**[历史短时写入、已恢复]**

```text
VPP_MISC（0xFF907498）原值：0x00000482
短时值：0x00000082（清除 0x400）
用户现场反馈：毫无变化
复查值：0x00000482
```

命令中使用约 6 秒测试、15 秒独立恢复保险。**实际观察结果支持降低这一旧混合位作为根因的优先级**。不再对此做重复写入。

### 2.7 VPP_HOLD_LINES：短时单变量测试

**动机**：前期上游记录称原厂 Android 值 `0x08080808`，而 Linux 实机 `0x00000404`，主线前/后混合保持行数为 4；测试低位两个字段由 4 改为 8，不照搬 Android 全寄存器值。

**[历史短时写入、已恢复]**

```text
VPP_HOLD_LINES（0xFF907488）原值：0x00000404
短时值：0x00000808
用户现场反馈：毫无变化
复查值：0x00000404
```

测试约 6 秒，另设 15 秒恢复保险。**结论**：这次改动对现有屏幕异常没有可见帮助，停止此方向；上游 Android 数值不是本次在原厂 Android 直接采样所得。

### 2.8 DSI Host 初始化 / 时钟 / PHY 状态

**[实测]**

```text
uname -r: 6.12.41-trim
[10.070265] c10h-dsi-host: requested=456642000 actual=531000000 pixel_target=76107000
[10.070317] c10h-dsi-host: final bit=531000000 px=75857143 configured_hs=531000000
[10.711909] panel-c10h-jd9366tc: C10H stock panel initialization complete
[10.722321] meson_encl_set_gamma_table: GAMMA WR_RDY timeout
[13.075272] meson_encl_set_gamma_table: GAMMA ADR_RDY timeout
[13.085315] ... ADR_RDY timeout
[15.659274] ... ADR_RDY timeout
[15.669299] ... ADR_RDY timeout
[18.242926] ... ADR_RDY timeout

历史诊断：DSI PHY=0x1529
稍后诊断：DSI PHY=0x1FB9
DSI PWR=1, COLOR=5, MODE=0, VIDMODE=0x3F02, PKTSIZE=800,
HSA=7, HBP=25, HLINE=764, VSA=8, VBP=20, VFP=140, VACT=1280,
LANES=0x2003, TOP=0x512400
```

`/sys/kernel/debug/clk/clk_summary` 本次不可读取，因此**没有取得运行时完整时钟树**。前期对照称 Arrow36 另一环境也有 531MHz / 75.857MHz 附近的时钟但能显示完整几何图；因此单凭 0.33% 像素时钟偏差不足以解释 fnOS 的严重空间分区。

前期源码解读认为 PHY 读数来自 `DSI_PHY_STATUS`，是动态状态而非固定配置。`0x1529 → 0x1FB9` 可能是 Lane Stop State 的正常变化；**不是已证实的 PHY 故障**。

### 2.9 DSI 错误状态与中断掩码

**目的**：排查 Synopsys DesignWare DSI 自身报告的视频/PHY 错误，包括 `INT_ST1` bit19 视频负载缓冲欠载。

**[实测]**

```text
DSI_PHY_STATUS, 0xFFD070B0 = 0x00001FB9
INT_ST0, 0xFFD070BC = 0x00000000
INT_ST1, 0xFFD070C0 = 0x00000000
INT_MSK0, 0xFFD070C4 = 0x00000000
INT_MSK1, 0xFFD070C8 = 0x0003E000
再次读取 INT_ST0 / INT_ST1 仍为 0
```

`INT_MSK1=0x0003E000` 设置的是 bit13–17，不包括先前关注的 bit19 (`0x00080000`)。**[推断]** 这次没有观察到 DSI 报告的 DPI 视频载荷缓冲欠载；但采样为零不能证明全部传输时序或像素排序正确，也不能证明错误寄存器在此平台能够报告所有故障。检查至此停止重复读寄存器。

### 2.10 `simplefb` / `simpledrm` 是否干扰 DRM 接管

**[实测]**

```text
/proc/fb: 0 mesondrmfb
/sys/class/graphics/fb0/name: mesondrmfb
fb0 驱动：/sys/bus/platform/drivers/meson-drm
/chosen/framebuffer-hdmi: status = disabled
/chosen/framebuffer-cvbs: status = disabled
启动日志： [18.388108] meson-drm ff900000.vpu: [drm] fb0: mesondrmfb frame buffer device
```

**结论**：未发现还有一个已启用的 simplefb 占用 `fb0`，也没有证据证明 simpledrm 接管了当前显示。该方向本轮结束，**无需为此重新生成 DTB**。

### 2.11 Windows：公开 Linux 6.12.41 与 6.12.114 的 10 文件 SHA-256 对照

**目的**：检验 6.12.41→6.12.114 是否在主线 Meson 显示文件中存在直接可复用的修复。

**[实测] Windows PowerShell 逐文件哈希对照，全部 `SAME`：**

```text
SAME meson_crtc.c
SAME meson_plane.c
SAME meson_viu.c
SAME meson_vpp.c
SAME meson_venc.c
SAME meson_vclk.c
SAME meson_drv.c
SAME meson_dw_mipi_dsi.c
SAME meson_encoder_dsi.c
SAME meson_registers.h
```

下载路径为公开 `gregkh/linux` 对应 tag 下的 `drivers/gpu/drm/meson/`。**结论非常有限但重要**：这 10 个文件的**公开主线版本**内容相同，不能据此证明 `6.12.41-trim`（飞牛定制）与 `6.12.114-ophub`（另一构建）的实际内核二进制相同；也不排除其他文件、构建配置或补丁差异。本轮不再机械地比对这同一组公开文件。

### 2.12 fnOS 实际内核配置提取、上传与重点比较

**[实测] 设备输出：**

```text
Linux c10h 6.12.41-trim #219 SMP PREEMPT Thu Feb 12 01:38:06 UTC 2026 aarch64 GNU/Linux
Linux version 6.12.41-trim ([构建账户已脱敏])
GCC: aarch64-linux-gnu-gcc (Debian 12.2.0-14) 12.2.0
LD: GNU ld (GNU Binutils for Debian) 2.40
内核配置来源：/boot/config-6.12.41-trim
```

用户将其经 `scp` 保存成 `c10h-fnos-config.txt` 并上传；实际配置为约 11,112 行，完整文件另行保留。关键项：

```text
CONFIG_ARCH_MESON=y
CONFIG_ARM64_4K_PAGES=y
CONFIG_CMA=y
CONFIG_CMA_AREAS=7
CONFIG_DMA_CMA=y
CONFIG_CMA_SIZE_MBYTES=128
CONFIG_CMA_ALIGNMENT=8
CONFIG_DRM_MESON=y
CONFIG_DRM_MIPI_DSI=y
CONFIG_DRM_DW_MIPI_DSI=y
CONFIG_DRM_MESON_DW_MIPI_DSI=y
CONFIG_MESON_CANVAS=y
CONFIG_DRM_FBDEV_EMULATION=y
CONFIG_DRM_FBDEV_OVERALLOC=100
CONFIG_SWIOTLB=y
```

`CONFIG_DRM_MESON=y` 表示主显示驱动**内建**，所以没有 `meson_drm` 的 `lsmod` 项是正常的。`lsmod` 中确实可见 `phy_meson_g12a_mipi_dphy_analog`、`phy_meson_axg_mipi_dphy` 两个模块已加载。

当时对照的 **ophub 公开 6.12 stable 配置**（不是 Arrow36 实机完整配置）中：

| 配置选项 | fnOS 实测 | 当时查阅的 ophub 公开配置 | 是否已经证明是根因？ |
|---|---:|---:|---|
| `CMA_SIZE_MBYTES` | 128 | 256 | 否；缓冲分配与 Canvas 映射已成功 |
| `DRM_FBDEV_OVERALLOC` | 100 | 300 | 否；DRM 原生 dumb buffer 也异常 |
| Meson 模拟 D-PHY 驱动 | `m` | `y` | 否；模块已加载、面板已驱动 |
| AXG MIPI D-PHY 驱动 | `m` | `y` | 否；模块已加载 |
| Meson DRM / Canvas | `y` | `y` | 未见差异 |
| 内核抢占 | PREEMPT | 另一抢占模式（历史比较） | 没有可解释空间分区的证据 |
| `HZ` | 250 | 历史比较记为 300 | 优先级低 |

**范围提醒**：该比较是对显示、内存、DMA、PHY 等重点项的核对，**不是已完成两个完整配置文件的逐项机器 diff**；也不是来自 Arrow36 实机。同样，CMA=128MB 不足以证明 DMA 一定没有别的问题，不能因差异简单把 128 改成 256。

### 2.13 DTB 全属性 diff：131 行，最终已审阅到末尾

**目的**：核实此前设备树叠加是否意外覆盖了 VPU / clocks / phys 等属性，从而导致 Linux 图像异常。

**[实测]**

```text
原 DTB：/boot/c10h-lcd-stage/base-original.dtb
现 DTB：/boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb
中间文件：/tmp/c10h-original.dts、/tmp/c10h-current.dts
差异文件：/tmp/c10h-dtb-diff.txt
wc -l /tmp/c10h-dtb-diff.txt → 131
```

用户先贴出差异行 1–100，随后补齐 101–160 的余下内容。按本轮贴出的 diff 和前期 FDT 解析，变化包括：

| 修改 | 观察与判断 |
|---|---|
| 新增 `/backlight-c10h` | `pwm-backlight`；配置 brightness levels、PWM 和延时，预期改动 |
| DSI `dsi@7000` | `disabled → okay`，原有 `assigned-clocks` 和父时钟属性在所示 diff 中未被替换 |
| DSI `panel@0` | 新增 `arrow36,c10h-jd9366tc`，包括 `c1-gpios`、`a11-gpios`、`c2-gpios`、backlight phandle |
| DSI/panel graph | 新增互相引用的 endpoints |
| MIPI D-PHY 两级节点 | `disabled → okay`，预期改动 |
| `pwm@1a000` | 添加 `pinctrl-0`、`pinctrl-names`、启用，预期用于背光 |
| HDMI / 音频等其他节点 | 包含历史的 HDMI 禁用等预期变更；以完整 diff 为准 |
| `mmc@ffe05000`（SD） | `okay → disabled` |
| `mmc@ffe07000`（eMMC） | `okay → disabled`，用于当前 USB Linux/原厂 Android 隔离方案 |
| `__symbols__` | 新增 `c10h_dsi_out`、`c10h_panel_in`、`c10h_backlight`，仅符号元数据 |

前期二进制 FDT 解析已报告原 DTB 565 节点、当前 570 节点，新增 5 个、删除 0 个。本次文本 diff 进一步核实属性变化。

**结论**：在本轮看到的完整 131 行 DTS 差异范围内，**未发现 VPU 主节点被意外修改，也未看到原 DSI assigned-clocks 被意外替换**。这降低了“叠加时改坏 VPU”作为根因的优先级。仍不能据此保证所有引脚、时钟、硬件连线对真实 C10H 都绝对正确；也不能把“DTB 文本不同”与“显示故障”简单画等号。**不建议在现阶段重制 DTB**。

### 2.14 既有驱动编译目录与来源追溯：当前停下的地方

**目的**：准备核对当前已安装的实验 DSI Host/面板 `.ko` 是否确实与 Arrow36 某个提交版本一致。

**[实测]**

```text
尝试查找：/tmp/c10h-lcd.0AY24B/repo
结果：原编译目录已不存在
在 /tmp /usr/src 搜索 c10h-dsi-host.c、panel-c10h-jd9366tc.c：没有结果
```

**解释**：先前的构建目录建在 `/tmp`，经过重启或清理后消失完全正常；并不意味着系统当前加载模块异常。已安装模块 `.ko` 仍在 `/lib/modules/6.12.41-trim/extra/`。此前整理曾记录 SHA-256：

```text
c10h-dsi-host.ko
  d90c5ed1bcca4595eb1c843a79e86e2765bedcac7af058c49a1bfe13ae0d378c
panel-c10h-jd9366tc.ko
  fb50a48ba01ff2884133a4a7b12048e207d25280bd10a934242fedd3ec1ad135
```

上述哈希是**旧记录中的历史值，本轮尚未重新执行校验**。不能仅凭 ELF 文件哈希与重新构建版不同就证明 C 源码不同，因为路径、编译器选项、构建时间/调试信息等都可能影响二进制。

**最后提出但尚未执行的动作**：`modinfo`、`sha256sum` 重新校验两个已安装模块，并搜索 `/boot/c10h-lcd-stage` 中可能存在的 `.ko/.c/.patch/.diff/.tar*`。下一次接续从这里开始即可。

---

## 3. 浏览器诊断台：实现、实测与 CSV 定量结果

### 3.1 前期滑杆版本与本轮诊断台的区别

之前使用 Windows 浏览器通过 SSH `-L 8765:127.0.0.1:8765` 连接 C10H 本地的 Python `ThreadingHTTPServer`。先后生成：

- `c10h_live_slider.py`：可移动色块、R/G/B、背光调节；
- `c10h_live_slider_rgb_test.py`：新增 RGB 循环补偿；
- `c10h_fullscreen_color_probe.py`：新增整帧纯色模式，用于验证“左侧不响应”的空间问题。

**本轮新建的是只读诊断台**：`c10h_display_monitor.py`，配套 ZIP `C10H_显示链路实时诊断台_只读版.zip`；新增定时采样 ENCL / OSD / VPP、网页趋势图、现场现象标记、CSV 导出。它**不写 fb0、不写 OSD/VPP/DSI 寄存器、也不修改背光**；只用 `busybox devmem <addr> 32` 做固定地址读取，在 `/tmp` 保存日志。该程序通过模拟数据的本地检查；用户随后上传了实机 CSV，证明在其 C10H 上至少采样/导出链路能够工作。

### 3.2 本轮实机 CSV 统计（已重新对上传的原文件核算）

文件名：`c10h-display-monitor.csv`。带有 `seq,timestamp,monotonic_s,read_ms,...` 等列。

| 指标 | 结果 |
|---|---|
| 采样行数 | 186 |
| 首次 | t = 0 秒 |
| 末次 | t = 138.449 秒 |
| 首尾跨度 | 138.449 秒 |
| `error` 非空数 | 0 |
| `encl` 不同值数 | 186 |
| `osd` | `0x00040000`，186/186 相同 |
| `vpp` | `0x00000000`，186/186 相同 |
| `venc_mux` | `0x00000000`，186/186 相同 |
| `dimm` | `0x00000000`，186/186 相同 |
| `fifo` | `0x028207E1` ×178；`0x029207E1` ×8 |
| `hold` | `0x00000404`，186/186 相同 |
| `misc` | `0x00000482`，186/186 相同 |
| `vpp_fifo` | `0x0003D000` ×130；`0x0003CFFE` ×32；`0x00039000` ×24 |
| `osd_blend` | `0x8F100001`，186/186 相同 |
| `osd_path` | `0x00000000`，186/186 相同 |

**关于 FIFO 的短时变化**：`0x028207E1 → 0x029207E1` 差 `0x00100000`（bit20）。程序对慢速寄存器的读取频率低于每行采样频率，CSV 会在后续几行复用最近一次慢速读数，因此**8 行不同不等于 8 次独立硬件异常**；按前期程序实现解读约对应一次新的 FIFO 读取。bit20 含义与运行时行为没有被充分验证，不能据此宣布“FIFO 欠载/供数断裂”。

**主要贡献**：在较长采样窗口内确认 ENCL 读数持续变化，而 OSD/VPP 当前 XY 保持固定；但尚无法由此单独定位画面丢失的硬件阶段。

### 3.3 启动流程与安全边界

当时使用的 PowerShell 示例（IP 可能失效）：

```powershell
scp "$env:USERPROFILE\Downloads\c10h_display_monitor.py" test-user@C10H_IP:/tmp/
ssh -t -L 8765:127.0.0.1:8765 test-user@C10H_IP "sudo python3 /tmp/c10h_display_monitor.py"
```

浏览器访问 `http://127.0.0.1:8765`。诊断台只监听 C10H 的 loopback 地址，不能对公网开放。历史服务断开需正常结束，避免与其他监听同一端口 `8765` 的滑杆版本冲突。**不要把设备任意 MMIO 写操作做成无认证的网页滑杆。**

---

## 4. 重点寄存器与实验状态索引

> 下表把前期与本轮反复出现的地址集中列出，供对照查找。地址、命名来自本轮所用 Linux/DesignWare/厂商资料的解释；**不同 SoC 版本可能存在差异**。表内“只读”不代表 MMIO 访问绝对无风险。

| 寄存器/用途 | 地址 | 本轮重要读数/短时测试 | 现状 |
|---|---|---|---|
| OSD1 FIFO `VIU_OSD1_FIFO_CTRL_STAT` | `0xFF9068AC` | `0x028207E1`；CSV 有 `0x029207E1` | 初始化符合预期；无明确欠载证据 |
| OSD1 MALI UNPACK | `0xFF9068BC` | `0x00101234` | 未发现 AFBC 错误开启证据 |
| VPP FIFO STATUS | `0xFF9074A0` | `0x3D000`, `0x3CFFE`, `0x39000` | 有变化，无确诊 |
| OSD1 DIMM | `0xFF906B7C` | 初始与恢复 `0x0`；短时白 `0x7FFFFFFF` | 右侧变白，左侧紫色不变 |
| OSD_BLEND 当前 XY | `0xFF90E6FC` | `0x00040000` | 多次恒定，含义未完全解释 |
| VPP_POSTBLEND 当前 XY | `0xFF907494` | `0x00000000` | 多次恒定，含义未完全解释 |
| VIU→VENC 选择 | `0xFF909C68` | `0x00000000` | 前期解释为 VIU1→ENCL |
| ENCL_INFO_READ | `0xFF909C7C` | 每次变动 | ENCL 时序在运行 |
| VPP_MISC | `0xFF907498` | `0x482 → 0x82 → 0x482` | 临时关闭 VD1 Postblend 无效，已恢复 |
| VPP_HOLD_LINES | `0xFF907488` | `0x404 → 0x808 → 0x404` | 保持行数变化无效，已恢复 |
| OSD Blend 控制 | `0xFF90E6C0` | `0x8F100001` | 长期固定 |
| DSI PHY STATUS | `0xFFD070B0` | `0x1529`, `0x1FB9` | 动态状态，未见直接报错 |
| DSI INT_ST0 / ST1 | `0xFFD070BC` / `0xFFD070C0` | `0x0` / `0x0` | 未报告错误 |
| DSI INT_MSK0 / MSK1 | `0xFFD070C4` / `0xFFD070C8` | `0x0` / `0x3E000` | bit19 未被该掩码置位 |

**不要照着表格盲写寄存器。** 其中既包括动态只读状态、当前配置，也包括本轮曾做过的一次性临时测试；永久写入或在不同启动阶段写入可能产生不可预期后果。

---

## 5. 已排除或明显降级的方向（避免下一轮绕圈）

| 假设 | 证据 | 结论强度 |
|---|---|---|
| 用户态画图函数单独写错 | DRM 官方风格的 linear dumb buffer 测试同样异常；fb0 全白后左紫不变 | 不能作为唯一解释；并非绝对排除所有 DMA/缓存问题 |
| RGB 通道置换就是全部原因 | 颜色逆置换有效，但空间紫色区依旧；白色本身 R=G=B | 可排除“仅仅 RGB 顺序”的简单解释 |
| OSD1 FIFO 初始配置错误 | `0x028207E1` 与历史预期一致 | 初始配置未见问题；运行态仍未完全排除 |
| 错误打开 AFBC | MALI UNPACK 高使能位没开；之前路径选择为 0 | 无直接支持证据 |
| VD1 旧 Postblend 位造成覆盖 | 清除该位 6 秒，屏幕完全无变化；已恢复 | 优先级低 |
| `VPP_HOLD_LINES` 太低导致当前紫屏 | 4→8 的短时测试无任何可见变化；已恢复 | 该简单调整无效 |
| DSI 已报告 payload underflow | INT_ST1=0；相关 mask bit19 未置位 | 未观察到报告，但不能保证所有像素传输正确 |
| simplefb 与 DRM 冲突 | 两个 simplefb 节点 disabled；唯一 fb0 为 mesondrmfb | 当前环境不支持该假设 |
| DTB 合并误改 VPU 主节点 | 131 行 diff 全部检查，未见意外修改 | 该假设被明显削弱 |
| 6.12.41→6.12.114 的这 10 个公开 DRM 源码修复了问题 | 10 个文件均 SAME | 没有这种**公开文件级**差异；不等同于实际内核一致 |
| 把 CMA 128MB 改成 256MB 就能修复 | 缺乏 DMA 分配失败/欠载证据 | 不支持直接重配内核 |

**未排除/仍需研究**：实际 Meson DRM/VPP/ENCL 初始化路径、不同构建的内核补丁与运行时选择、正常像素流进入 ENCL 的路径、某些不能被错误状态寄存器报告的时序或像素路由问题、实际 C10H 自制 DSI Host 模块与上游代码的差异。以上只是候选，不可在没有 A/B 验证时当作结论。

---

## 6. 历史上引用过、但不可过度解读的线索

1. **Gamma 超时与 2026-10 的 Linux 讨论**：前期聊天曾引用一个“ENCL 未启用时加载 Gamma 表”的修复补丁，说明顺序错误可能导致 `WR_RDY/ADR_RDY timeout`；本轮没有对补丁重新进行独立适用性审计，也没有修复 Gamma。Gamma 超时是事实，补丁是否解决本机空间花屏仍未证明。
2. **原厂 Android 的寄存器差异**：例如 `VPP_HOLD_LINES=0x08080808`、`0x1D5F=0`、`0x38A0=0x1C040` 来自历史引用的上游记录，**不是本轮在原厂 Android 实机采集**；不能整批照搬。
3. **Arrow36 的“完整几何图”**：历史引用称其公开实验版 `6.12.114-ophub` 可显示完整边框、十字和白块，但仍有偏色/抖动。应按仓库提交 ID 与实际测试条件确认，不应把这个结果等同于已经有“完全正常显示”的现成补丁。
4. **OSD/VPP 当前 XY 不变化**：采样层面可信；是否本 SoC 路径上的实时有效坐标、为什么不变仍需对比硬件实现。不能仅靠这点判定管线停止。
5. **本轮无装置断电验证**：从终端可确认临时写入的两个参数已恢复、单色使能已恢复；这不等于所有瞬态状态都已经由一次重新上电校正。当前可登录并采集数据。

---

## 7. 下一次接续的具体任务与安全顺序

### 第一优先：确认现有实验 `.ko` 的来源和版本

**该步骤在本轮结束时尚未执行。** 从已经运行的系统中读取模块文件元信息及持久备份，**不替换文件、不重启**。

```bash
echo "===== 已安装模块信息 ====="
for MOD in c10h_dsi_host panel_c10h_jd9366tc; do
  echo "===== $MOD ====="
  P=$(modinfo -n "$MOD" 2>/dev/null)
  if [ -f "$P" ]; then
    echo "路径: $P"
    modinfo "$P" | grep -E '^(filename|name|version|srcversion|vermagic|depends):'
    sha256sum "$P"
  else
    echo "未找到模块文件"
  fi
done

echo "===== 检查持久备份 ====="
find /boot/c10h-lcd-stage -maxdepth 2 -type f \
  \( -name '*.ko' -o -name '*.c' -o -name '*.patch' -o -name '*.diff' -o -name '*.tar*' \) \
  2>/dev/null | sort
```

说明：`modinfo` / `sha256sum` 能验证文件版本指纹与内核兼容信息，但**不能自动还原源码提交号**；若没有 `srcversion`，应与上游具体 commit 及重构建环境结合分析。若模块文件的编译路径在 `/tmp` 丢失，可在 Windows 下载上游源码作**离线对照**，不得直接用新 `.ko` 覆盖当前稳定可启动的模块。

### 第二优先：建立有控制变量的“同条件 A/B”

- 取得上游 Arrow36 精确 commit、DSI Host/Panel/设备树版本、测试画面和该环境相关的运行状态；优先同一物理硬件/同一面板。
- 若能获得两个系统的实际 Meson DRM 源码/补丁集和编译产物，再比较，而非继续比较 10 个已经 `SAME` 的公开主线文件。
- 关注 **普通像素经过 OSD/Blend/VPP 到 ENCL** 的路径与代码初始化先后顺序；把 ENCL 内部彩条仅视为“后段测试通路正确”的部分证据。
- 修改前保持独立回退能力，单变量测试、记录原值、预设自动恢复、现场拍照。**不要无依据批量写 VPP、HDR、OSD、DSI 寄存器**。

### 第三优先：系统改动之前做可启动备份

- 保留原 `/boot` DTB、U-Boot 脚本、自动加载配置、现有 `.ko` 的备份。
- `/boot/aml_autoscript` 与 `/boot/s905_autoscript` 可能包含硬编码 DTB 路径和 bootargs；**只改 `/boot/uEnv.txt` 未必能改变启动流程**。
- 原厂 Android eMMC 保留，当前 Linux 从 USB 启动。不要因为 Linux 显示故障去刷 eMMC。
- 先确保能在 Windows/其他机器离线访问 USB FAT32 `/boot` 分区进行恢复，再做高风险试验。

---

## 8. 文件、路径与资料索引

| 名称 / 路径 | 用途与状态 |
|---|---|
| `C10H_fnOS_内屏适配_完整对话与最新进展_2026-11-04.md` | 前一阶段的大型整理，含初始化、几何图、彩条、RGB/Canvas、原始终端日志等 |
| `C10H_浏览器实时滑杆诊断_方法与成果_2026-11-04.md` | 前期浏览器滑杆方案与三版 framebuffer 测试方法 |
| `C10H_显示链路实时诊断台_只读版.zip` | 本轮生成的硬件状态只读网页诊断台 |
| `c10h_monitor/c10h_display_monitor.py` | 只读网页服务 Python 实现 |
| `c10h-display-monitor.csv` | 本轮实测 186 行、138.449 秒硬件状态采样 |
| `c10h-fnos-config.txt` | 本轮上传的完整 Linux 6.12.41-trim 内核配置 |
| `/boot/config-6.12.41-trim` | C10H 的内核配置源文件 |
| `/boot/dtb/amlogic/meson-g12a-s905l3a-c10h-lcd-hdmi-no-emmc.dtb` | 当前 USB 启动引用的 DTB |
| `/boot/c10h-lcd-stage/base-original.dtb` | 原始 DTB 备份 |
| `/boot/c10h-lcd-stage/lcd-hdmi-working.dtb` | 曾可用 HDMI 的设备树备份 |
| `/boot/c10h-lcd-stage/aml_autoscript.original` | U-Boot 脚本原始备份（历史列出） |
| `/boot/c10h-lcd-stage/s905_autoscript.original` | U-Boot 脚本原始备份（历史列出） |
| `/lib/modules/6.12.41-trim/extra/` | 当前两个自编译模块安装位置 |
| `/etc/modules-load.d/c10h-lcd-test.conf` | 自编译模块开机加载设置 |
| `/tmp/c10h-dtb-diff.txt` | 本轮 131 行 DTB diff；位于临时目录，重启可能丢失 |
| `/tmp/c10h-kernel.config` | 本轮从 `/boot/config-*` 复制的临时配置 |
| `/tmp/c10h-lcd.0AY24B/repo` | **已不存在**的旧源码构建目录 |
| `<WORKDIR>\C10H-kernel-compare` | Windows 上存放两套公开 Linux Meson 源文件的比较目录（历史操作路径） |
| https://github.com/Arrow36/c10h-sk-l1001-s905l3a-armbian | 上游实验项目，仍需核实精确提交版本 |

---

## 9. 下次会话可以直接使用的接续摘要

> 我在适配创维 C10H / SK-L1001（Amlogic S905L3A，10.1 英寸 JD9366TC，800×1280 MIPI DSI）上的非官方 ARM fnOS 1.1.20 / Debian 12 / Linux 6.12.41-trim #219。Linux 从 USB 启动，原厂 Android eMMC 保留且当前 DT 禁用；HDMI 禁用、DSI 自动点亮。已移植 Arrow36 的 C10H DSI Host + JD9366TC panel 模块，DRM XR24/linear framebuffer 800×1280，pitch 3200，主 fb 旧编号 43，DMA `DMA_ADDR` 与 Canvas 地址一致。ENCL 内部硬件彩条完整、无重复，但普通图像仍是左侧大块紫色固定不变、右侧重复闪动。浏览器写 fb0 全白时左紫不变右白；OSD1 `DIMM_CTRL` 短时强制白色同样左紫不变右白，随后寄存器恢复 0。RGB 常规显示 R→G/G→B/B→R，测试代码用反向排列可补偿颜色，但未修复空间问题。
>
> 本轮排查：OSD1 FIFO 初值 `0x028207E1` 符合上游预期；MALI UNPACK `0x00101234` 未见 AFBC 错开；VPP FIFO 有动态值但无确认欠载；ENCL INFO 持续变化，OSD Blend 当前 XY 固定 `0x00040000`，VPP Postblend 当前 XY 固定 `0x00000000`（不可直接认定扫描停顿）。旧 `VPP_MISC` VD1 位 `0x482→0x82→0x482` 临时实验**无变化**；`VPP_HOLD_LINES 0x404→0x808→0x404` 临时实验也**无变化**。DSI INT_ST0/ST1 均为 0、INT_MSK0=0、INT_MSK1=`0x3E000`，未报告 payload underflow；PHY 状态 `0x1529/0x1FB9` 属动态状态，不能说明失锁。两处 simplefb disabled，只有 `mesondrmfb`。Windows SHA 比较主线 v6.12.41/v6.12.114 的 10 个 Meson DRM 文件全 SAME；不能证明两个定制运行内核相同。fnOS 配置 `CONFIG_DRM_MESON=y`、`CONFIG_MESON_CANVAS=y`、CMA=128MB、FBDEV_OVERALLOC=100；与 ophub 公开配置在 CMA 128/256、FBDEV_OVERALLOC 100/300、PHY 模块/内建等有差异但未证实是根因。原/现 DTB 转 DTS diff 总计 131 行，全部已读，只有预期面板/背光/DSI/PHY/PWM/HDMI/SD/eMMC/symbols 变化，没有发现 VPU 被误改。只读浏览器诊断台实机 CSV 186 行/138.449 秒、无采样错误；ENCL 值各异、OSD/VPP 固定，FIFO `0x029207E1` 一次慢速读数曾短暂出现（CSV 复用 8 行），不能解释为已确认欠载。
>
> **当前停止点**：此前 `/tmp/c10h-lcd.0AY24B/repo` 源码目录已消失；还没有核对当前已安装两个 `.ko` 的 `modinfo` / `sha256sum` / `/boot/c10h-lcd-stage` 是否有源码或 patch。下一步从这组只读检查开始，尝试确认当前模块与 Arrow36 精确提交的关系；之后再研究实际内核定制补丁及 OSD/VPP→ENCL 数据路径。不要重做彩条、几何图、纯白、Canvas、simplefb、CMA 猜测、已无效的两次寄存器写入；不盲写寄存器、不刷 eMMC、不先升级内核。**屏幕尚未修复。**

---

## 10. 本文范围与数据质量声明

- 文中所有“本轮实测”以用户实际贴出的命令输出、描述及上传 CSV/内核配置为准。**没有在 C10H 上远程执行本次归档中的任何新命令**。
- 本文保留前期研究文档和本轮聊天中的推断，但对缺少实机证据的部分明确保留限定，不把先前助手的每一条寄存器命名、位解释或上游研究概括视为已全面证实的真理。
- 本文只整理 **2026-11-04 的当前聊天与此前两份上下文整理的关键衔接点**；旧文件包含更多早期逐条终端记录、启动脚本修改历史与照片索引。若需要原始字节级证据，应对照原始 CSV、DTB、模块文件及已有完整日志。
- 目前唯一应当作为结案结论的事实是：**内屏自动点亮、硬件彩条完整；普通画面左右分区与闪动尚未修复**。
