#include "render.h"
#include "game.h"
#include "bmp_loader.h"
#include "font.h"
#include "freestanding.h"

static int  s_screen_w = 480;
static int  s_screen_h = 640;
static bool s_is_qvga  = false;

static uint32_t s_backbuffer[SCREEN_MAX_W * SCREEN_MAX_H];

static HDC     s_hdc_mem = NULL;
static HBITMAP s_hbm_dib = NULL;
static HBITMAP s_hbm_old = NULL;
static void   *s_dib_bits = NULL;

static uint32_t s_bg_cache[SCREEN_MAX_W * SCREEN_MAX_H];
static char     s_loaded_bg[MAX_PATH] = "";
static bool     s_bg_loaded = false;

/* 5-way D-Pad Cursor */
static int s_cursor_r = 3;
static int s_cursor_c = 3;

static const uint32_t s_grim_colors[GRIMOIRE_MAX] = {
    0x00FACC15, /* Pos 0: Bright Gold */
    0x00D4D4D8, /* Pos 1: Silver */
    0x00A1A1AA, /* Pos 2: Light Gray */
    0x0071717A, /* Pos 3: Medium Gray */
    0x0052525B  /* Pos 4: Dark Gray */
};

int Render_GetWidth(void) {
    return s_screen_w;
}

int Render_GetHeight(void) {
    return s_screen_h;
}

BOOL Render_IsQVGA(void) {
    return s_is_qvga ? TRUE : FALSE;
}

/* --------------------------------------------------------------------------
 * Software Graphics Primitives (Resolution-Aware)
 * -------------------------------------------------------------------------- */
static void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > s_screen_w)  w = s_screen_w - x;
    if (y + h > s_screen_h) h = s_screen_h - y;
    if (w <= 0 || h <= 0) return;

    for (int dy = 0; dy < h; dy++) {
        uint32_t *row = s_backbuffer + (y + dy) * s_screen_w + x;
        for (int dx = 0; dx < w; dx++) {
            row[dx] = color;
        }
    }
}

static void gfx_draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    gfx_fill_rect(x, y, w, 1, color);
    gfx_fill_rect(x, y + h - 1, w, 1, color);
    gfx_fill_rect(x, y, 1, h, color);
    gfx_fill_rect(x + w - 1, y, 1, h, color);
}

static void gfx_draw_beveled_rect(int x, int y, int w, int h,
                                  uint32_t fill_color, uint32_t hi_color, uint32_t lo_color) {
    gfx_fill_rect(x, y, w, h, fill_color);
    gfx_fill_rect(x, y, w, 1, hi_color);
    gfx_fill_rect(x, y, 1, h, hi_color);
    gfx_fill_rect(x, y + h - 1, w, 1, lo_color);
    gfx_fill_rect(x + w - 1, y, 1, h, lo_color);
}

static void gfx_fill_circle(int cx, int cy, int radius, uint32_t color) {
    if (radius <= 0) return;
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        int py = cy + dy;
        if (py < 0 || py >= s_screen_h) continue;

        int dy2 = dy * dy;
        int dx_max = 0;
        while ((dx_max + 1) * (dx_max + 1) + dy2 <= r2) {
            dx_max++;
        }

        int x1 = cx - dx_max;
        int x2 = cx + dx_max;
        if (x1 < 0) x1 = 0;
        if (x2 >= s_screen_w) x2 = s_screen_w - 1;

        uint32_t *row = s_backbuffer + py * s_screen_w;
        for (int px = x1; px <= x2; px++) {
            row[px] = color;
        }
    }
}

static void gfx_draw_circle_outline(int cx, int cy, int radius, uint32_t color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        int pts[8][2] = {
            {cx + x, cy + y}, {cx + y, cy + x}, {cx - y, cy + x}, {cx - x, cy + y},
            {cx - x, cy - y}, {cx - y, cy - x}, {cx + y, cy - x}, {cx + x, cy - y}
        };
        for (int i = 0; i < 8; i++) {
            int px = pts[i][0];
            int py = pts[i][1];
            if (px >= 0 && px < s_screen_w && py >= 0 && py < s_screen_h) {
                s_backbuffer[py * s_screen_w + px] = color;
            }
        }
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

static void gfx_draw_char(int x, int y, char ch, uint32_t fg, uint32_t bg, int scale) {
    unsigned char c = (unsigned char)ch;
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font8x8_basic[c - 32];

    for (int gy = 0; gy < 8; gy++) {
        uint8_t bits = glyph[gy];
        for (int gx = 0; gx < 8; gx++) {
            bool on = (bits & (0x80 >> gx)) != 0;
            uint32_t col = on ? fg : bg;
            if (on || bg != 0) {
                if (scale <= 1) {
                    int px = x + gx;
                    int py = y + gy;
                    if (px >= 0 && px < s_screen_w && py >= 0 && py < s_screen_h) {
                        s_backbuffer[py * s_screen_w + px] = col;
                    }
                } else {
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            int px = x + gx * scale + sx;
                            int py = y + gy * scale + sy;
                            if (px >= 0 && px < s_screen_w && py >= 0 && py < s_screen_h) {
                                s_backbuffer[py * s_screen_w + px] = col;
                            }
                        }
                    }
                }
            }
        }
    }
}

