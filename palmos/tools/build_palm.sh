#!/bin/bash
set -e

# Change to repo root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

echo "=== Building Ftaghn: Cosmic Horror Ataxx for Palm T|X ==="

BUILD_DIR="palmos/build"
RELEASE_DIR="palmos/release"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

echo "[1/4] Compiling Palm OS resources with pilrc..."
pilrc -q palmos/rsc/ftaghn.rcp "$BUILD_DIR/"

echo "[2/4] Compiling 68K / PACE source files with m68k-palmos-gcc (-palmos5r3 -O2)..."
m68k-palmos-gcc -palmos5r3 -O2 -Ipalmos/src -c palmos/src/game.c -o "$BUILD_DIR/game.o"
m68k-palmos-gcc -palmos5r3 -O2 -Ipalmos/src -c palmos/src/sound.c -o "$BUILD_DIR/sound.o"
m68k-palmos-gcc -palmos5r3 -O2 -Ipalmos/src -c palmos/src/render.c -o "$BUILD_DIR/render.o"
m68k-palmos-gcc -palmos5r3 -O2 -Ipalmos/src -c palmos/src/main.c -o "$BUILD_DIR/main.o"

echo "[3/4] Linking Palm OS executable..."
m68k-palmos-gcc -palmos5r3 -O2 "$BUILD_DIR/main.o" "$BUILD_DIR/game.o" "$BUILD_DIR/render.o" "$BUILD_DIR/sound.o" -o "$BUILD_DIR/ftaghn"

echo "[4/4] Assembling Palm OS PRC database..."
build-prc -o "$RELEASE_DIR/Ftaghn.prc" -n "Ftaghn" -c FTAG "$BUILD_DIR/ftaghn" "$BUILD_DIR"/*.bin

echo "=== Build Complete! ==="
ls -lh "$RELEASE_DIR/Ftaghn.prc"
