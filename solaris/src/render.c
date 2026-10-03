#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Pixmap s_pixmap = 0;
static GC     s_gc = 0;
static XFontStruct *s_font_bold = NULL;
static XFontStruct *s_font_norm = NULL;

static unsigned long s_col_bg;
static unsigned long s_col_board_bg;
static unsigned long s_col_tile;
static unsigned long s_col_border;

static unsigned long s_col_red_body;
static unsigned long s_col_red_core;
static unsigned long s_col_red_dark;

static unsigned long s_col_blue_body;
static unsigned long s_col_blue_iris;
static unsigned long s_col_blue_dark;

static unsigned long s_col_white;
static unsigned long s_col_gold;
static unsigned long s_col_slate;
static unsigned long s_col_dim;

static unsigned long s_col_mono_base;
static unsigned long s_col_mono_light;
static unsigned long s_col_mono_shadow;
static unsigned long s_col_mono_rune;

static unsigned long s_col_clone;
static unsigned long s_col_leap;

static unsigned long AllocRGB(Display *dpy, Colormap cmap, int r, int g, int b) {
    XColor xc;
    xc.red   = (unsigned short)((r << 8) | r);
    xc.green = (unsigned short)((g << 8) | g);
    xc.blue  = (unsigned short)((b << 8) | b);
    xc.flags = DoRed | DoGreen | DoBlue;
    if (XAllocColor(dpy, cmap, &xc)) {
        return xc.pixel;
    }
    return (unsigned long)((r << 16) | (g << 8) | b);
}

void Render_Init(Display *dpy, Window win, int screen, int depth, Visual *visual, Colormap cmap) {
    if (s_pixmap == 0) {
        s_pixmap = XCreatePixmap(dpy, win, WINDOW_WIDTH, WINDOW_HEIGHT, depth);
    }
    if (s_gc == 0) {
        s_gc = XCreateGC(dpy, win, 0, NULL);
    }

    /* Load fonts with graceful fallback */
    s_font_bold = XLoadQueryFont(dpy, "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*");
    if (!s_font_bold) s_font_bold = XLoadQueryFont(dpy, "9x15");
    if (!s_font_bold) s_font_bold = XLoadQueryFont(dpy, "fixed");

    s_font_norm = XLoadQueryFont(dpy, "-*-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*");
    if (!s_font_norm) s_font_norm = XLoadQueryFont(dpy, "6x13");
    if (!s_font_norm) s_font_norm = XLoadQueryFont(dpy, "fixed");

    /* Allocate palettes */
    s_col_bg          = AllocRGB(dpy, cmap, 18, 19, 28);
    s_col_board_bg    = AllocRGB(dpy, cmap, 24, 26, 38);
    s_col_tile        = AllocRGB(dpy, cmap, 30, 32, 46);
    s_col_border      = AllocRGB(dpy, cmap, 45, 49, 66);

    s_col_red_body    = AllocRGB(dpy, cmap, 220, 38, 38);
    s_col_red_core    = AllocRGB(dpy, cmap, 255, 112, 67);
    s_col_red_dark    = AllocRGB(dpy, cmap, 136, 17, 17);

    s_col_blue_body   = AllocRGB(dpy, cmap, 37, 99, 235);
    s_col_blue_iris   = AllocRGB(dpy, cmap, 0, 229, 255);
    s_col_blue_dark   = AllocRGB(dpy, cmap, 26, 35, 126);

    s_col_white       = AllocRGB(dpy, cmap, 255, 255, 255);
    s_col_gold        = AllocRGB(dpy, cmap, 250, 204, 21);
    s_col_slate       = AllocRGB(dpy, cmap, 180, 180, 190);
    s_col_dim         = AllocRGB(dpy, cmap, 110, 110, 125);

    s_col_mono_base   = AllocRGB(dpy, cmap, 38, 50, 56);
    s_col_mono_light  = AllocRGB(dpy, cmap, 69, 90, 100);
    s_col_mono_shadow = AllocRGB(dpy, cmap, 16, 23, 26);
    s_col_mono_rune   = AllocRGB(dpy, cmap, 186, 104, 200);

    s_col_clone       = AllocRGB(dpy, cmap, 16, 185, 129);
    s_col_leap        = AllocRGB(dpy, cmap, 139, 92, 246);
}

