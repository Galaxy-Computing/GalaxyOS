#!/bin/bash
set -e
. ./build.sh "$@"

mkdir -p isodir
mkdir -p imgdir
mkdir -p imgdir/galaxyos/config
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp -r sysroot/* imgdir/

# build config file
sdk/bin/conftool set init.cmdline "local:galaxyos/i386/gsh.elf" -f imgdir/galaxyos/config/system.gcf
sdk/bin/conftool set init.path "local:galaxyos/i386;local:galaxyos" -f imgdir/galaxyos/config/system.gcf
sdk/bin/conftool compact -f imgdir/galaxyos/config/system.gcf

# we don't need includes in the system files once the OS is actually built
rm -r imgdir/galaxyos/include

# keep boot.img close to its contents, room for fat overhead
bootimg_size=$(( ($(du -sk imgdir | cut -f1) + 1023) / 1024 + 2 ))
if [ "${bootimg_size}" -lt 2 ]; then bootimg_size=2; fi
dd if=/dev/zero of=isodir/boot.img bs=1M count=${bootimg_size}

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
