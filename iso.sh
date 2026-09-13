#!/bin/sh
set -e
. ./build.sh "$@"

mkdir -p isodir
mkdir -p imgdir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

dd if=/dev/zero of=isodir/boot.img bs=1M count=32

cp -r sysroot/* imgdir/
cp -r files/${HOSTARCH}/* imgdir/

mkfs.fat isodir/boot.img
mcopy -i isodir/boot.img -s imgdir/* ::

cat > isodir/boot/grub/grub.cfg << EOF
set color_normal=white/cyan
menuentry "GalaxyOS Neptune" {
	set img="/boot.img"
	search --set=disc --file \$img
	loopback loop (\$disc)\$img
	multiboot (loop)/galaxyos/${HOSTARCH}/glxykrnl.elf "cd"
}
EOF
grub-mkrescue -d /usr/lib/grub/i386-pc -o galaxyos.iso isodir
