#include "game.h"
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

GameState g_game;

/* Default Colors */
#define COLOR_DEFAULT_RED   RGB(239, 68, 68)
#define COLOR_DEFAULT_BLUE  RGB(59, 130, 246)

static void to_lower_str(char *dest, const char *src) {
    int i = 0;
    while (src[i] && i < 63) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') c += ('a' - 'A');
        dest[i] = c;
        i++;
    }
    dest[i] = '\0';
}

static BOOL str_contains(const char *haystack, const char *needle) {
    return (strstr(haystack, needle) != NULL);
}

void Game_AddStatus(const char *msg, BOOL is_temporary) {
    int i;
    if (!msg || msg[0] == '\0') return;

    if (is_temporary && g_game.grimoire_count > 0) {
        strncpy(g_game.grimoire[0].text, msg, sizeof(g_game.grimoire[0].text) - 1);
        g_game.grimoire[0].text[sizeof(g_game.grimoire[0].text) - 1] = '\0';
        return;
    }

    /* Shift older messages down */
    for (i = GRIMOIRE_MAX - 1; i > 0; i--) {
        g_game.grimoire[i] = g_game.grimoire[i - 1];
    }
    strncpy(g_game.grimoire[0].text, msg, sizeof(g_game.grimoire[0].text) - 1);
    g_game.grimoire[0].text[sizeof(g_game.grimoire[0].text) - 1] = '\0';

    if (g_game.grimoire_count < GRIMOIRE_MAX) {
        g_game.grimoire_count++;
    }
}