static void gfx_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg, int scale) {
    if (!str) return;
    int cur_x = x;
    int step = 8 * scale;
    while (*str) {
        gfx_draw_char(cur_x, y, *str, fg, bg, scale);
        cur_x += step;
        str++;
    }
}

static void gfx_draw_string_centered(int x, int y, int w, const char *str, uint32_t fg, uint32_t bg, int scale) {
    if (!str) return;
    int len = (int)strlen(str);
    int text_w = len * 8 * scale;
    int cx = x + (w - text_w) / 2;
    if (cx < x) cx = x;
    gfx_draw_string(cx, y, str, fg, bg, scale);
}

/* --------------------------------------------------------------------------
 * Board Geometry Helpers (Dynamic QVGA 240x320 & VGA 480x640)
 * -------------------------------------------------------------------------- */
static void GetBoardMetrics(int *out_ox, int *out_oy, int *out_csize) {
    int n = g_game.board_size;
    if (s_is_qvga) {
        if (n >= 9) {
            *out_csize = 19;
            *out_ox = (s_screen_w - (n * 19)) / 2;
            *out_oy = 27;
        } else {
            *out_csize = 22;
            *out_ox = (s_screen_w - (n * 22)) / 2;
            *out_oy = 28;
        }
    } else {
        if (n >= 9) {
            *out_csize = 38;
            *out_ox = (s_screen_w - (n * 38)) / 2;
            *out_oy = 54;
        } else {
            *out_csize = 44;
            *out_ox = (s_screen_w - (n * 44)) / 2;
            *out_oy = 56;
        }
    }
}

BOOL Render_GetCellFromPoint(int x, int y, int *out_r, int *out_c) {
    int ox, oy, csize;
    int n = g_game.board_size;
    GetBoardMetrics(&ox, &oy, &csize);

    if (x >= ox && x < ox + (n * csize) && y >= oy && y < oy + (n * csize)) {
        *out_c = (x - ox) / csize;
        *out_r = (y - oy) / csize;
        s_cursor_r = *out_r;
        s_cursor_c = *out_c;
        return TRUE;
    }
    return FALSE;
}

int Render_GetButtonClicked(int x, int y) {
    if (s_is_qvga) {
        if (y >= 270 && y <= 296) {
            int bw = (s_screen_w - 16) / 4;
            int x1 = 4;
            int x2 = x1 + bw + 2;
            int x3 = x2 + bw + 2;
            int x4 = x3 + bw + 2;

            if (x >= x1 && x < x2) return BTN_NEW_GAME;
            if (x >= x2 && x < x3) return BTN_DIFF;
            if (x >= x3 && x < x4) return BTN_TOME;
            if (x >= x4 && x <= s_screen_w) return BTN_MUTE;
        }
    } else {
        if (y >= 538 && y <= 586) {
            if (x >= 10 && x <= 122)  return BTN_NEW_GAME;
            if (x >= 126 && x <= 238) return BTN_DIFF;
            if (x >= 242 && x <= 362) return BTN_TOME;
            if (x >= 366 && x <= 470) return BTN_MUTE;
        }
    }
    return BTN_NONE;
}

void Render_SetCursor(int r, int c) {
    s_cursor_r = r;
    s_cursor_c = c;
}

void Render_MoveCursor(int dr, int dc) {
    int n = g_game.board_size;
    s_cursor_r += dr;
    s_cursor_c += dc;
    if (s_cursor_r < 0) s_cursor_r = 0;
    if (s_cursor_r >= n) s_cursor_r = n - 1;
    if (s_cursor_c < 0) s_cursor_c = 0;
    if (s_cursor_c >= n) s_cursor_c = n - 1;
}

void Render_GetCursor(int *out_r, int *out_c) {
    *out_r = s_cursor_r;
    *out_c = s_cursor_c;
}

/* --------------------------------------------------------------------------
 * Render Initialization & Cleanup
 * -------------------------------------------------------------------------- */
