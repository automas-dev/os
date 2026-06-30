#!/bin/bash

set -e

FORCE_DOWNLOAD=${FORCE_DOWNLOAD:-}

# Make parallel
J=${J:-}
# J="-j"
# J="-j128"

export PREFIX=${PREFIX:-"$HOME/.local/opt/cross"}
export TARGET=${TARGET:-i386-elf}
export PATH="${PREFIX}/bin:$PATH"

BINUTILS_VERSION=${BINUTILS_VERSION:-"2.46.1"}
GCC_VERSION=${GCC_VERSION:-"16.1.0"}
GDB_VERSION=${GDB_VERSION:-"17.2"}

CROSS_BUILD=${PWD}/cross-build

COLOR_GREEN="\033[0;32m"
COLOR_BLUE="\033[0;34m"
COLOR_WHITE="\033[1;37m"
COLOR_RESET="\033[0m"
echo_section() { echo -e "${COLOR_BLUE}:: ${COLOR_WHITE}$*${COLOR_RESET}"; }
echo_task() { echo -e "${COLOR_GREEN}==> ${COLOR_WHITE}$*${COLOR_RESET}"; }
echo_step() { echo -e " ${COLOR_BLUE} -> ${COLOR_RESET}$*"; }

echo_section Building with Configuration

echo PREFIX=${PREFIX}
echo TARGET=${TARGET}
echo J=${J}
echo FORCE_DOWNLOAD=${FORCE_DOWNLOAD}
echo BINUTILS_VERSION=${BINUTILS_VERSION}
echo GCC_VERSION=${GCC_VERSION}
echo GDB_VERSION=${GDB_VERSION}
echo CROSS_BUILD=${CROSS_BUILD}

install_dependencies() {
    echo_section "Installing dependencies"

    echo_step "Updating package cache"
    sudo apt-get update >/dev/null

    echo_step "Installing dependencies"
    sudo apt-get install -y \
        build-essential \
        wget \
        gcc \
        xz-utils \
        bison \
        flex \
        libgmp3-dev \
        libgmp-dev \
        libmpfr-dev \
        texinfo \
        >/dev/null
}

download_and_extract() {
    local name url
    name=${1}.tar.xz
    url=$2

    echo_task Fetch ${name} from ${url}

    cd "${CROSS_BUILD}"

    if [ ! -e ${name} ] || [ ! -z ${FORCE_DOWNLOAD} ]; then
        echo_step "Downloading ${url}"
        wget -O ${name} ${url}
    else
        echo_step "Archive ${name} already exists"
    fi

    if [ ! -e ${1} ] || [ ! -z ${FORCE_DOWNLOAD} ]; then
        echo_step "Extracting ${name}"
        tar -xf ${name}
    else
        echo_step "Source directory ${1} already exists"
    fi
}

install_binutils() {
    local name build_dir
    name=binutils-${BINUTILS_VERSION}
    build_dir=build-${TARGET}

    echo_section "Build & Install ${name}"

    cd "${CROSS_BUILD}/${name}"

    echo_task "Creating build dir ${build_dir}"

    # Create temporary build dir
    mkdir -p "${build_dir}"
    cd "${build_dir}"

    echo_task Configure project

    ../configure \
        --prefix="${PREFIX}" \
        --target=${TARGET} \
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

install_gdb() {
    local name build_dir
    name=gdb-${GDB_VERSION}
    build_dir=build-${TARGET}

    echo_section "Build & Install ${name}"

    download_and_extract ${name} ${GDB_URL}

    cd ${name}

    echo_task "Creating build dir ${build_dir}"

    # Create temporary build dir
    mkdir -p "${build_dir}"
    cd "${build_dir}"

    echo_task Configure project

    ../configure \
        --target=${TARGET} \
         --prefix="${PREFIX}"

    echo_task Building...

    # Build
    make ${J}

    echo_task Installing...

    # Package
    make install

    echo_task "Finished ${name}"
}

install_gcc() {
    local name build_dir
    name=gcc-${GCC_VERSION}
    build_dir=build-${TARGET}

    echo_section "Build & Install ${name}"

    cd "${CROSS_BUILD}/${name}"

    echo_task Installing project dependencies

    ./contrib/download_prerequisites

    echo_task "Creating build dir ${build_dir}"

    # Create temporary build dir
    mkdir -p "${build_dir}"
    cd "${build_dir}"

    echo_task Configure project

    ../configure \
        --prefix="${PREFIX}" \
        --target=${TARGET} \
        --disable-nls \
        --disable-werror \
        --disable-multilib \
        --without-headers \
        --enable-languages=c,c++

    echo_task Building...

    # Build
    make all-gcc ${J}
    make all-target-libgcc ${J}

    echo_task Installing...

    # Package
    make install-gcc
    make install-target-libgcc

    echo_task "Finished ${name}"
}

START_TIME=${SECONDS}

install_dependencies

echo_section "Create directories"

echo_step "Create cross target install prefix ${PREFIX}"
mkdir -p ${PREFIX}

echo_step "Create cross target build directory ${CROSS_BUILD}"
mkdir -p ${CROSS_BUILD}
echo '*' > ${CROSS_BUILD}/.gitignore

echo_step "Enter cross target build directory ${CROSS_BUILD}"
cd ${CROSS_BUILD}

echo_section "Downloading Source"

download_and_extract binutils-${BINUTILS_VERSION} "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz"
download_and_extract gdb-${GDB_VERSION} "https://ftp.gnu.org/gnu/gdb/gdb-${GDB_VERSION}.tar.xz"
download_and_extract gcc-${GCC_VERSION} "https://ftp.gnu.org/gnu/gcc/gcc-16.1.0/gcc-${GCC_VERSION}.tar.xz"

install_binutils
install_gdb
install_gcc

ELAPSED=$((SECONDS - START_TIME))

echo_section "Finished in ${ELAPSED} seconds"