void Game_UpdatePlayerThemes(void) {
    char red_lower[64];
    char blue_lower[64];

    to_lower_str(red_lower, g_game.player_name_red);
    to_lower_str(blue_lower, g_game.player_name_blue);

    /* Check Red Character */
    g_game.char_red = CHAR_DEFAULT;
    g_game.color_red = COLOR_DEFAULT_RED;

    if (str_contains(red_lower, "santa") || str_contains(red_lower, "satan")) {
        g_game.char_red = CHAR_SANTA;
        g_game.color_red = RGB(255, 0, 0);
    } else if (str_contains(red_lower, "dean") || str_contains(red_lower, "sam")) {
        g_game.char_red = CHAR_SUPERNATURAL;
        g_game.color_red = RGB(0, 0, 0);
    } else if (str_contains(red_lower, "klf") || str_contains(red_lower, "justified") || str_contains(red_lower, "jams")) {
        g_game.char_red = CHAR_KLF;
        g_game.color_red = RGB(255, 255, 255);
    } else if (str_contains(red_lower, "fox") || str_contains(red_lower, "dana")) {
        g_game.char_red = CHAR_XFILES;
        g_game.color_red = RGB(0, 0, 0);
    } else if (str_contains(red_lower, "rhan-tegoth") || str_contains(red_lower, "fear") || str_contains(red_lower, "frozen")) {
        g_game.char_red = CHAR_RHAN_TEGOTH;
        g_game.color_red = RGB(135, 206, 235);
    } else if (str_contains(red_lower, "hastur")) {
        g_game.char_red = CHAR_HASTUR;
        g_game.color_red = RGB(255, 200, 0);
    } else if (str_contains(red_lower, "yog-sothoth")) {
        g_game.char_red = CHAR_YOG_SOTHOTH;
        g_game.color_red = RGB(176, 224, 230);
    } else if (str_contains(red_lower, "ghroth")) {
        g_game.char_red = CHAR_GHROTH;
        g_game.color_red = RGB(138, 43, 226);
    } else if (str_contains(red_lower, "azathoth")) {
        g_game.char_red = CHAR_AZATHOTH;
        g_game.color_red = RGB(255, 105, 180);
    } else if (str_contains(red_lower, "ithaqua")) {
        g_game.char_red = CHAR_ITHAQUA;
        g_game.color_red = RGB(0, 255, 255);
    } else if (str_contains(red_lower, "abhoth")) {
        g_game.char_red = CHAR_ABHOTH;
        g_game.color_red = RGB(105, 139, 105);
    } else if (str_contains(red_lower, "eihort")) {
        g_game.char_red = CHAR_EIHORT;
        g_game.color_red = RGB(0, 128, 128);
    } else if (str_contains(red_lower, "idha") || str_contains(red_lower, "sleeper")) {
        g_game.char_red = CHAR_IDHA;
        g_game.color_red = RGB(128, 128, 128);
    } else if (str_contains(red_lower, "shudde") || str_contains(red_lower, "mell") || str_contains(red_lower, "glutton") || str_contains(red_lower, "excess")) {
        g_game.char_red = CHAR_SHUDDE_MELL;
        g_game.color_red = RGB(101, 67, 33);
    } else if (str_contains(red_lower, "cthulhu")) {
        g_game.char_red = CHAR_CTHULHU;
        g_game.color_red = RGB(0, 180, 120);
    } else if (str_contains(red_lower, "george w.") || str_contains(red_lower, "w.")) {
        g_game.char_red = CHAR_GWB;
        g_game.color_red = COLOR_DEFAULT_RED;
    }

    /* Check Blue Character */
    g_game.char_blue = CHAR_DEFAULT;
    g_game.color_blue = COLOR_DEFAULT_BLUE;

    if (str_contains(blue_lower, "santa") || str_contains(blue_lower, "satan")) {
        g_game.char_blue = CHAR_SANTA;
        g_game.color_blue = RGB(255, 0, 0);
    } else if (strcmp(blue_lower, "trent") == 0) {
        g_game.char_blue = CHAR_NIN;
        g_game.color_blue = RGB(0, 96, 128);
    } else if (g_game.char_red == CHAR_SUPERNATURAL) {
        g_game.char_blue = CHAR_SUPERNATURAL;
        g_game.color_blue = RGB(255, 140, 0); /* Rowena Witch Orange */
        strcpy(g_game.player_name_blue, "Rowena");
    } else if (str_contains(blue_lower, "klf") || str_contains(blue_lower, "justified") || str_contains(blue_lower, "jams")) {
        g_game.char_blue = CHAR_KLF;
        g_game.color_blue = RGB(255, 255, 255);
    } else if (strcmp(blue_lower, "groovie mann") == 0) {
        g_game.char_blue = CHAR_TKK;
        g_game.color_blue = RGB(0, 0, 0);
    } else if (str_contains(blue_lower, "fox") || str_contains(blue_lower, "dana")) {
        g_game.char_blue = CHAR_XFILES;
        g_game.color_blue = RGB(0, 0, 0);
    } else if (strcmp(blue_lower, "doktor avalanche") == 0) {
        g_game.char_blue = CHAR_DOKTOR;
        g_game.color_blue = RGB(0, 0, 0);
    } else if (str_contains(blue_lower, "shoggoth")) {
        g_game.char_blue = CHAR_SHOGGOTH;
        g_game.color_blue = RGB(56, 118, 29);
    } else if (str_contains(blue_lower, "abhoth")) {
        g_game.char_blue = CHAR_ABHOTH;
        g_game.color_blue = RGB(105, 139, 105);
    } else if (str_contains(blue_lower, "eihort")) {
        g_game.char_blue = CHAR_EIHORT;
        g_game.color_blue = RGB(0, 128, 128);
    } else if (str_contains(blue_lower, "shudde") || str_contains(blue_lower, "mell") || str_contains(blue_lower, "glutton") || str_contains(blue_lower, "excess")) {
        g_game.char_blue = CHAR_SHUDDE_MELL;
        g_game.color_blue = RGB(101, 67, 33);
    }

    /* Time distortion */
    if (g_game.char_red == CHAR_IDHA) {
        g_game.time_distortion = 0.0f;
    } else if (g_game.char_red == CHAR_EIHORT) {
        g_game.time_distortion = 0.5f;
    } else if (g_game.char_blue == CHAR_EIHORT) {
        g_game.time_distortion = 2.0f;
    } else {
        g_game.time_distortion = 1.0f;
    }

    /* Shudde M'ell 9x9 expansion */
    if (g_game.is_first_move && (g_game.char_red == CHAR_SHUDDE_MELL || g_game.char_blue == CHAR_SHUDDE_MELL)) {
        g_game.board_size = 9;
    } else if (g_game.is_first_move) {
        g_game.board_size = DEFAULT_BOARD;
    }

    /* Determine background */
    g_game.has_special_bg = TRUE;
    if (g_game.char_red == CHAR_SANTA || g_game.char_blue == CHAR_SANTA) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_santa.bmp");
    } else if (g_game.char_red == CHAR_KLF || g_game.char_blue == CHAR_KLF) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_KLF.bmp");
    } else if (g_game.char_red == CHAR_SUPERNATURAL) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_supernatural.bmp");
    } else if (g_game.char_blue == CHAR_TKK) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_tkk.bmp");
    } else if (g_game.char_red == CHAR_XFILES || g_game.char_blue == CHAR_XFILES) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_xfiles.bmp");
    } else if (g_game.char_red == CHAR_GWB) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_GWB.bmp");
    } else if (g_game.char_blue == CHAR_DOKTOR) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_sisters.bmp");
    } else if (g_game.char_blue == CHAR_NIN) {
        strcpy(g_game.current_bg_filename, "IMAGES\\BG\\bg_nin.bmp");
    } else {
        g_game.has_special_bg = FALSE;
        /* Choose a random standard background */
        int bg_num = (rand() % 39) + 1;
        sprintf(g_game.current_bg_filename, "IMAGES\\BG\\bg_%02d.bmp", bg_num);
    }
}

