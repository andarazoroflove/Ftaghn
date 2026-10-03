#include "game.h"
#include <string.h>
#include <stdlib.h>
#include <rand.h>

GameState g_game;

static uint8_t CountAdjacentOpponents(uint8_t r, uint8_t c, uint8_t opp) {
    int8_t dr, dc;
    uint8_t count = 0;
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            int8_t nr, nc;
            if (dr == 0 && dc == 0) continue;
            nr = (int8_t)r + dr;
            nc = (int8_t)c + dc;
            if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE) {
                if (g_game.board[nr][nc] == opp) {
                    count++;
                }
            }
        }
    }
    return count;
}

static uint8_t IsCorner(uint8_t r, uint8_t c) {
    return (r == 0 || r == BOARD_SIZE - 1) && (c == 0 || c == BOARD_SIZE - 1);
}

static uint8_t IsEdge(uint8_t r, uint8_t c) {
    return (r == 0 || r == BOARD_SIZE - 1 || c == 0 || c == BOARD_SIZE - 1);
}

void Game_Init(void) {
    g_game.difficulty = DIFF_MEDIUM;
    g_game.obstacle_mode = 0;
    Game_Reset();
}

void Game_Reset(void) {
    memset(g_game.board, 0, sizeof(g_game.board));
    Game_ClearValidMovesMap();

    /* Corners */
    g_game.board[0][0] = CELL_RED;
    g_game.board[BOARD_SIZE - 1][BOARD_SIZE - 1] = CELL_RED;
    g_game.board[0][BOARD_SIZE - 1] = CELL_BLUE;
    g_game.board[BOARD_SIZE - 1][0] = CELL_BLUE;

    /* Optional obstacles */
    if (g_game.obstacle_mode) {
        /* Balanced symmetric monoliths */
        g_game.board[3][2] = CELL_OBSTACLE;
        g_game.board[3][4] = CELL_OBSTACLE;
        g_game.board[2][3] = CELL_OBSTACLE;
        g_game.board[4][3] = CELL_OBSTACLE;
    }

    g_game.score_red = 2;
    g_game.score_blue = 2;
    g_game.current_player = CELL_RED;
    g_game.has_selected = 0;
    g_game.game_state = STATE_PLAYING;
    g_game.winner = 0;
    g_game.game_timer_sec = 180;
    g_game.turn_timer_sec = 0;

    strcpy(g_game.status_msg, "YOUR TURN (RED)");
}

uint8_t Game_GetValidMoveType(uint8_t from_r, uint8_t from_c, uint8_t to_r, uint8_t to_c) {
    int8_t dr, dc, mdr, mdc, dist;
    if (to_r >= BOARD_SIZE || to_c >= BOARD_SIZE) return MOVE_NONE;
    if (g_game.board[to_r][to_c] != CELL_EMPTY) return MOVE_NONE;

    dr = (int8_t)to_r - (int8_t)from_r;
    dc = (int8_t)to_c - (int8_t)from_c;
    mdr = (dr < 0) ? -dr : dr;
    mdc = (dc < 0) ? -dc : dc;
    dist = (mdr > mdc) ? mdr : mdc;

    if (dist == 1) return MOVE_CLONE;
    if (dist == 2) return MOVE_LEAP;
    return MOVE_NONE;
}

void Game_ClearValidMovesMap(void) {
    memset(g_game.valid_moves_map, 0, sizeof(g_game.valid_moves_map));
}

void Game_ComputeValidMovesForPiece(uint8_t r, uint8_t c) {
    int8_t dr, dc;
    Game_ClearValidMovesMap();
    if (g_game.board[r][c] != g_game.current_player) return;

    for (dr = -2; dr <= 2; dr++) {
        for (dc = -2; dc <= 2; dc++) {
            int8_t tr = (int8_t)r + dr;
            int8_t tc = (int8_t)c + dc;
            if (tr >= 0 && tr < BOARD_SIZE && tc >= 0 && tc < BOARD_SIZE) {
                uint8_t mtype = Game_GetValidMoveType(r, c, (uint8_t)tr, (uint8_t)tc);
                if (mtype != MOVE_NONE) {
                    g_game.valid_moves_map[tr][tc] = mtype;
                }
            }
        }
    }
}

