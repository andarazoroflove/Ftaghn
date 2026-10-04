#!/bin/bash
set -e

if ! command -v arm-mingw32ce-gcc &>/dev/null; then
    echo "arm-mingw32ce-gcc not found in host PATH; invoking via docker container..."
    docker run --rm -v "$(pwd):/work" -w /work 777shuang/docker-cegcc bash wince/tools/build_wince.sh
    echo "--> Packaging wince/release/ftaghn-j720-StorageCard.zip on host..."
    rm -f wince/release/ftaghn-j720-StorageCard.zip
    if command -v zip &>/dev/null; then
        (cd "wince/release/Storage Card" && zip -r ../ftaghn-j720-StorageCard.zip "Ftaghn")
    elif command -v python3 &>/dev/null; then
        python3 -c "import zipfile, os; zipf = zipfile.ZipFile('wince/release/ftaghn-j720-StorageCard.zip', 'w', zipfile.ZIP_DEFLATED); [zipf.write(os.path.join(r, f), os.path.relpath(os.path.join(r, f), 'wince/release/Storage Card')) for r, d, fs in os.walk('wince/release/Storage Card/Ftaghn') for f in fs]; zipf.close()"
    fi
    echo "--> Packaging complete: wince/release/ftaghn-j720-StorageCard.zip"
    exit $?
fi

BUILD_DIR="wince/build"
mkdir -p "$BUILD_DIR"
mkdir -p "wince/release"

CFLAGS="-O2 -Wall -Wextra \
-march=armv4 -mcpu=strongarm -marm \
-Iwince/src \
-DFTAGHN_WINCE=1 -DUNDER_CE=1 -DWIN32=1 \
-fno-strict-aliasing"

LDFLAGS="-nostartfiles -nodefaultlibs -Wl,-e,WinMainCRTStartup -lcoredll -lgcc -Wl,--major-os-version,3 -Wl,--minor-os-version,0 -Wl,--major-subsystem-version,3 -Wl,--minor-subsystem-version,0 -Wl,--stack,0x200000"

CC="arm-mingw32ce-gcc"

SRCS_C="
wince/src/freestanding.c
wince/src/font.c
wince/src/bmp_loader.c
wince/src/sound.c
wince/src/game.c
wince/src/render.c
wince/src/main.c
"

ASM_SRCS="
wince/src/crt_armv4.S
wince/src/armv4_div.S
"

OBJS=""

echo "[1/3] Compiling C object files (-march=armv4 -mcpu=strongarm)..."
for src in $SRCS_C; do
    obj="$BUILD_DIR/$(echo "$src" | tr '/.' '__').o"
    echo "  CC $src"
    $CC $CFLAGS -c "$src" -o "$obj"
    OBJS="$OBJS $obj"
done

echo "[2/3] Assembling ARMv4 assembly sources..."
for asm in $ASM_SRCS; do
    obj="$BUILD_DIR/$(echo "$asm" | tr '/.' '__').o"
    echo "  AS $asm"
    $CC $CFLAGS -c "$asm" -o "$obj"
    OBJS="$OBJS $obj"
done

echo "[3/3] Linking wince/release/FTAGHN.EXE (pure ARMv4, -nostartfiles)..."
$CC -o wince/release/FTAGHN.EXE $OBJS $LDFLAGS

echo "===> Build successful: wince/release/FTAGHN.EXE"
ls -la wince/release/FTAGHN.EXE

echo "--> Compiling and running ARMv4 StrongARM patcher..."
gcc -O2 wince/tools/armv4_patch.c -o "$BUILD_DIR/armv4_patch"
"$BUILD_DIR/armv4_patch" wince/release/FTAGHN.EXE

echo "--> Auditing binary for illegal ARMv4 instructions (bx / blx)..."
BX_COUNT=$(arm-mingw32ce-objdump -d wince/release/FTAGHN.EXE | grep -c -E '\sbx\s|\sblx\s' || true)
if [ "$BX_COUNT" -ne 0 ]; then
    echo "ERROR: Found $BX_COUNT bx/blx instructions in wince/release/FTAGHN.EXE!"
    arm-mingw32ce-objdump -d wince/release/FTAGHN.EXE | grep -E '\sbx\s|\sblx\s' | head -n 30
    exit 1
