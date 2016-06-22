#!/bin/bash
set -e
insmod ../srandom.o
mknod /dev/srandom c 62 0
../utils/test2
rm /dev/srandom
rmmod srandom
