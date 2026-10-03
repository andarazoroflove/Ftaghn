#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include <stdio.h>
#include <rand.h>
#include "tiles.h"
#include "sound.h"
#include "game.h"

/* CGB Palettes */
/* Palette 0: HUD / Text / Frame */
static const uint16_t s_pal_hud[4] = {
    RGB(1, 1, 2),    /* Dark Void */
    RGB(12, 12, 15), /* Muted Stone */
    RGB(28, 25, 10), /* Pale Gold */
    RGB(31, 31, 31)  /* Bone White */
};

/* Palette 1: Empty Stone Cell */
static const uint16_t s_pal_cell[4] = {
    RGB(2, 2, 4),    /* Dark Void */
    RGB(4, 3, 7),    /* Shadow Bevel */
    RGB(8, 7, 12),   /* Highlight Bevel */
    RGB(13, 11, 18)  /* Rim Highlight */
};

/* Palette 2: Red Horror */
static const uint16_t s_pal_red[4] = {
    RGB(2, 2, 4),    /* Dark Background */
    RGB(12, 2, 3),   /* Crimson Shadow */
    RGB(27, 3, 5),   /* Blood Red */
    RGB(31, 20, 22)  /* Eldritch Specular Glow */
};

/* Palette 3: Blue Horror */
static const uint16_t s_pal_blue[4] = {
    RGB(2, 2, 4),    /* Dark Background */
    RGB(2, 5, 13),   /* Abyssal Shadow */
    RGB(4, 14, 28),  /* Cosmic Azure */
    RGB(20, 26, 31)  /* Astral Specular Glow */
};

/* Palette 4: Obstacle Monolith */
static const uint16_t s_pal_obs[4] = {
    RGB(2, 2, 4),    /* Dark Background */
    RGB(6, 6, 7),    /* Obsidian Shadow */
    RGB(15, 16, 18), /* Granite Grey */
    RGB(25, 27, 30)  /* Chiseled Rune Line */
};

/* Palette 5: Valid Move Spore/Diamond (Green) */
static const uint16_t s_pal_move[4] = {
    RGB(2, 2, 4),    /* Dark Background */
    RGB(2, 9, 3),    /* Moss Shadow */
    RGB(5, 26, 7),   /* Slime Green */
    RGB(18, 31, 22)  /* Neon Spore Glow */
};

/* Sprite Palette 0: Arcane Gold Cursor */
static const uint16_t s_pal_cursor[4] = {
    RGB(0, 0, 0),    /* Transparent */
    RGB(20, 16, 2),  /* Deep Amber */
    RGB(31, 26, 4),  /* Radiant Arcane Gold */
    RGB(31, 31, 31)  /* White Rim */
};

static uint8_t s_cursor_r = 0;
static uint8_t s_cursor_c = 0;
static uint8_t s_prev_keys = 0;
static uint8_t s_anim_counter = 0;
static uint8_t s_ai_delay_frames = 0;

static void SetupPalettes(void) {
    set_bkg_palette(0, 1, s_pal_hud);
    set_bkg_palette(1, 1, s_pal_cell);
    set_bkg_palette(2, 1, s_pal_red);
    set_bkg_palette(3, 1, s_pal_blue);
    set_bkg_palette(4, 1, s_pal_obs);
    set_bkg_palette(5, 1, s_pal_move);

    set_sprite_palette(0, 1, s_pal_cursor);
}

static void DrawText(uint8_t x, uint8_t y, const char *str) {
    uint8_t tile_buf[20];
    uint8_t attr_buf[20];
    uint8_t len = 0;

    while (str[len] && len < 20 && (x + len) < 20) {
        tile_buf[len] = GetFontTile(str[len]);
        attr_buf[len] = 0; /* Palette 0 for text */
        len++;
    }

    VBK_REG = 1;
    set_bkg_tiles(x, y, len, 1, attr_buf);
    VBK_REG = 0;
    set_bkg_tiles(x, y, len, 1, tile_buf);
}

static void DrawHUD(void) {
    char buf[22];

    /* Line 0: Scores */
    sprintf(buf, "R:%02d  FTAGHN  B:%02d", g_game.score_red, g_game.score_blue);
    DrawText(0, 0, buf);

    /* Line 1: Timer & Difficulty */
    if (g_game.difficulty == DIFF_EASY) {
        sprintf(buf, "EASY   TIME %d:%02d", g_game.game_timer_sec / 60, g_game.game_timer_sec % 60);
    } else if (g_game.difficulty == DIFF_MEDIUM) {
        sprintf(buf, "MED    TIME %d:%02d", g_game.game_timer_sec / 60, g_game.game_timer_sec % 60);
    } else {
        sprintf(buf, "HARD   TIME %d:%02d", g_game.game_timer_sec / 60, g_game.game_timer_sec % 60);
    }
    DrawText(0, 1, buf);
}

