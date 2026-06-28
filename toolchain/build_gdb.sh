#!/bin/bash

set -e

GDB_VERSION=${GDB_VERSION:-"17.2"}

GDB_URL="https://ftp.gnu.org/gnu/gdb/gdb-${GDB_VERSION}.tar.xz"

if [ ! -e "gdb-${GDB_VERSION}.tar.xz" ]; then
    wget $GDB_URL
    tar -xf "gdb-${GDB_VERSION}.tar.xz"
fi

export PREFIX=${PREFIX:-"$HOME/.local/opt/cross"}
export TARGET=${TARGET:-aarch64-unknown-elf}
export PATH="$PREFIX/bin:$PATH"

mkdir -p "$PREFIX"

cd "gdb-${GDB_VERSION}"

mkdir -p build-${TARGET}
cd build-${TARGET}

../configure --target=${TARGET} --prefix="${PREFIX}" --disable-werror
make all-gdb -j16
make install-gdb
