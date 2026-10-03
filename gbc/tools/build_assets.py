import os

def row_to_2bpp(row_pixels):
    b0 = 0
    b1 = 0
    for i in range(8):
        c = row_pixels[i]
        bit = 7 - i
        if c & 1: b0 |= (1 << bit)
        if c & 2: b1 |= (1 << bit)
    return [b0, b1]

def tile_to_2bpp(grid):
    data = []
    for row in grid:
        data.extend(row_to_2bpp(row))
    return data

def split_16x16_to_4_tiles(grid16):
    tl = [grid16[r][0:8] for r in range(0, 8)]
    tr = [grid16[r][8:16] for r in range(0, 8)]
    bl = [grid16[r][0:8] for r in range(8, 16)]
    br = [grid16[r][8:16] for r in range(8, 16)]
    return [tl, tr, bl, br]

font_def = {
    '0': ['01110','10001','10011','10101','11001','10001','01110'],
    '1': ['00100','01100','00100','00100','00100','00100','01110'],
    '2': ['01110','10001','00001','00010','00100','01000','11111'],
    '3': ['01110','10001','00001','00110','00001','10001','01110'],
    '4': ['00010','00110','01010','10010','11111','00010','00010'],
    '5': ['11111','10000','11110','00001','00001','10001','01110'],
    '6': ['01110','10001','10000','11110','10001','10001','01110'],
    '7': ['11111','00001','00010','00100','01000','01000','01000'],
    '8': ['01110','10001','10001','01110','10001','10001','01110'],
    '9': ['01110','10001','10001','01111','00001','10001','01110'],
    'A': ['01110','10001','10001','11111','10001','10001','10001'],
    'B': ['11110','10001','10001','11110','10001','10001','11110'],
    'C': ['01110','10001','10000','10000','10000','10001','01110'],
    'D': ['11110','10001','10001','10001','10001','10001','11110'],
    'E': ['11111','10000','10000','11110','10000','10000','11111'],
    'F': ['11111','10000','10000','11110','10000','10000','10000'],
    'G': ['01110','10001','10000','10111','10001','10001','01110'],
    'H': ['10001','10001','10001','11111','10001','10001','10001'],
    'I': ['01110','00100','00100','00100','00100','00100','01110'],
    'J': ['00001','00001','00001','00001','10001','10001','01110'],
    'K': ['10001','10010','10100','11000','10100','10010','10001'],
    'L': ['10000','10000','10000','10000','10000','10000','11111'],
    'M': ['10001','11011','10101','10101','10001','10001','10001'],
    'N': ['10001','11001','10101','10011','10001','10001','10001'],
    'O': ['01110','10001','10001','10001','10001','10001','01110'],
    'P': ['11110','10001','10001','11110','10000','10000','10000'],
    'Q': ['01110','10001','10001','10001','10101','10010','01101'],
    'R': ['11110','10001','10001','11110','10100','10010','10001'],
    'S': ['01111','10000','10000','01110','00001','00001','11110'],
    'T': ['11111','00100','00100','00100','00100','00100','00100'],
    'U': ['10001','10001','10001','10001','10001','10001','01110'],
    'V': ['10001','10001','10001','10001','01010','01010','00100'],
    'W': ['10001','10001','10001','10101','10101','11011','10001'],
    'X': ['10001','10001','01010','00100','01010','10001','10001'],
    'Y': ['10001','10001','01010','00100','00100','00100','00100'],
    'Z': ['11111','00001','00010','00100','01000','10000','11111'],
    ':': ['00000','00100','00100','00000','00100','00100','00000'],
    '!': ['00100','00100','00100','00100','00000','00100','00000'],
    '-': ['00000','00000','00000','11111','00000','00000','00000'],
    '+': ['00000','00100','00100','11111','00100','00100','00000'],
    '.': ['00000','00000','00000','00000','00000','01100','01100'],
    '/': ['00001','00010','00010','00100','01000','01000','10000'],
    '?': ['01110','10001','00001','00110','00100','00000','00100'],
    '(': ['00010','00100','01000','01000','01000','00100','00010'],
    ')': ['01000','00100','00010','00010','00010','00100','01000'],
    '<': ['00010','00100','01000','10000','01000','00100','00010'],
    '>': ['01000','00100','00010','00001','00010','00100','01000'],
    '*': ['00100','10101','01110','11111','01110','10101','00100']
}

char_order = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:!-+./?()<>*'

all_tiles = []
# Tile 0: Blank space
all_tiles.append([[0]*8 for _ in range(8)])

# Font tiles: 1 .. 48
for ch in char_order:
    grid = [[0]*8 for _ in range(8)]
    rows = font_def[ch]
    for r, row_str in enumerate(rows):
        for c, bit in enumerate(row_str):
            if bit == '1': grid[r][c+1] = 3
    all_tiles.append(grid)

