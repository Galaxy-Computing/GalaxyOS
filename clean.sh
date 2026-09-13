#!/bin/sh
set -e
. ./config.sh "$@"

rm -rf isodir
rm -rf galaxyos.iso
rm -rf sysroot
rm -rf imgdir

for PROJECT in $PROJECTS; do
  (cd $PROJECT && $MAKE clean)
done


