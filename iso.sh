#!/bin/sh
set -e
. ./build.sh "$@"

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/glxykrnl isodir/boot/glxykrnl
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "GalaxyOS Neptune" {
	multiboot /boot/glxykrnl
}
EOF
grub-mkrescue -o galaxyos.iso isodir
