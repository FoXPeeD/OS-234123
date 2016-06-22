#!/bin/bash
set -e
insmod ../srandom.o
mknod /dev/srandom c 62 0
../utils/get_ent_cnt
cat ../utils/clr_pool.c | ../utils/add_ent 1000
../utils/get_ent_cnt
echo "hello" | ../utils/add_ent 10
../utils/get_ent_cnt
head -c 100 /dev/srandom | xxd -p
../utils/get_ent_cnt
../utils/clr_pool
../utils/get_ent_cnt
cat ../utils/get_ent_cnt.c | ../utils/add_ent 2000
../utils/get_ent_cnt
cat ../utils/get_ent_cnt.c | ../utils/add_ent 2000
../utils/get_ent_cnt
cat ../utils/get_ent_cnt.c | ../utils/add_ent 2000
../utils/get_ent_cnt
head -c 500 /dev/srandom | xxd -p
../utils/get_ent_cnt
head -c 10 /dev/srandom | xxd -p
../utils/get_ent_cnt
cat ../utils/get_ent_cnt.c > /dev/srandom
cat ../utils/clr_pool.c | ../utils/add_ent 1100
cat ../utils/clr_pool.c | ../utils/add_ent 1200
cat ../utils/get_ent_cnt.c | ../utils/add_ent 2000
../utils/get_ent_cnt
head -c 510 /dev/srandom | xxd -p
../utils/get_ent_cnt
rm /dev/srandom
rmmod srandom
