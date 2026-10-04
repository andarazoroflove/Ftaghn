# Ftaghn: Cosmic Horror Ataxx (Palm T|X Edition)

An authentic port of **Ftaghn: Cosmic Horror Ataxx** targeting the **Palm T|X** handheld computer running **Palm OS Garnet 5.4.9** (312 MHz Intel XScale PXA270, 320×480 HVGA display).

Also fully compatible with other Palm OS 5 devices featuring 320×480 or 320×320 displays (Tungsten T3, Tungsten T5, LifeDrive, Tapwave Zodiac, etc.).

---

## Architecture & Features

- **Native HVGA (320×480) Display Support**:
  - Leverages Palm OS 5 High Density Display Manager (`WinSetCoordinateSystem(kCoordinatesNative)`).
  - Automatically collapses the Dynamic Input Area (DIA / soft Graffiti) via `PINSetInputAreaState(pinInputAreaClosed)` and `StatHide()` to expose the full 320×480 screen.
  - **Flicker-Free Double Buffering**: Offscreen rendering window (`WinCreateOffscreenWindow(320, 480, screenFormat, ...)`), blitting full frames in a single pass (`WinCopyRectangle`).

- **Screen Layout (320×480 Portrait)**:
  - **Top Area (320×236)**: 7×7 Cosmic Horror Ataxx grid (or 9×9 when Shudde M'ell is invoked). 3D shaded glowing orbs, specular highlights, obstacle monoliths, ice blocks, selection aura, and valid clone/leap move targets.
  - **Mid Area (Y: 238–276)**: Faction scoreboard, Player vs AI scores, active turn indicator, and Cosmic Game Timer / Stasis.
  - **The Grimoire (Y: 280–358)**: Eldritch status chronicle displaying the latest invocations, clone buddings, and cosmic anomalies.
  - **Active Deity Card (Y: 362–404)**: Summary of active deity powers and titles.
  - **Touch Toolbar (Y: 410–472)**: Stylus touch buttons for `[ NEW GAME ]`, `[ AI: MORTAL / ELDER / ANCIENT ]`, `[ TOME OF LORE ]`, and `[ SOUND: ON / OFF ]`.

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
