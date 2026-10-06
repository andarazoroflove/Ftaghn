# Ftaghn: Cosmic Horror Ataxx (Palm OS Edition - Palm T|X & Palm Z22)

An authentic, dual-resolution port of **Ftaghn: Cosmic Horror Ataxx** targeting Palm OS Garnet devices:
- **Palm T|X**: 312 MHz Intel XScale PXA270, 320×480 HVGA display, collapsible Dynamic Input Area (DIA).
- **Palm Z22**: 200 MHz Samsung S3C2410 (ARM920T), fixed 160×160 CSTN color display, physical silkscreen Graffiti.
- Also compatible with Tungsten T3/T5, LifeDrive, Tapwave Zodiac, Treo 650, Tungsten E/E2, and classic 160×160 Palm OS 5 devices.

---

## Architecture & Features

- **Adaptive Resolution Engine (160×240 HVGA, 160×160 Square, & 320×480 High-Res)**:
  - Dynamically detects display extents via `WinGetDisplayExtent()` at runtime.
  - Automatically queries `FtrGet(pinCreator, pinFtrAPIVersion)`: safely collapses DIA on Palm T|X/T3/LifeDrive, while preserving physical silkscreen Graffiti on Palm Z22 without crashes.
  - **Flicker-Free Double Buffering**: Offscreen rendering window (`WinCreateOffscreenWindow`), blitting full frames in a single pass (`WinCopyRectangle`).

- **Screen Layout (HVGA 160×240 for Palm T|X)**:
  - **Board Area (Y: 2–136)**: 7×7 Cosmic Horror Ataxx grid (19×19 px cells, 133×133 px active area, centered) or 9×9 grid (15×15 px cells, 135×135 px area). 3D shaded glowing orbs, specular highlights, obstacle monoliths, ice blocks, selection aura, and valid clone/leap move targets.
  - **Faction & Score HUD (Y: 138–160)**:
    - Line 1: Crimson score `R:12` with glowing pip, turn status (`YOUR TURN` / `AI THINKING` / `DUEL OVER`), sapphire blue score `10:B`.
    - Line 2: Active player deity lore name (`Cthulhu`), game timer (`TIME: 1:45` or `STASIS`), and `Elder AI`.
  - **The Grimoire (Y: 162–196)**: Real-time eldritch status chronicle displaying the latest 2 invocations, clone buddings, and cosmic anomalies (latest in gold highlight).
  - **Touch Toolbar (Y: 198–222)**: Four large tactile stylus touch buttons: `[ NEW ]`, `[ DIF ]` (Mortal/Elder/Ancient), `[ LORE ]`, and `[ SND ]` (On/Off). Fits with margin to spare whether the Palm OS status bar is visible (height 225) or hidden (height 240).
  - **Paginated Tome of Forbidden Lore (Y: 2–222)**: 4 cards per page, 6 pages for all 23 deities/secrets, with `[ < PREV ]`, `[ NEXT > ]`, and `[ DONE ]` navigation buttons.

- **Complete Tome of Forbidden Lore (15 Deities + 8 Secrets)**:
  - All 15 Elder Gods & Great Old Ones: Cthulhu (Instant Win), Hastur (Unspeakable Oath), Rhan-Tegoth (Eternal Ice), Yog-Sothoth (Dimensional Toll), Ghroth (Spacetime Shuffle), Azathoth (Entropy), Ithaqua (Cold Wind), Abhoth (Sludge Annihilation), Shoggoth (The Sprawl), Nyarlathotep (Crawling Chaos), Yibb-Tstll, Idha (Timer Stasis), Eihort (Time Warp), Shudde M'ell (9×9 expansion), and Default.
  - All 8 Mortal Easter Eggs: George W., The KLF, The X-Files, Nine Inch Nails, Doktor Avalanche, Thrill Kill Kult, Supernatural, and Santa Egg.
  - Interactive paginated Tome modal with direct stylus invocation.

- **Dual Controls**:
  - **Stylus Touch**: Tap directly on board cells, move targets, or touch buttons.
  - **5-Way Navigator D-Pad**: Use the Rocker (Up, Down, Left, Right) to steer the cursor and press Rocker Center to select/move.
  - **Hard Keys / Escape**: Press the Escape key or Hard Home button to cleanly exit the duel.

- **Arcane Sound Synthesis**:
  - Palm OS Sound Manager synthesis (`SndDoCmd`, `sndCmdFreqDurationAmp`).
  - Distinct tones for piece placement, leaps, major captures, eerie anomaly glissandos, and victory fanfare.

---

## Project Structure

```
palmos/
├── src/
│   ├── game.h          # Core game definitions, state, deity matrix
│   ├── game.c          # Ataxx rules, deity triggers, heuristic AI
│   ├── render.h        # HVGA 320x480 rendering declarations
│   ├── render.c        # Double-buffered offscreen renderer, Tome modal
│   ├── sound.h         # Palm OS sound declarations
│   ├── sound.c         # Hardware tone synthesis and SFX dispatcher
│   └── main.c          # PilotMain, DIA setup, event loop & input handling
├── rsc/
│   ├── ftaghn.rcp      # PilRC resource file (Forms, version, icon families)
│   ├── icon_lg_1.bmp   # 22x22 1-bpp monochrome launcher icon
│   ├── icon_lg_8.bmp   # 44x44 8-bpp color hi-res launcher icon
│   ├── icon_sm_1.bmp   # 15x9 1-bpp monochrome small icon
│   └── icon_sm_8.bmp   # 30x18 8-bpp color hi-res small icon
├── tools/
│   ├── build_palm.sh   # Automated build script using prc-tools-remix & pilrc
│   └── make_icons.py   # Python generator for Palm OS icon BMPs
├── release/
│   └── Ftaghn.prc      # Standalone Palm OS application database
└── README.md
```

---

## Building from Source

### Prerequisites (Debian/Ubuntu or WSL)
- `prc-tools-remix` (`m68k-palmos-gcc`, `build-prc`, `palmdev-prep`)
- `pilrc` (v3.2.91+)
- Palm OS SDK 5 R3 or R4 (installed in `/opt/palmdev/sdk-5r3`)

### Build Steps
```bash
# Run the automated build script
bash palmos/tools/build_palm.sh
```

The resulting `Ftaghn.prc` will be generated in `palmos/release/`.

---

## Installation on Palm T|X

1. **SD Card (Easiest)**:
   - Insert an SD card (FAT/FAT16/FAT32 formatted) into your PC card reader.
   - Copy `Ftaghn.prc` into the `/Palm/Launcher/` folder on the SD card (create the folder if it does not exist).
   - Insert the SD card into your Palm T|X. The game will immediately appear in the **Card** or **All** category on the Palm OS Application Launcher!

2. **Palm Desktop / HotSync**:
   - Double-click `Ftaghn.prc` or use the Palm Quick Install tool in Palm Desktop.
   - Connect your Palm T|X to the USB cradle or cable and press the HotSync button.

3. **Card Reader / Filez**:
   - If using a file utility such as *Filez* or *CardBkp* directly on the device, copy `Ftaghn.prc` into RAM or run directly from external storage.
