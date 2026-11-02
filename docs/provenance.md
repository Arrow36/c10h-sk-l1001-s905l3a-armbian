# 测试镜像来源

上游发布资产：

- 文件：`Armbian_26.11.0_amlogic_s905l3a_trixie_6.12.114_server_2026.11.02.img.gz`
- 下载链接：https://github.com/ophub/amlogic-s9xxx-armbian/releases/download/Armbian_trixie_arm64_server_2026.11/Armbian_26.11.0_amlogic_s905l3a_trixie_6.12.114_server_2026.11.02.img.gz
- 压缩文件长度：899418763 字节。
- GitHub 资产 SHA-256：`db4732a41a03c2de729e6ab6bd8b3119cbf3df6d279e2a86ec7bab46b0d2d76f`；下载后已核对。
- 初次修改并写盘的镜像长度：3686793216 字节。
- 初次修改镜像 SHA-256：`ae568d1ff6be177358920192d3c408c37c9ced488ee5d1982fa15c7b7fd6264f`；写入 U 盘后整个镜像区域读回一致。

这个修改镜像哈希仅对应初次写盘时的文件。之后 U 盘增加了启动记录服务、首次初始化、根分区扩容、密码设置和网络配置，不能用它校验当前已运行的 U 盘内容。

完整镜像未上传 Git。仓库中的构建工具是从本次实际使用的修改逻辑整理而来；make_boot_script 默认时间戳为 0，可通过 SOURCE_DATE_EPOCH 指定时间戳，因此重新构建的脚本容器头部可能与初次制作的容器不同。
