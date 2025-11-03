#!/bin/sh
set -e
. ./iso.sh "$@"

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom galaxyos.iso -s -S -d int -no-reboot
