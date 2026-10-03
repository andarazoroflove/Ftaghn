#include "render.h"
#include "game.h"
#include <stdio.h>
#include <math.h>

static int s_screen_w = DEFAULT_WIDTH;
static int s_screen_h = DEFAULT_HEIGHT;

static HDC s_mem_dc = NULL;
static HBITMAP s_mem_bmp = NULL;
static HBITMAP s_old_bmp = NULL;

static HBITMAP s_runes_bmp = NULL;
static HBITMAP s_gwb_bmp = NULL;
static HBITMAP s_bg_bmp = NULL;
static char s_current_bg[MAX_PATH] = "";

static HFONT s_font_title = NULL;
static HFONT s_font_score = NULL;
static HFONT s_font_hud = NULL;
static HFONT s_font_grimoire[GRIMOIRE_MAX] = { NULL, NULL, NULL, NULL, NULL };

static COLORREF s_grimoire_colors[GRIMOIRE_MAX] = {
    RGB(250, 204, 21),  /* Yellow (pos 0) */
    RGB(180, 180, 180), /* Light Gray (pos 1) */
    RGB(130, 130, 130), /* Gray (pos 2) */
    RGB(90, 90, 90),    /* Dark Gray (pos 3) */
    RGB(60, 60, 60)     /* Faint Gray (pos 4) */
};

static void GetBoardLayout(int *out_x, int *out_y, int *out_cell_size) {
    int hud_h = 38;
    int grim_h = 62;
    int margin_y = 12;

    int max_h = s_screen_h - hud_h - grim_h - margin_y;
    int max_w = s_screen_w - 16;

    int cell_size = max_w / g_game.board_size;
    if (cell_size * g_game.board_size > max_h) {
        cell_size = max_h / g_game.board_size;
    }
    if (cell_size < 32) cell_size = 32;

    int board_w = cell_size * g_game.board_size;
    *out_cell_size = cell_size;
    *out_x = (s_screen_w - board_w) / 2;
    *out_y = hud_h + 8;
}

