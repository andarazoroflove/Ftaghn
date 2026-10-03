#include "game.h"
#include "sound.h"
#include "freestanding.h"

GameState g_game;

/* Default Faction Colors */
#define COLOR_DEFAULT_RED   RGB(239, 68, 68)
#define COLOR_DEFAULT_BLUE  RGB(59, 130, 246)

/* Complete Tome of Forbidden Knowledge Secret Matrix */
const SecretInfo g_secrets[CHAR_MAX_COUNT] = {
    { CHAR_DEFAULT,      "Default",           "Cosmic Horror",          "Standard Ataxx rules across the 7x7 grid.",                             "bg_music.wav",        "IMAGES\\BG\\bg_01.bmp",          RGB(239, 68, 68) },
    { CHAR_HASTUR,       "Hastur",            "Unspeakable Oath",       "Making 4 consecutive Clone moves summons an extra Red piece randomly.", "player_special.wav",  "IMAGES\\BG\\bg_02.bmp",          RGB(255, 200, 0) },
    { CHAR_RHAN_TEGOTH,  "Rhan-Tegoth",       "The Frozen Fear",        "Leap with 0 captures into a cluster freezes adjacent foes in ice!",     "rhan.wav",            "IMAGES\\BG\\bg_03.bmp",          RGB(135, 206, 235) },
    { CHAR_YOG_SOTHOTH,  "Yog-Sothoth",       "Dimensional Toll",       "Leap capturing >= 2 pieces seals all 8 surrounding cells as Obstacles.","anomaly.wav",         "IMAGES\\BG\\bg_04.bmp",          RGB(176, 224, 230) },
    { CHAR_GHROTH,       "Ghroth",            "Ghroth's Orbit",         "5% chance after Red turn to shuffle all pieces in a random 3x3 area.",  "anomaly.wav",         "IMAGES\\BG\\bg_05.bmp",          RGB(138, 43, 226) },
    { CHAR_AZATHOTH,     "Azathoth",          "Cosmic Entropy",         "Clones when pieces abound convert 3 random empty cells into Obstacles.","anomaly.wav",         "IMAGES\\BG\\bg_06.bmp",          RGB(255, 105, 180) },
    { CHAR_ITHAQUA,      "Ithaqua",           "Cold Wind",              "Leaping with 0 captures from isolated space converts origin to Obstacle.","tick.wav",          "IMAGES\\BG\\bg_07.bmp",          RGB(0, 255, 255) },
    { CHAR_ABHOTH,       "Abhoth",            "Annihilation",           "Enemy landing adjacent to >= 6 Abhoth pieces gets instantly destroyed!","anomaly.wav",         "IMAGES\\BG\\bg_08.bmp",          RGB(105, 139, 105) },
    { CHAR_SHOGGOTH,     "Shoggoth",          "The Sprawl",             "AI has a 10% chance per turn to execute up to 3 rapid clone moves!",    "enemyplace.wav",      "IMAGES\\BG\\bg_09.bmp",          RGB(56, 118, 29) },
    { CHAR_NYARLATHOTEP, "Nyarlathotep",      "Crawling Chaos",         "AI has a 5% chance to hijack piece selection to a neighbor piece!",     "anomaly.wav",         "IMAGES\\BG\\bg_10.bmp",          RGB(120, 60, 180) },
    { CHAR_YIBB_TSTLL,   "Yibb-Tstll",        "Obstacle Volatility",    "Captures mutate or fortify nearby obstacles into permanent barriers.",  "anomaly.wav",         "IMAGES\\BG\\bg_11.bmp",          RGB(70, 130, 180) },
    { CHAR_IDHA,         "Idha",              "Idha's Slumber",         "The Sleeper awakens! The game timer ceases and time stops completely!", "player_special.wav",  "IMAGES\\BG\\bg_12.bmp",          RGB(128, 128, 128) },
    { CHAR_EIHORT,       "Eihort",            "Dimensional Stasis",     "Player Eihort slows timer by 50%; AI Eihort surges timer by 200%!",     "tick.wav",            "IMAGES\\BG\\bg_13.bmp",          RGB(0, 128, 128) },
    { CHAR_SHUDDE_MELL,  "Shudde M'ell",      "Excessive Space",        "Gluttonous titan expands the board dimensions permanently to 9x9!",     "big_capture.wav",     "IMAGES\\BG\\bg_14.bmp",          RGB(101, 67, 33) },
    { CHAR_CTHULHU,      "Cthulhu",           "Call of Cthulhu",        "Holding the center and all 4 corners awakens Cthulhu for an INSTANT WIN!","win.wav",           "IMAGES\\BG\\bg_15.bmp",          RGB(0, 180, 120) },
    /* Mortal Easter Eggs */
    { CHAR_GWB,          "George W.",         "Presidential Dread",     "Presidential portrait avatar overlay and patriotic fanfare.",           "gwb_theme.wav",       "IMAGES\\BG\\bg_GWB.bmp",         RGB(239, 68, 68) },
    { CHAR_KLF,          "The KLF",           "Justified Ancients",     "Pure white cult pieces, Dillinger and MuMu sound themes.",              "klf_dillinger.wav",   "IMAGES\\BG\\bg_KLF.bmp",         RGB(255, 255, 255) },
    { CHAR_XFILES,       "The X-Files",       "The Truth Is Out There", "Paranormal theme audio and eerie deep-space investigation backdrop.",   "xfiles.wav",          "IMAGES\\BG\\bg_xfiles.bmp",      RGB(20, 20, 20) },
    { CHAR_NIN,          "Nine Inch Nails",   "Downward Spiral",        "Industrial dread music and obsidian metallic themes.",                  "nin.wav",             "IMAGES\\BG\\bg_nin.bmp",         RGB(0, 96, 128) },
    { CHAR_DOKTOR,       "Doktor Avalanche",  "Sisters of Mercy",       "Gothic drum machine ambiance and silver-on-black iconography.",         "doktor.wav",          "IMAGES\\BG\\bg_sisters.bmp",     RGB(30, 30, 30) },
    { CHAR_TKK,          "Groovie Mann",      "Thrill Kill Kult",       "Sleazy acid house disco horror and custom industrial backdrop.",        "tkk.wav",             "IMAGES\\BG\\bg_tkk.bmp",         RGB(180, 20, 80) },
    { CHAR_SUPERNATURAL, "Supernatural",      "Winchester Hunt",        "Demon-hunting hunter runes with Rowena orange opposing witch pieces.",  "supernatural.wav",    "IMAGES\\BG\\bg_supernatural.bmp",RGB(40, 40, 40) },
    { CHAR_SANTA,        "Santa Egg",         "Yuletide Terror",        "Santa holiday present obstacles, crimson/gold gift boxes, ho-ho-ho!",   "santa.wav",           "IMAGES\\BG\\bg_santa.bmp",       RGB(255, 0, 0) }
};

