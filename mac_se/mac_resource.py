"""
Classic Macintosh Resource Fork, MacBinary II, and BinHex 4.0 packager.
Compatible with System 7.0+ and 68000 Macintosh hardware.
"""

import struct
import time

MAC_EPOCH_DIFF = 2082844800 # Seconds between 1904-01-01 and 1970-01-01

def get_mac_timestamp():
    return int(time.time()) + MAC_EPOCH_DIFF

class Resource:
    def __init__(self, res_type, res_id, data, name="", attrs=0):
        if isinstance(res_type, str):
            res_type = res_type.encode('latin1')
        if len(res_type) != 4:
            raise ValueError(f"Resource type must be 4 bytes, got {res_type}")
        self.type = res_type
        self.id = int(res_id)
        self.data = bytes(data)
        self.name = name
        self.attrs = attrs

def build_resource_fork(resources):
    """
    Build a classic Macintosh Resource Fork byte stream.
    resources: list of Resource objects.
    """
    # 1. Prepare resource data section
    # Starts at offset 256
    data_section = bytearray()
    res_data_offsets = [] # offset from start of data section
    
    for r in resources:
        offset = len(data_section)
        res_data_offsets.append(offset)
        data_section += struct.pack('>I', len(r.data))
        data_section += r.data

    data_len = len(data_section)
    data_offset = 256
    map_offset = data_offset + data_len

    # 2. Build Resource Map
    # Group by resource type in original order
    types_map = {}
    for i, r in enumerate(resources):
        if r.type not in types_map:
            types_map[r.type] = []
        types_map[r.type].append((i, r))

    # Name list
    name_list = bytearray()
    res_name_offsets = []
    for r in resources:
        if r.name:
            name_bytes = r.name.encode('mac_roman', errors='replace')[:255]
            offset = len(name_list)
            res_name_offsets.append(offset)
            name_list.append(len(name_bytes))
            name_list += name_bytes
        else:
            res_name_offsets.append(0xFFFF)

    # Calculate offsets within the map
    # Map header: 28 bytes
    # Type list header: 2 bytes (num_types - 1)
    # Type list entries: len(types) * 8 bytes
    num_types = len(types_map)
    type_list_offset = 28
    ref_list_start_offset = type_list_offset + 2 + num_types * 8

    # Reference list offsets for each type
    current_ref_offset = ref_list_start_offset
    type_entries = []
    ref_lists = []

    for r_type, items in types_map.items():
        type_entries.append((r_type, len(items) - 1, current_ref_offset - type_list_offset))
        ref_entries = bytearray()
        for idx, r in items:
            d_off = res_data_offsets[idx]
            n_off = res_name_offsets[idx]
            # 2 bytes ID, 2 bytes name offset, 1 byte attrs, 3 bytes data offset, 4 bytes reserved
            ref_entries += struct.pack('>hHB', r.id, n_off, r.attrs)
            ref_entries += bytes([(d_off >> 16) & 0xFF, (d_off >> 8) & 0xFF, d_off & 0xFF])
            ref_entries += struct.pack('>I', 0) # reserved
        ref_lists.append(ref_entries)
        current_ref_offset += len(ref_entries)

    name_list_offset = current_ref_offset
    map_total_len = name_list_offset + len(name_list)

    # Assemble Map
    map_data = bytearray()
    # Map header: copy of resource header (16 bytes)
    map_data += struct.pack('>IIII', data_offset, map_offset, data_len, map_total_len)
    map_data += struct.pack('>IHH', 0, 0, 0) # Next map handle (0), File ref num (0), Fork attrs (0)
    map_data += struct.pack('>HH', type_list_offset, name_list_offset)

    # Type list
    map_data += struct.pack('>H', num_types - 1)
    for r_type, count_minus_1, ref_off in type_entries:
        map_data += r_type + struct.pack('>HH', count_minus_1, ref_off)

    # Reference lists
    for rl in ref_lists:
        map_data += rl

    # Name list
    map_data += name_list

    assert len(map_data) == map_total_len

    # Assemble complete Resource Fork
    fork = bytearray()
    # Resource header (16 bytes)
    fork += struct.pack('>IIII', data_offset, map_offset, data_len, map_total_len)
    # 112 bytes system reserved
    fork += bytes(112)
    # 128 bytes application reserved
    fork += bytes(128)
    assert len(fork) == 256
    # Data section
    fork += data_section
    assert len(fork) == map_offset
    # Map section
    fork += map_data

    return bytes(fork)


