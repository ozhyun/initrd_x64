#!/bin/sh

cd /
DIRS="bin sbin lib etc dev"
for dir in $DIRS
do
   cp -a $dir /sysroot
done

cd /sysroot
mkdir -p proc
mkdir -p sys
mkdir -p mnt
mkdir -p tmp
mkdir -p root

#umount /proc
#umount /sys

#echo "List disk..."
#cat /proc/partitions
#echo ""
#cat /proc/diskstats
#echo ""
#cat /proc/devices

#echo "List sysroot..."
#ls /sysroot

