#ifndef GAME_H
#define GAME_H

#include <PalmOS.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif

#ifndef COLORREF
typedef UInt32 COLORREF;
#endif

#ifndef RGB
#define RGB(r,g,b) (((UInt32)(r) << 16) | ((UInt32)(g) << 8) | ((UInt32)(b)))
#endif

#define MAX_BOARD 9
#define DEFAULT_BOARD 7

/* Board Cell Types */
#define CELL_EMPTY              0
#define CELL_RED                1
#define CELL_BLUE               2
#define CELL_OBSTACLE           3
#define CELL_PERM_OBSTACLE      4
#define CELL_PRESENT_OBSTACLE   5

/* Move Types */
#define MOVE_NONE   0
#define MOVE_CLONE  1
#define MOVE_LEAP   2

/* Difficulties (Optimized for 312 MHz XScale / Palm OS 5) */
#define DIFF_EASY   0 /* Mortal (Fast Random-Greedy) */
#define DIFF_MEDIUM 1 /* Elder (Positional Heuristic) */
#define DIFF_HARD   2 /* Ancient One (Tactical Anti-Threat) */

/* Obstacle Settings */
#define OBS_NONE    0
#define OBS_SOME    1
#define OBS_MORE    2
#define OBS_MADNESS 3

/* Turn Timer Settings */
#define TIMER_UNLIMITED 0
#define TIMER_30        30
#define TIMER_15        15
#define TIMER_5         5

#define GAME_TIME_DEFAULT 180 /* 3 minutes */
#define GRIMOIRE_MAX 5

/* Character & Secret Types for Tome of Forbidden Knowledge */
enum CharType {
    CHAR_DEFAULT = 0,
    CHAR_HASTUR,
    CHAR_RHAN_TEGOTH,
    CHAR_YOG_SOTHOTH,
    CHAR_GHROTH,
    CHAR_AZATHOTH,
    CHAR_ITHAQUA,
    CHAR_ABHOTH,
    CHAR_SHOGGOTH,
    CHAR_NYARLATHOTEP,
    CHAR_YIBB_TSTLL,
    CHAR_IDHA,
    CHAR_EIHORT,
    CHAR_SHUDDE_MELL,
    CHAR_CTHULHU,
    /* Mortal Easter Eggs */
    CHAR_GWB,
    CHAR_KLF,
    CHAR_XFILES,
    CHAR_NIN,
    CHAR_DOKTOR,
    CHAR_TKK,
    CHAR_SUPERNATURAL,
    CHAR_SANTA,
    CHAR_MAX_COUNT
};

typedef struct {
    enum CharType type;
    const char *name;
    const char *title;
    const char *description;
    const char *sound_file;
    const char *bg_file;
    COLORREF color;
} SecretInfo;

typedef struct {
    int from_r, from_c;
    int to_r, to_c;
    int type; /* MOVE_CLONE or MOVE_LEAP */
    int captures;
} GameMove;

typedef struct {
    char text[128];
} GrimoireEntry;

typedef struct {
    int board_size;
    int board[MAX_BOARD][MAX_BOARD];
    int current_player;
    int scores[3]; /* 1: RED (Player), 2: BLUE (AI) */

    int selected_r, selected_c;
    Boolean has_selected;

    int difficulty;
    int obstacle_setting;
    int turn_timer_setting;

    int game_timer;
    int turn_timer;
    float time_distortion;
    Boolean game_over;
    Boolean is_first_move;

    char player_name_red[64];
    char player_name_blue[64];
    enum CharType char_red;
    enum CharType char_blue;
    COLORREF color_red;
    COLORREF color_blue;

    int hastur_clone_count;
    char current_bg_filename[MAX_PATH];
    Boolean has_special_bg;

    GrimoireEntry grimoire[GRIMOIRE_MAX];
    int grimoire_count;

    char winner_msg[128];
    Boolean is_muted;
} GameState;

extern GameState g_game;
extern const SecretInfo g_secrets[CHAR_MAX_COUNT];

#ifdef __cplusplus
extern "C" {
#endif

void Game_Init(void);
void Game_ResetGame(void);
void Game_UpdatePlayerThemes(void);
void Game_ApplySecret(enum CharType secret, int player);

void Game_AddStatus(const char *msg, Boolean is_temporary);
int  Game_GetValidMoveType(int from_r, int from_c, int to_r, int to_c);
int  Game_GetAllValidMoves(int player, GameMove *out_moves, int max_moves);

int  Game_MakeMove(int from_r, int from_c, int to_r, int to_c, int move_type);
void Game_TriggerNextTurn(Boolean forced);
void Game_CheckGameOver(void);

void Game_MakeAIMove(void);
Boolean Game_FindBestMoveGreedy(int player, GameMove *best_move);

void Game_OnGameTimerTick(void);
void Game_OnTurnTimerTick(void);

void Game_HandleClick(int r, int c);

#ifdef __cplusplus
}
#endif

#endif /* GAME_H */
