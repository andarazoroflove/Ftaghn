# Ftaghn: Cosmic Horror Ataxx for Macintosh SE

A native classic Macintosh adaptation of **Ftaghn - Cosmic Horror Ataxx**, engineered specifically for the **Macintosh SE** with **2 MB of RAM** running **System 7.0 or newer** (also backwards-compatible with System 6.0.8+).

---

## Technical Specifications

| Parameter | Macintosh SE Specification | Implementation Details |
| :--- | :--- | :--- |
| **Processor** | Motorola 68000 @ 7.8336 MHz | Pure 68000 machine code (no 68020/030 or FPU instructions) |
| **RAM** | 2 MB | Memory partition configured via `'SIZE'` resource: **512 KB preferred**, **256 KB minimum** |
| **Display** | 9-inch 1-bit CRT (512 × 342) | High-contrast 1-bit QuickDraw graphics with dithered patterns and 3D orb highlights |
| **Operating System** | System 7.0+ (and System 6.0.8+) | Cooperative multitasking with `WaitNextEvent` and 32-bit clean addressing flags |
| **Sound** | 4-voice audio / System Beep | Tactile audio tones via `_SysBeep` for selections, clones, leaps, and captures |

---

## Game Features

- **7×7 Ataxx Grid**: Compact layout (238 × 238 pixels) with custom 35×35 pixel cells.
- **Piece Design**:
  - **Cult of Cthulhu (Player 1)**: Solid black arcane orbs with a specular highlight glint.
  - **Elder Sign (Player 2 / AI)**: Double concentric rings with an inner protective Elder Cross.
- **Movement Rules**:
  - **Clone (Distance 1)**: Click an orb, then click an adjacent empty square. Spawns a new orb while keeping the original.
  - **Leap (Distance 2)**: Click an orb, then click a square 2 steps away. Warps the orb to the new location, emptying the source cell.
  - **Infection / Corruption**: Any enemy orbs orthogonally or diagonally touching the destination square are instantly assimilated into your faction!
- **Game Modes**:
  - **Human vs AI**: Battle against the Cosmic Intelligence.
  - **Human vs Human**: Two-player pass-and-play.
  - **AI vs AI**: Watch the ancient entities clash autonomously.
- **Cosmic AI**:
  - 3 Difficulty settings: **Mortal (Easy)**, **Elder (Medium)**, and **Ancient One (Hard)**.
  - Evaluates clone expansion, leap tactics, and capture counts in under 100 ms on an 8 MHz 68000.

---

## File Deliverables in this Directory

| File | Format | Description |
| :--- | :--- | :--- |
| **`FtaghnAtaxx.bin`** | MacBinary II | Universal classic Mac archive. Drag-and-drop into Mini vMac / Basilisk II, or unpack with StuffIt Expander. |
| **`FtaghnAtaxx.dsk`** | 800K HFS Floppy Image | Standard double-density 800K Macintosh floppy disk image. Mount directly in Floppy Emu or write to physical 800K disks. |
| **`FtaghnAtaxx_1440k.dsk`** | 1.44MB HD Floppy Image | High-density 1.44MB SuperDrive disk image for Macintosh SE SuperDrive models or SCSI2SD. |
| **`FtaghnAtaxx.hqx`** | BinHex 4.0 | 7-bit ASCII encoded classic Mac archive for transfer over serial cable or retro BBS. |
| **`Ftaghn Ataxx` & `._Ftaghn Ataxx`** | AppleDouble | Raw data fork and resource fork files for Netatalk / AppleShare AFP servers. |

---

## How to Run

### 1. On Physical Macintosh SE Hardware
- **Using Floppy Emu**: Copy `FtaghnAtaxx.dsk` (800K) or `FtaghnAtaxx_1440k.dsk` (1.44MB) to your SD card. Insert the Floppy Emu into the DB-19 floppy port and select the disk image.
- **Writing to a Real 800K Floppy**: Write `FtaghnAtaxx.dsk` using a Greaseweazle, KryoFlux, or an older Mac with a SuperDrive using Disk Copy 4.2 / 6.3.3.
- **Using Serial Cable / Modem**: Transfer `FtaghnAtaxx.bin` using ZTerm or MacTerminal over a modem port, then double-click the application.

### 2. In Mini vMac
1. Launch **Mini vMac** (configured for Mac SE or Mac Plus with System 7.0.1 or System 6.0.8).
2. Drag and drop `FtaghnAtaxx.dsk` directly into the Mini vMac window.
3. The disk "Ftaghn Ataxx" will mount on the desktop. Double-click the custom Cthulhu orb icon to launch!

### 3. In Basilisk II
1. Open BasiliskIIGUI, go to the **Volumes** tab, and add `FtaghnAtaxx.dsk`.
2. Start Basilisk II. The disk will appear on your desktop with the game and ReadMe file.

---

## Rebuilding from Source

The build pipeline is completely standalone and written in pure Python:

```bash
cd mac_se
python3 build_mac_app.py
```

This compiles `ftaghn_ataxx.s` using the included 68000 assembler (`m68k_assembler.py`), generates the resource fork (`mac_resource.py`), and builds all distribution disk images and archives (`machfs/`).

