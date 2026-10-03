#ifndef FTAGHN_MAC68K_GAME_H
#define FTAGHN_MAC68K_GAME_H

#include <Types.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
typedef int BOOL;

typedef unsigned long COLORREF;
#ifndef RGB
#define RGB(r,g,b) ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
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

/* Difficulties */
#define DIFF_EASY   0 /* Simple */
#define DIFF_MEDIUM 1 /* Mortal */
#define DIFF_HARD   2 /* Elder Godlike */

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

/* Character Types for Special Easter Eggs */
enum CharType {
    CHAR_DEFAULT = 0,
    CHAR_HASTUR,
    CHAR_YOG_SOTHOTH,
    CHAR_GHROTH,
    CHAR_AZATHOTH,
    CHAR_ITHAQUA,
    CHAR_ABHOTH,
    CHAR_SHOGGOTH,
    CHAR_EIHORT,
    CHAR_IDHA,
    CHAR_SHUDDE_MELL,
    CHAR_RHAN_TEGOTH,
    CHAR_NYARLATHOTEP,
    CHAR_YIBB_TSTLL,
    CHAR_CTHULHU,
    CHAR_GWB,
    CHAR_DOKTOR,
    CHAR_KLF,
    CHAR_XFILES,
    CHAR_TKK,
    CHAR_SUPERNATURAL,
    CHAR_NIN,
    CHAR_SANTA
};

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
    int scores[3]; /* Index 1: RED, Index 2: BLUE */

    int selected_r, selected_c;
    BOOL has_selected;

    int game_timer;           /* seconds remaining */
    float time_distortion;    /* 1.0 = normal, 0.5 = slow, 2.0 = fast, 0.0 = stopped */
    int turn_timer;           /* turn countdown */
    int difficulty;
    int obstacle_setting;
    int turn_timer_setting;

    BOOL is_first_move;
    BOOL game_over;
    BOOL is_muted;
    BOOL effect_locked;
    int effect_delay_ticks;

    char player_name_red[64];
    char player_name_blue[64];
    COLORREF color_red;
    COLORREF color_blue;

    enum CharType char_red;
    enum CharType char_blue;

    int score_blindness_duration;
    int hastur_clone_count;
    int santa_move_count[3];
    BOOL whisper_triggered;
    BOOL blindness_triggered;
    BOOL ai_logic_torn;

    char current_bg_filename[MAX_PATH];
    BOOL has_special_bg;

    GrimoireEntry grimoire[GRIMOIRE_MAX];
    int grimoire_count;

    char winner_msg[128];
} GameState;

extern GameState g_game;

/* Core Game Functions */
void Game_Init(void);
void Game_ResetGame(void);
void Game_UpdatePlayerThemes(void);
int  Game_GetValidMoveType(int from_r, int from_c, int to_r, int to_c);
int  Game_GetAllValidMoves(int player, GameMove *out_moves, int max_moves);
int  Game_MakeMove(int from_r, int from_c, int to_r, int to_c, int move_type);
void Game_HandleClick(int r, int c);
void Game_TriggerNextTurn(BOOL forced);
void Game_CheckGameOver(void);
void Game_OnGameTimerTick(void);
void Game_OnTurnTimerTick(void);
void Game_AddStatus(const char *msg, BOOL is_temporary);
void Game_SetNames(const char *red_name, const char *blue_name);

/* AI Functions */
BOOL Game_FindBestMoveGreedy(int player, GameMove *best_move);
void Game_MakeAIMove(void);

#endif /* FTAGHN_MAC68K_GAME_H */