void Game_Init(void) {
    srand((unsigned int)time(NULL));
    memset(&g_game, 0, sizeof(GameState));

    g_game.board_size = DEFAULT_BOARD;
    strcpy(g_game.player_name_red, "Aforgomon");
    strcpy(g_game.player_name_blue, "Xexanoth");
    g_game.difficulty = DIFF_MEDIUM;
    g_game.obstacle_setting = OBS_NONE;
    g_game.turn_timer_setting = TIMER_UNLIMITED;
    g_game.is_muted = FALSE;

    Game_ResetGame();
}

void Game_ResetGame(void) {
    int r, c;
    int last;
    char start_msg[128];

    g_game.is_first_move = TRUE;
    g_game.game_over = FALSE;
    g_game.effect_locked = FALSE;
    g_game.effect_delay_ticks = 0;
    g_game.has_selected = FALSE;
    g_game.score_blindness_duration = 0;
    g_game.hastur_clone_count = 0;
    g_game.santa_move_count[CELL_RED] = 0;
    g_game.santa_move_count[CELL_BLUE] = 0;
    g_game.whisper_triggered = FALSE;
    g_game.blindness_triggered = FALSE;
    g_game.ai_logic_torn = FALSE;
    g_game.grimoire_count = 0;
    g_game.winner_msg[0] = '\0';

    Game_UpdatePlayerThemes();

    /* Clear board */
    for (r = 0; r < MAX_BOARD; r++) {
        for (c = 0; c < MAX_BOARD; c++) {
            g_game.board[r][c] = CELL_EMPTY;
        }
    }

    last = g_game.board_size - 1;
    /* Starting corners */
    g_game.board[0][0] = CELL_RED;
    g_game.board[last][last] = CELL_RED;
    g_game.board[0][last] = CELL_BLUE;
    g_game.board[last][0] = CELL_BLUE;

    /* Obstacles placement */
    if (g_game.obstacle_setting != OBS_NONE) {
        int min_obs = 0, max_obs = 0;
        int count, placed = 0;
        switch (g_game.obstacle_setting) {
            case OBS_SOME: min_obs = 4; max_obs = 8; break;
            case OBS_MORE: min_obs = 12; max_obs = 20; break;
            case OBS_MADNESS: min_obs = 20; max_obs = 24; break;
        }
        count = min_obs + (rand() % (max_obs - min_obs + 1));
        while (placed < count) {
            r = rand() % g_game.board_size;
            c = rand() % g_game.board_size;
            if (g_game.board[r][c] == CELL_EMPTY) {
                g_game.board[r][c] = CELL_OBSTACLE;
                placed++;
            }
        }
    }

    g_game.scores[CELL_RED] = 2;
    g_game.scores[CELL_BLUE] = 2;
    g_game.current_player = CELL_RED;

    g_game.game_timer = GAME_TIME_DEFAULT;
    g_game.turn_timer = (g_game.turn_timer_setting == TIMER_UNLIMITED) ? 0 : g_game.turn_timer_setting;

    sprintf(start_msg, "%s's Turn (You)", g_game.player_name_red);
    Game_AddStatus(start_msg, FALSE);

    /* Play music */
    if (!g_game.is_muted) {
        if (g_game.char_red == CHAR_GWB) {
            Sound_PlaySFX("gwb_theme.wav");
        } else if (g_game.char_blue == CHAR_DOKTOR) {
            Sound_PlaySFX("doktor.wav");
        } else if (g_game.char_blue == CHAR_NIN) {
            Sound_PlayBGM("nin.wav");
        } else {
            Sound_PlayBGM("bg_music.wav");
        }
    }
}

int Game_GetValidMoveType(int from_r, int from_c, int to_r, int to_c) {
    int dr, dc, dist;
    if (to_r < 0 || to_r >= g_game.board_size || to_c < 0 || to_c >= g_game.board_size) {
        return MOVE_NONE;
    }
    if (g_game.board[to_r][to_c] != CELL_EMPTY) {
        return MOVE_NONE;
    }
    dr = abs(to_r - from_r);
    dc = abs(to_c - from_c);
    dist = (dr > dc) ? dr : dc;

    if (dist == 1) return MOVE_CLONE;
    if (dist == 2) return MOVE_LEAP;
    return MOVE_NONE;
}