static unsigned int s_rand_seed = 123456789;
static int fast_rand(void) {
    s_rand_seed = (1103515245 * s_rand_seed + 12345) & 0x7fffffff;
    return (int)(s_rand_seed % 32768);
}

static void to_lower_str(char *dest, const char *src) {
    int i = 0;
    while (src[i] && i < 63) {
        dest[i] = (char)tolower((unsigned char)src[i]);
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

    /* Detect Red Character / Secret */
    if (g_game.char_red == CHAR_DEFAULT) {
        if (str_contains(red_lower, "santa") || str_contains(red_lower, "satan")) g_game.char_red = CHAR_SANTA;
        else if (str_contains(red_lower, "hastur")) g_game.char_red = CHAR_HASTUR;
        else if (str_contains(red_lower, "rhan-tegoth") || str_contains(red_lower, "frozen")) g_game.char_red = CHAR_RHAN_TEGOTH;
        else if (str_contains(red_lower, "yog-sothoth")) g_game.char_red = CHAR_YOG_SOTHOTH;
        else if (str_contains(red_lower, "ghroth")) g_game.char_red = CHAR_GHROTH;
        else if (str_contains(red_lower, "azathoth")) g_game.char_red = CHAR_AZATHOTH;
        else if (str_contains(red_lower, "ithaqua")) g_game.char_red = CHAR_ITHAQUA;
        else if (str_contains(red_lower, "abhoth")) g_game.char_red = CHAR_ABHOTH;
        else if (str_contains(red_lower, "shoggoth")) g_game.char_red = CHAR_SHOGGOTH;
        else if (str_contains(red_lower, "nyarlathotep")) g_game.char_red = CHAR_NYARLATHOTEP;
        else if (str_contains(red_lower, "yibb-tstll")) g_game.char_red = CHAR_YIBB_TSTLL;
        else if (str_contains(red_lower, "idha") || str_contains(red_lower, "sleeper")) g_game.char_red = CHAR_IDHA;
        else if (str_contains(red_lower, "eihort")) g_game.char_red = CHAR_EIHORT;
        else if (str_contains(red_lower, "shudde") || str_contains(red_lower, "mell")) g_game.char_red = CHAR_SHUDDE_MELL;
        else if (str_contains(red_lower, "cthulhu")) g_game.char_red = CHAR_CTHULHU;
        else if (str_contains(red_lower, "george w.") || str_contains(red_lower, "w.")) g_game.char_red = CHAR_GWB;
        else if (str_contains(red_lower, "klf") || str_contains(red_lower, "justified")) g_game.char_red = CHAR_KLF;
        else if (str_contains(red_lower, "fox") || str_contains(red_lower, "dana")) g_game.char_red = CHAR_XFILES;
        else if (str_contains(red_lower, "nin") || str_contains(red_lower, "trent")) g_game.char_red = CHAR_NIN;
        else if (str_contains(red_lower, "doktor")) g_game.char_red = CHAR_DOKTOR;
        else if (str_contains(red_lower, "groovie") || str_contains(red_lower, "tkk")) g_game.char_red = CHAR_TKK;
        else if (str_contains(red_lower, "dean") || str_contains(red_lower, "sam")) g_game.char_red = CHAR_SUPERNATURAL;
    }

    g_game.color_red = g_secrets[g_game.char_red].color;
    g_game.color_blue = COLOR_DEFAULT_BLUE;

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

    /* 9x9 board for Shudde M'ell */
    if (g_game.is_first_move && (g_game.char_red == CHAR_SHUDDE_MELL || g_game.char_blue == CHAR_SHUDDE_MELL)) {
        g_game.board_size = 9;
    } else if (g_game.is_first_move) {
        g_game.board_size = DEFAULT_BOARD;
    }

    /* Background image & theme audio */
    if (g_game.char_red != CHAR_DEFAULT) {
        strcpy(g_game.current_bg_filename, g_secrets[g_game.char_red].bg_file);
        g_game.has_special_bg = TRUE;
    } else {
        g_game.has_special_bg = FALSE;
        int bg_num = (fast_rand() % 15) + 1;
        snprintf(g_game.current_bg_filename, sizeof(g_game.current_bg_filename), "IMAGES\\BG\\bg_%02d.bmp", bg_num);
    }
}

void Game_ApplySecret(enum CharType secret, int player) {
    if (secret < 0 || secret >= CHAR_MAX_COUNT) return;

    if (player == CELL_RED) {
        g_game.char_red = secret;
        strncpy(g_game.player_name_red, g_secrets[secret].name, 63);
    } else {
        g_game.char_blue = secret;
        strncpy(g_game.player_name_blue, g_secrets[secret].name, 63);
    }

    Game_UpdatePlayerThemes();

    char buf[128];
    snprintf(buf, sizeof(buf), "Invoked %s: %s!", g_secrets[secret].name, g_secrets[secret].title);
    Game_AddStatus(buf, FALSE);

    /* Play custom theme sound */
    Sound_PlaySFX(g_secrets[secret].sound_file);
}

void Game_Init(void) {
    s_rand_seed = (unsigned int)GetTickCount();
    memset(&g_game, 0, sizeof(GameState));

    g_game.board_size = DEFAULT_BOARD;
    strcpy(g_game.player_name_red, "Cult of Cthulhu");
    strcpy(g_game.player_name_blue, "Elder God AI");
    g_game.char_red = CHAR_DEFAULT;
    g_game.char_blue = CHAR_DEFAULT;
    g_game.difficulty = DIFF_MEDIUM;
    g_game.obstacle_setting = OBS_SOME;
    g_game.turn_timer_setting = TIMER_UNLIMITED;
    g_game.is_muted = FALSE;

    Game_ResetGame();
}

void Game_ResetGame(void) {
    int r, c;
    for (r = 0; r < MAX_BOARD; r++) {
        for (c = 0; c < MAX_BOARD; c++) {
            g_game.board[r][c] = CELL_EMPTY;
        }
    }

    g_game.is_first_move = TRUE;
    Game_UpdatePlayerThemes();

    int n = g_game.board_size;

    /* Starting piece placements (Four corners) */
    g_game.board[0][0]         = CELL_RED;
    g_game.board[n - 1][n - 1] = CELL_RED;
    g_game.board[0][n - 1]     = CELL_BLUE;
    g_game.board[n - 1][0]     = CELL_BLUE;

    /* Obstacles */
    if (g_game.obstacle_setting == OBS_SOME) {
        g_game.board[n / 2][n / 2] = CELL_OBSTACLE;
    } else if (g_game.obstacle_setting == OBS_MORE) {
        g_game.board[n / 2][n / 2] = CELL_OBSTACLE;
        g_game.board[n / 2 - 1][n / 2 - 1] = CELL_OBSTACLE;
        g_game.board[n / 2 + 1][n / 2 + 1] = CELL_OBSTACLE;
    } else if (g_game.obstacle_setting == OBS_MADNESS) {
        g_game.board[n / 2][n / 2] = CELL_OBSTACLE;
        g_game.board[n / 2 - 1][n / 2] = CELL_OBSTACLE;
        g_game.board[n / 2 + 1][n / 2] = CELL_OBSTACLE;
        g_game.board[n / 2][n / 2 - 1] = CELL_OBSTACLE;
        g_game.board[n / 2][n / 2 + 1] = CELL_OBSTACLE;
    }

    /* Santa Egg present obstacles */
    if (g_game.char_red == CHAR_SANTA || g_game.char_blue == CHAR_SANTA) {
        g_game.board[1][1] = CELL_PRESENT_OBSTACLE;
        g_game.board[n - 2][n - 2] = CELL_PRESENT_OBSTACLE;
    }

    g_game.current_player = CELL_RED;
    g_game.scores[CELL_RED] = 2;
    g_game.scores[CELL_BLUE] = 2;

    g_game.has_selected = FALSE;
    g_game.selected_r = -1;
    g_game.selected_c = -1;

    g_game.game_timer = GAME_TIME_DEFAULT;
    g_game.turn_timer = g_game.turn_timer_setting;
    g_game.game_over = FALSE;
    g_game.effect_locked = FALSE;
    g_game.hastur_clone_count = 0;
    g_game.whisper_triggered = FALSE;
    g_game.blindness_triggered = FALSE;

    g_game.grimoire_count = 0;
    Game_AddStatus("The Duel of Eldritch Spawn Begins...", FALSE);

    if (g_game.char_red != CHAR_DEFAULT) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Active Deity: %s (%s)", g_secrets[g_game.char_red].name, g_secrets[g_game.char_red].title);
        Game_AddStatus(buf, FALSE);
    }
}

