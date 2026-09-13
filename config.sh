if [[ ! -v PROJECTS ]]; then
  SYSTEM_HEADER_PROJECTS="libc kernel userland"
  PROJECTS="libc kernel userland"
fi

export MAKE=${MAKE:-make "$@"}
export HOST=${HOST:-$(./default-host.sh)}
export HOSTARCH=${HOSTARCH:-$(./target-triplet-to-arch.sh ${HOST})}

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc

export PREFIX=/galaxyos
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/${HOSTARCH}
export INCLUDEDIR=$PREFIX/include

export CFLAGS='-Og -g -Wall -Wextra -std=gnu17 -mtune=i686 -march=i486'
export CPPFLAGS=''
export LDFLAGS="-L=/galaxyos/${HOSTARCH}"

# Configure the cross-compiler to use the desired system root.
export SYSROOT="$(pwd)/sysroot"
export CC="$CC --sysroot=$SYSROOT"

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