int Game_GetAllValidMoves(int player, GameMove *out_moves, int max_moves) {
    int r, c, dr, dc;
    int count = 0;

    for (r = 0; r < g_game.board_size; r++) {
        for (c = 0; c < g_game.board_size; c++) {
            if (g_game.board[r][c] == player) {
                for (dr = -2; dr <= 2; dr++) {
                    for (dc = -2; dc <= 2; dc++) {
                        int to_r, to_c, type;
                        if (dr == 0 && dc == 0) continue;
                        to_r = r + dr;
                        to_c = c + dc;
                        type = Game_GetValidMoveType(r, c, to_r, to_c);
                        if (type != MOVE_NONE) {
                            if (count < max_moves) {
                                out_moves[count].from_r = r;
                                out_moves[count].from_c = c;
                                out_moves[count].to_r = to_r;
                                out_moves[count].to_c = to_c;
                                out_moves[count].type = type;
                                count++;
                            }
                        }
                    }
                }
            }
        }
    }
    return count;
}

static int CountAdjacentOpponents(int r, int c, int opponent) {
    int dr, dc;
    int count = 0;
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            int nr, nc;
            if (dr == 0 && dc == 0) continue;
            nr = r + dr;
            nc = c + dc;
            if (nr >= 0 && nr < g_game.board_size && nc >= 0 && nc < g_game.board_size) {
                if (g_game.board[nr][nc] == opponent) {
                    count++;
                }
            }
        }
    }
    return count;
}

static int CountAdjacentPlayerPieces(int r, int c, int player) {
    int dr, dc;
    int count = 0;
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            int nr, nc;
            nr = r + dr;
            nc = c + dc;
            if (nr >= 0 && nr < g_game.board_size && nc >= 0 && nc < g_game.board_size) {
                if (g_game.board[nr][nc] == player) {
                    count++;
                }
            }
        }
    }
    return count;
}

static BOOL CheckCallOfCthulhu(void) {
    int center, last;
    if (g_game.current_player != CELL_RED) return FALSE;
    if (g_game.char_red != CHAR_CTHULHU) return FALSE;

    center = g_game.board_size / 2;
    last = g_game.board_size - 1;

    if (g_game.board[center][center] == CELL_RED &&
        g_game.board[0][0] == CELL_RED &&
        g_game.board[0][last] == CELL_RED &&
        g_game.board[last][0] == CELL_RED &&
        g_game.board[last][last] == CELL_RED) {
        return TRUE;
    }
    return FALSE;
}