int Game_GetValidMoveType(int from_r, int from_c, int to_r, int to_c) {
    int n = g_game.board_size;
    if (from_r < 0 || from_r >= n || from_c < 0 || from_c >= n) return MOVE_NONE;
    if (to_r < 0 || to_r >= n || to_c < 0 || to_c >= n) return MOVE_NONE;

    if (g_game.board[to_r][to_c] != CELL_EMPTY && g_game.board[to_r][to_c] != CELL_PRESENT_OBSTACLE) {
        return MOVE_NONE;
    }

    int dr = to_r - from_r;
    int dc = to_c - from_c;
    if (dr < 0) dr = -dr;
    if (dc < 0) dc = -dc;

    int max_d = (dr > dc) ? dr : dc;
    if (max_d == 1) return MOVE_CLONE;
    if (max_d == 2) return MOVE_LEAP;
    return MOVE_NONE;
}

int Game_GetAllValidMoves(int player, GameMove *out_moves, int max_moves) {
    int count = 0;
    int n = g_game.board_size;
    int fr, fc, tr, tc;

    for (fr = 0; fr < n; fr++) {
        for (fc = 0; fc < n; fc++) {
            if (g_game.board[fr][fc] != player) continue;

            for (tr = fr - 2; tr <= fr + 2; tr++) {
                for (tc = fc - 2; tc <= fc + 2; tc++) {
                    int mtype = Game_GetValidMoveType(fr, fc, tr, tc);
                    if (mtype != MOVE_NONE) {
                        if (count < max_moves) {
                            out_moves[count].from_r = fr;
                            out_moves[count].from_c = fc;
                            out_moves[count].to_r = tr;
                            out_moves[count].to_c = tc;
                            out_moves[count].type = mtype;
                            out_moves[count].captures = 0;
                            count++;
                        }
                    }
                }
            }
        }
    }
    return count;
}

