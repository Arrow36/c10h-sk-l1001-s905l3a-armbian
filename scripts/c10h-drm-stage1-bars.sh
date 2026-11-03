#!/bin/sh
set -eu
mkdir -p /boot/c10h-drm-stage1
log=/boot/c10h-drm-stage1/acceptance.txt
exec > "$log" 2>&1
printf 'DRM stage1 hardware ENCL bars; boot ID: '
cat /proc/sys/kernel/random/boot_id
cat /proc/fb
# Run only after native DRM fbdev exists; never touch a retained simplefb boot.
if ! grep -q mesondrmfb /proc/fb; then echo 'No native DRM framebuffer; abort'; exit 1; fi
sleep 5
dmesg > /boot/c10h-drm-stage1/before-bars-dmesg.txt
sync
insmod /root/c10h-screen-work/c10h-lcd-diagnostic-current.ko pattern=1
printf 'ENCL hardware bars enabled; user visual confirmation required.\n'
cat /sys/module/c10h_lcd_diagnostic/parameters/pattern
sync
