#!/bin/sh
set -e

if command -v i686-galaxyos-gcc >/dev/null 2>&1; then
    echo "The cross compiler is already installed, we will delete the existing files. Please terminate the script if you do not want to delete the existing build files or repositories."

    read -s -p "Press [Enter] to continue..."
    echo "" # Adds a newline since -s suppresses the automatic newline

    rm -rf ../gcc
    rm -rf ../binutils
    rm -rf ../build-binutils
    rm -rf ../build-gcc
fi

target=i686-galaxyos

# The tools will be installed in ~/opt/cross/$target.
prefix=~/opt/cross/$target

sysroot="$(pwd)/sysroot"

echo "We will now install the i686-galaxyos toolchain required for building the OS and any applications."
echo "The cross compiler will be installed to ${prefix}"
echo "The files for the GCC and Binutils repositories will be placed in the parent directory, so I'd recommend having the parent directory be empty except for this one."
echo "If you do not wish to proceed, you may terminate the script now."
echo ""

read -s -p "Press [Enter] to continue..."
echo "" # Adds a newline since -s suppresses the automatic newline

# We need the headers to be in the sysroot
./headers.sh

if [ ! -d "../gcc" ]; then
    git clone https://github.com/Galaxy-Computing/gcc ../gcc
fi

if [ ! -d "../binutils" ]; then
    git clone https://github.com/Galaxy-Computing/binutils ../binutils
fi

# Create build paths.
mkdir -p ../build-binutils
mkdir -p ../build-gcc

# Build binutils.
cd ../build-binutils
../binutils/configure --target=$target --prefix=$prefix --with-sysroot=$sysroot --disable-nls --disable-werror 2>&1
make all -j$(nproc) 2>&1
make install -j$(nproc) 2>&1

# Build gcc and libgcc.
cd ../build-gcc
/tmp/toolchain/gcc-$gcc_version/configure --target=$target --prefix=$prefix --with-sysroot=$sysroot --disable-nls --enable-languages=c 2>&1
make all-gcc 2>&1
make install-gcc -j$(nproc) 2>&1
make all-target-libgcc -j$(nproc) 2>&1
make install-target-libgcc -j$(nproc) 2>&1

# Make sure that our cross compiler will be found by creating links.
# Alternative: Add the $prefix/bin directory to your $PATH.
sudo ln -s -f $prefix/bin/* /usr/local/bin/
exit 0