static int CountAdjacentOpponents(int r, int c, int opponent) {
    int dr, dc, count = 0;
    int n = g_game.board_size;
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr;
            int nc = c + dc;
            if (nr >= 0 && nr < n && nc >= 0 && nc < n) {
                if (g_game.board[nr][nc] == opponent) {
                    count++;
                }
            }
        }
    }
    return count;
}

static int ConvertAdjacentOpponents(int r, int c, int player) {
    int opponent = (player == CELL_RED) ? CELL_BLUE : CELL_RED;
    int dr, dc, count = 0;
    int n = g_game.board_size;

    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr;
            int nc = c + dc;
            if (nr >= 0 && nr < n && nc >= 0 && nc < n) {
                if (g_game.board[nr][nc] == opponent) {
                    g_game.board[nr][nc] = player;
                    g_game.scores[player]++;
                    g_game.scores[opponent]--;
                    count++;
                }
            }
        }
    }
    return count;
}

int Game_MakeMove(int from_r, int from_c, int to_r, int to_c, int move_type) {
    int player = g_game.current_player;
    int opponent = (player == CELL_RED) ? CELL_BLUE : CELL_RED;
    int captures = 0;
    int n = g_game.board_size;

    /* Abhoth Annihilation Check */
    if ((player == CELL_RED && g_game.char_blue == CHAR_ABHOTH) ||
        (player == CELL_BLUE && g_game.char_red == CHAR_ABHOTH)) {
        int abhoth_adj = CountAdjacentOpponents(to_r, to_c, opponent);
        if (abhoth_adj >= 6) {
            if (move_type == MOVE_LEAP) {
                g_game.board[from_r][from_c] = CELL_EMPTY;
                g_game.scores[player]--;
            }
            Game_AddStatus("Abhoth's Annihilation! Piece consumed by primordial sludge!", FALSE);
            Sound_PlaySFX("anomaly.wav");
            return 0;
        }
    }

    /* Core Move Execution */
    if (move_type == MOVE_CLONE) {
        g_game.board[to_r][to_c] = player;
        g_game.scores[player]++;
        if (player == CELL_RED) g_game.hastur_clone_count++;
    } else if (move_type == MOVE_LEAP) {
        g_game.board[to_r][to_c] = player;
        g_game.board[from_r][from_c] = CELL_EMPTY;
        if (player == CELL_RED) g_game.hastur_clone_count = 0;
    }

    /* Conversions */
    captures = ConvertAdjacentOpponents(to_r, to_c, player);

    /* Sound Effects */
    if (player == CELL_RED) {
        if (captures >= 3) {
            Sound_PlaySFX("player_big_capture.wav");
        } else if (captures > 0) {
            Sound_PlaySFX("place.wav");
        } else {
            Sound_PlaySFX("place.wav");
        }
    } else {
        if (captures >= 3) {
            Sound_PlaySFX("big_capture.wav");
        } else {
            Sound_PlaySFX("enemyplace.wav");
        }
    }

    /* Grimoire Log */
    char move_msg[128];
    const char *pname = (player == CELL_RED) ? g_game.player_name_red : g_game.player_name_blue;
    if (captures > 0) {
        snprintf(move_msg, sizeof(move_msg), "%s %s to (%d,%d) converting %d %s!",
            pname, (move_type == MOVE_CLONE ? "cloned" : "leaped"),
            to_c + 1, to_r + 1, captures, (captures == 1 ? "horror" : "horrors"));
    } else {
        snprintf(move_msg, sizeof(move_msg), "%s %s into void (%d,%d)",
            pname, (move_type == MOVE_CLONE ? "sprouted" : "warped"),
            to_c + 1, to_r + 1);
    }
    Game_AddStatus(move_msg, FALSE);

    /* --- DEITY LORE TRIGGERS --- */

    /* Hastur: 4 Clones summon a bonus Red horror */
    if (player == CELL_RED && g_game.char_red == CHAR_HASTUR && g_game.hastur_clone_count >= 4) {
        g_game.hastur_clone_count = 0;
        int empty_r[MAX_BOARD * MAX_BOARD], empty_c[MAX_BOARD * MAX_BOARD], empty_cnt = 0;
        int r, c;
        for (r = 0; r < n; r++) {
            for (c = 0; c < n; c++) {
                if (g_game.board[r][c] == CELL_EMPTY) {
                    empty_r[empty_cnt] = r;
                    empty_c[empty_cnt] = c;
                    empty_cnt++;
                }
            }
        }
        if (empty_cnt > 0) {
            int pick = fast_rand() % empty_cnt;
            g_game.board[empty_r[pick]][empty_c[pick]] = CELL_RED;
            g_game.scores[CELL_RED]++;
            Game_AddStatus("Hastur's Unspeakable Oath! A bonus Crimson Horror awakens!", FALSE);
            Sound_PlaySFX("player_special.wav");
        }
    }

    /* Rhan-Tegoth: Leap with 0 captures into cluster >= 5 freezes foes into permanent ice */
    if (player == CELL_RED && g_game.char_red == CHAR_RHAN_TEGOTH && move_type == MOVE_LEAP && captures == 0) {
        int cluster = 0, dr, dc;
        for (dr = -1; dr <= 1; dr++) {
            for (dc = -1; dc <= 1; dc++) {
                int nr = to_r + dr, nc = to_c + dc;
                if (nr >= 0 && nr < n && nc >= 0 && nc < n && g_game.board[nr][nc] == CELL_RED) {
                    cluster++;
                }
            }
        }
        if (cluster >= 5) {
            int frozen = 0;
            for (dr = -1; dr <= 1; dr++) {
                for (dc = -1; dc <= 1; dc++) {
                    int nr = to_r + dr, nc = to_c + dc;
                    if (nr >= 0 && nr < n && nc >= 0 && nc < n && g_game.board[nr][nc] == CELL_BLUE) {
                        g_game.board[nr][nc] = CELL_PERM_OBSTACLE;
                        g_game.scores[CELL_BLUE]--;
                        frozen++;
                    }
                }
            }
            if (frozen > 0) {
                char fmsg[128];
                snprintf(fmsg, sizeof(fmsg), "Rhan-Tegoth's Stasis! %d foes encased in eternal ice!", frozen);
                Game_AddStatus(fmsg, FALSE);
                Sound_PlaySFX("rhan.wav");
            }
        }
    }

    /* Yog-Sothoth: Leap capturing >= 2 seals surrounding cells with obstacles */
    if (player == CELL_RED && g_game.char_red == CHAR_YOG_SOTHOTH && move_type == MOVE_LEAP && captures >= 2) {
        int dr, dc, sealed = 0;
        for (dr = -1; dr <= 1; dr++) {
            for (dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                int nr = to_r + dr, nc = to_c + dc;
                if (nr >= 0 && nr < n && nc >= 0 && nc < n && g_game.board[nr][nc] == CELL_EMPTY) {
                    g_game.board[nr][nc] = CELL_OBSTACLE;
                    sealed++;
                }
            }
        }
        if (sealed > 0) {
            Game_AddStatus("Yog-Sothoth's Dimensional Toll! Surrounding cells sealed!", FALSE);
            Sound_PlaySFX("anomaly.wav");
        }
    }

    /* Cthulhu: Center and 4 corners occupied = Instant Win */
    if (player == CELL_RED && g_game.char_red == CHAR_CTHULHU) {
        int mid = n / 2;
        if (g_game.board[mid][mid] == CELL_RED &&
            g_game.board[0][0] == CELL_RED &&
            g_game.board[0][n - 1] == CELL_RED &&
            g_game.board[n - 1][0] == CELL_RED &&
            g_game.board[n - 1][n - 1] == CELL_RED) {
            g_game.game_over = TRUE;
            strcpy(g_game.winner_msg, "Cthulhu Awakens! Instant Cosmic Victory!");
            Game_AddStatus(g_game.winner_msg, FALSE);
            Sound_PlaySFX("win.wav");
            return captures;
        }
    }

    g_game.is_first_move = FALSE;
    return captures;
}

