#!/usr/bin/env python3
"""
Ftaghn Ataxx - Build & Packaging Script for Macintosh SE
Compiles 68000 assembly, creates Classic Mac Resource Fork,
and outputs MacBinary II (.bin), BinHex (.hqx), and HFS Floppy Images (.dsk).
"""

import os
import sys
import struct
import math

from m68k_assembler import Assembler
from mac_resource import Resource, build_resource_fork, make_macbinary, make_binhex
from machfs import Volume, File

def generate_icon_and_mask():
    """Generate 32x32 1-bit icon bitmap and mask (128 bytes each)."""
    grid = [[0 for _ in range(32)] for _ in range(32)]
    mask = [[0 for _ in range(32)] for _ in range(32)]

    cx, cy, r = 15.5, 15.5, 11.0

    for y in range(32):
        for x in range(32):
            d = math.hypot(x - cx, y - cy)
            # Main orb
            if d <= r:
                mask[y][x] = 1
                # Outer ring
                if d >= r - 1.5:
                    grid[y][x] = 1
                # Arcane elder star inside
                elif abs(x - cx) <= 1 or abs(y - cy) <= 1:
                    grid[y][x] = 1
                # Center eye
                elif math.hypot(x - cx, y - cy) <= 3:
                    grid[y][x] = 1
                # Highlight glint
                elif math.hypot(x - (cx - 4), y - (cy - 4)) <= 2:
                    grid[y][x] = 0
            # Tentacles below orb
            if 24 <= y <= 30:
                if x in (8, 9, 14, 15, 16, 17, 22, 23):
                    grid[y][x] = 1
                    mask[y][x] = 1
            # Horns/crests above orb
            if 2 <= y <= 6:
                if (6 <= x <= 8 and y <= 8 - x + 6) or (23 <= x <= 25 and y <= 8 - (25 - x)):
                    grid[y][x] = 1
                    mask[y][x] = 1

    icon_bytes = bytearray(128)
    mask_bytes = bytearray(128)

    for y in range(32):
        for byte_idx in range(4):
            b_icon = 0
            b_mask = 0
            for bit in range(8):
                x = byte_idx * 8 + bit
                if grid[y][x]:
                    b_icon |= (1 << (7 - bit))
                if mask[y][x]:
                    b_mask |= (1 << (7 - bit))
            icon_bytes[y * 4 + byte_idx] = b_icon
            mask_bytes[y * 4 + byte_idx] = b_mask

    return bytes(icon_bytes) + bytes(mask_bytes)