# Tile 49: Dark filler / background pattern
filler = [
    [1,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,1,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,1,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,1],
    [0,0,0,0,0,0,0,0]
]
all_tiles.append(filler) # index 49

# 16x16 Elements:
# 1. Empty Cell (Stone Slab with sunken bevel) -> tiles 50..53
cell_16 = [[0]*16 for _ in range(16)]
for r in range(16):
    for c in range(16):
        if r == 0 or c == 0: cell_16[r][c] = 1 # dark top/left
        elif r == 15 or c == 15: cell_16[r][c] = 2 # light bottom/right
        else: cell_16[r][c] = 0 # dark slab center
cell_16[1][1] = 1; cell_16[1][14] = 1; cell_16[14][1] = 2; cell_16[14][14] = 2
for t in split_16x16_to_4_tiles(cell_16): all_tiles.append(t) # 50, 51, 52, 53

# Helper for 3D sphere
def make_sphere(color_hi, color_mid, color_lo):
    sp = [[0]*16 for _ in range(16)]
    for r in range(16):
        for c in range(16):
            if r == 0 or c == 0: sp[r][c] = 1
            elif r == 15 or c == 15: sp[r][c] = 2

    for r in range(2, 14):
        for c in range(2, 14):
            dy = r - 7.5
            dx = c - 7.5
            dist_sq = dx*dx + dy*dy
            if dist_sq <= 34.0:
                light = (dx * -0.7) + (dy * -0.7)
                if light > 2.2:
                    sp[r][c] = color_hi
                elif light > -1.2:
                    sp[r][c] = color_mid
                else:
                    sp[r][c] = color_lo
    return sp

# 2. Red Piece -> tiles 54..57
red_16 = make_sphere(3, 2, 1)
for t in split_16x16_to_4_tiles(red_16): all_tiles.append(t) # 54, 55, 56, 57

# 3. Blue Piece -> tiles 58..61
blue_16 = make_sphere(3, 2, 1)
for t in split_16x16_to_4_tiles(blue_16): all_tiles.append(t) # 58, 59, 60, 61

# 4. Obstacle Monolith -> tiles 62..65
obs_16 = [[0]*16 for _ in range(16)]
for r in range(16):
    for c in range(16):
        if r in (0, 15) or c in (0, 15): obs_16[r][c] = 1
        elif r in (1, 2, 13, 14) or c in (1, 2, 13, 14): obs_16[r][c] = 2
        else: obs_16[r][c] = 3
for r in range(6, 10):
    for c in range(6, 10):
        obs_16[r][c] = 1
obs_16[7][7] = 3; obs_16[8][8] = 3
for t in split_16x16_to_4_tiles(obs_16): all_tiles.append(t) # 62, 63, 64, 65

# 5. Ice Monolith (Perm Obstacle) -> tiles 66..69
ice_16 = [[0]*16 for _ in range(16)]
for r in range(16):
    for c in range(16):
        if (r + c) % 4 == 0 or (r - c) % 4 == 0:
            ice_16[r][c] = 3
        elif (r + c) % 2 == 0:
            ice_16[r][c] = 2
        else:
            ice_16[r][c] = 1
for t in split_16x16_to_4_tiles(ice_16): all_tiles.append(t) # 66, 67, 68, 69

# 6. Valid Clone Move -> tiles 70..73 (Spore dot)
clone_16 = [row[:] for row in cell_16]
for r in range(6, 10):
    for c in range(6, 10):
        clone_16[r][c] = 2
clone_16[7][7] = 3; clone_16[7][8] = 3; clone_16[8][7] = 3; clone_16[8][8] = 3
for t in split_16x16_to_4_tiles(clone_16): all_tiles.append(t) # 70, 71, 72, 73

# 7. Valid Leap Move -> tiles 74..77 (Diamond star)
leap_16 = [row[:] for row in cell_16]
for r in range(5, 11):
    for c in range(5, 11):
        if abs(r - 7.5) + abs(c - 7.5) <= 3.5:
            leap_16[r][c] = 2
leap_16[7][7] = 3; leap_16[7][8] = 3; leap_16[8][7] = 3; leap_16[8][8] = 3
for t in split_16x16_to_4_tiles(leap_16): all_tiles.append(t) # 74, 75, 76, 77