void Game_TriggerNextTurn(BOOL forced) {
    if (g_game.game_over) return;

    int next_player = (g_game.current_player == CELL_RED) ? CELL_BLUE : CELL_RED;

    GameMove next_moves[128];
    int count = Game_GetAllValidMoves(next_player, next_moves, 128);

    if (count > 0) {
        g_game.current_player = next_player;
        g_game.turn_timer = g_game.turn_timer_setting;
        g_game.has_selected = FALSE;
        g_game.selected_r = -1;
        g_game.selected_c = -1;
    } else {
        /* Opponent has no moves, check if current has moves */
        int cur_count = Game_GetAllValidMoves(g_game.current_player, next_moves, 128);
        if (cur_count > 0) {
            char skip_msg[128];
            snprintf(skip_msg, sizeof(skip_msg), "%s has no valid moves! Turn forfeited.",
                (next_player == CELL_RED ? g_game.player_name_red : g_game.player_name_blue));
            Game_AddStatus(skip_msg, FALSE);
            g_game.turn_timer = g_game.turn_timer_setting;
        } else {
            g_game.game_over = TRUE;
            Game_CheckGameOver();
        }
    }

    /* Ghroth's Orbit: 5% chance after Red turn to shuffle 3x3 zone */
    if (g_game.char_red == CHAR_GHROTH && g_game.current_player == CELL_BLUE && (fast_rand() % 100) < 5) {
        int n = g_game.board_size;
        int cr = 1 + (fast_rand() % (n - 2));
        int cc = 1 + (fast_rand() % (n - 2));
        int pieces[9], dr, dc, pidx = 0;

        for (dr = -1; dr <= 1; dr++) {
            for (dc = -1; dc <= 1; dc++) {
                pieces[pidx++] = g_game.board[cr + dr][cc + dc];
            }
        }
        /* Shuffle */
        for (pidx = 8; pidx > 0; pidx--) {
            int swap_idx = fast_rand() % (pidx + 1);
            int tmp = pieces[pidx];
            pieces[pidx] = pieces[swap_idx];
            pieces[swap_idx] = tmp;
        }
        pidx = 0;
        for (dr = -1; dr <= 1; dr++) {
            for (dc = -1; dc <= 1; dc++) {
                g_game.board[cr + dr][cc + dc] = pieces[pidx++];
            }
        }
        Game_AddStatus("Ghroth's Orbit! Gravitational chaos rearranged local spacetime!", FALSE);
        Sound_PlaySFX("anomaly.wav");
    }

    Game_CheckGameOver();
}

