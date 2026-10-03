#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BASE_DIR}/build"
RELEASE_DIR="${BASE_DIR}/release"

echo "=== 1. Building FtaghnSE for Macintosh 68k (Motorola 68000) ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake .. --toolchain /Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
make

echo "=== 2. Preparing Release Directory ==="
mkdir -p "${RELEASE_DIR}"

# 800 KB DSDD Floppy Disk Image (Standard Mac SE internal drive)
cp "${BUILD_DIR}/FtaghnSE.dsk" "${RELEASE_DIR}/FTAGHN_SE_800K.DSK"

# MacBinary II Archive
cp "${BUILD_DIR}/FtaghnSE.bin" "${RELEASE_DIR}/FTAGHN_SE.BIN"

# 1.44 MB DSHD Floppy Disk Image (SuperDrive / FDHD / Gotek / Floppy Emu)
echo "=== 3. Creating 1.44 MB Floppy Disk Image ==="
dd if=/dev/zero of="${RELEASE_DIR}/FTAGHN_SE_1440K.IMG" bs=1024 count=1440 status=none
hformat -l "Ftaghn SE" "${RELEASE_DIR}/FTAGHN_SE_1440K.IMG"
hmount "${RELEASE_DIR}/FTAGHN_SE_1440K.IMG"
hcopy -m "${BUILD_DIR}/FtaghnSE.bin" :FtaghnSE
humount

echo "=== 4. Verifying Release Deliverables ==="
ls -lh "${RELEASE_DIR}"

echo "=== Build & Packaging Complete! ==="

