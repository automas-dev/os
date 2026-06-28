#!/bin/bash

set -e

GCC_VERSION=${GCC_VERSION:-"16.1.0"}

GCC_URL="https://ftp.gnu.org/gnu/gcc/gcc-16.1.0/gcc-${GCC_VERSION}.tar.xz"

if [ ! -e "gcc-${GCC_VERSION}.tar.xz" ]; then
    wget $GCC_URL
    tar -xf "gcc-${GCC_VERSION}.tar.xz"
fi

export PREFIX=${PREFIX:-"$HOME/.local/opt/cross"}
export TARGET=${TARGET:-aarch64-unknown-elf}
export PATH="$PREFIX/bin:$PATH"

if ! which -- ${TARGET}-as; then
    echo ${TARGET}-as is not in the PATH
    exit 1
fi

mkdir -p "$PREFIX"

cd "gcc-${GCC_VERSION}"

mkdir -p build-${TARGET}
cd build-${TARGET}

../configure --target=${TARGET} --prefix="${PREFIX}" --disable-nls --enable-languages=c,c++ --without-headers --enable-initfini-array --disable-hosted-libstdcxx

make all-gcc -j16
make all-target-libgcc -j16
make all-target-libstdc++-v3 -j16
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3