# 8. Border / Decorative Frame -> tiles 78..81
h_line = [[0]*8 for _ in range(8)]; h_line[3] = [2]*8; h_line[4] = [1]*8
v_line = [[0]*8 for _ in range(8)]
for r in range(8): v_line[r][3] = 2; v_line[r][4] = 1
corner = [[0]*8 for _ in range(8)]
corner[3] = [2]*8; corner[4] = [1]*8
for r in range(8): corner[r][3] = 2; corner[r][4] = 1
solid = [[1]*8 for _ in range(8)]
all_tiles.extend([h_line, v_line, corner, solid]) # 78, 79, 80, 81

# 9. Cursor Sprites (8x8 x 4 = 16x16) -> tiles 82..85 (stored in sprite pattern memory)
cur_tl = [[0]*8 for _ in range(8)]
for c in range(6): cur_tl[0][c] = 3; cur_tl[1][c] = 2
for r in range(6): cur_tl[r][0] = 3; cur_tl[r][1] = 2

cur_tr = [[0]*8 for _ in range(8)]
for c in range(2, 8): cur_tr[0][c] = 3; cur_tr[1][c] = 2
for r in range(6): cur_tr[r][6] = 2; cur_tr[r][7] = 3

cur_bl = [[0]*8 for _ in range(8)]
for c in range(6): cur_bl[6][c] = 2; cur_bl[7][c] = 3
for r in range(2, 8): cur_bl[r][0] = 3; cur_bl[r][1] = 2

cur_br = [[0]*8 for _ in range(8)]
for c in range(2, 8): cur_br[6][c] = 2; cur_br[7][c] = 3
for r in range(2, 8): cur_br[r][6] = 2; cur_br[r][7] = 3

all_tiles.extend([cur_tl, cur_tr, cur_bl, cur_br]) # 82, 83, 84, 85

print('Total tiles compiled:', len(all_tiles))

os.makedirs('gbc/src', exist_ok=True)
with open('gbc/src/tiles.h', 'w') as fh:
    fh.write(f'''#ifndef FTAGHN_GBC_TILES_H
#define FTAGHN_GBC_TILES_H

#include <gb/gb.h>

#define TILE_COUNT {len(all_tiles)}

#define TILE_BLANK       0
#define TILE_FONT_START  1
#define TILE_BG_FILLER   49

#define TILE_CELL_TL     50
#define TILE_CELL_TR     51
#define TILE_CELL_BL     52
#define TILE_CELL_BR     53

#define TILE_RED_TL      54
#define TILE_RED_TR      55
#define TILE_RED_BL      56
#define TILE_RED_BR      57

#define TILE_BLUE_TL     58
#define TILE_BLUE_TR     59
#define TILE_BLUE_BL     60
#define TILE_BLUE_BR     61

#define TILE_OBS_TL      62
#define TILE_OBS_TR      63
#define TILE_OBS_BL      64
#define TILE_OBS_BR      65

#define TILE_ICE_TL      66
#define TILE_ICE_TR      67
#define TILE_ICE_BL      68
#define TILE_ICE_BR      69

#define TILE_CLONE_TL    70
#define TILE_CLONE_TR    71
#define TILE_CLONE_BL    72
#define TILE_CLONE_BR    73

#define TILE_LEAP_TL     74
#define TILE_LEAP_TR     75
#define TILE_LEAP_BL     76
#define TILE_LEAP_BR     77

#define TILE_BORDER_H    78
#define TILE_BORDER_V    79
#define TILE_BORDER_CR   80
#define TILE_FRAME_SOLID 81

#define TILE_CUR_TL      82
#define TILE_CUR_TR      83
#define TILE_CUR_BL      84
#define TILE_CUR_BR      85

extern const unsigned char g_game_tiles[{len(all_tiles) * 16}];

uint8_t GetFontTile(char c);

#endif /* FTAGHN_GBC_TILES_H */
''')

with open('gbc/src/tiles.c', 'w') as fc:
    fc.write(f'''#include "tiles.h"

const unsigned char g_game_tiles[{len(all_tiles) * 16}] = {{\n''')
    for idx, t in enumerate(all_tiles):
        b = tile_to_2bpp(t)
        hex_str = ', '.join(f'0x{val:02X}' for val in b)
        fc.write(f'    /* Tile {idx:2d} */ {hex_str},\n')
    fc.write('''};\n
uint8_t GetFontTile(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(1 + (c - '0'));
    if (c >= 'A' && c <= 'Z') return (uint8_t)(11 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (uint8_t)(11 + (c - 'a'));
    switch(c) {
        case ':': return 37;
        case '!': return 38;
        case '-': return 39;
        case '+': return 40;
        case '.': return 41;
        case '/': return 42;
        case '?': return 43;
        case '(': return 44;
        case ')': return 45;
        case '<': return 46;
        case '>': return 47;
        case '*': return 48;
        default:  return 0; /* Space */
    }
}
''')
print('Finished building tiles!')

