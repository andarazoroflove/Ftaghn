#!/bin/bash
set -e

# Change to repo root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

if ! command -v arm-mingw32ce-gcc &>/dev/null; then
    echo "arm-mingw32ce-gcc not found in host PATH; invoking via docker container..."
    docker run --rm -v "$REPO_ROOT:/work" -w /work 777shuang/docker-cegcc bash axim_x50v/tools/build_axim.sh
    echo "--> Packaging axim_x50v/release/ftaghn-axim-x50v-StorageCard.zip on host..."
    rm -f axim_x50v/release/ftaghn-axim-x50v-StorageCard.zip
    if command -v zip &>/dev/null; then
        (cd "axim_x50v/release" && zip -r ftaghn-axim-x50v-StorageCard.zip Ftaghn.exe AUDIO IMAGES)
    elif command -v python3 &>/dev/null; then
        python3 -c "
import zipfile, os
zip_path = 'axim_x50v/release/ftaghn-axim-x50v-StorageCard.zip'
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
    base = 'axim_x50v/release'
    zipf.write(os.path.join(base, 'Ftaghn.exe'), 'Ftaghn/Ftaghn.exe')
    for folder in ['AUDIO', 'IMAGES']:
        folder_path = os.path.join(base, folder)
        for r, d, fs in os.walk(folder_path):
            for f in fs:
                full_p = os.path.join(r, f)
                rel_p = os.path.relpath(full_p, base)
                zipf.write(full_p, os.path.join('Ftaghn', rel_p))
print('Created zip:', zip_path)
"
    fi
    echo "=== Dell Axim X50v build and packaging complete! ==="
    exit 0
fi

echo "=== Building Ftaghn: Cosmic Horror Ataxx for Dell Axim X50v ==="

BUILD_DIR="axim_x50v/build"
RELEASE_DIR="axim_x50v/release"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

CFLAGS="-O2 -Wall -Wextra \
-mcpu=xscale -march=armv5te -marm \
-Iaxim_x50v/src \
-DFTAGHN_WINCE=1 -DUNDER_CE=1 -DWIN32=1 -D_ARM_=1 \
-fno-strict-aliasing"

LDFLAGS="-lcoredll"

CC="arm-mingw32ce-gcc"
WINDRES="arm-mingw32ce-windres"

echo "[1/4] Compiling Windows Mobile VGA resources with windres..."
$WINDRES -i axim_x50v/rsc/ftaghn.rc -o "$BUILD_DIR/ftaghn_res.o"

SRCS_C="
axim_x50v/src/freestanding.c
axim_x50v/src/font.c
axim_x50v/src/bmp_loader.c
axim_x50v/src/sound.c
axim_x50v/src/game.c
axim_x50v/src/render.c
axim_x50v/src/main.c
"

OBJS="$BUILD_DIR/ftaghn_res.o"

echo "[2/4] Compiling C source files for Intel XScale PXA270 624 MHz..."
for src in $SRCS_C; do
    obj="$BUILD_DIR/$(basename "$src" .c).o"
    echo "  CC $src"
    $CC $CFLAGS -c "$src" -o "$obj"
    OBJS="$OBJS $obj"
done

echo "[3/4] Linking $RELEASE_DIR/Ftaghn.exe (Native ARMv5TE XScale)..."
$CC -o "$RELEASE_DIR/Ftaghn.exe" $OBJS $LDFLAGS

echo "[4/4] Verifying PE executable header..."
arm-mingw32ce-objdump -p "$RELEASE_DIR/Ftaghn.exe" | grep -E 'DLL Name|OperatingSystemVersion|Subsystem'

echo "=== Axim X50v Compilation Successful! ==="
ls -lh "$RELEASE_DIR/Ftaghn.exe"