void Game_CheckGameOver(void) {
    if (g_game.game_over) return;

    if (g_game.scores[CELL_RED] == 0 || g_game.scores[CELL_BLUE] == 0) {
        g_game.game_over = TRUE;
    }

    int n = g_game.board_size;
    int r, c, empty_cnt = 0;
    for (r = 0; r < n; r++) {
        for (c = 0; c < n; c++) {
            if (g_game.board[r][c] == CELL_EMPTY) empty_cnt++;
        }
    }

    if (empty_cnt == 0) {
        g_game.game_over = TRUE;
    }

    if (g_game.game_over) {
        if (g_game.scores[CELL_RED] > g_game.scores[CELL_BLUE]) {
            snprintf(g_game.winner_msg, sizeof(g_game.winner_msg), "%s Prevails! Cosmic Dominance Achieved!", g_game.player_name_red);
            Sound_PlaySFX("win.wav");
        } else if (g_game.scores[CELL_BLUE] > g_game.scores[CELL_RED]) {
            snprintf(g_game.winner_msg, sizeof(g_game.winner_msg), "%s Awakens! Mortals Consumed by the Void!", g_game.player_name_blue);
            Sound_PlaySFX("big_capture.wav");
        } else {
            strcpy(g_game.winner_msg, "Stalemate! Both deities sink into slumber.");
        }
        Game_AddStatus(g_game.winner_msg, FALSE);
    }
}

