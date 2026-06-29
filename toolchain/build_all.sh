#!/bin/bash

set -e

export BINUTILS_VERSION="2.46.1"
export GDB_VERSION="17.2"
export GCC_VERSION="16.1.0"

export TARGET=aarch64-unknown-elf
./build_binutils.sh
./build_gdb.sh
./build_gcc.sh

export TARGET=i386-elf
./build_binutils.sh
./build_gdb.sh
./build_gcc.sh

export TARGET=i686-elf
./build_binutils.sh
./build_gdb.sh
./build_gcc.sh
