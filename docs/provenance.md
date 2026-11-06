# 测试镜像来源

## Armbian 历史镜像记录

以下来自初始实验文档中的资产与校验记录，本轮没有重新下载镜像或复算哈希：

- 文件：`Armbian_26.11.0_amlogic_s905l3a_trixie_6.12.114_server_2026.11.02.img.gz`
- 下载链接：https://github.com/ophub/amlogic-s9xxx-armbian/releases/download/Armbian_trixie_arm64_server_2026.11/Armbian_26.11.0_amlogic_s905l3a_trixie_6.12.114_server_2026.11.02.img.gz
- 压缩文件长度：899418763 字节。
- 原记录所载 GitHub 资产 SHA-256：`db4732a41a03c2de729e6ab6bd8b3119cbf3df6d279e2a86ec7bab46b0d2d76f`；下载后已核对。
- 初次修改并写盘的镜像长度：3686793216 字节。
- 初次修改镜像 SHA-256：`ae568d1ff6be177358920192d3c408c37c9ced488ee5d1982fa15c7b7fd6264f`；写入 U 盘后整个镜像区域读回一致。

这个修改镜像哈希仅对应初次写盘时的文件。之后 U 盘增加了启动记录服务、首次初始化、根分区扩容、密码设置和网络配置，不能用它校验当前已运行的 U 盘内容。

完整镜像未上传 Git。仓库中的构建工具是从本次实际使用的修改逻辑整理而来；make_boot_script 默认时间戳为 0，可通过 SOURCE_DATE_EPOCH 指定时间戳，因此重新构建的脚本容器头部可能与初次制作的容器不同。

## fnOS 测试镜像

- 社区 ARM fnOS 1.1.20，Debian 12，运行内核 `6.12.41-trim #219`。
- 文件：`fnnas_amlogic_s905l3a_k6.12.41_2026.09.23.img.gz`。
- [原记录引用的发布页](https://github.com/ophub/fnnas/releases/tag/fnnas_amlogic_693)。
- 原记录中的压缩包 SHA-256：`94652b7556fa8c1bd78298003fd6cf0db31d03fa8ecd681e997b74c77b367a87`，当时下载后已核对。
- 写盘前替换了测试 DTB 与一次性启动脚本；根文件系统为 U 盘 Btrfs，与 Armbian 的 ext4 脚本参数不能混用。

以上版本、链接与校验值用于追溯历史实验，本次整理未重新下载镜像或检查当前发布资产。替换镜像后须重新核对校验值、UUID、文件系统和启动配置。

## 原始资料与来源局限

[原厂 Android 项目](https://github.com/Arrow36/tianyi-c10h-skyworth-SK-L1001-mod)公开页面可查到 SK-L1001 / p291_iptv、2 GB / 64 GB、Android 9 / 4.9.113 等描述。这是同一维护者的研究资料，不是厂商正式规格书或独立认证；本仓库未附原厂完整 DTB/固件、原始提取日志、寄存器手册和初始化逐项对照文件。

`drivers/c10h-display/c10h-panel-init.h` 中初始化字节确实存在，`display-followup.md` 记载 152 项中 151 项匹配，但缺原始 Android 发送日志，不能在本轮重新核算匹配比例。公开参考内核版本也不等于实际 `trim/ophub` 内核，不能由局部文件相同推定完整构建相同。见 [证据索引](../evidence/README.md)。
