#!/bin/sh
set -e
. ./build.sh "$@"

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp -r sysroot/* isodir/
cat > isodir/boot/grub/grub.cfg << EOF
set color_normal=white/cyan
menuentry "GalaxyOS Neptune" {
	multiboot /boot/glxykrnl cd
}
EOF
grub-mkrescue -o galaxyos.iso isodir