int Game_MakeMove(int from_r, int from_c, int to_r, int to_c, int move_type) {
    int opponent = (g_game.current_player == CELL_RED) ? CELL_BLUE : CELL_RED;
    int captures = 0;
    int dr, dc;
    char msg[128];

    g_game.is_first_move = FALSE;

    /* Abhoth Annihilation Check */
    if ((g_game.char_red == CHAR_ABHOTH && g_game.current_player == CELL_BLUE) ||
        (g_game.char_blue == CHAR_ABHOTH && g_game.current_player == CELL_RED)) {
        int abhoth_color = (g_game.char_red == CHAR_ABHOTH) ? CELL_RED : CELL_BLUE;
        int adj = CountAdjacentPlayerPieces(to_r, to_c, abhoth_color);
        if (adj >= 6) {
            Sound_PlaySFX("anomaly.wav");
            Game_AddStatus("Annihilated by Abhoth's corruption! Move failed.", FALSE);
            return 0;
        }
    }

    /* Place piece */
    if (move_type == MOVE_CLONE) {
        g_game.board[to_r][to_c] = g_game.current_player;
        g_game.scores[g_game.current_player]++;
    } else if (move_type == MOVE_LEAP) {
        g_game.board[to_r][to_c] = g_game.current_player;
        g_game.board[from_r][from_c] = CELL_EMPTY;
    }

    /* Convert surrounding pieces */
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            int nr, nc;
            if (dr == 0 && dc == 0) continue;
            nr = to_r + dr;
            nc = to_c + dc;
            if (nr >= 0 && nr < g_game.board_size && nc >= 0 && nc < g_game.board_size) {
                if (g_game.board[nr][nc] == opponent) {
                    g_game.board[nr][nc] = g_game.current_player;
                    g_game.scores[g_game.current_player]++;
                    g_game.scores[opponent]--;
                    captures++;
                }
            }
        }
    }

    /* Easter Egg: Hastur 4 Clones */
    if (g_game.current_player == CELL_RED && g_game.char_red == CHAR_HASTUR) {
        if (move_type == MOVE_CLONE) {
            g_game.hastur_clone_count++;
            if (g_game.hastur_clone_count >= 4) {
                int er, ec;
                for (er = 0; er < g_game.board_size; er++) {
                    for (ec = 0; ec < g_game.board_size; ec++) {
                        if (g_game.board[er][ec] == CELL_EMPTY) {
                            g_game.board[er][ec] = CELL_RED;
                            g_game.scores[CELL_RED]++;
                            Game_AddStatus("The Unspeakable Oath is complete! An Anchor is set.", FALSE);
                            goto hastur_done;
                        }
                    }
                }
            hastur_done:
                g_game.hastur_clone_count = 0;
            }
        } else {
            g_game.hastur_clone_count = 0;
        }
    }

    /* Easter Egg: Yog-Sothoth Toll */
    if (g_game.current_player == CELL_RED && g_game.char_red == CHAR_YOG_SOTHOTH && move_type == MOVE_LEAP && captures >= 2) {
        for (dr = -1; dr <= 1; dr++) {
            for (dc = -1; dc <= 1; dc++) {
                int nr = to_r + dr, nc = to_c + dc;
                if (nr >= 0 && nr < g_game.board_size && nc >= 0 && nc < g_game.board_size) {
                    if (g_game.board[nr][nc] == CELL_EMPTY) {
                        g_game.board[nr][nc] = CELL_OBSTACLE;
                    }
                }
            }
        }
        Game_AddStatus("Yog-Sothoth demands a toll! Gate sealed with obstacles.", FALSE);
    }

    /* Easter Egg: Rhan-Tegoth Frozen Fear */
    if (g_game.current_player == CELL_RED && g_game.char_red == CHAR_RHAN_TEGOTH && move_type == MOVE_LEAP && captures == 0) {
        int cluster = CountAdjacentPlayerPieces(to_r, to_c, CELL_RED);
        if (cluster >= 5) {
            int frozen = 0;
            for (dr = -1; dr <= 1; dr++) {
                for (dc = -1; dc <= 1; dc++) {
                    int nr = to_r + dr, nc = to_c + dc;
                    if (nr >= 0 && nr < g_game.board_size && nc >= 0 && nc < g_game.board_size) {
                        if (g_game.board[nr][nc] == CELL_BLUE) {
                            g_game.board[nr][nc] = CELL_PERM_OBSTACLE;
                            g_game.scores[CELL_BLUE]--;
                            frozen++;
                        }
                    }
                }
            }
            if (frozen > 0) {
                sprintf(msg, "Rhan-Tegoth's Stasis! %d Blue pieces frozen into ice!", frozen);
                Game_AddStatus(msg, FALSE);
                Sound_PlaySFX("rhan.wav");
            }
        }
    }

    /* Easter Egg: Santa Present */
    if (g_game.char_red == CHAR_SANTA || g_game.char_blue == CHAR_SANTA) {
        g_game.santa_move_count[g_game.current_player]++;
        if (g_game.santa_move_count[g_game.current_player] % 4 == 0) {
            int pr = rand() % g_game.board_size;
            int pc = rand() % g_game.board_size;
            if (g_game.board[pr][pc] == CELL_EMPTY) {
                g_game.board[pr][pc] = CELL_PRESENT_OBSTACLE;
                Game_AddStatus("Ho Ho Ho! A gift appears on the board!", FALSE);
                Sound_PlaySFX("santa.wav");
            }
        }
    }

    /* Sound playback for move */
    if (g_game.current_player == CELL_RED) {
        if (captures >= 4) {
            Sound_PlaySFX("player_big_capture.wav");
        } else if (g_game.char_red == CHAR_KLF) {
            Sound_PlaySFX("klf_mumu.wav");
        } else if (g_game.char_red == CHAR_RHAN_TEGOTH) {
            Sound_PlaySFX("rhan.wav");
        } else if (g_game.char_red != CHAR_DEFAULT && g_game.char_red != CHAR_GWB) {
            Sound_PlaySFX("player_special.wav");
        } else {
            Sound_PlaySFX("place.wav");
        }
    } else {
        if (captures >= 5) {
            Sound_PlaySFX("big_capture.wav");
        } else if (g_game.char_blue == CHAR_KLF) {
            Sound_PlaySFX("klf_mumu.wav");
        } else {
            Sound_PlaySFX("enemyplace.wav");
        }
    }

    return captures;
}