static void DrawCell(uint8_t r, uint8_t c) {
    uint8_t sx = 3 + c * 2;
    uint8_t sy = 2 + r * 2;
    uint8_t tiles[4];
    uint8_t attrs[4];
    uint8_t pal = 1;
    uint8_t cell = g_game.board[r][c];

    if (cell == CELL_RED) {
        tiles[0] = TILE_RED_TL; tiles[1] = TILE_RED_TR;
        tiles[2] = TILE_RED_BL; tiles[3] = TILE_RED_BR;
        pal = 2;
    } else if (cell == CELL_BLUE) {
        tiles[0] = TILE_BLUE_TL; tiles[1] = TILE_BLUE_TR;
        tiles[2] = TILE_BLUE_BL; tiles[3] = TILE_BLUE_BR;
        pal = 3;
    } else if (cell == CELL_OBSTACLE) {
        tiles[0] = TILE_OBS_TL; tiles[1] = TILE_OBS_TR;
        tiles[2] = TILE_OBS_BL; tiles[3] = TILE_OBS_BR;
        pal = 4;
    } else {
        /* Empty Cell */
        uint8_t mtype = g_game.has_selected ? g_game.valid_moves_map[r][c] : MOVE_NONE;
        if (mtype == MOVE_CLONE) {
            tiles[0] = TILE_CLONE_TL; tiles[1] = TILE_CLONE_TR;
            tiles[2] = TILE_CLONE_BL; tiles[3] = TILE_CLONE_BR;
            pal = 5;
        } else if (mtype == MOVE_LEAP) {
            tiles[0] = TILE_LEAP_TL; tiles[1] = TILE_LEAP_TR;
            tiles[2] = TILE_LEAP_BL; tiles[3] = TILE_LEAP_BR;
            pal = 5;
        } else {
            tiles[0] = TILE_CELL_TL; tiles[1] = TILE_CELL_TR;
            tiles[2] = TILE_CELL_BL; tiles[3] = TILE_CELL_BR;
            pal = 1;
        }
    }

    attrs[0] = pal; attrs[1] = pal;
    attrs[2] = pal; attrs[3] = pal;

    VBK_REG = 1;
    set_bkg_tiles(sx, sy, 2, 2, attrs);
    VBK_REG = 0;
    set_bkg_tiles(sx, sy, 2, 2, tiles);
}

static void DrawBoard(void) {
    uint8_t r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            DrawCell(r, c);
        }
    }
}

static void DrawScreenBackground(void) {
    uint8_t r, c;
    uint8_t filler_row[20];
    uint8_t attr_row[20];

    for (c = 0; c < 20; c++) {
        filler_row[c] = TILE_BG_FILLER;
        attr_row[c] = 0;
    }

    VBK_REG = 1;
    for (r = 0; r < 18; r++) set_bkg_tiles(0, r, 20, 1, attr_row);
    VBK_REG = 0;
    for (r = 0; r < 18; r++) set_bkg_tiles(0, r, 20, 1, filler_row);
}

static void DrawStatus(void) {
    char line2[21];

    /* Clear status lines */
    DrawText(0, 16, "                    ");
    DrawText(0, 17, "                    ");

    if (g_game.game_state == STATE_GAME_OVER) {
        DrawText(1, 16, g_game.status_msg);
        DrawText(1, 17, "PRESS START TO RESET");
    } else {
        DrawText(1, 16, g_game.status_msg);
        if (g_game.has_selected) {
            sprintf(line2, "[A]MOVE   [B]CANCEL");
        } else {
            sprintf(line2, "[A]PICK [SEL]DIFF");
        }
        DrawText(1, 17, line2);
    }
}

static void UpdateCursorSprites(void) {
    uint8_t px = 24 + s_cursor_c * 16;
    uint8_t py = 16 + s_cursor_r * 16;
    uint8_t sp_x = px + 8;
    uint8_t sp_y = py + 16;

    /* Subtle breathing pulsation on cursor */
    uint8_t pulse = (s_anim_counter & 0x10) ? 0 : 0;

    move_sprite(0, sp_x - pulse, sp_y - pulse);
    move_sprite(1, sp_x + 8 + pulse, sp_y - pulse);
    move_sprite(2, sp_x - pulse, sp_y + 8 + pulse);
    move_sprite(3, sp_x + 8 + pulse, sp_y + 8 + pulse);
}

static void HideCursorSprites(void) {
    move_sprite(0, 0, 0);
    move_sprite(1, 0, 0);
    move_sprite(2, 0, 0);
    move_sprite(3, 0, 0);
}

