# 硬件支持状态

“驱动已加载 / 设备已注册”不代表设备的所有功能已经实测。当前 Linux 的 model 字符串仍为 SKYWORTH E900V22C，这是借用的 DTB 名称，不是实物型号。

| 设备 | 现状与证据 | 后续工作 |
| --- | --- | --- |
| CPU / 内存 | ARM64 Linux 已运行；约 1.89 GiB 可用内存 | 稳定性与功耗测试 |
| USB / 存储 | U 盘根文件系统、集线器、外接键盘已使用 | 长时读写与断电恢复测试 |
| Ethernet | eth0，100 Mbps 链路、SSH 和互联网已验证 | 保持可用的调试连接 |
| HDMI | 外接显示器可见控制台；meson DRM 已注册 | 分辨率、色彩、热插拔；日志仍有显示模式警告 |
| GPU | panfrost 已加载，renderD128 已注册 | 未验证实际渲染加速 |
| 内置 LCD / 背光 | 无对应显示输出与 backlight 设备 | 原厂屏幕型号、接口、供电、时序 |
| 内置触摸 | Linux input 列表未见触摸设备 | 原厂 DT 包含 Goodix/FocalTech 等候选节点，必须确认实际启用的芯片 |
| Wi-Fi | Linux lspci 未枚举设备；现 DTB 仍有 SDIO wifi32k deferred probe | 原厂是 PCIe W2 / w2_comm；先适配 PCIe、供电和复位，再移植驱动与固件 |
| 音频 | 已注册 axg-sound-card / U200 | 扬声器、功放、麦克风尚未实际测试 |
| 蓝牙 / 摄像头 | 未完成识别和验证 | 对照原厂 DT、驱动和固件 |
| 温度 | 已注册 thermal zones | 进一步核对传感器和温控 |
| eMMC | 测试 DTB 主动禁用 | 不应解释成内核不支持 eMMC |

原厂 Android 4.9 的 32 位 `.ko` 不能直接加载到当前 6.12 ARM64 内核。应逐项核对主线驱动、设备树绑定、固件及厂商驱动源码兼容性。