BOOL Render_Init(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    if (!hdc) return FALSE;

    int cx = GetSystemMetrics(SM_CXSCREEN);
    int cy = GetSystemMetrics(SM_CYSCREEN);
    if (cx <= 0) cx = 240;
    if (cy <= 0) cy = 320;
    if (cx > SCREEN_MAX_W) cx = SCREEN_MAX_W;
    if (cy > SCREEN_MAX_H) cy = SCREEN_MAX_H;

    s_screen_w = cx;
    s_screen_h = cy;
    s_is_qvga  = (s_screen_w <= 320);

    s_hdc_mem = CreateCompatibleDC(hdc);

    struct {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[3];
    } bmi;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = s_screen_w;
    bmi.bmiHeader.biHeight = -s_screen_h; /* Top-down DIB */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 16;
    bmi.bmiHeader.biCompression = BI_BITFIELDS;

    DWORD *masks = (DWORD *)bmi.bmiColors;
    masks[0] = 0xF800; /* Red 5 */
    masks[1] = 0x07E0; /* Green 6 */
    masks[2] = 0x001F; /* Blue 5 */

    s_hbm_dib = CreateDIBSection(hdc, (BITMAPINFO *)&bmi, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
    if (!s_hbm_dib || !s_dib_bits) {
        ReleaseDC(hwnd, hdc);
        return FALSE;
    }

    s_hbm_old = (HBITMAP)SelectObject(s_hdc_mem, s_hbm_dib);
    ReleaseDC(hwnd, hdc);

    memset(s_backbuffer, 0, sizeof(s_backbuffer));
    memset(s_bg_cache, 0, sizeof(s_bg_cache));
    s_loaded_bg[0] = '\0';
    s_bg_loaded = false;

    return TRUE;
}

void Render_Cleanup(void) {
    if (s_hdc_mem) {
        if (s_hbm_old) SelectObject(s_hdc_mem, s_hbm_old);
        if (s_hbm_dib) DeleteObject(s_hbm_dib);
        DeleteDC(s_hdc_mem);
        s_hdc_mem = NULL;
    }
}

/* --------------------------------------------------------------------------
 * Drawing Core Components
 * -------------------------------------------------------------------------- */
static void DrawBackground(void) {
    if (strcmp(s_loaded_bg, g_game.current_bg_filename) != 0) {
        WCHAR path[MAX_PATH];
        WCHAR exe_path[MAX_PATH];
        DWORD len = GetModuleFileNameW(NULL, exe_path, MAX_PATH);
        if (len > 0) {
            int last_slash = -1;
            for (DWORD i = 0; i < len; i++) {
                if (exe_path[i] == L'\\' || exe_path[i] == L'/') last_slash = (int)i;
            }
            if (last_slash >= 0) exe_path[last_slash + 1] = L'\0';
            lstrcpyW(path, exe_path);
        } else {
            lstrcpyW(path, L"");
        }

        WCHAR rel_bg[MAX_PATH];
        ascii_to_wide(rel_bg, g_game.current_bg_filename, MAX_PATH);
        lstrcatW(path, rel_bg);

        s_bg_loaded = BMP_LoadScaled(path, s_bg_cache, s_screen_w, s_screen_h, s_screen_w);
        strncpy(s_loaded_bg, g_game.current_bg_filename, MAX_PATH - 1);
        s_loaded_bg[MAX_PATH - 1] = '\0';
    }

    if (s_bg_loaded) {
        memcpy(s_backbuffer, s_bg_cache, s_screen_w * s_screen_h * sizeof(uint32_t));
    } else {
        /* Deep space eldritch void */
        gfx_fill_rect(0, 0, s_screen_w, s_screen_h, 0x000C0E18);
        for (int i = 0; i < 80; i++) {
            int rx = (i * 97) % s_screen_w;
            int ry = (i * 131) % s_screen_h;
            s_backbuffer[ry * s_screen_w + rx] = 0x00404860;
        }
    }
}

static void DrawBoard(void) {
    int ox, oy, csize;
    int n = g_game.board_size;
    GetBoardMetrics(&ox, &oy, &csize);

    int margin = s_is_qvga ? (n >= 9 ? 1 : 2) : (n >= 9 ? 2 : 4);
    /* Outer board shadow & beveled frame */
    gfx_draw_beveled_rect(ox - margin, oy - margin, (n * csize) + (margin * 2), (n * csize) + (margin * 2),
                          0x00141624, 0x00555F7A, 0x000A0A12);

    int target_dot_r1 = s_is_qvga ? 4 : 6;
    int target_dot_r2 = s_is_qvga ? 2 : 3;
    int leap_r1 = s_is_qvga ? 5 : 8;
    int leap_r2 = s_is_qvga ? 4 : 7;

    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            int x = ox + c * csize;
            int y = oy + r * csize;
            int cx = x + csize / 2;
            int cy = y + csize / 2;
            int pradius = s_is_qvga ? ((csize / 2) - 2) : ((csize / 2) - 4);
            int cell = g_game.board[r][c];

            /* Tile background with subtle alternating stone shade */
            uint32_t tile_bg = ((r + c) % 2 == 0) ? 0x00242838 : 0x001C2030;
            uint32_t tile_hi = ((r + c) % 2 == 0) ? 0x00404860 : 0x00323850;
            uint32_t tile_lo = ((r + c) % 2 == 0) ? 0x0010121C : 0x000C0E16;

            gfx_draw_beveled_rect(x + 1, y + 1, csize - 2, csize - 2, tile_bg, tile_hi, tile_lo);

            /* Valid Move Targets */
            if (g_game.has_selected) {
                int mtype = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
                if (mtype == MOVE_CLONE) {
                    /* Clone (Distance 1): Glowing green target dot */
                    gfx_fill_circle(cx, cy, target_dot_r1, 0x0034D399);
                    gfx_fill_circle(cx, cy, target_dot_r2, 0x00A7F3D0);
                } else if (mtype == MOVE_LEAP) {
                    /* Leap (Distance 2): Glowing amber target ring */
                    gfx_draw_circle_outline(cx, cy, leap_r1, 0x00FBBF24);
                    gfx_draw_circle_outline(cx, cy, leap_r2, 0x00FDE68A);
                }
            }

            /* Draw Pieces */
            if (cell == CELL_RED) {
                /* Crimson Cult piece: glowing 3D orb with specular highlight */
                COLORREF cr = g_game.color_red;
                uint32_t col = ((GetRValue(cr)) << 16) | ((GetGValue(cr)) << 8) | (GetBValue(cr));
                uint32_t dark = (((col >> 16) & 0xFF) / 2 << 16) | (((col >> 8) & 0xFF) / 2 << 8) | ((col & 0xFF) / 2);

                gfx_fill_circle(cx, cy, pradius, dark);
                gfx_fill_circle(cx, cy, pradius - (s_is_qvga ? 1 : 2), col);
                int h_off = s_is_qvga ? 3 : (pradius / 3);
                int h_rad = s_is_qvga ? 2 : (pradius / 4);
                gfx_fill_circle(cx - h_off, cy - h_off, h_rad, 0x00FFFFFF);
            } else if (cell == CELL_BLUE) {
                /* Elder God AI piece: cosmic sapphire 3D orb */
                gfx_fill_circle(cx, cy, pradius, 0x000F2D8C);
                gfx_fill_circle(cx, cy, pradius - (s_is_qvga ? 1 : 2), 0x003282F5);
                int h_off = s_is_qvga ? 3 : (pradius / 3);
                int h_rad = s_is_qvga ? 2 : (pradius / 4);
                gfx_fill_circle(cx - h_off, cy - h_off, h_rad, 0x00C8EBFF);
            } else if (cell == CELL_OBSTACLE) {
                /* Stone Monolith with red eye */
                int hw = pradius - (s_is_qvga ? 1 : 2);
                gfx_draw_beveled_rect(cx - hw, cy - hw, hw * 2, hw * 2, 0x00454B55, 0x00788090, 0x00202228);
                gfx_fill_circle(cx, cy, s_is_qvga ? 3 : 5, 0x00DC1E1E);
                gfx_fill_circle(cx, cy, s_is_qvga ? 1 : 2, 0x00FFC8C8);
            } else if (cell == CELL_PERM_OBSTACLE) {
                /* Rhan-Tegoth Permanent Ice Block */
                int hw = pradius - (s_is_qvga ? 1 : 2);
                gfx_draw_beveled_rect(cx - hw, cy - hw, hw * 2, hw * 2, 0x0078D2F0, 0x00DCF5FF, 0x003C82AA);
                gfx_fill_rect(cx - hw + (s_is_qvga ? 2 : 3), cy - hw + (s_is_qvga ? 2 : 3),
                              hw * 2 - (s_is_qvga ? 4 : 6), s_is_qvga ? 1 : 2, 0x00FFFFFF);
            } else if (cell == CELL_PRESENT_OBSTACLE) {
                /* Santa Present Obstacle */
                int hw = pradius - (s_is_qvga ? 1 : 2);
                gfx_draw_beveled_rect(cx - hw, cy - hw, hw * 2, hw * 2, 0x00C81414, 0x00F55050, 0x00640A0A);
                gfx_fill_rect(cx - (s_is_qvga ? 1 : 2), cy - hw, s_is_qvga ? 2 : 4, hw * 2, 0x00FFD700);
                gfx_fill_rect(cx - hw, cy - (s_is_qvga ? 1 : 2), hw * 2, s_is_qvga ? 2 : 4, 0x00FFD700);
            }

            /* Selected Cell Aura */
            if (g_game.has_selected && r == g_game.selected_r && c == g_game.selected_c) {
                gfx_draw_circle_outline(cx, cy, pradius + (s_is_qvga ? 2 : 3), 0x00FFD700);
                gfx_draw_circle_outline(cx, cy, pradius + (s_is_qvga ? 1 : 2), 0x00FFF080);
            }

            /* 5-way D-Pad Cursor */
            if (r == s_cursor_r && c == s_cursor_c) {
                gfx_draw_rect(x + 1, y + 1, csize - 2, csize - 2, 0x00FFFF80);
            }
        }
    }
}

