echo C10H USB test - no saved environment changes
setenv kernel_addr_r 0x11000000
setenv ramdisk_addr_r 0x15000000
setenv fdt_addr_r 0x1000000
setenv bootargs root=UUID=bee3dd87-44ee-4457-8d25-5ad136e40646 rootflags=data=writeback rw rootwait rootfstype=ext4 console=ttyAML0,115200n8 console=tty0 no_console_suspend consoleblank=0 fsck.fix=yes fsck.repair=yes net.ifnames=0 max_loop=128 cgroup_enable=cpuset cgroup_memory=1 cgroup_enable=memory swapaccount=1 video=HDMI-A-1:1920x1080@60e plymouth.enable=0 systemd.mask=armbian-resize-filesystem.service
if fatload usb 0:1 ${kernel_addr_r} /zImage; then
 if fatload usb 0:1 ${ramdisk_addr_r} /uInitrd; then
  if fatload usb 0:1 ${fdt_addr_r} /dtb/amlogic/meson-g12a-s905l3a-c10h-usb-test.dtb; then
   booti ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}
  fi
 fi
fi
echo C10H USB test returned - power off and remove USB
while true; do sleep 1; done