def build_app():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    asm_source_path = os.path.join(script_dir, "ftaghn_ataxx.s")

    print(f"=== Compiling Ftaghn Ataxx for Macintosh SE (68000 / System 7) ===")
    print(f"Reading assembly source: {asm_source_path}")
    with open(asm_source_path, "r") as f:
        src = f.read()

    # 1. Assemble 68000 code
    print("Assembling Motorola 68000 machine code...")
    asm = Assembler()
    machine_code = asm.assemble(src)
    print(f"-> Assembly successful: {len(machine_code)} bytes of pure 68000 machine code.")

    # 2. Build CODE 0 (Jump Table)
    # Header: Above A5 (0), Below A5 (0x1000 = 4KB), Jump table len (8), Jump table offset (0x0020)
    # Entry 1: Seg 1, Offset 0, Push 1 (0x3F3C 0x0001), _LoadSeg (0xA9F0)
    code_0 = struct.pack('>IIII', 0, 0x1000, 8, 0x0020)
    code_0 += struct.pack('>HH', 1, 0)
    code_0 += struct.pack('>HH', 0x3F3C, 1)
    code_0 += struct.pack('>H', 0xA9F0)

    # 3. Build CODE 1 (Segment 1)
    # Segment header: First jump table entry offset (0), Number of entries (1)
    code_1 = struct.pack('>II', 0, 1) + machine_code

    # 4. Build SIZE resource (ID -1)
    # Flags: 0x5800 (acceptSuspendResume, is32BitCompatible, isHighLevelEventAware)
    # Preferred Size: 512 KB (524,288 bytes)
    # Minimum Size: 256 KB (262,144 bytes)
    size_res = struct.pack('>HII', 0x5800, 512 * 1024, 256 * 1024)

    # 5. Build BNDL resource (ID 128)
    # Signature 'FTAG', ID 0
    # Type count - 1 = 1 (ICN# and FREF)
    bndl = b'FTAG\x00\x00'
    bndl += struct.pack('>H', 1) # 2 types
    # ICN# mapping
    bndl += b'ICN#' + struct.pack('>H', 0) # 1 entry
    bndl += struct.pack('>hh', 0, 128)     # local ID 0 -> res ID 128
    # FREF mapping
    bndl += b'FREF' + struct.pack('>H', 0) # 1 entry
    bndl += struct.pack('>hh', 0, 128)     # local ID 0 -> res ID 128

    # 6. Build FREF resource (ID 128)
    # Type 'APPL', local icon ID 0, empty filename
    fref = b'APPL\x00\x00\x00'

    # 7. Build ICN# resource (ID 128)
    icn_data = generate_icon_and_mask()

    # 8. Build vers resource (ID 1 & 2)
    # Version: 1.0.0, country 0, "1.0", "Ftaghn Ataxx 1.0 (Macintosh SE)"
    vers = bytearray([1, 0, 0, 0, 0, 0]) # 1.0.0 release
    vers += bytes([3]) + b'1.0'
    vers_desc = b'Ftaghn Ataxx 1.0 for Macintosh SE'
    vers += bytes([len(vers_desc)]) + vers_desc

    # 9. Assemble Resource Fork
    resources = [
        Resource(b'CODE', 0, code_0, 'Jump Table'),
        Resource(b'CODE', 1, code_1, 'Main Segment'),
        Resource(b'SIZE', -1, size_res, 'Memory Size'),
        Resource(b'BNDL', 128, bndl, 'Bundle'),
        Resource(b'FREF', 128, fref, 'File Reference'),
        Resource(b'ICN#', 128, icn_data, 'Application Icon'),
        Resource(b'vers', 1, vers, 'Version'),
        Resource(b'STR ', 0, bytes([28]) + b'Ftaghn Ataxx for Mac SE 1.0', 'Signature'),
    ]

    rsrc_fork = build_resource_fork(resources)
    data_fork = b'' # Classic Mac apps put executable in resource fork

    print(f"-> Resource fork built: {len(rsrc_fork)} bytes, {len(resources)} resources.")

    # 10. Output 1: MacBinary II (.bin)
    bin_path = os.path.join(script_dir, "FtaghnAtaxx.bin")
    # Finder flags: 0x2000 (hasBundle)
    bin_data = make_macbinary("Ftaghn Ataxx", b'APPL', b'FTAG', data_fork, rsrc_fork, finder_flags=0x2000)
    with open(bin_path, "wb") as f:
        f.write(bin_data)
    print(f"[OK] Created MacBinary II archive: {bin_path} ({len(bin_data):,} bytes)")

    # 11. Output 2: BinHex 4.0 (.hqx)
    hqx_path = os.path.join(script_dir, "FtaghnAtaxx.hqx")
    hqx_text = make_binhex("Ftaghn Ataxx", b'APPL', b'FTAG', data_fork, rsrc_fork)
    with open(hqx_path, "w", encoding="ascii") as f:
        f.write(hqx_text)
    print(f"[OK] Created BinHex 4.0 text file: {hqx_path} ({len(hqx_text):,} bytes)")

    # 12. Output 3: 800K Macintosh HFS Floppy Disk Image (.dsk)
    dsk_800_path = os.path.join(script_dir, "FtaghnAtaxx.dsk")
    v800 = Volume()
    v800.name = 'Ftaghn Ataxx'
    
    app_file = File()
    app_file.type = b'APPL'
    app_file.creator = b'FTAG'
    app_file.flags = 0x2000 # hasBundle
    app_file.data = data_fork
    app_file.rsrc = rsrc_fork
    v800['Ftaghn Ataxx'] = app_file

    readme_file = File()
    readme_file.type = b'TEXT'
    readme_file.creator = b'ttxt'
    readme_content = (
        "FTAGHN: Cosmic Horror Ataxx (Macintosh SE Edition)\r\r"
        "Platform: Macintosh SE (68000 CPU, 2 MB RAM, System 7.0 or newer)\r"
        "Screen: 512 x 342 1-bit Monochrome\r\r"
        "HOW TO PLAY:\r"
        "1. Click one of your orbs to awaken it.\r"
        "2. Click an adjacent empty square (distance 1) to CLONE.\r"
        "3. Click a square 2 steps away to LEAP.\r"
        "4. Any opponent orbs touching your landing square are instantly\r"
        "   corrupted and converted to your faction!\r"
        "5. Fill the board or wipe out your opponent to prevail.\r\r"
        "May the Ancient Ones guide your moves.\r"
    ).encode('mac_roman')
    readme_file.data = readme_content
    v800['ReadMe'] = readme_file

    dsk_800_data = v800.write(size=800 * 1024)
    with open(dsk_800_path, "wb") as f:
        f.write(dsk_800_data)
    print(f"[OK] Created 800K HFS Floppy Image: {dsk_800_path} ({len(dsk_800_data):,} bytes)")

    # 13. Output 4: 1.44MB High-Density HFS Floppy Disk Image (.dsk)
    dsk_1440_path = os.path.join(script_dir, "FtaghnAtaxx_1440k.dsk")
    v1440 = Volume()
    v1440.name = 'Ftaghn Ataxx HD'
    v1440['Ftaghn Ataxx'] = app_file
    v1440['ReadMe'] = readme_file

    dsk_1440_data = v1440.write(size=1440 * 1024)
    with open(dsk_1440_path, "wb") as f:
        f.write(dsk_1440_data)
    print(f"[OK] Created 1.44MB HD HFS Floppy Image: {dsk_1440_path} ({len(dsk_1440_data):,} bytes)")

    # 14. Output 5: Raw AppleDouble files for netatalk / AFP / vintage file transfer
    raw_app_path = os.path.join(script_dir, "Ftaghn Ataxx")
    with open(raw_app_path, "wb") as f:
        f.write(data_fork)
    
    # AppleDouble header (%AppleDouble format)
    # Magic 0x00051607, Version 0x00020000, 2 entries (Finder info + Resource fork)
    ad_header = struct.pack('>II16sH', 0x00051607, 0x00020000, b'Mac OS X        ', 2)
    # Entry 1: Finder Info (ID 9), offset 26 + 24 = 50, len 32
    # Entry 2: Resource Fork (ID 2), offset 82, len len(rsrc_fork)
    ad_entries = struct.pack('>III', 9, 50, 32)
    ad_entries += struct.pack('>III', 2, 82, len(rsrc_fork))
    # Finder Info: Type 'APPL', Creator 'FTAG', Flags 0x2000, rest zeros
    finfo = b'APPLFTAG\x20\x00' + bytes(22)
    
    ad_data = ad_header + ad_entries + finfo + rsrc_fork
    ad_path = os.path.join(script_dir, "._Ftaghn Ataxx")
    with open(ad_path, "wb") as f:
        f.write(ad_data)
    print(f"[OK] Created AppleDouble files: {raw_app_path} and {ad_path}")

    print("\n=== BUILD COMPLETE! ===")
    print("All distribution files have been successfully generated in the mac_se/ directory.")

if __name__ == "__main__":
    build_app()