static void DrawHUD(void) {
    int ox, oy, csize;
    GetBoardMetrics(&ox, &oy, &csize);
    int n = g_game.board_size;
    int margin = s_is_qvga ? (n >= 9 ? 1 : 2) : (n >= 9 ? 2 : 4);
    int board_bottom = oy + (n * csize) + margin;
    const SecretInfo *sec = &g_secrets[g_game.char_red];

    if (s_is_qvga) {
        int hy = board_bottom + 2;
        int hh = (n >= 9) ? 30 : 32;
        gfx_draw_beveled_rect(4, hy, s_screen_w - 8, hh, 0x00141622, 0x00373C50, 0x000A0A10);

        /* Left: Crimson Cult */
        gfx_fill_circle(14, hy + 9, 4, 0x00EF4444);
        char buf_red[16];
        snprintf(buf_red, sizeof(buf_red), "R:%d", g_game.scores[CELL_RED]);
        gfx_draw_string(22, hy + 3, buf_red, 0x00FF6464, 0, 1);
        char p1_name[12];
        strncpy(p1_name, g_game.player_name_red, 8);
        p1_name[8] = '\0';
        gfx_draw_string(8, hy + 13, p1_name, 0x00C8A0A0, 0, 1);

        /* Center: Turn Status & Timer & Compact Deity */
        const char *status_text;
        uint32_t status_col;
        if (g_game.game_over) {
            status_text = "DUEL OVER";
            status_col = 0x00FFD700;
        } else if (g_game.current_player == CELL_RED) {
            status_text = "YOUR TURN";
            status_col = 0x00FF5050;
        } else {
            status_text = "AI THINK";
            status_col = 0x0064B4FF;
        }
        gfx_draw_string_centered(65, hy + 3, s_screen_w - 130, status_text, status_col, 0, 1);

        char s_status[32];
        if (g_game.time_distortion == 0.0f) {
            strcpy(s_status, "STASIS");
        } else {
            int m = g_game.game_timer / 60;
            int s = g_game.game_timer % 60;
            snprintf(s_status, sizeof(s_status), "%d:%02d", m, s);
        }
        gfx_draw_string_centered(65, hy + 12, s_screen_w - 130, s_status, 0x00C8C8D2, 0, 1);

        /* Compact Deity status line */
        char deity_buf[32];
        snprintf(deity_buf, sizeof(deity_buf), "[%s]", sec->name);
        deity_buf[24] = '\0';
        gfx_draw_string_centered(65, hy + 21, s_screen_w - 130, deity_buf, 0x00FFD250, 0, 1);

        /* Right: Elder God AI */
        gfx_fill_circle(s_screen_w - 14, hy + 9, 4, 0x003B82F6);
        char buf_blue[16];
        snprintf(buf_blue, sizeof(buf_blue), "B:%d", g_game.scores[CELL_BLUE]);
        int text_w_blue = (int)strlen(buf_blue) * 8;
        gfx_draw_string(s_screen_w - 22 - text_w_blue, hy + 3, buf_blue, 0x0078B4FF, 0, 1);
        gfx_draw_string(s_screen_w - 48, hy + 13, "Elder", 0x00A0B4DC, 0, 1);
    } else {
        int hy = board_bottom + 4;
        int hh = (n >= 9) ? 58 : 64;
        gfx_draw_beveled_rect(10, hy, 460, hh, 0x00141622, 0x00373C50, 0x000A0A10);

        /* Left: Crimson Cult */
        gfx_fill_circle(24, hy + 20, 8, 0x00EF4444);
        char buf_red[32];
        snprintf(buf_red, sizeof(buf_red), "RED: %d", g_game.scores[CELL_RED]);
        gfx_draw_string(38, hy + 6, buf_red, 0x00FF6464, 0, 2);
        gfx_draw_string(38, hy + 28, g_game.player_name_red, 0x00C8A0A0, 0, 1);

        /* Center: Turn Status & Timer & Deity */
        if (g_game.game_over) {
            gfx_draw_string_centered(160, hy + 6, 160, "DUEL CONCLUDED", 0x00FFD700, 0, 2);
        } else {
            if (g_game.current_player == CELL_RED) {
                gfx_draw_string_centered(160, hy + 6, 160, "YOUR TURN", 0x00FF5050, 0, 2);
            } else {
                gfx_draw_string_centered(160, hy + 6, 160, "AI THINKING...", 0x0064B4FF, 0, 2);
            }
        }

        char s_status[32];
        if (g_game.time_distortion == 0.0f) {
            strcpy(s_status, "TIME: STASIS");
        } else {
            int m = g_game.game_timer / 60;
            int s = g_game.game_timer % 60;
            snprintf(s_status, sizeof(s_status), "TIME: %d:%02d", m, s);
        }
        gfx_draw_string_centered(160, hy + 28, 160, s_status, 0x00C8C8D2, 0, 1);

        char deity_buf[64];
        snprintf(deity_buf, sizeof(deity_buf), "[%s: %s]", sec->name, sec->title);
        deity_buf[42] = '\0';
        gfx_draw_string_centered(10, hy + 44, 460, deity_buf, 0x00FFD250, 0, 1);

        /* Right: Elder God AI */
        gfx_fill_circle(456, hy + 20, 8, 0x003B82F6);
        char buf_blue[32];
        snprintf(buf_blue, sizeof(buf_blue), "BLUE: %d", g_game.scores[CELL_BLUE]);
        gfx_draw_string(340, hy + 6, buf_blue, 0x0078B4FF, 0, 2);
        gfx_draw_string(340, hy + 28, "Elder God AI", 0x00A0B4DC, 0, 1);
    }
}

