#!/bin/sh
set -eu
out=/boot/c10h-linux-proof
mkdir -p "$out"
{
 echo 'C10H Linux userspace reached'
 date -u
 uname -a
 cat /proc/cmdline
 cat /proc/sys/kernel/random/boot_id
 findmnt /
 lsblk
 ip -brief address
} > "$out/boot.txt" 2>&1
dmesg > "$out/dmesg.txt" 2>&1 || true
sync
