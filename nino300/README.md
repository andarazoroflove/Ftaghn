# Ftaghn: Cosmic Horror Ataxx (Philips Nino 300 Edition)

An authentic, native port of **Ftaghn: Cosmic Horror Ataxx** targeting the **Philips Nino 300** Palm-size PC running **Windows CE 2.0 / 2.11** on the **Philips PR3910 75 MHz MIPS R4000** 32-bit RISC processor.

Also fully compatible with Philips Nino 312 / 320, Casio Cassiopeia E-10 / E-11 / E-15 / E-100 / E-105 (MIPS VR41xx), Compaq Aero 1500 / 1520, and other Windows CE MIPS Palm-size and Pocket PC handhelds.

---

## Hardware Specifications & Target Environment

- **Target Device**: Philips Nino 300 / Nino 312 (Released 1998–1999).
- **Form Factor**: Palm-size PC 1.0 / 1.1 (the precursor to Pocket PC 2000).
- **Processor**: Philips PR3910 (MIPS-I/II 32-bit Little-Endian RISC @ 75 MHz).
- **Binary Architecture**: `IMAGE_FILE_MACHINE_R4000` (`0x0166`), Subsystem 9 (`IMAGE_SUBSYSTEM_WINDOWS_CE_GUI`), Subsystem Version `2.0`.
- **Display**: 240×320 pixels Portrait, 4-level grayscale STN monochrome LCD with electroluminescent backlight.
- **Audio**: Built-in speaker, WaveAudio driver (`COREDLL.dll` / `PlaySoundW` / `MessageBeep`).
- **Storage / Media**: CompactFlash (CF) Type I slot + internal RAM Object Store.

---

## Grayscale Horror Visuals & UI Architecture

Because the Philips Nino 300 features a 4-level grayscale LCD, standard color art would wash out or become illegible. The Nino 300 port was specially designed from the ground up for high-contrast monochrome cosmic horror:

1. **4-Level Calibrated Grayscale Palette**:
   - **Pitch Void (`#000000`)**: Deep shadow, cosmic background, piece outlines, eye slits.
   - **Dark Slate (`#323238`)**: Board checker grid, AI piece base, bezel shadows.
   - **Ash Gray (`#A0A0AA`)**: Inactive cards, secondary HUD text, monolith stone.
   - **Bone White (`#FFFFFF`)**: Player cultist piece, specular highlights, active card borders, move targets.

2. **Dithering & Texture Differentiation**:
   - **Player (Cultist) Piece**: Solid Bone White circular disk with black central slit pupil. Instantly identifiable at a glance.
   - **AI (Elder God) Piece**: Textured Dark Slate disk with Bone White inverted cross/pentagram and glowing outer halo. Never confused with player pieces even in 2-bit grayscale.
   - **Monolith Obstacles**: Slate blocks with beveled 3D borders and central abyssal eye.
   - **Rhan-Tegoth Ice**: Faceted crystalline diamond cross pattern.
   - **Move Targets**:
     - *Clone (Distance 1)*: Solid Bone White center diamond.
     - *Leap (Distance 2)*: White dashed outline circle.

3. **Screen Layout (240×320 Portrait)**:
   - **Y: 0–202**: 7×7 Cosmic Horror Ataxx board (28×28 pixel cells, centered at X: 22).
   - **Y: 206–238**: Faction scoreboard, Player vs AI piece count, Turn indicator, and Cosmic Timer.
   - **Y: 242–268**: The Grimoire chronicle banner, displaying real-time cosmic invocations and events.
   - **Y: 274–314**: 4 Stylus touch buttons: `[ NEW ]`, `[ DIFF: MORTAL / ELDER / ANCIENT ]`, `[ TOME OF LORE ]`, and `[ SND: ON / OFF ]`.

4. **Full-Screen Tome of Forbidden Lore (240×320 Modal)**:
   - Displays all 15 Elder Gods & Great Old Ones and 8 Mortal Easter Eggs.
   - 4 large cards per page with deity names, titles, and secret power perk descriptions.
   - Active deity is outlined with a bold white double-border.
   - Stylus navigation buttons: `[ < PREV ]`, `[ NEXT > ]`, and `[ RESUME ]`.

---

## Dual Control System