static void DrawGrimoire(void) {
    int ox, oy, csize;
    GetBoardMetrics(&ox, &oy, &csize);
    int n = g_game.board_size;
    int margin = s_is_qvga ? (n >= 9 ? 1 : 2) : (n >= 9 ? 2 : 4);
    int board_bottom = oy + (n * csize) + margin;

    if (s_is_qvga) {
        int hy = board_bottom + 2;
        int hh = (n >= 9) ? 30 : 32;
        int gy = hy + hh + 2;
        int gh = 270 - gy; /* Fill up to just before buttons at y=272 */
        if (gh < 24) gh = 24;

        gfx_draw_beveled_rect(4, gy, s_screen_w - 8, gh, 0x00100E16, 0x003C324B, 0x0008080C);
        gfx_draw_string_centered(4, gy + 2, s_screen_w - 8, "- THE GRIMOIRE -", 0x00B4A0D2, 0, 1);

        int max_lines = (gh - 14) / 10;
        if (max_lines > 3) max_lines = 3;
        for (int i = 0; i < max_lines && i < g_game.grimoire_count; i++) {
            int line_y = gy + 12 + (i * 10);
            uint32_t col = s_grim_colors[i];
            char line[32];
            strncpy(line, g_game.grimoire[i].text, 27);
            line[27] = '\0';
            gfx_draw_string(8, line_y, line, col, 0, 1);
        }
    } else {
        int hy = board_bottom + 4;
        int hh = (n >= 9) ? 58 : 64;
        int gy = hy + hh + 4;
        int gh = 536 - gy; /* Fill up to buttons at y=540 */
        if (gh < 50) gh = 50;

        gfx_draw_beveled_rect(10, gy, 460, gh, 0x00100E16, 0x003C324B, 0x0008080C);
        gfx_draw_string_centered(10, gy + 4, 460, "--- THE GRIMOIRE ---", 0x00B4A0D2, 0, 1);

        int max_lines = (gh - 22) / 13;
        if (max_lines > GRIMOIRE_MAX) max_lines = GRIMOIRE_MAX;
        for (int i = 0; i < max_lines && i < g_game.grimoire_count; i++) {
            int line_y = gy + 18 + (i * 13);
            uint32_t col = s_grim_colors[i];
            gfx_draw_string(20, line_y, g_game.grimoire[i].text, col, 0, 1);
        }
    }
}