void Game_CheckGameOver(void) {
    GameMove red_moves[128];
    GameMove blue_moves[128];
    int num_red, num_blue;
    int occupied = 0, r, c;

    /* Call of Cthulhu instant awakening */
    if (CheckCallOfCthulhu()) {
        g_game.game_over = TRUE;
        sprintf(g_game.winner_msg, "Ph'nglui mglw'nafh Cthulhu! %s Awakens Cthulhu and WINS!", g_game.player_name_red);
        Game_AddStatus(g_game.winner_msg, FALSE);
        Sound_PlaySFX("win.wav");
        return;
    }

    num_red = Game_GetAllValidMoves(CELL_RED, red_moves, 128);
    num_blue = Game_GetAllValidMoves(CELL_BLUE, blue_moves, 128);

    for (r = 0; r < g_game.board_size; r++) {
        for (c = 0; c < g_game.board_size; c++) {
            if (g_game.board[r][c] == CELL_RED || g_game.board[r][c] == CELL_BLUE) {
                occupied++;
            }
        }
    }

    if ((num_red == 0 && num_blue == 0) ||
        occupied == g_game.board_size * g_game.board_size ||
        g_game.scores[CELL_RED] == 0 ||
        g_game.scores[CELL_BLUE] == 0) {
        g_game.game_over = TRUE;

        if (g_game.scores[CELL_RED] > g_game.scores[CELL_BLUE]) {
            sprintf(g_game.winner_msg, "%s Wins! (%d - %d)", g_game.player_name_red, g_game.scores[CELL_RED], g_game.scores[CELL_BLUE]);
        } else if (g_game.scores[CELL_BLUE] > g_game.scores[CELL_RED]) {
            sprintf(g_game.winner_msg, "%s Wins! (%d - %d)", g_game.player_name_blue, g_game.scores[CELL_BLUE], g_game.scores[CELL_RED]);
        } else {
            sprintf(g_game.winner_msg, "It's a Draw! (%d - %d)", g_game.scores[CELL_RED], g_game.scores[CELL_BLUE]);
        }
        Game_AddStatus(g_game.winner_msg, FALSE);
        Sound_PlaySFX("win.wav");
    }
}

void Game_TriggerNextTurn(BOOL forced) {
    GameMove next_moves[128];
    int count;
    int next_player = (g_game.current_player == CELL_RED) ? CELL_BLUE : CELL_RED;
    char msg[128];

    Game_CheckGameOver();
    if (g_game.game_over) return;

    count = Game_GetAllValidMoves(next_player, next_moves, 128);
    if (count > 0 || forced) {
        g_game.current_player = next_player;
    } else {
        /* Next player has no moves, current player moves again */
        GameMove curr_moves[128];
        int curr_count = Game_GetAllValidMoves(g_game.current_player, curr_moves, 128);
        if (curr_count == 0) {
            g_game.game_over = TRUE;
            Game_CheckGameOver();
            return;
        }
        sprintf(msg, "%s has no moves! %s goes again.",
            (next_player == CELL_RED) ? g_game.player_name_red : g_game.player_name_blue,
            (g_game.current_player == CELL_RED) ? g_game.player_name_red : g_game.player_name_blue);
        Game_AddStatus(msg, FALSE);
    }

    g_game.turn_timer = (g_game.turn_timer_setting == TIMER_UNLIMITED) ? 0 : g_game.turn_timer_setting;

    if (g_game.current_player == CELL_RED) {
        sprintf(msg, "%s's Turn (You)", g_game.player_name_red);
        Game_AddStatus(msg, FALSE);
    } else {
        sprintf(msg, "%s's Turn (AI thinking...)", g_game.player_name_blue);
        Game_AddStatus(msg, TRUE);
        Game_MakeAIMove();
    }
}

void Game_HandleClick(int r, int c) {
    int move_type;
    if (g_game.game_over || g_game.current_player != CELL_RED || g_game.effect_locked) return;
    if (r < 0 || r >= g_game.board_size || c < 0 || c >= g_game.board_size) return;

    /* Nyarlathotep Crawling Chaos (5% chance to redirect click) */
    if (g_game.char_blue == CHAR_NYARLATHOTEP && g_game.board[r][c] == CELL_RED && (rand() % 100 < 5)) {
        int dr, dc;
        for (dr = -1; dr <= 1; dr++) {
            for (dc = -1; dc <= 1; dc++) {
                int nr = r + dr, nc = c + dc;
                if ((dr != 0 || dc != 0) && nr >= 0 && nr < g_game.board_size && nc >= 0 && nc < g_game.board_size) {
                    if (g_game.board[nr][nc] == CELL_RED) {
                        r = nr; c = nc;
                        Game_AddStatus("The Crawling Chaos distracts your vision!", FALSE);
                        goto chaos_done;
                    }
                }
            }
        }
    chaos_done:;
    }

    if (g_game.has_selected) {
        if (g_game.selected_r == r && g_game.selected_c == c) {
            /* Deselect */
            g_game.has_selected = FALSE;
            return;
        }
        if (g_game.board[r][c] == CELL_RED) {
            /* Select other friendly piece */
            g_game.selected_r = r;
            g_game.selected_c = c;
            Sound_PlaySFX("select.wav");
            return;
        }

        move_type = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
        if (move_type != MOVE_NONE) {
            int from_r = g_game.selected_r;
            int from_c = g_game.selected_c;
            g_game.has_selected = FALSE;

            Game_MakeMove(from_r, from_c, r, c, move_type);
            Game_TriggerNextTurn(FALSE);
        } else {
            g_game.has_selected = FALSE;
        }
    } else {
        if (g_game.board[r][c] == CELL_RED) {
            g_game.has_selected = TRUE;
            g_game.selected_r = r;
            g_game.selected_c = c;
            Sound_PlaySFX("select.wav");
        }
    }
}

