#ifndef FTAGHN_GBC_GAME_H
#define FTAGHN_GBC_GAME_H

#include <gb/gb.h>

#define BOARD_SIZE 7

/* Board Cell Types */
#define CELL_EMPTY         0
#define CELL_RED           1
#define CELL_BLUE          2
#define CELL_OBSTACLE      3
#define CELL_PERM_OBSTACLE 4

/* Move Types */
#define MOVE_NONE  0
#define MOVE_CLONE 1
#define MOVE_LEAP  2

/* Difficulties */
#define DIFF_EASY   0
#define DIFF_MEDIUM 1
#define DIFF_HARD   2

/* Game States */
#define STATE_TITLE     0
#define STATE_PLAYING   1
#define STATE_GAME_OVER 2

typedef struct {
    uint8_t from_r;
    uint8_t from_c;
    uint8_t to_r;
    uint8_t to_c;
    uint8_t type;
} GameMove;

typedef struct {
    uint8_t board[BOARD_SIZE][BOARD_SIZE];
    uint8_t valid_moves_map[BOARD_SIZE][BOARD_SIZE]; /* 0: none, 1: clone, 2: leap */
    uint8_t current_player;
    uint8_t difficulty;
    uint8_t obstacle_mode;
    uint8_t score_red;
    uint8_t score_blue;
    uint8_t has_selected;
    uint8_t selected_r;
    uint8_t selected_c;
    uint8_t game_state;
    uint8_t winner;
    uint16_t game_timer_sec;
    uint8_t turn_timer_sec;
    char status_msg[24];
} GameState;

extern GameState g_game;

void Game_Init(void);
void Game_Reset(void);
uint8_t Game_GetValidMoveType(uint8_t from_r, uint8_t from_c, uint8_t to_r, uint8_t to_c);
void Game_ComputeValidMovesForPiece(uint8_t r, uint8_t c);
void Game_ClearValidMovesMap(void);
uint8_t Game_GetAllValidMoves(uint8_t player, GameMove *out_moves, uint8_t max_moves);
uint8_t Game_MakeMove(uint8_t from_r, uint8_t from_c, uint8_t to_r, uint8_t to_c, uint8_t type);
uint8_t Game_MakeAIMove(GameMove *out_move);
void Game_CheckGameOver(void);

#endif /* FTAGHN_GBC_GAME_H */