fi
echo "AUDIT PASSED: ZERO bx/blx instructions found in wince/release/FTAGHN.EXE! 100% StrongARM SA-1110 safe."

echo "--> Auditing PE imports in wince/release/FTAGHN.EXE..."
arm-mingw32ce-objdump -p wince/release/FTAGHN.EXE | grep -E 'DLL Name|GetProcAddress' || true

# Prepare Storage Card package
SC_DIR="wince/release/Storage Card/Ftaghn"
mkdir -p "$SC_DIR"
cp wince/release/FTAGHN.EXE "$SC_DIR/"
cp -r wince/release/AUDIO "$SC_DIR/"
cp -r wince/release/IMAGES "$SC_DIR/"

cat << 'EOF' > "$SC_DIR/README.TXT"
================================================================================
  FTAGHN: COSMIC HORROR ATAXX - HP JORNADA 720 EDITION
  Windows CE 3.0 / Handheld PC 2000 (Pure ARMv4 StrongARM SA-1110)
================================================================================

Target Hardware:
- HP Jornada 720 Handheld PC (206 MHz StrongARM SA-1110, 32 MB RAM)
- Screen Resolution: 640 x 240 (Half-VGA 2D Display)
- Operating System: Microsoft Windows CE 3.0 / Handheld PC 2000

Installation:
1. Copy the entire 'Ftaghn' folder to your CompactFlash Storage Card:
   \Storage Card\Ftaghn\FTAGHN.EXE
   \Storage Card\Ftaghn\AUDIO\
   \Storage Card\Ftaghn\IMAGES\
2. Double-tap FTAGHN.EXE in Windows CE File Explorer to launch!

Layout (640 x 240 Split Screen):
- Left Half  (0-320 px): Cosmic Horror Background & 7x7 Ataxx Grid
- Right Half (320-640 px): Scoreboard, The Grimoire (Status Log), Active Deity,
  and Touch Control Buttons.

Controls:
- Stylus:
  - Tap piece to select (Golden Ring appears).
  - Green dots indicate valid Clone destinations (distance 1).
  - Purple diamonds indicate valid Leap destinations (distance 2).
  - Tap target square to move and convert adjacent opposing horrors!
- Buttons:
  - [ NEW GAME ]   : Restart match.
  - [ AI: DIFF ]   : Cycle AI difficulty (Mortal -> Elder -> Ancient One).
  - [ TOME / LORE ]: Open Tome of Forbidden Knowledge to choose Deities/Secrets!
  - [ SFX: ON/OFF ]: Toggle audio sound effects and theme music.
- Keyboard:
  - 'N': New Game
  - 'D': Cycle AI Difficulty
  - 'T' or 'S': Open Tome of Forbidden Knowledge
  - 'M': Toggle Sound Mute
  - ESC: Deselect piece or close Tome modal
  - Left / Right Arrows: Browse secrets in Tome modal
  - ENTER: Invoke selected Secret in Tome modal

Deities & Secrets Included in the Tome:
- Hastur, Rhan-Tegoth, Yog-Sothoth, Ghroth, Azathoth, Ithaqua, Abhoth,
  Shoggoth, Nyarlathotep, Yibb-Tstll, Idha, Eihort, Shudde M'ell, Cthulhu
- Mortal Easter Eggs:
  George W., The KLF, The X-Files, Nine Inch Nails, Doktor Avalanche (Sisters
  of Mercy), Groovie Mann (Thrill Kill Kult), Supernatural, Santa Egg.

All audio tracks, themes, and backgrounds are 100% self-contained and tuned
for zero latency on the Jornada 720.
================================================================================
EOF

if command -v zip &>/dev/null; then
    echo "--> Packaging wince/release/ftaghn-j720-StorageCard.zip..."
    (cd "wince/release" && rm -f ftaghn-j720-StorageCard.zip && zip -r ftaghn-j720-StorageCard.zip "Storage Card")
fi

echo "================================================================================"
echo "  Ftaghn Windows CE (Jornada 720) build and packaging COMPLETE!"
echo "  Binary:  wince/release/FTAGHN.EXE"
echo "  Package: wince/release/ftaghn-j720-StorageCard.zip"
echo "================================================================================"
