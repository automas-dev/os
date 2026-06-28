#!/bin/bash

set -e

BINUTILS_VERSION=${BINUTILS_VERSION:-"2.46.1"}

BINUTILS_URL="https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz"

if [ ! -e "binutils-${BINUTILS_VERSION}.tar.xz" ]; then
    wget $BINUTILS_URL
    tar -xf "binutils-${BINUTILS_VERSION}.tar.xz"
fi

export PREFIX=${PREFIX:-"$HOME/.local/opt/cross"}
export TARGET=${TARGET:-aarch64-unknown-elf}
export PATH="$PREFIX/bin:$PATH"

mkdir -p "$PREFIX"

cd "binutils-${BINUTILS_VERSION}"

mkdir -p build-${TARGET}
cd build-${TARGET}

../configure --target=${TARGET} --prefix="${PREFIX}" --with-sysroot --disable-nls --disable-werror --enable-default-execstack=no
make -j16
make install
