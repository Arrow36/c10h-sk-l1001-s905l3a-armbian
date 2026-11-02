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