def calc_crc_ccitt(data):
    """Calculate CRC-CCITT for MacBinary header."""
    crc = 0
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def make_macbinary(filename, file_type, file_creator, data_fork=b'', rsrc_fork=b'', finder_flags=0):
    """
    Package data fork and resource fork into MacBinary II format (.bin).
    """
    if isinstance(file_type, str):
        file_type = file_type.encode('latin1')
    if isinstance(file_creator, str):
        file_creator = file_creator.encode('latin1')
    
    fname_bytes = filename.encode('mac_roman', errors='replace')[:63]
    
    header = bytearray(128)
    header[0] = 0 # Old version zero
    header[1] = len(fname_bytes)
    header[2:2+len(fname_bytes)] = fname_bytes
    header[65:69] = file_type[:4]
    header[69:73] = file_creator[:4]
    header[73] = (finder_flags >> 8) & 0xFF # High byte of Finder flags
    # Position (75..78) = 0
    # Window ID (79..80) = 0
    # Protected (81) = 0
    struct.pack_into('>I', header, 83, len(data_fork))
    struct.pack_into('>I', header, 87, len(rsrc_fork))
    now = get_mac_timestamp()
    struct.pack_into('>I', header, 91, now) # Creation date
    struct.pack_into('>I', header, 95, now) # Mod date
    header[101] = finder_flags & 0xFF # Low byte of Finder flags
    header[122] = 129 # MacBinary II version
    header[123] = 129 # Minimum MacBinary II version
    
    crc = calc_crc_ccitt(header[:124])
    struct.pack_into('>H', header, 124, crc)
    
    # Pad forks to 128-byte boundaries
    pad_data = ((len(data_fork) + 127) // 128) * 128 - len(data_fork)
    pad_rsrc = ((len(rsrc_fork) + 127) // 128) * 128 - len(rsrc_fork)
    
    return bytes(header) + data_fork + bytes(pad_data) + rsrc_fork + bytes(pad_rsrc)


def make_binhex(filename, file_type, file_creator, data_fork=b'', rsrc_fork=b''):
    """
    Package into BinHex 4.0 (.hqx) format.
    """
    if isinstance(file_type, str):
        file_type = file_type.encode('latin1')
    if isinstance(file_creator, str):
        file_creator = file_creator.encode('latin1')
        
    fname_bytes = filename.encode('mac_roman', errors='replace')[:63]
    
    # BinHex table
    BH_CHARS = b"!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr"
    
    def binhex_crc(data, crc=0):
        for b in data:
            temp = ((crc >> 8) & 0xFF) | ((crc << 8) & 0xFF00)
            temp ^= b
            temp ^= (temp & 0xFF) >> 4
            temp ^= (temp << 12) & 0xFFFF
            temp ^= ((temp & 0xFF) << 5) & 0xFFFF
            crc = temp & 0xFFFF
        return crc

    # Build raw stream
    hdr = bytearray()
    hdr.append(len(fname_bytes))
    hdr += fname_bytes
    hdr.append(0) # Version
    hdr += file_type[:4]
    hdr += file_creator[:4]
    hdr += struct.pack('>H', 0) # Flags
    hdr += struct.pack('>I', len(data_fork))
    hdr += struct.pack('>I', len(rsrc_fork))
    crc_hdr = binhex_crc(hdr)
    hdr += struct.pack('>H', crc_hdr)

    data_stream = bytearray(hdr)
    data_stream += data_fork
    crc_data = binhex_crc(data_fork)
    data_stream += struct.pack('>H', crc_data)
    
    data_stream += rsrc_fork
    crc_rsrc = binhex_crc(rsrc_fork)
    data_stream += struct.pack('>H', crc_rsrc)

    # Run length encoding (0x90 is escape)
    rle = bytearray()
    i = 0
    n = len(data_stream)
    while i < n:
        byte = data_stream[i]
        count = 1
        while i + count < n and data_stream[i + count] == byte and count < 255:
            count += 1
        if byte == 0x90:
            rle += bytes([0x90, 0x00])
            i += 1
        elif count >= 4:
            rle += bytes([byte, 0x90, count])
            i += count
        else:
            rle.append(byte)
            i += 1

    # 6-bit packing
    bit_buf = 0
    bit_count = 0
    packed = bytearray()
    for b in rle:
        bit_buf = (bit_buf << 8) | b
        bit_count += 8
        while bit_count >= 6:
            bit_count -= 6
            val = (bit_buf >> bit_count) & 0x3F
            packed.append(BH_CHARS[val])
            
    if bit_count > 0:
        val = (bit_buf << (6 - bit_count)) & 0x3F
        packed.append(BH_CHARS[val])

    # Format into lines
    lines = ["(This file must be converted with BinHex 4.0)", ":"]
    cur_line = bytearray()
    for ch in packed:
        cur_line.append(ch)
        if len(cur_line) == 64:
            lines.append(cur_line.decode('ascii'))
            cur_line = bytearray()
    if cur_line:
        lines.append(cur_line.decode('ascii') + ":")
    else:
        lines[-1] += ":"
        
    return "\r\n".join(lines) + "\r\n"

