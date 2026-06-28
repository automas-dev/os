#!/bin/bash

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