static void DrawButtons(void) {
    if (s_is_qvga) {
        const char *diff_names[] = { "MORTAL", "ELDER", "ANCIENT" };
        const char *snd_text = g_game.is_muted ? "MUTE" : "SOUND";
        int by = 272;
        int bh = 22;
        int bw = (s_screen_w - 16) / 4;

        /* Button 1: NEW GAME */
        int x1 = 4;
        gfx_draw_beveled_rect(x1, by, bw, bh, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(x1, by + 6, bw, "NEW", 0x00FFFFFF, 0, 1);

        /* Button 2: AI DIFF */
        int x2 = x1 + bw + 2;
        gfx_draw_beveled_rect(x2, by, bw, bh, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(x2, by + 6, bw, diff_names[g_game.difficulty % 3], 0x00FFFFFF, 0, 1);

        /* Button 3: TOME OF LORE */
        int x3 = x2 + bw + 2;
        gfx_draw_beveled_rect(x3, by, bw, bh, 0x00411E50, 0x008246A0, 0x001E0F28);
        gfx_draw_string_centered(x3, by + 6, bw, "TOME", 0x00FFE678, 0, 1);

        /* Button 4: SOUND */
        int x4 = x3 + bw + 2;
        gfx_draw_beveled_rect(x4, by, s_screen_w - 4 - x4, bh, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(x4, by + 6, s_screen_w - 4 - x4, snd_text, 0x00FFFFFF, 0, 1);
    } else {
        const char *diff_names[] = { "AI: MORTAL", "AI: ELDER", "AI: ANCIENT" };
        const char *snd_text = g_game.is_muted ? "SOUND: OFF" : "SOUND: ON";
        int by = 540;
        int bh = 44;

        /* Button 1: NEW GAME */
        gfx_draw_beveled_rect(10, by, 112, bh, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(10, by + 14, 112, "NEW GAME", 0x00FFFFFF, 0, 1);

        /* Button 2: AI DIFF */
        gfx_draw_beveled_rect(126, by, 112, bh, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(126, by + 14, 112, diff_names[g_game.difficulty % 3], 0x00FFFFFF, 0, 1);

        /* Button 3: TOME OF LORE */
        gfx_draw_beveled_rect(242, by, 120, bh, 0x00411E50, 0x008246A0, 0x001E0F28);
        gfx_draw_string_centered(242, by + 14, 120, "TOME OF LORE", 0x00FFE678, 0, 1);

        /* Button 4: SOUND */
        gfx_draw_beveled_rect(366, by, 104, bh, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(366, by + 14, 104, snd_text, 0x00FFFFFF, 0, 1);
    }
}

void Render_DrawFrame(void) {
    DrawBackground();
    DrawBoard();
    DrawHUD();
    DrawGrimoire();
    DrawButtons();
}

void Render_DrawTomeModal(int selected_secret) {
    if (selected_secret < 0 || selected_secret >= CHAR_MAX_COUNT) return;
    const SecretInfo *sec = &g_secrets[selected_secret];

    /* Dimmed backdrop */
    gfx_fill_rect(0, 0, s_screen_w, s_screen_h, 0x000E0A14);

    if (s_is_qvga) {
        /* Modal dialog frame */
        gfx_draw_beveled_rect(4, 26, s_screen_w - 8, 266, 0x001A1424, 0x00645078, 0x000D0A12);
        gfx_draw_rect(6, 28, s_screen_w - 12, 262, 0x00FFD700);

        /* Header */
        gfx_draw_string_centered(4, 30, s_screen_w - 8, "THE TOME OF FORBIDDEN LORE", 0x00FFD700, 0, 1);

        /* Deity Showcase Card */
        gfx_draw_beveled_rect(8, 42, s_screen_w - 16, 150, 0x00221A30, 0x00504064, 0x00110D18);

        /* Glowing Deity Orb */
        COLORREF cr = sec->color;
        uint32_t col = ((GetRValue(cr)) << 16) | ((GetGValue(cr)) << 8) | (GetBValue(cr));
        int orb_cx = s_screen_w / 2;
        gfx_fill_circle(orb_cx, 60, 14, col);
        gfx_fill_circle(orb_cx - 4, 60 - 4, 4, 0x00FFFFFF);

        /* Name & Title */
        gfx_draw_string_centered(8, 78, s_screen_w - 16, sec->name, 0x00FFD250, 0, 1);
        gfx_draw_string_centered(8, 88, s_screen_w - 16, sec->title, 0x00E0D0FF, 0, 1);

        /* Arcane Description Box */
        gfx_draw_beveled_rect(12, 100, s_screen_w - 24, 72, 0x00161020, 0x003A3048, 0x000B0810);
        gfx_draw_string(16, 104, "COSMIC LAW:", 0x00FFD700, 0, 1);

        char desc_line1[32], desc_line2[32], desc_line3[32];
        memset(desc_line1, 0, sizeof(desc_line1));
        memset(desc_line2, 0, sizeof(desc_line2));
        memset(desc_line3, 0, sizeof(desc_line3));
        strncpy(desc_line1, sec->description, 26);
        if (strlen(sec->description) > 26) {
            strncpy(desc_line2, sec->description + 26, 26);
        }
        if (strlen(sec->description) > 52) {
            strncpy(desc_line3, sec->description + 52, 26);
        }
        gfx_draw_string(16, 114, desc_line1, 0x00E6E6FA, 0, 1);
        if (desc_line2[0]) gfx_draw_string(16, 124, desc_line2, 0x00E6E6FA, 0, 1);
        if (desc_line3[0]) gfx_draw_string(16, 134, desc_line3, 0x00E6E6FA, 0, 1);

        char snd_buf[48];
        snprintf(snd_buf, sizeof(snd_buf), "AUDIO: %s", sec->sound_file);
        snd_buf[26] = '\0';
        gfx_draw_string(16, 154, snd_buf, 0x008CA0DC, 0, 1);

        /* Counter */
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "Elder Secret %d of %d", selected_secret + 1, CHAR_MAX_COUNT);
        gfx_draw_string_centered(8, 178, s_screen_w - 16, page_str, 0x00C8C8DC, 0, 1);

        /* Navigation Buttons: Y = 198..234 */
        /* Prev: X = 10..74 */
        gfx_draw_beveled_rect(10, 198, 64, 36, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(10, 198 + 13, 64, "< PREV", 0x00FFFFFF, 0, 1);

        /* Invoke: X = 78..162 */
        gfx_draw_beveled_rect(78, 198, 84, 36, 0x00781E28, 0x00D24650, 0x003C0F14);
        gfx_draw_string_centered(78, 198 + 13, 84, "INVOKE", 0x00FFD700, 0, 1);

        /* Next: X = 166..230 */
        gfx_draw_beveled_rect(166, 198, 64, 36, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(166, 198 + 13, 64, "NEXT >", 0x00FFFFFF, 0, 1);

        /* Resume: X = 24..216, Y = 240..276 */
        gfx_draw_beveled_rect(24, 240, 192, 36, 0x001E283C, 0x00465A82, 0x000F141E);
        gfx_draw_string_centered(24, 240 + 13, 192, "RESUME DUEL", 0x00E6E6FA, 0, 1);
    } else {
        /* Modal dialog frame */
        gfx_draw_beveled_rect(10, 54, 460, 530, 0x001A1424, 0x00645078, 0x000D0A12);
        gfx_draw_rect(14, 58, 452, 522, 0x00FFD700);

        /* Header */
        gfx_draw_string_centered(10, 66, 460, "THE TOME OF FORBIDDEN LORE", 0x00FFD700, 0, 2);
        gfx_draw_string_centered(10, 88, 460, "Awaken an Ancient One into the Dual Realm", 0x00C8C8DC, 0, 1);

        /* Deity Showcase Card */
        gfx_draw_beveled_rect(24, 108, 432, 330, 0x00221A30, 0x00504064, 0x00110D18);

        /* Glowing Deity Orb */
        COLORREF cr = sec->color;
        uint32_t col = ((GetRValue(cr)) << 16) | ((GetGValue(cr)) << 8) | (GetBValue(cr));
        gfx_fill_circle(240, 160, 36, col);
        gfx_fill_circle(240 - 10, 160 - 10, 9, 0x00FFFFFF);

        /* Name & Title */
        gfx_draw_string_centered(24, 206, 432, sec->name, 0x00FFD250, 0, 2);
        gfx_draw_string_centered(24, 230, 432, sec->title, 0x00E0D0FF, 0, 1);

        /* Arcane Description Box */
        gfx_draw_beveled_rect(36, 252, 408, 126, 0x00161020, 0x003A3048, 0x000B0810);
        gfx_draw_string(48, 264, "COSMIC LAW:", 0x00FFD700, 0, 1);

        /* Description with simple wrapping */
        char desc_line1[48], desc_line2[48];
        memset(desc_line1, 0, sizeof(desc_line1));
        memset(desc_line2, 0, sizeof(desc_line2));
        strncpy(desc_line1, sec->description, 46);
        if (strlen(sec->description) > 46) {
            strncpy(desc_line2, sec->description + 46, 46);
        }
        gfx_draw_string(48, 284, desc_line1, 0x00E6E6FA, 0, 1);
        if (desc_line2[0]) {
            gfx_draw_string(48, 302, desc_line2, 0x00E6E6FA, 0, 1);
        }

        char snd_buf[64];
        snprintf(snd_buf, sizeof(snd_buf), "THEME AUDIO: %s", sec->sound_file);
        gfx_draw_string(48, 336, snd_buf, 0x008CA0DC, 0, 1);

        /* Counter */
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "Elder Secret %d of %d", selected_secret + 1, CHAR_MAX_COUNT);
        gfx_draw_string_centered(24, 398, 432, page_str, 0x00C8C8DC, 0, 1);

        /* Navigation Buttons: Y = 448..494 */
        /* Prev */
        gfx_draw_beveled_rect(24, 448, 110, 46, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(24, 448 + 15, 110, "< PREV", 0x00FFFFFF, 0, 2);

        /* Invoke */
        gfx_draw_beveled_rect(144, 448, 192, 46, 0x00781E28, 0x00D24650, 0x003C0F14);
        gfx_draw_string_centered(144, 448 + 15, 192, "INVOKE DEITY", 0x00FFD700, 0, 2);

        /* Next */
        gfx_draw_beveled_rect(346, 448, 110, 46, 0x00282D3C, 0x005A6482, 0x0014161E);
        gfx_draw_string_centered(346, 448 + 15, 110, "NEXT >", 0x00FFFFFF, 0, 2);

        /* Resume: Y = 506..552 */
        gfx_draw_beveled_rect(120, 506, 240, 46, 0x001E283C, 0x00465A82, 0x000F141E);
        gfx_draw_string_centered(120, 506 + 15, 240, "RESUME DUEL", 0x00E6E6FA, 0, 2);
    }
}

void Render_Flip(HWND hwnd) {
    if (!s_hdc_mem || !s_dib_bits) return;

    /* Fast 32-bit ARGB to 16-bit RGB565 conversion for Intel 2700G Marathon blitter */
    uint16_t *dst16 = (uint16_t *)s_dib_bits;
    const uint32_t *src32 = s_backbuffer;
    int total_pixels = s_screen_w * s_screen_h;

    for (int i = 0; i < total_pixels; i++) {
        uint32_t c = src32[i];
        uint16_t r5 = (uint16_t)((c >> 19) & 0x1F);
        uint16_t g6 = (uint16_t)((c >> 10) & 0x3F);
        uint16_t b5 = (uint16_t)((c >> 3) & 0x1F);
        dst16[i] = (r5 << 11) | (g6 << 5) | b5;
    }

    HDC hdc = GetDC(hwnd);
    if (hdc) {
        BitBlt(hdc, 0, 0, s_screen_w, s_screen_h, s_hdc_mem, 0, 0, SRCCOPY);
        ReleaseDC(hwnd, hdc);
    }
}
