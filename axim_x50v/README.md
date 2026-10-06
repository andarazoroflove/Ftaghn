# Ftaghn: Cosmic Horror Ataxx (Dell Axim X50v Edition)

An authentic, high-resolution port of **Ftaghn: Cosmic Horror Ataxx** targeting the **Dell Axim X50v** (and X51v) handheld computer running **Windows Mobile 2003 Second Edition** / **Windows Mobile 5.0** (Intel XScale PXA270 @ 624 MHz, Intel 2700G Marathon multimedia graphics accelerator with 16MB VRAM, 480×640 VGA portrait display).

---

## Hardware & Architecture Highlights

- **Target Device**: Dell Axim X50v / X51v Pocket PC.
- **CPU**: Intel XScale PXA270 at 624 MHz (ARMv5TE).
- **GPU**: Intel 2700G (Marathon) multimedia accelerator with 16MB dedicated video RAM.
- **Display**: 3.7" Transflective TFT LCD, 480×640 VGA Portrait (true 1:1 hardware pixel rendering).
- **Native High-DPI VGA Awareness**:
  - Embedded `HI_RES_AWARE CEUX` resource (`0x0001`) prevents Windows Mobile 2003SE / WM5 from pixel-doubling from QVGA.
  - Automatically invokes Pocket PC `SHFullScreen(hwnd, SHFS_HIDETASKBAR | SHFS_HIDESIPBUTTON | SHFS_HIDESTARTICON)` to achieve edge-to-edge 480×640 VGA gaming.
- **Intel 2700G Accelerated Double Buffering**:
  - 32-bit ARGB software composition backbuffer with fast conversion to native 16-bit RGB565 DIB section (`BI_BITFIELDS` masks `0xF800`, `0x07E0`, `0x001F`).
  - Single-pass blit (`BitBlt`) accelerated by the Intel 2700G 2D engine for flicker-free 60 FPS gameplay.

---

## Screen Layout (Dynamic 240×320 QVGA & 480×640 VGA Portrait)

The engine automatically queries device metrics (`SM_CXSCREEN`, `SM_CYSCREEN`) and scales all board cells, piece graphics, HUD elements, grimoire scroll, and touch toolbar to fit the screen 100% edge-to-edge whether the Dell Axim X50v is running in native VGA (480×640) or Windows Mobile 2003SE standard QVGA mode (240×320).

1. **Board Section (Safe Top Margin)**:
   - **QVGA 240×320**: 7×7 grid uses 22×22 px cells (154×154 px active area, $y = 28..182$); 9×9 uses 19×19 px cells (171×171 px active area, $y = 27..198$). Starts strictly below the $y = 0..26$ Pocket PC title bar.
   - **VGA 480×640**: 7×7 grid uses 44×44 px cells (308×308 px active area, $y = 56..364$); 9×9 uses 38×38 px cells (342×342 px active area, $y = 54..396$). Starts strictly below the $y = 0..52$ Pocket PC title bar.
   - Glowing 3D orbs with specular highlights, monolith obstacles with glowing red eye slits, cyan frozen ice blocks (Rhan-Tegoth), present boxes with gold ribbons (Santa Egg), pulsing gold selection auras, and move indicators (green dot for clone, amber ring for leap).
2. **Unified Faction & Deity HUD**:
   - Scores (Red & Blue), player names, real-time turn indicator ("YOUR TURN" / "AI THINK"), Cosmic Game Timer / Stasis, and active Deity lore title in a single compact status box.
3. **The Grimoire**:
   - Real-time eldritch chronicle scroll dynamically sized to fill the space above the toolbar.
4. **Thinner Touch Toolbar (Safe Bottom Margin)**:
   - Reduced height to 22 px on QVGA ($y = 272..294$) and 44 px on VGA ($y = 540..584$).
   - Four stylus touch buttons: `[ NEW ]`, `[ DIFF ]`, `[ TOME ]`, and `[ SOUND ]` positioned safely above the bottom navigation/SIP bar.