void Render_Cleanup(Display *dpy) {
    if (s_pixmap) {
        XFreePixmap(dpy, s_pixmap);
        s_pixmap = 0;
    }
    if (s_gc) {
        XFreeGC(dpy, s_gc);
        s_gc = 0;
    }
    if (s_font_bold) {
        XFreeFont(dpy, s_font_bold);
        s_font_bold = NULL;
    }
    if (s_font_norm) {
        XFreeFont(dpy, s_font_norm);
        s_font_norm = NULL;
    }
}

int Render_GetCellAt(int x, int y, int *out_r, int *out_c) {
    int cx = x - BOARD_OFFSET_X;
    int cy = y - BOARD_OFFSET_Y;
    if (cx < 0 || cy < 0) return 0;

    int c = cx / CELL_SIZE;
    int r = cy / CELL_SIZE;

    if (r >= 0 && r < g_game.board_size && c >= 0 && c < g_game.board_size) {
        *out_r = r;
        *out_c = c;
        return 1;
    }
    return 0;
}

static void DrawHUD(Display *dpy) {
    /* Top banner background */
    XSetForeground(dpy, s_gc, s_col_bg);
    XFillRectangle(dpy, s_pixmap, s_gc, 0, 0, WINDOW_WIDTH, BOARD_OFFSET_Y);

    XSetForeground(dpy, s_gc, s_col_border);
    XDrawLine(dpy, s_pixmap, s_gc, 0, BOARD_OFFSET_Y - 1, WINDOW_WIDTH, BOARD_OFFSET_Y - 1);

    /* Red Pill (Score) */
    XSetForeground(dpy, s_gc, s_col_red_dark);
    XFillRectangle(dpy, s_pixmap, s_gc, 12, 8, 76, 26);
    XSetForeground(dpy, s_gc, s_col_red_core);
    XDrawRectangle(dpy, s_pixmap, s_gc, 12, 8, 76, 26);

    char red_str[32];
    sprintf(red_str, "YOU: %02d", g_game.scores[CELL_RED]);
    if (s_font_bold) XSetFont(dpy, s_gc, s_font_bold->fid);
    XSetForeground(dpy, s_gc, s_col_white);
    XDrawString(dpy, s_pixmap, s_gc, 24, 25, red_str, strlen(red_str));

    /* Center Status Banner */
    const char *status_str = "FTAGHN";
    unsigned long status_col = s_col_gold;
    if (g_game.game_over) {
        status_str = g_game.winner_msg;
        status_col = s_col_gold;
    } else if (g_game.current_player == CELL_RED) {
        status_str = "YOUR TURN (RED)";
        status_col = s_col_red_core;
    } else {
        status_str = "AI IS THINKING...";
        status_col = s_col_blue_iris;
    }

    int str_len = strlen(status_str);
    int text_w = s_font_bold ? XTextWidth(s_font_bold, status_str, str_len) : (str_len * 7);
    int center_x = (WINDOW_WIDTH - text_w) / 2;

    XSetForeground(dpy, s_gc, status_col);
    XDrawString(dpy, s_pixmap, s_gc, center_x, 25, status_str, str_len);

    /* Blue Pill (AI Score) */
    XSetForeground(dpy, s_gc, s_col_blue_dark);
    XFillRectangle(dpy, s_pixmap, s_gc, WINDOW_WIDTH - 88, 8, 76, 26);
    XSetForeground(dpy, s_gc, s_col_blue_iris);
    XDrawRectangle(dpy, s_pixmap, s_gc, WINDOW_WIDTH - 88, 8, 76, 26);

    char blue_str[32];
    sprintf(blue_str, "AI: %02d", g_game.scores[CELL_BLUE]);
    XSetForeground(dpy, s_gc, s_col_white);
    XDrawString(dpy, s_pixmap, s_gc, WINDOW_WIDTH - 76, 25, blue_str, strlen(blue_str));
}