uint8_t Game_GetAllValidMoves(uint8_t player, GameMove *out_moves, uint8_t max_moves) {
    uint8_t r, c;
    int8_t dr, dc;
    uint8_t count = 0;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (g_game.board[r][c] == player) {
                for (dr = -2; dr <= 2; dr++) {
                    for (dc = -2; dc <= 2; dc++) {
                        int8_t tr, tc;
                        uint8_t mtype;
                        if (dr == 0 && dc == 0) continue;
                        tr = (int8_t)r + dr;
                        tc = (int8_t)c + dc;
                        if (tr >= 0 && tr < BOARD_SIZE && tc >= 0 && tc < BOARD_SIZE) {
                            mtype = Game_GetValidMoveType(r, c, (uint8_t)tr, (uint8_t)tc);
                            if (mtype != MOVE_NONE) {
                                if (count < max_moves) {
                                    out_moves[count].from_r = r;
                                    out_moves[count].from_c = c;
                                    out_moves[count].to_r = (uint8_t)tr;
                                    out_moves[count].to_c = (uint8_t)tc;
                                    out_moves[count].type = mtype;
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return count;
}

uint8_t Game_MakeMove(uint8_t from_r, uint8_t from_c, uint8_t to_r, uint8_t to_c, uint8_t type) {
    uint8_t player = g_game.current_player;
    uint8_t opp = (player == CELL_RED) ? CELL_BLUE : CELL_RED;
    uint8_t captures = 0;
    int8_t dr, dc;

    if (type == MOVE_LEAP) {
        g_game.board[from_r][from_c] = CELL_EMPTY;
    }
    g_game.board[to_r][to_c] = player;

    /* Infect & convert adjacent enemy pieces */
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            int8_t nr = (int8_t)to_r + dr;
            int8_t nc = (int8_t)to_c + dc;
            if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE) {
                if (g_game.board[nr][nc] == opp) {
                    g_game.board[nr][nc] = player;
                    captures++;
                }
            }
        }
    }

    /* Recount pieces */
    {
        uint8_t r, c, r_count = 0, b_count = 0;
        for (r = 0; r < BOARD_SIZE; r++) {
            for (c = 0; c < BOARD_SIZE; c++) {
                if (g_game.board[r][c] == CELL_RED) r_count++;
                else if (g_game.board[r][c] == CELL_BLUE) b_count++;
            }
        }
        g_game.score_red = r_count;
        g_game.score_blue = b_count;
    }

    Game_ClearValidMovesMap();
    g_game.has_selected = 0;

    return captures;
}

static int16_t EvaluateMove(const GameMove *m) {
    uint8_t captures = CountAdjacentOpponents(m->to_r, m->to_c, CELL_RED);
    int16_t score = (int16_t)captures * 10;

    if (m->type == MOVE_CLONE) {
        score += 4;
    } else {
        if (captures == 0) score -= 4;
    }

    if (IsCorner(m->to_r, m->to_c)) score += 6;
    else if (IsEdge(m->to_r, m->to_c)) score += 2;

    return score;
}

uint8_t Game_MakeAIMove(GameMove *out_move) {
    GameMove moves[96];
    uint8_t count = Game_GetAllValidMoves(CELL_BLUE, moves, 96);
    uint8_t i, best_idx = 0;
    int16_t best_score = -9999;

    if (count == 0) return 0;

    if (g_game.difficulty == DIFF_EASY) {
        /* 60% random, 40% simple greedy */
        if ((rand() % 10) < 6) {
            *out_move = moves[rand() % count];
            return 1;
        }
        for (i = 0; i < count; i++) {
            uint8_t caps = CountAdjacentOpponents(moves[i].to_r, moves[i].to_c, CELL_RED);
            int16_t s = (int16_t)caps * 10 + (moves[i].type == MOVE_CLONE ? 2 : 0);
            if (s > best_score) {
                best_score = s;
                best_idx = i;
            }
        }
        *out_move = moves[best_idx];
        return 1;
    }

    if (g_game.difficulty == DIFF_MEDIUM) {
        for (i = 0; i < count; i++) {
            int16_t s = EvaluateMove(&moves[i]) + (rand() % 3);
            if (s > best_score) {
                best_score = s;
                best_idx = i;
            }
        }
        *out_move = moves[best_idx];
        return 1;
    }

    /* DIFF_HARD: Positional heuristic + distance-2 threat check */
    for (i = 0; i < count; i++) {
        int16_t s = EvaluateMove(&moves[i]);
        int8_t dr, dc;
        uint8_t threats = 0;

        for (dr = -2; dr <= 2; dr++) {
            for (dc = -2; dc <= 2; dc++) {
                int8_t tr = (int8_t)moves[i].to_r + dr;
                int8_t tc = (int8_t)moves[i].to_c + dc;
                if (tr >= 0 && tr < BOARD_SIZE && tc >= 0 && tc < BOARD_SIZE) {
                    int8_t mdr = (dr < 0) ? -dr : dr;
                    int8_t mdc = (dc < 0) ? -dc : dc;
                    int8_t dist = (mdr > mdc) ? mdr : mdc;
                    if (dist == 2 && g_game.board[tr][tc] == CELL_RED) {
                        threats++;
                    }
                }
            }
        }
        s -= (int16_t)threats * 2;

        if (s > best_score) {
            best_score = s;
            best_idx = i;
        }
    }
    *out_move = moves[best_idx];
    return 1;
}

void Game_CheckGameOver(void) {
    GameMove dummy[64];
    uint8_t red_moves, blue_moves;

    if (g_game.score_red == 0) {
        g_game.game_state = STATE_GAME_OVER;
        g_game.winner = CELL_BLUE;
        strcpy(g_game.status_msg, "BLUE HORROR WINS!");
        return;
    }
    if (g_game.score_blue == 0) {
        g_game.game_state = STATE_GAME_OVER;
        g_game.winner = CELL_RED;
        strcpy(g_game.status_msg, "RED VICTORY!");
        return;
    }

    red_moves = Game_GetAllValidMoves(CELL_RED, dummy, 64);
    blue_moves = Game_GetAllValidMoves(CELL_BLUE, dummy, 64);

    if (red_moves == 0 && blue_moves == 0) {
        g_game.game_state = STATE_GAME_OVER;
        if (g_game.score_red > g_game.score_blue) {
            g_game.winner = CELL_RED;
            strcpy(g_game.status_msg, "RED DOMINION!");
        } else if (g_game.score_blue > g_game.score_red) {
            g_game.winner = CELL_BLUE;
            strcpy(g_game.status_msg, "BLUE DEFEATS YOU!");
        } else {
            g_game.winner = 0;
            strcpy(g_game.status_msg, "COSMIC STALEMATE!");
        }
        return;
    }

    if (g_game.current_player == CELL_RED && red_moves == 0) {
        g_game.current_player = CELL_BLUE;
        strcpy(g_game.status_msg, "RED NO MOVES! PASS");
    } else if (g_game.current_player == CELL_BLUE && blue_moves == 0) {
        g_game.current_player = CELL_RED;
        strcpy(g_game.status_msg, "BLUE NO MOVES! PASS");
    }
}