void Render_Init(HWND hwnd) {
    HDC screen_dc = GetDC(hwnd);

    s_mem_dc = CreateCompatibleDC(screen_dc);
    s_mem_bmp = CreateCompatibleBitmap(screen_dc, s_screen_w, s_screen_h);
    s_old_bmp = (HBITMAP)SelectObject(s_mem_dc, s_mem_bmp);

    ReleaseDC(hwnd, screen_dc);

    /* Load persistent bitmaps */
    s_runes_bmp = (HBITMAP)LoadImageA(NULL, "IMAGES\\runes.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    s_gwb_bmp = (HBITMAP)LoadImageA(NULL, "IMAGES\\gwb_dot.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    /* Fonts */
    s_font_title = CreateFontA(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    s_font_score = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    s_font_hud = CreateFontA(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");

    /* Grimoire fonts */
    s_font_grimoire[0] = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    s_font_grimoire[1] = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    s_font_grimoire[2] = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    s_font_grimoire[3] = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    s_font_grimoire[4] = CreateFontA(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
}

void Render_Resize(HWND hwnd, int width, int height) {
    if (width <= 0 || height <= 0) return;
    s_screen_w = width;
    s_screen_h = height;

    if (s_mem_dc) {
        if (s_old_bmp) {
            SelectObject(s_mem_dc, s_old_bmp);
            s_old_bmp = NULL;
        }
        if (s_mem_bmp) {
            DeleteObject(s_mem_bmp);
            s_mem_bmp = NULL;
        }
        HDC screen_dc = GetDC(hwnd);
        s_mem_bmp = CreateCompatibleBitmap(screen_dc, s_screen_w, s_screen_h);
        s_old_bmp = (HBITMAP)SelectObject(s_mem_dc, s_mem_bmp);
        ReleaseDC(hwnd, screen_dc);
    }
}

void Render_Cleanup(void) {
    int i;
    if (s_mem_dc && s_old_bmp) {
        SelectObject(s_mem_dc, s_old_bmp);
    }
    if (s_mem_bmp) DeleteObject(s_mem_bmp);
    if (s_mem_dc) DeleteDC(s_mem_dc);

    if (s_runes_bmp) DeleteObject(s_runes_bmp);
    if (s_gwb_bmp) DeleteObject(s_gwb_bmp);
    if (s_bg_bmp) DeleteObject(s_bg_bmp);

    if (s_font_title) DeleteObject(s_font_title);
    if (s_font_score) DeleteObject(s_font_score);
    if (s_font_hud) DeleteObject(s_font_hud);
    for (i = 0; i < GRIMOIRE_MAX; i++) {
        if (s_font_grimoire[i]) DeleteObject(s_font_grimoire[i]);
    }
}

void Render_UpdateBackground(const char *filename) {
    if (!filename || filename[0] == '\0') return;
    if (strcmp(s_current_bg, filename) == 0 && s_bg_bmp != NULL) return;

    if (s_bg_bmp) {
        DeleteObject(s_bg_bmp);
        s_bg_bmp = NULL;
    }
    strncpy(s_current_bg, filename, MAX_PATH - 1);
    s_bg_bmp = (HBITMAP)LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}

static void Draw3DSphere(HDC hdc, int cx, int cy, int radius, COLORREF color) {
    HBRUSH brush;
    HPEN pen;
    int spec_x, spec_y, spec_r;

    brush = CreateSolidBrush(color);
    pen = CreatePen(PS_SOLID, 1, RGB(20, 20, 30));
    SelectObject(hdc, brush);
    SelectObject(hdc, pen);

    Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);

    DeleteObject(brush);
    DeleteObject(pen);

    spec_r = radius / 3;
    if (spec_r < 2) spec_r = 2;
    spec_x = cx - radius / 3;
    spec_y = cy - radius / 3;

    brush = CreateSolidBrush(RGB(255, 255, 255));
    pen = (HPEN)GetStockObject(NULL_PEN);
    SelectObject(hdc, brush);
    SelectObject(hdc, pen);

    Ellipse(hdc, spec_x - spec_r, spec_y - spec_r / 2, spec_x + spec_r, spec_y + spec_r / 2);

    DeleteObject(brush);
}

static void DrawObstacle(HDC hdc, int x, int y, int size, int type) {
    RECT r;
    r.left = x + 3;
    r.top = y + 3;
    r.right = x + size - 3;
    r.bottom = y + size - 3;

    if (type == CELL_OBSTACLE) {
        HBRUSH brush = CreateSolidBrush(RGB(100, 110, 120));
        FillRect(hdc, &r, brush);
        DeleteObject(brush);
        DrawEdge(hdc, &r, EDGE_RAISED, BF_RECT);
    } else if (type == CELL_PERM_OBSTACLE) {
        HBRUSH brush = CreateSolidBrush(RGB(30, 190, 220));
        FillRect(hdc, &r, brush);
        DeleteObject(brush);
        DrawEdge(hdc, &r, EDGE_RAISED, BF_RECT);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(240, 255, 255));
        HPEN old_pen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, r.left + 4, r.top + 4, NULL);
        LineTo(hdc, r.right - 4, r.bottom - 4);
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
    } else if (type == CELL_PRESENT_OBSTACLE) {
        HBRUSH brush = CreateSolidBrush(RGB(220, 30, 30));
        RECT ribbon_v, ribbon_h;
        FillRect(hdc, &r, brush);
        DeleteObject(brush);
        DrawEdge(hdc, &r, EDGE_RAISED, BF_RECT);

        brush = CreateSolidBrush(RGB(34, 197, 94));
        ribbon_v.left = r.left + (r.right - r.left)/2 - 2;
        ribbon_v.right = ribbon_v.left + 4;
        ribbon_v.top = r.top;
        ribbon_v.bottom = r.bottom;
        FillRect(hdc, &ribbon_v, brush);

        ribbon_h.left = r.left;
        ribbon_h.right = r.right;
        ribbon_h.top = r.top + (r.bottom - r.top)/2 - 2;
        ribbon_h.bottom = ribbon_h.top + 4;
        FillRect(hdc, &ribbon_h, brush);
        DeleteObject(brush);
    }
}

void Render_Frame(HDC hdc, HWND hwnd) {
    RECT client_rect;
    int bx, by, cell_size, board_w, board_h;
    int r, c;
    HDC img_dc;
    HBITMAP old_img_bmp;
    char buf[128];
    int y_cursor;

    client_rect.left = 0;
    client_rect.top = 0;
    client_rect.right = s_screen_w;
    client_rect.bottom = s_screen_h;

    /* 1. Draw Background */
    HBRUSH bg_brush = CreateSolidBrush(RGB(26, 26, 46));
    FillRect(s_mem_dc, &client_rect, bg_brush);
    DeleteObject(bg_brush);

    img_dc = CreateCompatibleDC(s_mem_dc);

    /* Tile rune background */
    if (s_runes_bmp) {
        int tx, ty;
        old_img_bmp = (HBITMAP)SelectObject(img_dc, s_runes_bmp);
        for (ty = 0; ty < s_screen_h; ty += 128) {
            for (tx = 0; tx < s_screen_w; tx += 128) {
                BitBlt(s_mem_dc, tx, ty, 128, 128, img_dc, 0, 0, SRCCOPY);
            }
        }
        SelectObject(img_dc, old_img_bmp);
    }

    /* Calculate Board Layout */
    GetBoardLayout(&bx, &by, &cell_size);
    board_w = g_game.board_size * cell_size;
    board_h = board_w;

    /* 2. Score Bar (HUD) - Stretched to EXACTLY match the board width */
    RECT hud_rect = { bx, 4, bx + board_w, by - 6 };
    HBRUSH hud_brush = CreateSolidBrush(RGB(35, 40, 55));
    FillRect(s_mem_dc, &hud_rect, hud_brush);
    DeleteObject(hud_brush);
    DrawEdge(s_mem_dc, &hud_rect, EDGE_SUNKEN, BF_RECT);

    SetBkMode(s_mem_dc, TRANSPARENT);

    /* Red Player Info (Left side of score bar: Sphere -> Score -> Name) */
    Draw3DSphere(s_mem_dc, bx + 14, 18, 8, g_game.color_red);

    SelectObject(s_mem_dc, s_font_score);
    SetTextColor(s_mem_dc, RGB(250, 204, 21));
    sprintf(buf, "%d", g_game.scores[CELL_RED]);
    TextOutA(s_mem_dc, bx + 26, 7, buf, strlen(buf));

    SelectObject(s_mem_dc, s_font_hud);
    SetTextColor(s_mem_dc, RGB(255, 255, 255));
    RECT red_name_rect = { bx + 48, 10, bx + 120, 26 };
    DrawTextA(s_mem_dc, g_game.player_name_red, -1, &red_name_rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    /* Center Timers */
    SelectObject(s_mem_dc, s_font_hud);
    int minutes = g_game.game_timer / 60;
    int seconds = g_game.game_timer % 60;
    if (g_game.game_timer <= 30) {
        SetTextColor(s_mem_dc, RGB(239, 68, 68));
    } else if (g_game.game_timer <= 60) {
        SetTextColor(s_mem_dc, RGB(250, 204, 21));
    } else {
        SetTextColor(s_mem_dc, RGB(74, 222, 128));
    }
    sprintf(buf, "Time: %d:%02d", minutes, seconds);
    RECT timer_rect = { bx + board_w / 2 - 38, 5, bx + board_w / 2 + 38, 19 };
    DrawTextA(s_mem_dc, buf, -1, &timer_rect, DT_CENTER | DT_SINGLELINE);

    if (g_game.turn_timer_setting != TIMER_UNLIMITED) {
        SetTextColor(s_mem_dc, RGB(200, 200, 200));
        sprintf(buf, "Turn: %ds", g_game.turn_timer);
        RECT turn_rect = { bx + board_w / 2 - 38, 18, bx + board_w / 2 + 38, 31 };
        DrawTextA(s_mem_dc, buf, -1, &turn_rect, DT_CENTER | DT_SINGLELINE);
    }

    /* Blue Player Info (Right side of score bar: Name -> Score -> Sphere) */
    SelectObject(s_mem_dc, s_font_hud);
    SetTextColor(s_mem_dc, RGB(255, 255, 255));
    RECT blue_name_rect = { bx + board_w - 120, 10, bx + board_w - 48, 26 };
    DrawTextA(s_mem_dc, g_game.player_name_blue, -1, &blue_name_rect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

    SelectObject(s_mem_dc, s_font_score);
    SetTextColor(s_mem_dc, RGB(250, 204, 21));
    sprintf(buf, "%d", g_game.scores[CELL_BLUE]);
    SIZE sz;
    GetTextExtentPoint32A(s_mem_dc, buf, strlen(buf), &sz);
    TextOutA(s_mem_dc, bx + board_w - 26 - sz.cx, 7, buf, strlen(buf));

    Draw3DSphere(s_mem_dc, bx + board_w - 14, 18, 8, g_game.color_blue);

    /* Visual Dynamic Score Ratio Bar (Tug-of-war balance bar spanning the score bar) */
    {
        int bar_x = bx + 4;
        int bar_y = hud_rect.bottom - 6;
        int bar_w = board_w - 8;
        int bar_h = 4;
        int total = g_game.scores[CELL_RED] + g_game.scores[CELL_BLUE];
        if (total <= 0) total = 4;

        int red_w = (g_game.scores[CELL_RED] * bar_w) / total;
        if (red_w < 2 && g_game.scores[CELL_RED] > 0) red_w = 2;
        if (red_w > bar_w - 2 && g_game.scores[CELL_BLUE] > 0) red_w = bar_w - 2;

        RECT r_bar = { bar_x, bar_y, bar_x + red_w, bar_y + bar_h };
        RECT b_bar = { bar_x + red_w, bar_y, bar_x + bar_w, bar_y + bar_h };

        HBRUSH r_brush = CreateSolidBrush(g_game.color_red);
        HBRUSH b_brush = CreateSolidBrush(g_game.color_blue);

        FillRect(s_mem_dc, &r_bar, r_brush);
        FillRect(s_mem_dc, &b_bar, b_brush);

        DeleteObject(r_brush);
        DeleteObject(b_brush);
    }

    /* Board Outer Sunken Bevel */
    RECT board_outer = { bx - 1, by - 1, bx + board_w + 1, by + board_h + 1 };
    DrawEdge(s_mem_dc, &board_outer, EDGE_SUNKEN, BF_RECT);

    /* 3. Atmospheric Center Backdrop (Stretched to match the board) */
    Render_UpdateBackground(g_game.current_bg_filename);
    if (s_bg_bmp) {
        old_img_bmp = (HBITMAP)SelectObject(img_dc, s_bg_bmp);
        StretchBlt(s_mem_dc, bx, by, board_w, board_h, img_dc, 0, 0, 320, 240, SRCCOPY);
        SelectObject(img_dc, old_img_bmp);
    }

    /* 4. Board Grid & Pieces */
    for (r = 0; r < g_game.board_size; r++) {
        for (c = 0; c < g_game.board_size; c++) {
            int cx = bx + c * cell_size;
            int cy = by + r * cell_size;
            RECT cell_rect = { cx, cy, cx + cell_size, cy + cell_size };

            /* Cell Border */
            DrawEdge(s_mem_dc, &cell_rect, BDR_SUNKENINNER, BF_RECT);

            /* Selected Cell Highlight */
            if (g_game.has_selected && g_game.selected_r == r && g_game.selected_c == c) {
                HBRUSH sel_brush = CreateSolidBrush(RGB(250, 204, 21));
                FrameRect(s_mem_dc, &cell_rect, sel_brush);
                DeleteObject(sel_brush);
            }

            /* Valid Move Indicator */
            if (g_game.has_selected) {
                int move_type = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
                if (move_type != MOVE_NONE) {
                    int mid_x = cx + cell_size / 2;
                    int mid_y = cy + cell_size / 2;
                    HBRUSH move_brush = CreateSolidBrush(RGB(34, 197, 94));
                    HPEN move_pen = CreatePen(PS_SOLID, 1, RGB(22, 101, 52));
                    HBRUSH old_b = (HBRUSH)SelectObject(s_mem_dc, move_brush);
                    HPEN old_p = (HPEN)SelectObject(s_mem_dc, move_pen);

                    Ellipse(s_mem_dc, mid_x - 5, mid_y - 5, mid_x + 6, mid_y + 6);

                    SelectObject(s_mem_dc, old_b);
                    SelectObject(s_mem_dc, old_p);
                    DeleteObject(move_brush);
                    DeleteObject(move_pen);
                }
            }

            /* Cell Contents */
            int cell = g_game.board[r][c];
            if (cell == CELL_RED) {
                int mid_x = cx + cell_size / 2;
                int mid_y = cy + cell_size / 2;
                Draw3DSphere(s_mem_dc, mid_x, mid_y, cell_size / 2 - 4, g_game.color_red);

                /* If GWB mode active, draw GWB face overlay */
                if (g_game.char_red == CHAR_GWB && s_gwb_bmp) {
                    old_img_bmp = (HBITMAP)SelectObject(img_dc, s_gwb_bmp);
                    BitBlt(s_mem_dc, mid_x - 10, mid_y - 10, 20, 20, img_dc, 6, 6, SRCCOPY);
                    SelectObject(img_dc, old_img_bmp);
                }
            } else if (cell == CELL_BLUE) {
                int mid_x = cx + cell_size / 2;
                int mid_y = cy + cell_size / 2;
                Draw3DSphere(s_mem_dc, mid_x, mid_y, cell_size / 2 - 4, g_game.color_blue);
            } else if (cell == CELL_OBSTACLE || cell == CELL_PERM_OBSTACLE || cell == CELL_PRESENT_OBSTACLE) {
                DrawObstacle(s_mem_dc, cx, cy, cell_size, cell);
            }
        }
    }

    DeleteDC(img_dc);

    /* 5. Bottom Grimoire Status Chronicle */
    y_cursor = by + board_h + 6;
    for (r = 0; r < g_game.grimoire_count && r < GRIMOIRE_MAX; r++) {
        SelectObject(s_mem_dc, s_font_grimoire[r]);
        SetTextColor(s_mem_dc, s_grimoire_colors[r]);
        RECT grim_rect = { bx, y_cursor, bx + board_w, y_cursor + 14 };
        DrawTextA(s_mem_dc, g_game.grimoire[r].text, -1, &grim_rect, DT_CENTER | DT_SINGLELINE);
        y_cursor += (r == 0) ? 14 : 11;
    }

    /* 6. Blit entire finished frame to window screen */
    BitBlt(hdc, 0, 0, s_screen_w, s_screen_h, s_mem_dc, 0, 0, SRCCOPY);
}

void Render_BoardClick(int mouse_x, int mouse_y) {
    int bx, by, cell_size;
    GetBoardLayout(&bx, &by, &cell_size);

    if (mouse_x >= bx && mouse_x < bx + g_game.board_size * cell_size &&
        mouse_y >= by && mouse_y < by + g_game.board_size * cell_size) {
        int c = (mouse_x - bx) / cell_size;
        int r = (mouse_y - by) / cell_size;
        Game_HandleClick(r, c);
    }
}