static void DrawPiece(Display *dpy, int r, int c, int cell_type) {
    int px = BOARD_OFFSET_X + c * CELL_SIZE;
    int py = BOARD_OFFSET_Y + r * CELL_SIZE;

    if (cell_type == CELL_RED) {
        /* Crimson Spawn */
        XSetForeground(dpy, s_gc, s_col_red_dark);
        XFillArc(dpy, s_pixmap, s_gc, px + 4, py + 4, 40, 40, 0, 360 * 64);

        XSetForeground(dpy, s_gc, s_col_red_body);
        XFillArc(dpy, s_pixmap, s_gc, px + 7, py + 7, 34, 34, 0, 360 * 64);

        XSetForeground(dpy, s_gc, s_col_red_core);
        XFillArc(dpy, s_pixmap, s_gc, px + 14, py + 14, 20, 20, 0, 360 * 64);

        /* Dark abyss center */
        XSetForeground(dpy, s_gc, s_col_bg);
        XFillArc(dpy, s_pixmap, s_gc, px + 19, py + 18, 10, 12, 0, 360 * 64);

        /* Glint */
        XSetForeground(dpy, s_gc, s_col_white);
        XFillArc(dpy, s_pixmap, s_gc, px + 21, py + 19, 3, 3, 0, 360 * 64);

    } else if (cell_type == CELL_BLUE) {
        /* Abyssal Watcher */
        XSetForeground(dpy, s_gc, s_col_blue_dark);
        XFillArc(dpy, s_pixmap, s_gc, px + 4, py + 4, 40, 40, 0, 360 * 64);

        XSetForeground(dpy, s_gc, s_col_blue_body);
        XFillArc(dpy, s_pixmap, s_gc, px + 7, py + 7, 34, 34, 0, 360 * 64);

        XSetForeground(dpy, s_gc, s_col_blue_iris);
        XFillArc(dpy, s_pixmap, s_gc, px + 14, py + 14, 20, 20, 0, 360 * 64);

        /* Slit */
        XSetForeground(dpy, s_gc, s_col_bg);
        XFillArc(dpy, s_pixmap, s_gc, px + 21, py + 15, 6, 18, 0, 360 * 64);

        /* Glint */
        XSetForeground(dpy, s_gc, s_col_white);
        XFillArc(dpy, s_pixmap, s_gc, px + 22, py + 18, 3, 3, 0, 360 * 64);

    } else if (cell_type == CELL_OBSTACLE || cell_type == CELL_PERM_OBSTACLE) {
        /* Obsidian Monolith */
        XSetForeground(dpy, s_gc, s_col_mono_base);
        XFillRectangle(dpy, s_pixmap, s_gc, px + 4, py + 4, 40, 40);

        XSetForeground(dpy, s_gc, s_col_mono_light);
        XDrawLine(dpy, s_pixmap, s_gc, px + 4, py + 43, px + 4, py + 4);
        XDrawLine(dpy, s_pixmap, s_gc, px + 4, py + 4, px + 43, py + 4);

        XSetForeground(dpy, s_gc, s_col_mono_shadow);
        XDrawLine(dpy, s_pixmap, s_gc, px + 43, py + 5, px + 43, py + 43);
        XDrawLine(dpy, s_pixmap, s_gc, px + 5, py + 43, px + 43, py + 43);

        /* Arcane rune */
        XSetForeground(dpy, s_gc, s_col_mono_rune);
        XDrawLine(dpy, s_pixmap, s_gc, px + 16, py + 24, px + 32, py + 24);
        XDrawLine(dpy, s_pixmap, s_gc, px + 24, py + 16, px + 24, py + 32);
        XDrawLine(dpy, s_pixmap, s_gc, px + 18, py + 18, px + 30, py + 30);
        XDrawLine(dpy, s_pixmap, s_gc, px + 18, py + 30, px + 30, py + 18);
    }
}