/* AI Logic: 486-Proportionate Zero-Recursion Algorithms */
static BOOL IsCorner(int r, int c, int size) {
    return (r == 0 || r == size - 1) && (c == 0 || c == size - 1);
}

static BOOL IsEdge(int r, int c, int size) {
    return (r == 0 || r == size - 1 || c == 0 || c == size - 1);
}

static int EvaluateMoveHeuristic(const GameMove *m, int opp, int size) {
    int captures = CountAdjacentOpponents(m->to_r, m->to_c, opp);
    int score = captures * 10;

    /* Clones add +1 piece to our total; Leaps leave empty space behind */
    if (m->type == MOVE_CLONE) {
        score += 4;
    } else {
        if (captures == 0) score -= 4; /* Leap with 0 captures is disadvantageous */
    }

    /* Anchors: Corners are unassailable, edges are safer */
    if (IsCorner(m->to_r, m->to_c, size)) {
        score += 6;
    } else if (IsEdge(m->to_r, m->to_c, size)) {
        score += 2;
    }

    return score;
}

/* Fast Greedy move search (compatible helper) */
BOOL Game_FindBestMoveGreedy(int player, GameMove *best_move) {
    GameMove moves[128];
    int count = Game_GetAllValidMoves(player, moves, 128);
    int i, opponent = (player == CELL_RED) ? CELL_BLUE : CELL_RED;
    int best_score = -999999;
    int best_idx = -1;

    if (count == 0) return FALSE;

    for (i = 0; i < count; i++) {
        int captures = CountAdjacentOpponents(moves[i].to_r, moves[i].to_c, opponent);
        int score = captures * 10;
        if (moves[i].type == MOVE_CLONE) score += 1;

        if (score > best_score || (score == best_score && (rand() % 2 == 0))) {
            best_score = score;
            best_idx = i;
        }
    }

    if (best_idx >= 0) {
        *best_move = moves[best_idx];
        return TRUE;
    }
    return FALSE;
}

/* EASY DIFFICULTY: Casual / Novice
 * 60% random valid move, 40% simple greedy.
 * Instantaneous (< 0.05ms on a 486), very forgiving. */
static BOOL Game_AIMove_Easy(GameMove *best_move) {
    GameMove moves[128];
    int count = Game_GetAllValidMoves(CELL_BLUE, moves, 128);
    int i, best_idx = -1, best_score = -999999;
    if (count == 0) return FALSE;

    if ((rand() % 100) < 60) {
        *best_move = moves[rand() % count];
        return TRUE;
    }

    for (i = 0; i < count; i++) {
        int captures = CountAdjacentOpponents(moves[i].to_r, moves[i].to_c, CELL_RED);
        int score = captures * 10 + (moves[i].type == MOVE_CLONE ? 2 : 0);
        if (score > best_score || (score == best_score && (rand() % 2 == 0))) {
            best_score = score;
            best_idx = i;
        }
    }
    if (best_idx >= 0) {
        *best_move = moves[best_idx];
        return TRUE;
    }
    return FALSE;
}

/* MEDIUM DIFFICULTY: Mortal / Standard
 * Fast 1-pass positional greedy heuristic.
 * Takes ~0.1ms on a 486 (Zero lag, zero choking). */
static BOOL Game_AIMove_Medium(GameMove *best_move) {
    GameMove moves[128];
    int count = Game_GetAllValidMoves(CELL_BLUE, moves, 128);
    int i, best_idx = -1, best_score = -999999;
    if (count == 0) return FALSE;

    for (i = 0; i < count; i++) {
        int score = EvaluateMoveHeuristic(&moves[i], CELL_RED, g_game.board_size);
        score += (rand() % 3); /* minor jitter */

        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }
    if (best_idx >= 0) {
        *best_move = moves[best_idx];
        return TRUE;
    }
    return FALSE;
}

/* HARD DIFFICULTY: Elder Godlike / Tactical
 * Evaluates candidate moves with positional bonuses AND a fast
 * localized counter-attack penalty.
 * Takes ~0.2ms on a 486 (Zero recursion, smooth 486 responsiveness). */