void Game_HandleClick(int r, int c) {
    if (g_game.game_over || g_game.current_player != CELL_RED) return;
    int n = g_game.board_size;
    if (r < 0 || r >= n || c < 0 || c >= n) return;

    /* Nyarlathotep Crawling Chaos: 5% chance to divert click to neighbor */
    if (g_game.char_blue == CHAR_NYARLATHOTEP && g_game.board[r][c] == CELL_RED && (fast_rand() % 100) < 5) {
        int dr = (fast_rand() % 3) - 1;
        int dc = (fast_rand() % 3) - 1;
        int nr = r + dr, nc = c + dc;
        if (nr >= 0 && nr < n && nc >= 0 && nc < n && g_game.board[nr][nc] == CELL_RED) {
            r = nr; c = nc;
            Game_AddStatus("Nyarlathotep's Chaos hijacked piece selection!", FALSE);
            Sound_PlaySFX("anomaly.wav");
        }
    }

    if (g_game.has_selected) {
        if (r == g_game.selected_r && c == g_game.selected_c) {
            g_game.has_selected = FALSE;
            g_game.selected_r = -1;
            g_game.selected_c = -1;
            return;
        }

        if (g_game.board[r][c] == CELL_RED) {
            g_game.selected_r = r;
            g_game.selected_c = c;
            Sound_PlaySFX("select.wav");
            return;
        }

        int mtype = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
        if (mtype != MOVE_NONE) {
            Game_MakeMove(g_game.selected_r, g_game.selected_c, r, c, mtype);
            g_game.has_selected = FALSE;
            g_game.selected_r = -1;
            g_game.selected_c = -1;
            Game_TriggerNextTurn(FALSE);
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

/* --- STRONGARM SA-1110 HEURISTIC AI IMPLEMENTATION --- */

static int EvaluateMoveHeuristic(const GameMove *m, int opponent, int n) {
    int captures = CountAdjacentOpponents(m->to_r, m->to_c, opponent);
    int score = captures * 10;
    if (m->type == MOVE_CLONE) {
        score += 3; /* Cloning adds net +1 piece */
    }

    /* Edge and corner control */
    BOOL is_edge = (m->to_r == 0 || m->to_r == n - 1 || m->to_c == 0 || m->to_c == n - 1);
    if (is_edge) score += 2;

    return score;
}

BOOL Game_FindBestMoveGreedy(int player, GameMove *best_move) {
    GameMove moves[128];
    int count = Game_GetAllValidMoves(player, moves, 128);
    if (count == 0) return FALSE;

    int i, best_idx = 0, best_score = -9999;
    int opp = (player == CELL_RED) ? CELL_BLUE : CELL_RED;

    for (i = 0; i < count; i++) {
        int score = EvaluateMoveHeuristic(&moves[i], opp, g_game.board_size);
        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }
    *best_move = moves[best_idx];
    return TRUE;
}

void Game_MakeAIMove(void) {
    if (g_game.game_over || g_game.current_player != CELL_BLUE) return;

    /* Shoggoth Sprawl: 10% chance to execute up to 3 rapid clone moves */
    if (g_game.char_blue == CHAR_SHOGGOTH && (fast_rand() % 100) < 10) {
        GameMove clone_moves[128];
        int num_moves = Game_GetAllValidMoves(CELL_BLUE, clone_moves, 128);
        int i, sprawl_count = 0;

        Game_AddStatus("Shoggoth's Sprawl! Rapid amoebic budding expands across board!", FALSE);
        for (i = 0; i < num_moves && sprawl_count < 3; i++) {
            if (clone_moves[i].type == MOVE_CLONE) {
                Game_MakeMove(clone_moves[i].from_r, clone_moves[i].from_c, clone_moves[i].to_r, clone_moves[i].to_c, MOVE_CLONE);
                sprawl_count++;
            }
        }
        Game_TriggerNextTurn(FALSE);
        return;
    }

    GameMove moves[128];
    int count = Game_GetAllValidMoves(CELL_BLUE, moves, 128);
    if (count == 0) {
        Game_TriggerNextTurn(TRUE);
        return;
    }

    GameMove best_move = moves[0];

    if (g_game.difficulty == DIFF_EASY) {
        /* Mortal: 60% random move, 40% greedy */
        if ((fast_rand() % 100) < 60) {
            best_move = moves[fast_rand() % count];
        } else {
            int i, max_caps = -1;
            for (i = 0; i < count; i++) {
                int c = CountAdjacentOpponents(moves[i].to_r, moves[i].to_c, CELL_RED);
                if (c > max_caps) {
                    max_caps = c;
                    best_move = moves[i];
                }
            }
        }
    } else if (g_game.difficulty == DIFF_MEDIUM) {
        /* Elder: 1-pass positional heuristic (<0.2ms on StrongARM) */
        int i, best_score = -9999;
        for (i = 0; i < count; i++) {
            int score = EvaluateMoveHeuristic(&moves[i], CELL_RED, g_game.board_size);
            score += (fast_rand() % 3); /* human-like jitter */
            if (score > best_score) {
                best_score = score;
                best_move = moves[i];
            }
        }
    } else {
        /* Ancient One: 1-pass positional heuristic + leap threat check (<0.5ms on StrongARM) */
        int i, best_score = -9999;
        int n = g_game.board_size;

        for (i = 0; i < count; i++) {
            int score = EvaluateMoveHeuristic(&moves[i], CELL_RED, n);

            /* Check if opponent has nearby piece 2 squares away that could counter-leap */
            int dr, dc, threats = 0;
            for (dr = -2; dr <= 2; dr++) {
                for (dc = -2; dc <= 2; dc++) {
                    int tr = moves[i].to_r + dr;
                    int tc = moves[i].to_c + dc;
                    if (tr >= 0 && tr < n && tc >= 0 && tc < n) {
                        int mdr = (dr < 0) ? -dr : dr;
                        int mdc = (dc < 0) ? -dc : dc;
                        int dist = (mdr > mdc) ? mdr : mdc;
                        if (dist == 2 && g_game.board[tr][tc] == CELL_RED) {
                            threats++;
                        }
                    }
                }
            }
            score -= (threats * 2);

            if (score > best_score) {
                best_score = score;
                best_move = moves[i];
            }
        }
    }

    Game_MakeMove(best_move.from_r, best_move.from_c, best_move.to_r, best_move.to_c, best_move.type);
    Game_TriggerNextTurn(FALSE);
}

void Game_OnGameTimerTick(void) {
    if (g_game.game_over || g_game.time_distortion == 0.0f) return;

    if (g_game.game_timer > 0) {
        g_game.game_timer--;

        if (fast_rand() % 100 == 0) {
            Sound_PlaySFX("anomaly.wav");
        }
        if (g_game.game_timer <= 10 && g_game.game_timer > 0) {
            Sound_PlaySFX("tick.wav");
        }
        if (g_game.game_timer == 0) {
            g_game.game_over = TRUE;
            Game_AddStatus("Time Expired! The Cosmic Rift Closes!", FALSE);
            Game_CheckGameOver();
        }
    }
}

void Game_OnTurnTimerTick(void) {
    if (g_game.game_over || g_game.turn_timer_setting == TIMER_UNLIMITED) return;

    if (g_game.turn_timer > 0) {
        g_game.turn_timer--;
        if (g_game.turn_timer == 0) {
            GameMove auto_m;
            if (Game_FindBestMoveGreedy(g_game.current_player, &auto_m)) {
                Game_AddStatus("Turn Timer Expired! Auto-playing best move.", FALSE);
                Game_MakeMove(auto_m.from_r, auto_m.from_c, auto_m.to_r, auto_m.to_c, auto_m.type);
                Game_TriggerNextTurn(FALSE);
            } else {
                Game_TriggerNextTurn(TRUE);
            }
        }
    }
}

void Game_SetNames(const char *red_name, const char *blue_name) {
    if (red_name && red_name[0]) {
        strncpy(g_game.player_name_red, red_name, 63);
        g_game.player_name_red[63] = '\0';
    }
    if (blue_name && blue_name[0]) {
        strncpy(g_game.player_name_blue, blue_name, 63);
        g_game.player_name_blue[63] = '\0';
    }
    Game_UpdatePlayerThemes();
}
