#!/bin/bash

set -e

CROSS_PREFIX=${CROSS_PREFIX:-${HOME}/.local/opt/cross}
FORCE_DOWNLOAD=${FORCE_DOWNLOAD:-}

CROSS_ARCH=i386-elf
CROSS_BUILD=${PWD}/cross-build/${CROSS_ARCH}

# Make parallel
J=""
# J="-j"
# J="-j128"

export PREFIX=${CROSS_PREFIX}
export TARGET=i386-elf
export PATH=${CROSS_PREFIX}/bin:$PATH

BINUTILS_VERSION=2.40
BINUTILS_URL="http://ftpmirror.gnu.org/binutils/binutils-${BINUTILS_VERSION}.tar.xz"

GCC_VERSION=12.2.0
GCC_URL="http://ftpmirror.gnu.org/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz"

GDB_VERSION=11.1
GDB_URL="http://ftpmirror.gnu.org/gdb/gdb-${GDB_VERSION}.tar.xz"

COLOR_GREEN="\033[0;32m"
COLOR_BLUE="\033[0;34m"
COLOR_WHITE="\033[1;37m"
COLOR_RESET="\033[0m"

echo_section() {
    echo -e "${COLOR_BLUE}:: ${COLOR_WHITE}$*${COLOR_RESET}"
}

echo_task() {
    echo -e "${COLOR_GREEN}==> ${COLOR_WHITE}$*${COLOR_RESET}"
}

echo_step() {
    echo -e " ${COLOR_BLUE} -> ${COLOR_RESET}$*"
}

download_and_extract() {
    local name url
    name=${1}.tar.xz
    url=$2

    echo_task Fetch ${name} from ${url}

    if [ ! -e ${name} ] || [ ! -z ${FORCE_DOWNLOAD} ]; then
        echo_step "Downloading ${url}"
        wget -O ${name} ${url}
    fi

    if [ ! -e ${1} ] || [ ! -z ${FORCE_DOWNLOAD} ] || [ -z ${FORCE_EXTRACT} ]; then
        echo_step "Extracting ${name}"
        tar -xf ${name}
    fi
}

install_binutils() {
    local name build_dir
    name=binutils-${BINUTILS_VERSION}
    build_dir=i386-${name}-build

    echo_section "Build & Install ${name}"

    download_and_extract ${name} ${BINUTILS_URL}

    cd ${name}

    echo_task "Creating build dir ${build_dir}"

    # Create temporary build dir
    mkdir -p ${build_dir}
    cd ${build_dir}

    echo_task Configure project

    ../configure \
        --prefix="${CROSS_PREFIX}" \
        --target=i386-elf \
        --with-sysroot \
        --disable-nls \
        --disable-werror \
        --disable-multilib \
        --enable-interwork

    echo_task Building...

    # Build
    make ${J}

    echo_task Checking...

    # Check
    make --keep-going check

    echo_task Installing...

    # Package
    make install

    echo_task "Finished ${name}"
}

install_gcc() {
    local name build_dir
    name=gcc-${GCC_VERSION}
    build_dir=i386-${name}-build

    echo_section "Build & Install ${name}"

    download_and_extract ${name} ${GCC_URL}

    cd ${name}

    echo_task Installing project dependencies

    ./contrib/download_prerequisites

    echo_task "Creating build dir ${build_dir}"

    # Create temporary build dir
    mkdir -p ${build_dir}
    cd ${build_dir}

    echo_task Configure project

    ../configure \
        --prefix="$PREFIX" \
        --target=i386-elf \
        --disable-nls \
        --disable-werror \
        --disable-multilib \
        --without-headers \
        --enable-languages=c,c++

    echo_task Building...

    # Build
    make all-gcc
    make all-target-libgcc

    echo_task Installing...

    # Package
    make ${J} install-gcc
    make ${J} install-target-libgcc

    echo_task "Finished ${name}"
}

install_gdb() {
    local name build_dir
    name=gdb-${GDB_VERSION}
    build_dir=i386-${name}-build

    echo_section "Build & Install ${name}"

    download_and_extract ${name} ${GDB_URL}

    cd ${name}

    echo_task "Creating build dir ${build_dir}"

    # Create temporary build dir
    mkdir -p ${build_dir}
    cd ${build_dir}

    echo_task Configure project

    ../configure \
        --target=i386-elf \
         --prefix="$PREFIX" \
         --program-prefix=i386-elf-

    echo_task Building...

    # Build
    make ${J}

    echo_task Installing...

    # Package
    make install

    echo_task "Finished ${name}"
}

START_TIME=${SECONDS}

echo_section "Create directories"

echo_step "Create cross target install prefix ${CROSS_PREFIX}"
mkdir -p ${CROSS_PREFIX}

echo_step "Create cross target build directory ${CROSS_BUILD}"
mkdir -p ${CROSS_BUILD}
echo '*' > ${CROSS_BUILD}/.gitignore

cd ${CROSS_BUILD}

echo_section "Installing dependencies..."

echo_step "Updating package cache"
sudo apt-get update >/dev/null

echo_step "Installing dependencies"
sudo apt-get install -y wget gcc xz-utils texinfo libgmp-dev >/dev/null

# install_binutils
# install_gdb
install_gcc

ELAPSED=$((SECONDS - START_TIME))

echo_section "Finished in ${ELAPSED} seconds"