static BOOL Game_AIMove_Hard(GameMove *best_move) {
    GameMove moves[128];
    int count = Game_GetAllValidMoves(CELL_BLUE, moves, 128);
    int i, best_idx = -1, best_score = -999999;
    if (count == 0) return FALSE;

    for (i = 0; i < count; i++) {
        int score = EvaluateMoveHeuristic(&moves[i], CELL_RED, g_game.board_size);

        /* Fast threat check around destination square */
        int dr, dc, threat_count = 0;
        for (dr = -2; dr <= 2; dr++) {
            for (dc = -2; dc <= 2; dc++) {
                int tr = moves[i].to_r + dr;
                int tc = moves[i].to_c + dc;
                if (tr >= 0 && tr < g_game.board_size && tc >= 0 && tc < g_game.board_size) {
                    /* If opponent piece is here and NOT adjacent (won't be converted),
                     * they can leap or clone next turn */
                    int mdr = abs(dr), mdc = abs(dc);
                    int dist = (mdr > mdc) ? mdr : mdc;
                    if (dist == 2 && g_game.board[tr][tc] == CELL_RED) {
                        threat_count++;
                    }
                }
            }
        }
        score -= (threat_count * 2);

        if (score > best_score || (score == best_score && (rand() % 2 == 0))) {
            best_score = score;
            best_idx = i;
        }
    }
    if (best_idx >= 0) {
        *best_move = moves[best_idx];
        return TRUE;
    }
    return FALSE;
}

void Game_MakeAIMove(void) {
    GameMove best_move;
    BOOL found = FALSE;
    int roll = rand() % 100;

    /* Easter Egg: Shoggoth Sprawl */
    if (g_game.char_blue == CHAR_SHOGGOTH && (roll < 10)) {
        GameMove clone_moves[128];
        int num_moves = Game_GetAllValidMoves(CELL_BLUE, clone_moves, 128);
        int i, sprawl_count = 0;

        Game_AddStatus("Shoggoth's Sprawl! Rapid expansion destabilizes board!", FALSE);
        for (i = 0; i < num_moves && sprawl_count < 3; i++) {
            if (clone_moves[i].type == MOVE_CLONE) {
                Game_MakeMove(clone_moves[i].from_r, clone_moves[i].from_c, clone_moves[i].to_r, clone_moves[i].to_c, MOVE_CLONE);
                sprawl_count++;
            }
        }
        Game_TriggerNextTurn(FALSE);
        return;
    }

    /* 486-Proportionate AI Difficulty Selection */
    if (g_game.difficulty == DIFF_EASY) {
        found = Game_AIMove_Easy(&best_move);
    } else if (g_game.difficulty == DIFF_MEDIUM) {
        found = Game_AIMove_Medium(&best_move);
    } else { /* DIFF_HARD */
        found = Game_AIMove_Hard(&best_move);
    }

    if (found) {
        Game_MakeMove(best_move.from_r, best_move.from_c, best_move.to_r, best_move.to_c, best_move.type);
        Game_TriggerNextTurn(FALSE);
    } else {
        Game_TriggerNextTurn(TRUE);
    }
}

void Game_OnGameTimerTick(void) {
    if (g_game.game_over || g_game.time_distortion == 0.0f) return;

    if (g_game.game_timer > 0) {
        g_game.game_timer--;

        /* 1% chance for unseen timer anomaly sound */
        if (rand() % 100 == 0) {
            Sound_PlaySFX("anomaly.wav");
        }
        /* Tick sound on last 10 seconds */
        if (g_game.game_timer <= 10 && g_game.game_timer > 0) {
            Sound_PlaySFX("tick.wav");
        }

        if (g_game.game_timer == 0) {
            g_game.game_over = TRUE;
            Game_AddStatus("Time's Up!", FALSE);
            Game_CheckGameOver();
        }
    }
}

void Game_OnTurnTimerTick(void) {
    if (g_game.game_over || g_game.turn_timer_setting == TIMER_UNLIMITED) return;

    if (g_game.turn_timer > 0) {
        g_game.turn_timer--;
        if (g_game.turn_timer == 0) {
            /* Auto-play best greedy move */
            GameMove auto_move;
            if (Game_FindBestMoveGreedy(g_game.current_player, &auto_move)) {
                char msg[128];
                sprintf(msg, "%s ran out of time! Auto-playing best move.",
                    (g_game.current_player == CELL_RED) ? g_game.player_name_red : g_game.player_name_blue);
                Game_AddStatus(msg, FALSE);
                Game_MakeMove(auto_move.from_r, auto_move.from_c, auto_move.to_r, auto_move.to_c, auto_move.type);
                Game_TriggerNextTurn(FALSE);
            } else {
                Game_TriggerNextTurn(TRUE);
            }
        }
    }
}

void Game_SetNames(const char *red_name, const char *blue_name) {
    if (red_name && red_name[0] != '\0') {
        strncpy(g_game.player_name_red, red_name, 63);
        g_game.player_name_red[63] = '\0';
    }
    if (blue_name && blue_name[0] != '\0') {
        strncpy(g_game.player_name_blue, blue_name, 63);
        g_game.player_name_blue[63] = '\0';
    }
    Game_UpdatePlayerThemes();
}

