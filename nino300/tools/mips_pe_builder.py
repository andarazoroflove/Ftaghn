#!/usr/bin/env python3
import sys
import os
import struct
import subprocess

FUNCS = [
    "RegisterClassW",
    "CreateWindowExW",
    "DefWindowProcW",
    "ShowWindow",
    "UpdateWindow",
    "GetMessageW",
    "TranslateMessage",
    "DispatchMessageW",
    "PostQuitMessage",
    "DestroyWindow",
    "BeginPaint",
    "EndPaint",
    "SetTimer",
    "KillTimer",
    "Sleep",
    "GetModuleHandleW",
    "CreateCompatibleDC",
    "DeleteDC",
    "CreateDIBSection",
    "SelectObject",
    "DeleteObject",
    "BitBlt",
    "InvalidateRect",
    "LoadLibraryW",
    "GetProcAddress",
    "FreeLibrary",
    "CreateFileW",
    "CloseHandle",
    "GetTickCount",
    "GetModuleFileNameW",
    "ExitProcess"
]

def align(val, alignment):
    return (val + alignment - 1) & ~(alignment - 1)

def build_pe(elf_path, out_exe):
    # 1. Read ELF symbols using mipsel-linux-gnu-readelf or objdump
    res = subprocess.run(["mipsel-linux-gnu-readelf", "-s", elf_path], stdout=subprocess.PIPE, text=True, check=True)
    syms = {}
    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 8:
            name = parts[7]
            try:
                addr = int(parts[1], 16)
                syms[name] = addr
            except ValueError:
                pass

    if "WinMainCRTStartup" not in syms:
        raise RuntimeError("WinMainCRTStartup not found in ELF symbols!")
    if "s_iat_start" not in syms:
        raise RuntimeError("s_iat_start not found in ELF symbols!")

    entry_addr = syms["WinMainCRTStartup"]
    iat_addr = syms["s_iat_start"]

    # 2. Extract raw sections from ELF
    res_sec = subprocess.run(["mipsel-linux-gnu-readelf", "-S", elf_path], stdout=subprocess.PIPE, text=True, check=True)
    # Get .text and .data info
    text_addr, text_size, text_off = 0, 0, 0
    data_addr, data_size, data_off = 0, 0, 0
    bss_addr, bss_size = 0, 0

    lines = res_sec.stdout.splitlines()
    for i, line in enumerate(lines):
        if ".text" in line:
            parts = line[line.find(".text"):].split()
            text_addr = int(parts[2], 16)
            text_off = int(parts[3], 16)
            text_size = int(parts[4], 16)
        elif ".data" in line:
            parts = line[line.find(".data"):].split()
            data_addr = int(parts[2], 16)
            data_off = int(parts[3], 16)
            data_size = int(parts[4], 16)
        elif ".bss" in line:
            parts = line[line.find(".bss"):].split()
            bss_addr = int(parts[2], 16)
            bss_size = int(parts[4], 16)

    print(f"ELF .text: addr={hex(text_addr)}, size={hex(text_size)}, off={hex(text_off)}")
    print(f"ELF .data: addr={hex(data_addr)}, size={hex(data_size)}, off={hex(data_off)}")
    print(f"ELF .bss:  addr={hex(bss_addr)}, size={hex(bss_size)}")
    print(f"WinMainCRTStartup: {hex(entry_addr)}")
    print(f"s_iat_start:       {hex(iat_addr)}")

    with open(elf_path, "rb") as f:
        elf_bytes = f.read()

    text_bytes = bytearray(elf_bytes[text_off : text_off + text_size])
    data_bytes = bytearray(elf_bytes[data_off : data_off + data_size])

    image_base = 0x00010000
    sec_align = 0x1000
    file_align = 0x200

    text_rva = text_addr - image_base
    data_rva = data_addr - image_base

    # 3. Construct .rdata section containing PE Import Directory and Hint/Name tables
    # Layout of .rdata:
    #   [Import Directory (2 descriptors * 20 bytes = 40 bytes)]
    #   [OriginalFirstThunk / INT array (len(FUNCS)+1 * 4 bytes)]
    #   [DLL Name: "COREDLL.dll\0"]
    #   [Hint/Name table entries]
    rdata_rva = align(data_rva + len(data_bytes), sec_align)

    import_desc_size = 40  # 20 bytes for coredll, 20 bytes null terminator
    int_offset = import_desc_size
    int_size = (len(FUNCS) + 1) * 4
    dll_name_offset = int_offset + int_size
    dll_name = b"COREDLL.dll\x00"
    hint_name_offset = align(dll_name_offset + len(dll_name), 4)

    # Build hint/name entries and collect their RVAs
    hint_name_bytes = bytearray()
    func_rvas = []
    for func in FUNCS:
        entry_rva = rdata_rva + hint_name_offset + len(hint_name_bytes)
        func_rvas.append(entry_rva)
        # 2 bytes hint = 0, name ASCII, null term, align to 2
        entry = struct.pack("<H", 0) + func.encode("ascii") + b"\x00"
        if len(entry) % 2 != 0:
            entry += b"\x00"
        hint_name_bytes += entry

    # Build INT (OriginalFirstThunk)
    int_bytes = bytearray()
    for rva in func_rvas:
        int_bytes += struct.pack("<I", rva)
    int_bytes += struct.pack("<I", 0)  # Null terminator

    # Build Import Directory
    # OriginalFirstThunk (RVA to INT)
    # TimeDateStamp (0)
    # ForwarderChain (0)
    # Name (RVA to "COREDLL.dll")
    # FirstThunk (RVA to IAT in .data!)
    iat_rva = iat_addr - image_base
    orig_first_thunk_rva = rdata_rva + int_offset
    dll_name_rva = rdata_rva + dll_name_offset

    import_desc = struct.pack("<IIIII",
        orig_first_thunk_rva,
        0,
        0,
        dll_name_rva,
        iat_rva
    )
    import_desc += b"\x00" * 20  # Null descriptor

    rdata_bytes = bytearray(hint_name_offset)
    rdata_bytes[0:len(import_desc)] = import_desc
    rdata_bytes[int_offset : int_offset + len(int_bytes)] = int_bytes
    rdata_bytes[dll_name_offset : dll_name_offset + len(dll_name)] = dll_name
    rdata_bytes += hint_name_bytes

    # Populate initial IAT in data_bytes with matching func_rvas!
    iat_offset_in_data = iat_addr - data_addr
    for i, rva in enumerate(func_rvas):
        pos = iat_offset_in_data + (i * 4)
        data_bytes[pos : pos + 4] = struct.pack("<I", rva)

    # Pad sections to file_align
    header_size = 0x400

    text_raw_size = align(len(text_bytes), file_align)
    text_bytes += b"\x00" * (text_raw_size - len(text_bytes))

    data_raw_size = align(len(data_bytes), file_align)
    data_bytes += b"\x00" * (data_raw_size - len(data_bytes))

    rdata_raw_size = align(len(rdata_bytes), file_align)
    rdata_bytes += b"\x00" * (rdata_raw_size - len(rdata_bytes))

    text_raw_ptr = header_size
    data_raw_ptr = text_raw_ptr + text_raw_size
    rdata_raw_ptr = data_raw_ptr + data_raw_size

    total_image_size = align(rdata_rva + len(rdata_bytes) + bss_size, sec_align)

    # 4. Construct DOS Header & DOS Stub (0x80 bytes total)
    dos_header = bytearray(64)
    dos_header[0:2] = b"MZ"
    dos_header[0x3C:0x40] = struct.pack("<I", 0x80)  # e_lfanew points to PE header

    dos_stub = (
        b"\x0e\x1f\xba\x0e\x00\xb4\x09\xcd\x21\xb8\x01\x4c\xcd\x21"
        b"This program cannot be run in DOS mode.\r\r\n$\x00\x00\x00\x00\x00\x00\x00"
    )
    dos_stub += b"\x00" * (64 - len(dos_stub))

    # 5. PE File Header (20 bytes)
    # Machine: 0x0166 (IMAGE_FILE_MACHINE_R4000)
    # Sections: 3 (.text, .data, .rdata)
    pe_sig = b"PE\x00\x00"
    num_sections = 3
    opt_header_size = 224
    characteristics = 0x0102  # EXECUTABLE_IMAGE | 32BIT_MACHINE

    file_header = struct.pack("<HHIIIHH",
        0x0166,             # Machine (MIPS R4000 little-endian)
        num_sections,       # NumberOfSections
        0x5F000000,         # TimeDateStamp
        0,                  # PointerToSymbolTable
        0,                  # NumberOfSymbols
        opt_header_size,    # SizeOfOptionalHeader
        characteristics     # Characteristics
    )

    # 6. Optional Header (224 bytes)
    magic = 0x010B          # PE32
    major_linker = 6
    minor_linker = 0
    size_of_code = text_raw_size
    size_of_init_data = data_raw_size + rdata_raw_size
    size_of_uninit_data = align(bss_size, sec_align)
    entry_point_rva = entry_addr - image_base
    base_of_code = text_rva
    base_of_data = data_rva

    opt_standard = struct.pack("<HBBIIIIII",
        magic, major_linker, minor_linker,
        size_of_code, size_of_init_data, size_of_uninit_data,
        entry_point_rva, base_of_code, base_of_data
    )

    subsystem = 9          # IMAGE_SUBSYSTEM_WINDOWS_CE_GUI
    major_subsystem = 2    # Windows CE 2.0
    minor_subsystem = 0

    opt_windows = struct.pack("<IIIHHHHHHIIIIHHIIIIII",
        image_base,         # ImageBase
        sec_align,          # SectionAlignment (0x1000)
        file_align,         # FileAlignment (0x200)
        2, 0,               # MajorOperatingSystemVersion, Minor
        1, 0,               # MajorImageVersion, Minor
        major_subsystem, minor_subsystem, # MajorSubsystemVersion, Minor
        0,                  # Win32VersionValue
        total_image_size,   # SizeOfImage
        header_size,        # SizeOfHeaders
        0,                  # CheckSum
        subsystem,          # Subsystem (Windows CE GUI = 9)
        0,                  # DllCharacteristics
        0x200000, 0x10000,  # SizeOfStackReserve, Commit
        0x100000, 0x10000,  # SizeOfHeapReserve, Commit
        0,                  # LoaderFlags
        16                  # NumberOfRvaAndSizes
    )

    # Data Directories (16 entries * 8 bytes = 128 bytes)
    data_dirs = bytearray(16 * 8)

    # Directory 1: Import Table
    import_table_rva = rdata_rva
    import_table_size = import_desc_size
    data_dirs[1*8 : 1*8 + 8] = struct.pack("<II", import_table_rva, import_table_size)

    # Directory 12: IAT
    iat_size = len(FUNCS) * 4
    data_dirs[12*8 : 12*8 + 8] = struct.pack("<II", iat_rva, iat_size)

    opt_header = opt_standard + opt_windows + data_dirs
    assert len(opt_header) == opt_header_size

    # 7. Section Headers (3 entries * 40 bytes = 120 bytes)
    # .text section
    sec_text = struct.pack("<8sIIIIIIHHI",
        b".text\x00\x00\x00",
        len(text_bytes),
        text_rva,
        text_raw_size,
        text_raw_ptr,
        0, 0, 0, 0,
        0x60000020          # CNT_CODE | MEM_EXECUTE | MEM_READ
    )

    # .data section
    sec_data = struct.pack("<8sIIIIIIHHI",
        b".data\x00\x00\x00",
        len(data_bytes),
        data_rva,
        data_raw_size,
        data_raw_ptr,
        0, 0, 0, 0,
        0xC0000040          # CNT_INITIALIZED_DATA | MEM_READ | MEM_WRITE
    )

    # .rdata section
    sec_rdata = struct.pack("<8sIIIIIIHHI",
        b".rdata\x00\x00",
        len(rdata_bytes),
        rdata_rva,
        rdata_raw_size,
        rdata_raw_ptr,
        0, 0, 0, 0,
        0x40000040          # CNT_INITIALIZED_DATA | MEM_READ
    )

    headers = (
        dos_header +
        dos_stub +
        pe_sig +
        file_header +
        opt_header +
        sec_text +
        sec_data +
        sec_rdata
    )

    headers += b"\x00" * (header_size - len(headers))
    assert len(headers) == header_size

    # 8. Assemble final PE executable
    final_pe = headers + text_bytes + data_bytes + rdata_bytes

    with open(out_exe, "wb") as f:
        f.write(final_pe)

    print(f"Successfully generated Windows CE 2.0 MIPS executable: {out_exe} ({len(final_pe)} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: mips_pe_builder.py <input.elf> <output.exe>")
        sys.exit(1)
    build_pe(sys.argv[1], sys.argv[2])
