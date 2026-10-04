#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

echo "=== Building Ftaghn: Cosmic Horror Ataxx for Philips Nino 300 ==="
echo "Target: Windows CE 2.0 / 2.11 Palm-size PC (MIPS R4000 / Philips PR3910 75 MHz)"

BUILD_DIR="nino300/build"
RELEASE_DIR="nino300/release"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

CC="mipsel-linux-gnu-gcc"
LD="mipsel-linux-gnu-ld"
CFLAGS="-O2 -Wall -Wextra -march=r3900 -msoft-float -mabi=32 -EL \
-ffreestanding -fshort-wchar -fno-builtin -nostdlib -fno-pic -mno-abicalls \
-Inino300/src -DUNDER_CE=200 -DWIN32=1 -D_MIPS_=1"

SRCS_C="
nino300/src/freestanding.c
nino300/src/font.c
nino300/src/sound.c
nino300/src/game.c
nino300/src/render.c
nino300/src/main.c
"

SRCS_ASM="
nino300/src/crt_mips.S
"

OBJS=""

echo "[1/4] Compiling MIPS C source files (-march=r3900 -msoft-float)..."
for src in $SRCS_C; do
    obj="$BUILD_DIR/$(basename "$src" .c).o"
    echo "  CC $src -> $obj"
    $CC $CFLAGS -c "$src" -o "$obj"
    OBJS="$OBJS $obj"
done

echo "[2/4] Assembling MIPS startup & import thunks..."
for asm in $SRCS_ASM; do
    obj="$BUILD_DIR/$(basename "$asm" .S).o"
    echo "  AS $asm -> $obj"
    $CC $CFLAGS -c "$asm" -o "$obj"
    OBJS="$OBJS $obj"
done

echo "[3/4] Linking intermediate MIPS ELF image..."
$LD -EL -T nino300/tools/wince_mips.ld -o "$BUILD_DIR/ftaghn_mips.elf" $OBJS

echo "[4/4] Generating Windows CE 2.0 MIPS PE executable (IMAGE_FILE_MACHINE_R4000)..."
python3 nino300/tools/mips_pe_builder.py "$BUILD_DIR/ftaghn_mips.elf" "$RELEASE_DIR/FTAGHN.EXE"

echo "=== Build Successful! ==="
ls -lh "$RELEASE_DIR/FTAGHN.EXE"

# Prepare CompactFlash / Storage Card package
CF_DIR="$RELEASE_DIR/Storage Card/Ftaghn"
mkdir -p "$CF_DIR"
cp "$RELEASE_DIR/FTAGHN.EXE" "$CF_DIR/"

if [ -d "wince/release/AUDIO" ]; then
    cp -r "wince/release/AUDIO" "$CF_DIR/"
fi

cat << 'EOF' > "$CF_DIR/README.TXT"
================================================================================
  FTAGHN: COSMIC HORROR ATAXX - PHILIPS NINO 300 EDITION
  Windows CE 2.0 / 2.11 Palm-size PC (MIPS R4000 Philips PR3910 75 MHz)
================================================================================

Hardware Compatibility:
  - Philips Nino 300 / 312 / 320 (Palm-size PC 1.0 / 1.1)
  - Casio Cassiopeia E-10 / E-11 / E-15 / E-100 / E-105 (MIPS VR41xx)
  - Compaq Aero 1500 / 1520 / 1530 (MIPS)
  - Any MIPS R4000 Windows CE 2.0+ device

Display:
  - 240x320 Portrait, 4-Level Grayscale High-Contrast STN LCD

Installation:
  1. Copy the "Ftaghn" folder to your CompactFlash card or device RAM.
  2. Launch FTAGHN.EXE using Windows CE File Explorer.
  3. Enjoy Cosmic Horror Ataxx!

Controls:
  - Stylus: Tap board cells, buttons, or Tome entries directly.
  - Rocker Up/Down: Navigate cursor across the board.
  - Action / Enter Button: Confirm move / select piece.
  - Escape Key: Cleanly exit the game.
================================================================================
EOF

echo "--> Packaging nino300/release/ftaghn-nino300-CFCard.zip..."
rm -f "$RELEASE_DIR/ftaghn-nino300-CFCard.zip"
if command -v zip &>/dev/null; then
    (cd "$RELEASE_DIR/Storage Card" && zip -r ../ftaghn-nino300-CFCard.zip "Ftaghn")
elif command -v python3 &>/dev/null; then
    python3 -c "import zipfile, os; zipf = zipfile.ZipFile('$RELEASE_DIR/ftaghn-nino300-CFCard.zip', 'w', zipfile.ZIP_DEFLATED); [zipf.write(os.path.join(r, f), os.path.relpath(os.path.join(r, f), '$RELEASE_DIR/Storage Card')) for r, d, fs in os.walk('$RELEASE_DIR/Storage Card/Ftaghn') for f in fs]; zipf.close()"
fi

echo "=== Nino 300 Packaging Complete! ==="
ls -lh "$RELEASE_DIR/ftaghn-nino300-CFCard.zip"
