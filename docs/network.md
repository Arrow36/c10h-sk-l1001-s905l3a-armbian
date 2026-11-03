# Windows 共享网络与 SSH

已验证拓扑：

```text
互联网 ← 电脑 Wi-Fi
              ↓ Windows Internet Connection Sharing
        电脑 USB 有线网卡 192.168.137.1/24
              ↓ 网线
        C10H eth0 192.168.137.2/24
```

Windows 侧将实际 Wi-Fi 连接设置为共享出口，把连接 C10H 的 USB 网卡作为共享内部接口。网卡名称和编号会变化，设置前必须核对实际连接；不要照搬某台电脑的 InterfaceIndex。

C10H 先通过控制台临时指定 IP，成功 ping 到电脑后，通过 SSH 保存 NetworkManager 配置。当前已验证的持久参数：

- 配置名：c10h-direct；接口：eth0；自动连接：是；优先级：100。
- IPv4：manual，192.168.137.2/24；网关：192.168.137.1。
- DNS：223.5.5.5、1.1.1.1；ignore-auto-dns：是。

Windows 的 DNS 代理在测试中未响应，因此使用独立 DNS。重新启用共享后，Linux ping 公网、解析 deb.debian.org 和 HTTPS 返回 200 均已验证。

当前账号 root 的密码由设备使用者自行设置，此处不记录。连接命令：

```sh
ssh root@192.168.137.2
```

只有 eth0 注册不代表有物理链路；需结合 carrier、电脑网卡状态和实际通信验证。此静态地址适用于上述直连网段，换路由器或网段后要重新配置。

## 后续测试情况（2026-11-03）

曾成功通过 Windows ICS DHCP 获取地址；后续多次启动又遇到 DHCP 等待超时、需要重新插拔网线才能恢复 SSH，故不能把自动联网标记为稳定。当前调试连接使用上述静态地址。DNS 也在后续测试失效，DRM 测试依赖改由电脑下载后传入，早期互联网验证不代表每次启动都可联网。

Android 曾把 Linux 分区内文件属主改为 media_rw，NetworkManager 会忽略非 root 所有的连接文件。`systemd/c10h-network-profile-owner.service` 在 NetworkManager 前恢复 `.nmconnection` 文件属主；它不是整个根文件系统的权限修复。后续日志仍存在 cron、NetworkManager dispatcher 文件属主异常，尚未完整处理。不要在 Linux 分区上笼统递归 chown：系统内不同文件原本有不同属主。