static void InitCursorSprites(void) {
    set_sprite_tile(0, TILE_CUR_TL);
    set_sprite_tile(1, TILE_CUR_TR);
    set_sprite_tile(2, TILE_CUR_BL);
    set_sprite_tile(3, TILE_CUR_BR);

    set_sprite_prop(0, 0);
    set_sprite_prop(1, 0);
    set_sprite_prop(2, 0);
    set_sprite_prop(3, 0);
}

static void RunTitleScreen(void) {
    uint8_t blink = 0;
    DrawScreenBackground();
    HideCursorSprites();

    DrawText(5, 3, "* FTAGHN *");
    DrawText(1, 5, "COSMIC HORROR ATAXX");
    DrawText(1, 8, "RED: YOU");
    DrawText(1, 9, "BLUE: ELDER GOD AI");

    DrawText(1, 12, "[SELECT] DIFFICULTY");
    DrawText(1, 15, "PRESS START TO PLAY");

    while (1) {
        uint8_t keys = joypad();
        uint8_t pressed = keys & ~s_prev_keys;
        s_prev_keys = keys;

        if (pressed & J_START) {
            initrand(((uint16_t)DIV_REG << 8) | (uint16_t)blink);
            Sound_PlaySelect();
            break;
        }

        if (pressed & J_SELECT) {
            g_game.difficulty = (g_game.difficulty + 1) % 3;
            Sound_PlaySelect();
        }

        if (g_game.difficulty == DIFF_EASY) {
            DrawText(2, 13, "MODE: SIMPLE (EASY) ");
        } else if (g_game.difficulty == DIFF_MEDIUM) {
            DrawText(2, 13, "MODE: MORTAL (MED)  ");
        } else {
            DrawText(2, 13, "MODE: GODLIKE (HARD)");
        }

        blink++;
        if ((blink & 0x20) == 0) {
            DrawText(1, 15, "PRESS START TO PLAY");
        } else {
            DrawText(1, 15, "                   ");
        }

        wait_vbl_done();
    }
}

