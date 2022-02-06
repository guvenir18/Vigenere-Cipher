#!/bin/sh
module="vigenere"
device="vigenere"
mode="664"

if grep -q $module "/proc/devices"; then
    /sbin/rmmod $module
fi

/sbin/insmod ./$module.ko $* || exit 1

rm -f /dev/${device}*

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

mknod /dev/${device} c $major 0

group="staff"
grep -q '^staff:' /etc/group || group="wheel"

chgrp $group /dev/${device}
chmod $mode /dev/${device}