5. **Tome of Forbidden Lore Modal**:
   - Comprehensive interactive codex showing all 15 Deities and 8 Mortal Easter Eggs, fitted safely within the visible client area.
   - Stylus navigation (`[ < PREV ]`, `[ NEXT > ]`), `[ INVOKE ]`, and `[ RESUME DUEL ]`.

---

## Dual Controls & Hardware Navigation

- **Stylus Touchscreen**: Tap directly on board cells, move targets, or touch buttons.
- **5-Way Navigator D-Pad**: Steer the cursor cell using the D-Pad (Up, Down, Left, Right) and press Center/Action (`VK_RETURN` / `VK_ACTION`) to select or move.
- **Clean Exit**: Press **Escape** (`VK_ESCAPE`), **'Q'**, or the hardware Home key to cleanly close the game and return to the Pocket PC Today screen.

---

## Arcane Audio Engine

- High-quality 22 kHz audio playback via Windows CE WaveAudio subsystem (`PlaySoundW` / `sndPlaySoundW`).
- Seamless background ambient music looping (`bg_music.wav`).
- Priority-based sound effects for placements, multi-horror leaps, anomaly glissandos, and victory fanfare, with automatic resumption of ambient music.
- Instant mute toggle.

---

## Project Structure

```
axim_x50v/
├── src/
│   ├── game.h          # Core game definitions, state, deity matrix
│   ├── game.c          # Ataxx rules, deity triggers, heuristic AI
│   ├── render.h        # 480x640 VGA rendering declarations
│   ├── render.c        # Double-buffered VGA renderer, GDI / DIB section
│   ├── sound.h         # WaveOut audio declarations
│   ├── sound.c         # WaveOut player & ambient BGM loop
│   ├── font.h          # 8x8 font table and scaling
│   ├── font.c          # Font glyph data
│   ├── bmp_loader.h    # 24bpp / 8bpp BMP background loader
│   ├── bmp_loader.c    # Fast BMP scaling loader
│   ├── freestanding.h  # CRT-independent runtime declarations
│   ├── freestanding.c  # CRT-independent string/memory runtime
│   └── main.c          # WinMain, SHFullScreen VGA setup, input loop
├── rsc/
│   ├── ftaghn.rc       # Resource script (HI_RES_AWARE CEUX, Icons)
│   └── ftaghn.ico      # Multi-resolution VGA application icon
├── tools/
│   ├── build_axim.sh   # Automated build script (arm-mingw32ce-gcc / docker)
│   └── make_icon.py    # Python generator for Pocket PC multi-res ICO
├── release/
│   ├── Ftaghn.exe      # Standalone Dell Axim X50v executable (480x640 VGA)
│   ├── AUDIO/          # Normalized 22 kHz audio files
│   ├── IMAGES/         # Background BMPs
│   └── ftaghn-axim-x50v-StorageCard.zip
└── README.md
```

---

## Building from Source

### Prerequisites
- Docker (with `777shuang/docker-cegcc`) or a local `arm-mingw32ce-gcc` toolchain.

### Build Steps
```bash
# Build the native ARMv5TE binary and release zip:
bash axim_x50v/tools/build_axim.sh
```

---

## Installation on Dell Axim X50v

1. **SD / CF Card (Fastest)**:
   - Extract `ftaghn-axim-x50v-StorageCard.zip` onto an SD or CompactFlash card.
   - Insert the card into your Dell Axim X50v.
   - In File Explorer, navigate to `\Storage Card\Ftaghn\` (or `\CF Card\Ftaghn\`) and tap **`Ftaghn.exe`**.

2. **ActiveSync / Windows Mobile Device Center**:
   - Connect your Axim X50v via USB cradle.
   - Using the ActiveSync "Explore" feature, copy the `Ftaghn` folder into `\Program Files\` or `\Storage Card\`.
   - Launch `Ftaghn.exe`.