void main(void) {
    cpu_fast(); /* Enable CGB Double-Speed Mode (8.38 MHz) */

    DISPLAY_OFF;

    /* Load all game tiles into VRAM */
    set_bkg_data(0, TILE_COUNT, g_game_tiles);
    set_sprite_data(0, TILE_COUNT, g_game_tiles);

    SetupPalettes();
    Sound_Init();
    InitCursorSprites();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    RunTitleScreen();

    Game_Init();
    DrawScreenBackground();
    DrawHUD();
    DrawBoard();
    DrawStatus();
    UpdateCursorSprites();

    while (1) {
        uint8_t keys = joypad();
        uint8_t pressed = keys & ~s_prev_keys;
        s_prev_keys = keys;

        s_anim_counter++;

        /* 1. Handle Game Over state */
        if (g_game.game_state == STATE_GAME_OVER) {
            HideCursorSprites();
            DrawHUD();
            DrawStatus();
            if (pressed & J_START) {
                Sound_PlaySelect();
                Game_Reset();
                DrawScreenBackground();
                DrawHUD();
                DrawBoard();
                DrawStatus();
                UpdateCursorSprites();
            }
            wait_vbl_done();
            continue;
        }

        /* 2. Handle AI Turn with brief delay for visual clarity */
        if (g_game.current_player == CELL_BLUE) {
            HideCursorSprites();
            s_ai_delay_frames++;
            if (s_ai_delay_frames >= 20) { /* 1/3 second delay */
                GameMove ai_move;
                uint8_t found = Game_MakeAIMove(&ai_move);
                s_ai_delay_frames = 0;

                if (found) {
                    uint8_t caps = Game_MakeMove(ai_move.from_r, ai_move.from_c, ai_move.to_r, ai_move.to_c, ai_move.type);
                    if (ai_move.type == MOVE_CLONE) Sound_PlayClone();
                    else Sound_PlayLeap();

                    if (caps > 0) Sound_PlayCapture(caps);

                    if (caps >= 2) sprintf(g_game.status_msg, "AI CAPTURED %d!", caps);
                    else sprintf(g_game.status_msg, "AI MOVED");

                    g_game.current_player = CELL_RED;
                }

                Game_CheckGameOver();
                if (g_game.game_state == STATE_GAME_OVER) {
                    if (g_game.winner == CELL_BLUE) Sound_PlayDefeat();
                    else if (g_game.winner == CELL_RED) Sound_PlayVictory();
                }

                DrawHUD();
                DrawBoard();
                DrawStatus();
                UpdateCursorSprites();
            }
            wait_vbl_done();
            continue;
        }

        /* 3. Player (RED) Controls */
        UpdateCursorSprites();

        /* D-Pad Cursor movement */
        if (pressed & J_UP) {
            s_cursor_r = (s_cursor_r > 0) ? s_cursor_r - 1 : BOARD_SIZE - 1;
            Sound_PlaySelect();
        }
        if (pressed & J_DOWN) {
            s_cursor_r = (s_cursor_r < BOARD_SIZE - 1) ? s_cursor_r + 1 : 0;
            Sound_PlaySelect();
        }
        if (pressed & J_LEFT) {
            s_cursor_c = (s_cursor_c > 0) ? s_cursor_c - 1 : BOARD_SIZE - 1;
            Sound_PlaySelect();
        }
        if (pressed & J_RIGHT) {
            s_cursor_c = (s_cursor_c < BOARD_SIZE - 1) ? s_cursor_c + 1 : 0;
            Sound_PlaySelect();
        }

        /* A Button: Select or Place */
        if (pressed & J_A) {
            if (!g_game.has_selected) {
                /* Selecting a friendly piece */
                if (g_game.board[s_cursor_r][s_cursor_c] == CELL_RED) {
                    g_game.has_selected = 1;
                    g_game.selected_r = s_cursor_r;
                    g_game.selected_c = s_cursor_c;
                    Game_ComputeValidMovesForPiece(s_cursor_r, s_cursor_c);
                    Sound_PlaySelect();
                    strcpy(g_game.status_msg, "SELECT TARGET");
                    DrawBoard();
                    DrawStatus();
                }
            } else {
                /* Target selection */
                if (s_cursor_r == g_game.selected_r && s_cursor_c == g_game.selected_c) {
                    /* Deselect */
                    g_game.has_selected = 0;
                    Game_ClearValidMovesMap();
                    strcpy(g_game.status_msg, "YOUR TURN (RED)");
                    DrawBoard();
                    DrawStatus();
                } else if (g_game.board[s_cursor_r][s_cursor_c] == CELL_RED) {
                    /* Switch to another friendly piece */
                    g_game.selected_r = s_cursor_r;
                    g_game.selected_c = s_cursor_c;
                    Game_ComputeValidMovesForPiece(s_cursor_r, s_cursor_c);
                    Sound_PlaySelect();
                    DrawBoard();
                } else {
                    /* Check if target is a valid move */
                    uint8_t mtype = g_game.valid_moves_map[s_cursor_r][s_cursor_c];
                    if (mtype != MOVE_NONE) {
                        uint8_t caps = Game_MakeMove(g_game.selected_r, g_game.selected_c, s_cursor_r, s_cursor_c, mtype);
                        if (mtype == MOVE_CLONE) Sound_PlayClone();
                        else Sound_PlayLeap();

                        if (caps > 0) Sound_PlayCapture(caps);

                        if (caps >= 2) sprintf(g_game.status_msg, "CORRUPTED %d!", caps);
                        else sprintf(g_game.status_msg, "YOU EXPANDED");

                        g_game.current_player = CELL_BLUE;
                        s_ai_delay_frames = 0;

                        Game_CheckGameOver();
                        if (g_game.game_state == STATE_GAME_OVER) {
                            if (g_game.winner == CELL_RED) Sound_PlayVictory();
                            else Sound_PlayDefeat();
                        }

                        DrawHUD();
                        DrawBoard();
                        DrawStatus();
                    }
                }
            }
        }

        /* B Button: Cancel Selection */
        if (pressed & J_B) {
            if (g_game.has_selected) {
                g_game.has_selected = 0;
                Game_ClearValidMovesMap();
                strcpy(g_game.status_msg, "YOUR TURN (RED)");
                DrawBoard();
                DrawStatus();
            }
        }

        /* SELECT Button: Cycle Difficulty */
        if (pressed & J_SELECT) {
            g_game.difficulty = (g_game.difficulty + 1) % 3;
            Sound_PlaySelect();
            if (g_game.difficulty == DIFF_EASY) strcpy(g_game.status_msg, "DIFF: SIMPLE (EASY)");
            else if (g_game.difficulty == DIFF_MEDIUM) strcpy(g_game.status_msg, "DIFF: MORTAL (MED)");
            else strcpy(g_game.status_msg, "DIFF: GODLIKE (HARD)");
            DrawHUD();
            DrawStatus();
        }

        /* START Button: Reset Game */
        if (pressed & J_START) {
            Sound_PlaySelect();
            Game_Reset();
            DrawHUD();
            DrawBoard();
            DrawStatus();
            UpdateCursorSprites();
        }

        wait_vbl_done();
    }
}