static void DrawBoard(Display *dpy) {
    int r, c;

    /* Board backing */
    XSetForeground(dpy, s_gc, s_col_board_bg);
    XFillRectangle(dpy, s_pixmap, s_gc, BOARD_OFFSET_X, BOARD_OFFSET_Y,
                   g_game.board_size * CELL_SIZE, g_game.board_size * CELL_SIZE);

    /* Grid tiles */
    for (r = 0; r < g_game.board_size; r++) {
        for (c = 0; c < g_game.board_size; c++) {
            int px = BOARD_OFFSET_X + c * CELL_SIZE;
            int py = BOARD_OFFSET_Y + r * CELL_SIZE;

            XSetForeground(dpy, s_gc, s_col_tile);
            XFillRectangle(dpy, s_pixmap, s_gc, px, py, CELL_SIZE, CELL_SIZE);

            XSetForeground(dpy, s_gc, s_col_border);
            XDrawRectangle(dpy, s_pixmap, s_gc, px, py, CELL_SIZE, CELL_SIZE);

            int cell_type = g_game.board[r][c];
            if (cell_type != CELL_EMPTY) {
                DrawPiece(dpy, r, c, cell_type);
            }
        }
    }

    /* Outer border */
    XSetForeground(dpy, s_gc, s_col_slate);
    XDrawRectangle(dpy, s_pixmap, s_gc, BOARD_OFFSET_X, BOARD_OFFSET_Y,
                   g_game.board_size * CELL_SIZE, g_game.board_size * CELL_SIZE);

    /* Move highlights */
    if (g_game.has_selected && !g_game.game_over && g_game.current_player == CELL_RED) {
        int sr = g_game.selected_r;
        int sc = g_game.selected_c;

        /* Gold highlight on selected cell */
        int sx = BOARD_OFFSET_X + sc * CELL_SIZE;
        int sy = BOARD_OFFSET_Y + sr * CELL_SIZE;
        XSetForeground(dpy, s_gc, s_col_gold);
        XDrawRectangle(dpy, s_pixmap, s_gc, sx + 2, sy + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        XDrawRectangle(dpy, s_pixmap, s_gc, sx + 3, sy + 3, CELL_SIZE - 6, CELL_SIZE - 6);

        for (r = 0; r < g_game.board_size; r++) {
            for (c = 0; c < g_game.board_size; c++) {
                int move_type = Game_GetValidMoveType(sr, sc, r, c);
                if (move_type != MOVE_NONE) {
                    int tx = BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2;
                    int ty = BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2;

                    if (move_type == MOVE_CLONE) {
                        /* Emerald Spore circle */
                        XSetForeground(dpy, s_gc, s_col_clone);
                        XFillArc(dpy, s_pixmap, s_gc, tx - 6, ty - 6, 12, 12, 0, 360 * 64);
                    } else if (move_type == MOVE_LEAP) {
                        /* Amethyst Diamond */
                        XSetForeground(dpy, s_gc, s_col_leap);
                        XFillRectangle(dpy, s_pixmap, s_gc, tx - 6, ty - 6, 12, 12);
                    }
                }
            }
        }
    }
}

static void DrawGrimoire(Display *dpy) {
    int gy = BOARD_OFFSET_Y + g_game.board_size * CELL_SIZE + 8;
    int gh = WINDOW_HEIGHT - gy - 8;

    /* Panel background */
    XSetForeground(dpy, s_gc, s_col_bg);
    XFillRectangle(dpy, s_pixmap, s_gc, BOARD_OFFSET_X, gy, g_game.board_size * CELL_SIZE, gh);

    XSetForeground(dpy, s_gc, s_col_border);
    XDrawRectangle(dpy, s_pixmap, s_gc, BOARD_OFFSET_X, gy, g_game.board_size * CELL_SIZE, gh);

    int max_lines = (g_game.grimoire_count < 3) ? g_game.grimoire_count : 3;
    int line_y = gy + 14;
    int i;

    for (i = 0; i < max_lines; i++) {
        if (i == 0) {
            XSetForeground(dpy, s_gc, s_col_gold);
            if (s_font_bold) XSetFont(dpy, s_gc, s_font_bold->fid);
        } else if (i == 1) {
            XSetForeground(dpy, s_gc, s_col_slate);
            if (s_font_norm) XSetFont(dpy, s_gc, s_font_norm->fid);
        } else {
            XSetForeground(dpy, s_gc, s_col_dim);
            if (s_font_norm) XSetFont(dpy, s_gc, s_font_norm->fid);
        }

        const char *msg = g_game.grimoire[i].text;
        XDrawString(dpy, s_pixmap, s_gc, BOARD_OFFSET_X + 8, line_y, msg, strlen(msg));
        line_y += 14;
    }
}

void Render_Draw(Display *dpy, Window win) {
    /* Fill full background */
    XSetForeground(dpy, s_gc, s_col_bg);
    XFillRectangle(dpy, s_pixmap, s_gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    DrawHUD(dpy);
    DrawBoard(dpy);
    DrawGrimoire(dpy);

    /* Blit offscreen Pixmap to Window */
    XCopyArea(dpy, s_pixmap, win, s_gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0);
    XFlush(dpy);
}

