#ifndef FTAGHN_GBC_TILES_H
#define FTAGHN_GBC_TILES_H

#include <gb/gb.h>

#define TILE_COUNT 86

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

extern const unsigned char g_game_tiles[1376];

uint8_t GetFontTile(char c);

#endif /* FTAGHN_GBC_TILES_H */