- **Stylus Touchscreen**:
  - Tap any piece to select, tap valid move targets to leap or clone.
  - Large 38px touch buttons at screen bottom for instant access.
- **Hardware Rocker & Action Buttons**:
  - **Rocker Up / Down**: Move cursor across the board.
  - **Action / Enter Button (`VK_RETURN` / `0x86`)**: Select cell and execute move.
  - **Escape Key (`VK_ESCAPE`)**: Cleanly exit back to Windows CE desktop.

---

## Installation on Device

### Method 1: CompactFlash (CF) Card (Recommended)
1. Download `ftaghn-nino300-CFCard.zip` from `nino300/release/`.
2. Extract the `Ftaghn` folder directly to the root of your CompactFlash card.
3. Insert the CF card into your Philips Nino 300.
4. Open **File Explorer** on the Nino, navigate to `\Storage Card\Ftaghn`, and tap `FTAGHN.EXE`.

### Method 2: ActiveSync / Windows CE Device Manager
1. Connect the Nino 300 to your host PC via serial cradle/cable using Microsoft ActiveSync 3.x / 4.x or RAPI.
2. Copy `FTAGHN.EXE` and the `AUDIO` directory into `\Program Files\Ftaghn\` on the device.
3. Create a shortcut in `\Windows\Start Menu\Programs\Games`.

---

## Building from Source

### Modern Linux / WSL Toolchain
The repository includes an automated cross-compilation pipeline using `mipsel-linux-gnu-gcc` and a custom Windows CE PE packager (`mips_pe_builder.py`):

```bash
# Run build from repository root
bash nino300/tools/build_nino.sh
```

The script will:
1. Compile pure integer, soft-float MIPS machine code (`-march=r3900 -msoft-float -mabi=32 -EL`).
2. Assemble MIPS `COREDLL.DLL` import thunks and `WinMainCRTStartup`.
3. Link the intermediate MIPS image.
4. Construct compliant Windows CE 2.0 PE headers (`IMAGE_FILE_MACHINE_R4000`), generate `FTAGHN.EXE`, and package `ftaghn-nino300-CFCard.zip`.

### Legacy Microsoft eMbedded Visual C++ 3.0 / 4.0
For developers running retro Windows NT / 2000 / XP development environments:
- Open `nino300/project/Ftaghn_Nino.vcw` in Microsoft eMbedded Visual C++ 3.0.
- Select target: **Palm-size PC 2.01 / 2.11 MIPS** or **Pocket PC MIPS**.
- Build **Release** (`FTAGHN.EXE`).

---

## Directory Structure

```
nino300/
├── src/
│   ├── freestanding.h     # Self-contained Win32/CE types, libc declarations
│   ├── freestanding.c     # Standalone libc implementation (memset, memcpy, snprintf, bump heap)
│   ├── font.h             # 8x8 bitmap font declarations
│   ├── font.c             # 8x8 font glyph matrix
│   ├── sound.h            # Audio dispatcher declarations
│   ├── sound.c            # Dynamic PlaySoundW binding and MessageBeep fallback
│   ├── game.h             # Ataxx rules, 15 Deities, 8 Secrets, GameState
│   ├── game.c             # Move logic, captures, MIPS heuristic AI
│   ├── render.h           # 240x320 DIBSection renderer declarations
│   ├── render.c           # Grayscale software backbuffer, board, HUD, buttons, Tome modal
│   ├── main.c             # WinMain, WndProc, message pump, hardware keys, timer
│   └── crt_mips.S         # WinMainCRTStartup and MIPS COREDLL.DLL import thunks
├── tools/
│   ├── wince_mips.ld      # MIPS Windows CE memory linker script
│   ├── mips_pe_builder.py # Windows CE 2.0 PE COFF assembler (IMAGE_FILE_MACHINE_R4000)
│   └── build_nino.sh      # Automated build & packaging script
├── project/
│   ├── Ftaghn_Nino.vcp    # Microsoft eVC 3.0 project file
│   └── Ftaghn_Nino.vcw    # Microsoft eVC 3.0 workspace file
└── release/
    ├── FTAGHN.EXE         # Windows CE 2.0 MIPS executable (36 KB)
    └── ftaghn-nino300-CFCard.zip # Complete CompactFlash distribution package
```
