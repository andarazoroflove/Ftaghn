# Ftaghn : Cosmic Horror Ataxx

> *"That is not dead which can eternal lie,  
> And with strange aeons even death may die."*

**Ftaghn** is an ancient infection strategy board game (based on the classic 1990 arcade hit *Ataxx* and *Hexxagon*) steeped in Lovecraftian cosmic horror and eldritch dread. Two elder deities clash for dimensional dominion across an unholy grid, cloning spores and warping across void rifts to assimilate their opponent's horrors.

This repository hosts the **source code** for Ftaghn across 8 distinct computing platforms, from modern web and mobile browsers to vintage PCs, workstations, and retro consoles.

---

## Supported Platform Matrix

| Platform | Target Architecture | Directory | Technologies / Toolchain |
| :--- | :--- | :--- | :--- |
| **Web & PWA** | Modern Web Browsers | [`public/`](./public/) | Vanilla ES6 JS, Tailwind CSS, Web Audio API |
| **iOS** | iPhone / iPad | [`ios/`](./ios/) | Capacitor iOS wrapper |
| **HP Jornada 720** | Windows CE 3.0 / HPC 2000 | [`wince/`](./wince/) | StrongARM SA-1110 pure ARMv4 C, 640×240 split GDI |
| **Windows 95** | Intel 80486 / Win95+ | [`win95/`](./win95/) | Pure Win32 GDI, WaveAudio, freestanding C runtime |
| **Game Boy Color** | Nintendo CGB / DMG | [`gbc/`](./gbc/) | GBDK-2020 C, custom tilemaps, GB APU audio |
| **Sun Solaris 10** | SPARC V8+ (32-bit) & V9 (64-bit) | [`solaris/`](./solaris/) | Pure X11 / Xlib, `/dev/audio` $\mu$-law streaming |
| **Macintosh SE** | Motorola 68000 @ 8 MHz | [`mac_se/`](./mac_se/) | Pure 68000 assembly, standalone Python HFS pipeline |
| **Classic Mac OS** | PowerPC Carbon & 68k | [`mac/`](./mac/), [`mac68k/`](./mac68k/) | Retro68 GCC, Rez resource compilers, QuickDraw |

---

## Core Gameplay Rules

Ataxx is played on a 7×7 grid with central monolith obstacles. On your turn, select one of your horrors:

1. **Spore Clone (Distance 1)**: Move to any horizontally, vertically, or diagonally adjacent empty cell. A new horror blooms at the destination, leaving the origin piece intact (**+1 net horror**).
2. **Void Leap (Distance 2)**: Warp two squares away in any direction. The piece teleports to the destination cell, leaving the origin cell empty (**0 net horror**).
3. **Eldritch Assimilation (Infection)**: Upon landing, **all opposing horrors adjacent to the destination cell are instantly corrupted and converted** into your faction!
4. **Dominance**: The deity controlling the greatest multitude of horrors when the grid is filled or time expires wins the duel.

---

## The Tome of Forbidden Knowledge

Players can invoke ancient entities or mortal secrets that alter game mechanics, board geometry, and timers:

### Elder Deities
- **Hastur (Unspeakable Oath)**: 4 consecutive clone moves summon an extra Red horror randomly.
- **Rhan-Tegoth (The Frozen Fear)**: A leap with 0 captures into a cluster of $\ge 5$ pieces encases adjacent foes into permanent ice blocks!
- **Yog-Sothoth (Dimensional Toll)**: A leap capturing $\ge 2$ pieces seals all 8 surrounding cells with immovable obstacles.
- **Ghroth (Ghroth's Orbit)**: 5% chance after player turn to scramble all pieces in a random 3×3 zone.
- **Azathoth (Cosmic Entropy)**: Clones when pieces abound convert 3 random empty cells into obstacles.
- **Ithaqua (Cold Wind)**: Leaping with 0 captures from an isolated space converts origin into an obstacle.
- **Abhoth (Annihilation)**: Landing adjacent to $\ge 6$ Abhoth pieces annihilates the attacking piece!
- **Shoggoth (The Sprawl)**: AI has a 10% chance per turn to execute up to 3 rapid clone moves.
- **Nyarlathotep (Crawling Chaos)**: AI has a 5% chance to hijack piece selection to a neighboring piece.
- **Yibb-Tstll (Obstacle Volatility)**: Captures mutate or fortify obstacles into permanent barriers.
- **Idha (Idha's Slumber)**: The Sleeper awakens—the main game timer stops completely.
- **Eihort (Dimensional Stasis / Surge)**: Slows time by 50% for player, or surges time 200% for AI.
- **Shudde M'ell (Excessive Space)**: Expands the board dimensions permanently to 9×9!
- **Cthulhu (Call of Cthulhu)**: Holding the center and all four corners triggers an **instant victory**!

### Mortal Easter Eggs
- **George W.** (*"w."*): Presidential portrait avatar overlay and fanfare audio.
- **The KLF** (*"klf"*): Pure white cult pieces with golden cores, Dillinger and MuMu audio themes.
- **The X-Files** (*"fox"* / *"dana"*): Paranormal theme audio and deep-space investigation backdrop.
- **Nine Inch Nails** (*"nin"*): Industrial dread soundtrack and obsidian metallic themes.
- **Doktor Avalanche** (*"sisters"*): Sisters of Mercy drum machine ambiance and silver-on-black iconography.
- **Groovie Mann** (*"tkk"*): Thrill Kill Kult acid house disco horror and custom backdrop.
- **Supernatural** (*"sam"* / *"dean"*): Winchester hunter runes and Rowena witch opposing pieces.
- **Santa Egg** (*"santa"*): Festive presents mode, gift boxes, ho-ho-ho sound!

---

## Building Each Platform from Source

### Web & Mobile (`public/`, `ios/`)
```bash
# Serve locally with any static web server:
python3 -m http.server 8000 -d public
```

### HP Jornada 720 (`wince/`)
Requires CeGCC Docker cross-toolchain (`777shuang/docker-cegcc`):
```bash
cd wince
bash tools/build_wince.sh
```

### Windows 95 (`win95/`)
Requires 32-bit MinGW GCC:
```bash
cd win95
mingw32-make
```

### Nintendo Game Boy Color (`gbc/`)
Requires GBDK-2020 (`lcc`):
```bash
cd gbc
make
```

### Sun Solaris 10 SPARC (`solaris/`)
Requires GCC on Solaris or cross-compiler:
```bash
cd solaris
make
```

### Macintosh SE 68000 (`mac_se/`)
Pure Python pipeline (zero external compiler dependencies):
```bash
cd mac_se
python3 build_mac_app.py
```

### Classic Mac OS 9 / PowerPC & 68k (`mac/`, `mac68k/`)
Requires Retro68:
```bash
cd mac
make
```

---

## Sanitization Notice

In accordance with release hygiene standards, this repository contains **only source code, assets, resource files, and build scripts**. No pre-compiled binaries, executables, object files, or disk images (`.exe`, `.bin`, `.dsk`, `.gbc`, `.o`) are committed to version control.
