# Ftaghn: Cosmic Horror Ataxx — HP Jornada 720 Edition

Native **Windows CE 3.0 / Handheld PC 2000** port of **Ftaghn: Cosmic Horror Ataxx**, engineered specifically for the **HP Jornada 720** handheld computer with pure **Motorola/Intel StrongARM SA-1110 (ARMv4)** compatibility.

---

## Technical Specifications

| Parameter | Specification | Implementation Details |
| :--- | :--- | :--- |
| **Target Device** | HP Jornada 720 Handheld PC | Windows CE 3.0 / Handheld PC 2000 (HPC 2000) |
| **Processor** | StrongARM SA-1110 @ 206 MHz | Pure ARMv4 machine code (zero `bx`/`blx` instructions, SA-1110 safe) |
| **Memory** | 32 MB Unified RAM | Freestanding lightweight binary (<96 KB executable, ~2 MB runtime RAM) |
| **Display** | 640 × 240 (Half-VGA 2D LCD) | Split-screen layout (320px Board & Art / 320px Scoreboard & Grimoire) |
| **Input** | Stylus Touchscreen & Keyboard | Touch-friendly buttons, stylus drag/tap, and physical keyboard hotkeys |
| **Audio** | WinCE WaveAudio / `PlaySoundW` | 20 compressed 11,025 Hz 8-bit mono WAV effects and ambient music |

---

## 640 × 240 Half-Screen Split Architecture

The HP Jornada 720 features a distinctive 640 × 240 widescreen LCD. Ftaghn divides this display into two halves:

```
+-------------------------------+-------------------------------+
|          LEFT HALF            |          RIGHT HALF           |
|         (320 x 240)           |         (320 x 240)           |
+-------------------------------+-------------------------------+
|  * 320x240 Cosmic Background  |  * Title Banner               |
|  * 7x7 Ataxx Grid (224x224)   |  * Score Badges (Red / Blue)  |
|  * 3D Specular Orbs           |  * Turn & Timer (Distortions) |
|  * Valid Move Spores/Diamonds |  * The Grimoire (Status Log)  |
|  * Obstacles & Ice Blocks     |  * Active Deity / Secret Box  |
|                               |  * Touch Stylus Action Buttons|
+-------------------------------+-------------------------------+
```

---

## Game Features & Tome of Forbidden Lore

### 1. Complete Tome of Forbidden Knowledge
Accessible at any time via the **`[ TOME / LORE ]`** touch button or pressing **`T`** / **`S`** on the Jornada keyboard. An interactive modal dialog allows browsing, audio previews, and instant invocation of all 23 deities and mortal easter eggs:

- **Rhan-Tegoth (The Frozen Fear)**: Leap with 0 captures into a cluster of $\ge 5$ pieces encases adjacent foes into permanent ice obstacles!
- **Hastur (Unspeakable Oath)**: 4 consecutive clone moves summons an extra Red horror randomly.
- **Yog-Sothoth (Dimensional Toll)**: Leap capturing $\ge 2$ pieces seals all 8 surrounding cells with Obstacles.
- **Ghroth (Ghroth's Orbit)**: 5% chance after player turn to shuffle all pieces in a random 3×3 zone.
- **Azathoth (Cosmic Entropy)**: Clones when pieces abound convert 3 random empty cells into obstacles.
- **Ithaqua (Cold Wind)**: Leaping with 0 captures from an isolated square turns origin into an obstacle.
- **Abhoth (Annihilation)**: Enemy landing adjacent to $\ge 6$ Abhoth pieces is instantly destroyed!
- **Shoggoth (The Sprawl)**: AI has a 10% chance per turn to execute up to 3 rapid clone moves.
- **Nyarlathotep (Crawling Chaos)**: AI has a 5% chance to hijack player piece selection to a neighbor.
- **Yibb-Tstll (Obstacle Volatility)**: Captures mutate or fortify obstacles into permanent barriers.
- **Idha (Idha's Slumber)**: The Sleeper awakens; main game timer ceases completely (time stops).
- **Eihort (Dimensional Stasis / Surge)**: Player slows time by 50%; AI accelerates time by 200%.
- **Shudde M'ell (Excessive Space)**: Expands the board grid to 9×9!
- **Cthulhu (Call of Cthulhu)**: Holding center and all 4 corners triggers an INSTANT WIN!

### 2. Mortal Easter Eggs
- **George W. Bush**: Presidential portrait avatar overlay and fanfare audio.
- **The KLF**: Pure stark white pieces with golden core, Dillinger and MuMu themes.
- **The X-Files**: Paranormal theme audio and eerie deep-space investigation backdrop.
- **Nine Inch Nails**: Industrial dread music and obsidian metallic themes.
- **Doktor Avalanche (Sisters of Mercy)**: Gothic drum machine ambiance and silver-on-black iconography.
- **Groovie Mann (Thrill Kill Kult)**: Sleazy acid house disco horror and custom industrial backdrop.
- **Supernatural**: Winchester hunter runes with Rowena orange opposing witch pieces.
- **Santa Egg**: Festive presents mode, gift boxes, ho-ho-ho sound!

---

## StrongARM SA-1110 Optimized AI

The StrongARM SA-1110 @ 206 MHz lacks hardware division and has no out-of-order execution. The AI has been tailored to guarantee sub-millisecond response times:

- **Mortal (Easy)**: Fast pseudo-random selection with a 40% bias toward greedy captures. Response time: **< 0.05 ms**.
- **Elder (Medium)**: Single-pass positional heuristic (captures × 10 + clone bonus + edge control). Response time: **< 0.2 ms**.
- **Ancient One (Hard)**: Single-pass heuristic + 1-ply leap threat penalty (avoids landing adjacent to counter-attacking leapers). Response time: **< 0.5 ms**.

---

## Installation on the HP Jornada 720

1. Unpack **`wince/release/ftaghn-j720-StorageCard.zip`** to your CompactFlash card:
   ```
   \Storage Card\Ftaghn\FTAGHN.EXE
   \Storage Card\Ftaghn\AUDIO\
   \Storage Card\Ftaghn\IMAGES\
   \Storage Card\Ftaghn\README.TXT
   ```
2. Insert the CompactFlash card into your HP Jornada 720.
3. Open Windows CE **File Explorer**, navigate to `\Storage Card\Ftaghn\`, and double-tap **`FTAGHN.EXE`**!

---

## Building from Source

The build pipeline is automated through the bundled CeGCC cross-compiler container:

```bash
# In WSL or Linux:
cd wince
bash tools/build_wince.sh
```

This compiles all C and pure ARMv4 assembly sources, runs `armv4_patch` to convert any lingering `bx` instructions to `mov pc, Rm`, verifies zero `bx`/`blx` instructions via `objdump`, and packages the distribution ZIP.
