# Ftaghn (Cosmic Horror Ataxx) - Agent Directives & Workspace Rules

## Autonomous Operation & Full Permissions
- **Full Operational Authority**: The assistant has full permission to inspect, create, edit, compile, build, and execute code and scripts throughout this workspace (`c:\Users\adam\code\ataxx`).
- **Autonomous Execution**: Proactively perform necessary file reads, edits, builds, tests, and analyses without asking for step-by-step confirmation unless an action is genuinely destructive or irreversibly overwrites unsaved user work.
- **Tooling Execution**: When compiling or testing platform targets (Web, GBC, Win95, Solaris, Mac/68k/SE, WinCE Jornada 720), use the appropriate native tools, Makefiles, or build scripts directly.

## Project Structure & Architecture
- `public/`: Canonical Web/Capacitor application (HTML5, Tailwind, JS, Web Audio).
- `ios/`: Capacitor iOS wrapper for the web build.
- `wince/`: HP Jornada 720 Windows CE 3.0 / HPC 2000 port (StrongARM SA-1110 pure ARMv4, 640x240 split screen, Tome of Forbidden Lore, WaveAudio).
- `win95/`: Native Win32 C port targeting Intel 486 / Windows 95 with GDI and WaveOut.
- `gbc/`: Game Boy Color port written in C (GBDK-2020) and Z80/GB assembly.
- `mac/`: Classic Mac OS 9 & OS X Carbon CFM PowerPC port (Retro68 / CMake).
- `mac68k/`: Motorola 68000 Classic Mac OS port.
- `mac_se/`: Macintosh SE System 6/7 edition (standalone 68000 assembly, custom Python assembler, and HFS disk image builder).
- `solaris/`: Sun Solaris 10 / SunOS 5.10 SPARC V8+/V9 edition using X11 and `/dev/audio`.
- `palmos/`: Palm T|X Palm OS Garnet 5.4.9 edition (320x480 HVGA, prc-tools-remix, PACE 68k, Tome of Forbidden Lore, Sound Manager).
- `axim_x50v/`: Dell Axim X50v / X51v Pocket PC 2003SE / WM5 edition (Intel XScale PXA270 624 MHz, Intel 2700G Marathon, 480x640 VGA Portrait, HI_RES_AWARE, WaveAudio).

## Guidelines
- Maintain documentation and comment integrity across all legacy and retro C/ASM source files.
- Preserve platform-specific memory, CRT, and display constraints (e.g. 640x240 Jornada 720, 1-bit Mac SE, 160x144 Game Boy Color screen, Win95 GDI, X11 SPARC).
