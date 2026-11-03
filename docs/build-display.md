# 屏幕实验源码构建说明

这不是完整安装教程；源码只对应本仓库记录的 6.12.114-ophub 实验，不能承诺其他内核/板卡兼容。先阅读 [display.md](display.md) 与 [checkpoint](checkpoint-2026-11-03.md)。

## 内核模块

`drivers/c10h-display/Makefile` 构建面板、DSI 主机和 ENCL 诊断模块。需要与运行内核相同的 headers、配置、Module.symvers，以及 ARM64 工具链；源码中的上游头文件应保持配套。示例：

```sh
make -C /path/to/matching-kernel-headers \
  M="$PWD/drivers/c10h-display" ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- modules
```

本次使用电脑 WSL、匹配的 6.12.114-ophub headers、aarch64-linux-gnu-gcc-14；内核运行版本由 GCC15.3 构建。内核编译警告不等于兼容性验证，加载和可见效果仍需单独验收。没有 DKMS 或一键安装服务。

自定义 DSI 主机与内置 meson DSI 主机不能同时绑定；本次专用启动参数用 `initcall_blacklist=meson_dw_mipi_dsi_platform_driver_init` 排除内置版本，再加载 `c10h_dsi_host`。该内核符号名并非其他版本的通用接口。

## 用户态原生图像探针

需要 libdrm 开发头文件和运行库。本次运行库及测试工具为 Debian ARM64 2.4.124-2，标准接口查询见脱敏证据。

```sh
cc -O2 -Wall -Wextra -Werror $(pkg-config --cflags libdrm) \
  experiments/display/drm-geometry.c $(pkg-config --libs libdrm) \
  -o drm-geometry
```

它只选择已有活动 DSI 模式、创建线性 dumb buffer 并原子提交主图层，不允许 modeset。预期模式固定为当前已确认的 800×1280，不是通用显示测试工具。程序运行后保持 DRM master；SIGUSR1 移动白块，SIGTERM 恢复原图层。错误或崩溃路径不保证自动恢复显示，须保留已有彩条与启动回退方案。

## 数据和许可来源

- 自定义 DSI 主机与配套寄存器头文件参考 unifreq/linux-6.12.y 的 meson 驱动，保留原版权及 GPL/SPDX。
- 面板命令/时序来自本机原厂 DT 与 Android 运行参数，公开版本不含整份原厂 DT、序列号或网络凭据。
- ENCL 诊断逻辑对照原厂 lcd_debug_test 和主线寄存器定义；测试时序修改均为实验，不能用已失败的临时值当正式修复。
- shell 脚本必须保持 LF；本次曾因 CRLF 导致 203/EXEC，已修正，`.gitattributes` 固定 LF。
