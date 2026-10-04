#include "render.h"
#include "game.h"
#include "bmp_loader.h"
#include "font.h"
#include "freestanding.h"

static uint32_t s_backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

static HDC     s_hdc_mem = NULL;
static HBITMAP s_hbm_dib = NULL;
static HBITMAP s_hbm_old = NULL;
static void   *s_dib_bits = NULL;

static uint32_t s_bg_cache[SCREEN_WIDTH * SCREEN_HEIGHT];
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

/* --------------------------------------------------------------------------
 * Software Graphics Primitives
 * -------------------------------------------------------------------------- */
static void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH)  w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    for (int dy = 0; dy < h; dy++) {
        uint32_t *row = s_backbuffer + (y + dy) * SCREEN_WIDTH + x;
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
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        int dy2 = dy * dy;
        int dx_max = 0;
        while ((dx_max + 1) * (dx_max + 1) + dy2 <= r2) {
            dx_max++;
        }

        int x1 = cx - dx_max;
        int x2 = cx + dx_max;
        if (x1 < 0) x1 = 0;
        if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;

        uint32_t *row = s_backbuffer + py * SCREEN_WIDTH;
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
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                s_backbuffer[py * SCREEN_WIDTH + px] = color;
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
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        s_backbuffer[py * SCREEN_WIDTH + px] = col;
                    }
                } else {
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            int px = x + gx * scale + sx;
                            int py = y + gy * scale + sy;
                            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                                s_backbuffer[py * SCREEN_WIDTH + px] = col;
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
 * Board Geometry Helpers (VGA 480x640)
 * -------------------------------------------------------------------------- */
static void GetBoardMetrics(int *out_ox, int *out_oy, int *out_csize) {
    int n = g_game.board_size;
    if (n >= 9) {
        *out_csize = 42;
        *out_ox = (SCREEN_WIDTH - (n * 42)) / 2;
        *out_oy = 8;
    } else {
        *out_csize = 52;
        *out_ox = (SCREEN_WIDTH - (n * 52)) / 2;
        *out_oy = 14;
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
    if (y >= 586 && y <= 632) {
        if (x >= 10 && x <= 122)  return BTN_NEW_GAME;
        if (x >= 126 && x <= 238) return BTN_DIFF;
        if (x >= 242 && x <= 362) return BTN_TOME;
        if (x >= 366 && x <= 470) return BTN_MUTE;
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

    s_hdc_mem = CreateCompatibleDC(hdc);

    struct {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[3];
    } bmi;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SCREEN_WIDTH;
    bmi.bmiHeader.biHeight = -SCREEN_HEIGHT; /* Top-down DIB */
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

        s_bg_loaded = BMP_LoadScaled(path, s_bg_cache, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH);
        strncpy(s_loaded_bg, g_game.current_bg_filename, MAX_PATH - 1);
        s_loaded_bg[MAX_PATH - 1] = '\0';
    }

    if (s_bg_loaded) {
        memcpy(s_backbuffer, s_bg_cache, sizeof(s_backbuffer));
    } else {
        /* Deep space eldritch void */
        gfx_fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x000C0E18);
        for (int i = 0; i < 80; i++) {
            int rx = (i * 97) % SCREEN_WIDTH;
            int ry = (i * 131) % SCREEN_HEIGHT;
            s_backbuffer[ry * SCREEN_WIDTH + rx] = 0x00404860;
        }
    }
}

static void DrawBoard(void) {
    int ox, oy, csize;
    int n = g_game.board_size;
    GetBoardMetrics(&ox, &oy, &csize);

    /* Outer board shadow & beveled frame */
    gfx_draw_beveled_rect(ox - 5, oy - 5, (n * csize) + 10, (n * csize) + 10,
                          0x00141624, 0x00555F7A, 0x000A0A12);

    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            int x = ox + c * csize;
            int y = oy + r * csize;
            int cx = x + csize / 2;
            int cy = y + csize / 2;
            int pradius = (csize / 2) - 4;
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
                    gfx_fill_circle(cx, cy, 6, 0x0034D399);
                    gfx_fill_circle(cx, cy, 3, 0x00A7F3D0);
                } else if (mtype == MOVE_LEAP) {
                    /* Leap (Distance 2): Glowing amber target ring */
                    gfx_draw_circle_outline(cx, cy, 8, 0x00FBBF24);
                    gfx_draw_circle_outline(cx, cy, 7, 0x00FDE68A);
                }
            }

            /* Draw Pieces */
            if (cell == CELL_RED) {
                /* Crimson Cult piece: glowing 3D orb with specular highlight */
                COLORREF cr = g_game.color_red;
                uint32_t col = ((GetRValue(cr)) << 16) | ((GetGValue(cr)) << 8) | (GetBValue(cr));
                uint32_t dark = (((col >> 16) & 0xFF) / 2 << 16) | (((col >> 8) & 0xFF) / 2 << 8) | ((col & 0xFF) / 2);

                gfx_fill_circle(cx, cy, pradius, dark);
                gfx_fill_circle(cx, cy, pradius - 2, col);
                gfx_fill_circle(cx - pradius / 3, cy - pradius / 3, pradius / 4, 0x00FFFFFF);
            } else if (cell == CELL_BLUE) {
                /* Elder God AI piece: cosmic sapphire 3D orb */
                gfx_fill_circle(cx, cy, pradius, 0x000F2D8C);
                gfx_fill_circle(cx, cy, pradius - 2, 0x003282F5);
                gfx_fill_circle(cx - pradius / 3, cy - pradius / 3, pradius / 4, 0x00C8EBFF);
            } else if (cell == CELL_OBSTACLE) {
                /* Stone Monolith with red eye */
                int hw = pradius - 2;
                gfx_draw_beveled_rect(cx - hw, cy - hw, hw * 2, hw * 2, 0x00454B55, 0x00788090, 0x00202228);
                gfx_fill_circle(cx, cy, 5, 0x00DC1E1E);
                gfx_fill_circle(cx, cy, 2, 0x00FFC8C8);
            } else if (cell == CELL_PERM_OBSTACLE) {
                /* Rhan-Tegoth Permanent Ice Block */
                int hw = pradius - 2;
                gfx_draw_beveled_rect(cx - hw, cy - hw, hw * 2, hw * 2, 0x0078D2F0, 0x00DCF5FF, 0x003C82AA);
                gfx_fill_rect(cx - hw + 3, cy - hw + 3, hw * 2 - 6, 2, 0x00FFFFFF);
            } else if (cell == CELL_PRESENT_OBSTACLE) {
                /* Santa Present Obstacle */
                int hw = pradius - 2;
                gfx_draw_beveled_rect(cx - hw, cy - hw, hw * 2, hw * 2, 0x00C81414, 0x00F55050, 0x00640A0A);
                gfx_fill_rect(cx - 2, cy - hw, 4, hw * 2, 0x00FFD700);
                gfx_fill_rect(cx - hw, cy - 2, hw * 2, 4, 0x00FFD700);
            }

            /* Selected Cell Aura */
            if (g_game.has_selected && r == g_game.selected_r && c == g_game.selected_c) {
                gfx_draw_circle_outline(cx, cy, pradius + 3, 0x00FFD700);
                gfx_draw_circle_outline(cx, cy, pradius + 2, 0x00FFF080);
            }

            /* 5-way D-Pad Cursor */
            if (r == s_cursor_r && c == s_cursor_c) {
                gfx_draw_rect(x + 2, y + 2, csize - 4, csize - 4, 0x00FFFF80);
            }
        }
    }
}

static void DrawHUD(void) {
    int hy = 384;
    int hh = 48;
    gfx_draw_beveled_rect(10, hy, 460, hh, 0x00141622, 0x00373C50, 0x000A0A10);

    /* Left: Crimson Cult */
    gfx_fill_circle(24, hy + 24, 9, 0x00EF4444);
    char buf_red[32];
    snprintf(buf_red, sizeof(buf_red), "RED: %d", g_game.scores[CELL_RED]);
    gfx_draw_string(40, hy + 8, buf_red, 0x00FF6464, 0, 2);
    gfx_draw_string(40, hy + 28, g_game.player_name_red, 0x00C8A0A0, 0, 1);

    /* Center: Turn Status & Timer */
    if (g_game.game_over) {
        gfx_draw_string_centered(170, hy + 8, 140, "DUEL CONCLUDED", 0x00FFD700, 0, 2);
    } else {
        if (g_game.current_player == CELL_RED) {
            gfx_draw_string_centered(170, hy + 8, 140, "YOUR TURN", 0x00FF5050, 0, 2);
        } else {
            gfx_draw_string_centered(170, hy + 8, 140, "AI THINKING...", 0x0064B4FF, 0, 2);
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
    gfx_draw_string_centered(170, hy + 28, 140, s_status, 0x00C8C8D2, 0, 1);

    /* Right: Elder God AI */
    gfx_fill_circle(456, hy + 24, 9, 0x003B82F6);
    char buf_blue[32];
    snprintf(buf_blue, sizeof(buf_blue), "BLUE: %d", g_game.scores[CELL_BLUE]);
    gfx_draw_string(340, hy + 8, buf_blue, 0x0078B4FF, 0, 2);
    gfx_draw_string(340, hy + 28, "Elder God AI", 0x00A0B4DC, 0, 1);
}

static void DrawGrimoire(void) {
    int gy = 438;
    int gh = 92;
    gfx_draw_beveled_rect(10, gy, 460, gh, 0x00100E16, 0x003C324B, 0x0008080C);
    gfx_draw_string_centered(10, gy + 5, 460, "--- THE GRIMOIRE ---", 0x00B4A0D2, 0, 1);

    for (int i = 0; i < GRIMOIRE_MAX && i < g_game.grimoire_count; i++) {
        int line_y = gy + 20 + (i * 13);
        uint32_t col = s_grim_colors[i];
        int scale = (i == 0) ? 1 : 1;
        gfx_draw_string(20, line_y, g_game.grimoire[i].text, col, 0, scale);
    }
}

static void DrawDeityBanner(void) {
    int by = 536;
    int bh = 44;
    const SecretInfo *sec = &g_secrets[g_game.char_red];

    gfx_draw_beveled_rect(10, by, 460, bh, 0x0016121C, 0x00463C55, 0x000A080E);

    char title_buf[80];
    snprintf(title_buf, sizeof(title_buf), "%s: %s", sec->name, sec->title);
    gfx_draw_string(20, by + 6, title_buf, 0x00FFD250, 0, 1);
    gfx_draw_string(20, by + 22, sec->description, 0x00D2D2DC, 0, 1);
}

static void DrawButtons(void) {
    const char *diff_names[] = { "AI: MORTAL", "AI: ELDER", "AI: ANCIENT" };
    const char *snd_text = g_game.is_muted ? "SOUND: OFF" : "SOUND: ON";

    /* Button 1: NEW GAME */
    gfx_draw_beveled_rect(10, 586, 112, 46, 0x00282D3C, 0x005A6482, 0x0014161E);
    gfx_draw_string_centered(10, 586 + 15, 112, "NEW GAME", 0x00FFFFFF, 0, 1);

    /* Button 2: AI DIFF */
    gfx_draw_beveled_rect(126, 586, 112, 46, 0x00282D3C, 0x005A6482, 0x0014161E);
    gfx_draw_string_centered(126, 586 + 15, 112, diff_names[g_game.difficulty % 3], 0x00FFFFFF, 0, 1);

    /* Button 3: TOME OF LORE */
    gfx_draw_beveled_rect(242, 586, 120, 46, 0x00411E50, 0x008246A0, 0x001E0F28);
    gfx_draw_string_centered(242, 586 + 15, 120, "TOME OF LORE", 0x00FFE678, 0, 1);

    /* Button 4: SOUND */
    gfx_draw_beveled_rect(366, 586, 104, 46, 0x00282D3C, 0x005A6482, 0x0014161E);
    gfx_draw_string_centered(366, 586 + 15, 104, snd_text, 0x00FFFFFF, 0, 1);
}

void Render_DrawFrame(void) {
    DrawBackground();
    DrawBoard();
    DrawHUD();
    DrawGrimoire();
    DrawDeityBanner();
    DrawButtons();
}

void Render_DrawTomeModal(int selected_secret) {
    if (selected_secret < 0 || selected_secret >= CHAR_MAX_COUNT) return;
    const SecretInfo *sec = &g_secrets[selected_secret];

    /* Dimmed backdrop */
    gfx_fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x000E0A14);

    /* Modal dialog frame */
    gfx_draw_beveled_rect(10, 10, 460, 620, 0x001A1424, 0x00645078, 0x000D0A12);
    gfx_draw_rect(14, 14, 452, 612, 0x00FFD700);

    /* Header */
    gfx_draw_string_centered(10, 26, 460, "THE TOME OF FORBIDDEN LORE", 0x00FFD700, 0, 2);
    gfx_draw_string_centered(10, 52, 460, "Awaken an Ancient One into the Dual Realm", 0x00C8C8DC, 0, 1);

    /* Deity Showcase Card */
    gfx_draw_beveled_rect(24, 80, 432, 380, 0x00221A30, 0x00504064, 0x00110D18);

    /* Glowing Deity Orb */
    COLORREF cr = sec->color;
    uint32_t col = ((GetRValue(cr)) << 16) | ((GetGValue(cr)) << 8) | (GetBValue(cr));
    gfx_fill_circle(240, 140, 40, col);
    gfx_fill_circle(240 - 12, 140 - 12, 10, 0x00FFFFFF);

    /* Name & Title */
    gfx_draw_string_centered(24, 196, 432, sec->name, 0x00FFD250, 0, 2);
    gfx_draw_string_centered(24, 222, 432, sec->title, 0x00E0D0FF, 0, 1);

    /* Arcane Description Box */
    gfx_draw_beveled_rect(36, 250, 408, 140, 0x00161020, 0x003A3048, 0x000B0810);
    gfx_draw_string(48, 266, "COSMIC LAW:", 0x00FFD700, 0, 1);

    /* Description with simple wrapping */
    char desc_line1[48], desc_line2[48];
    memset(desc_line1, 0, sizeof(desc_line1));
    memset(desc_line2, 0, sizeof(desc_line2));
    strncpy(desc_line1, sec->description, 46);
    if (strlen(sec->description) > 46) {
        strncpy(desc_line2, sec->description + 46, 46);
    }
    gfx_draw_string(48, 290, desc_line1, 0x00E6E6FA, 0, 1);
    if (desc_line2[0]) {
        gfx_draw_string(48, 308, desc_line2, 0x00E6E6FA, 0, 1);
    }

    char snd_buf[64];
    snprintf(snd_buf, sizeof(snd_buf), "THEME AUDIO: %s", sec->sound_file);
    gfx_draw_string(48, 350, snd_buf, 0x008CA0DC, 0, 1);

    /* Counter */
    char page_str[32];
    snprintf(page_str, sizeof(page_str), "Elder Secret %d of %d", selected_secret + 1, CHAR_MAX_COUNT);
    gfx_draw_string_centered(24, 430, 432, page_str, 0x00C8C8DC, 0, 1);

    /* Navigation Buttons */
    /* Prev */
    gfx_draw_beveled_rect(24, 480, 110, 50, 0x00282D3C, 0x005A6482, 0x0014161E);
    gfx_draw_string_centered(24, 480 + 17, 110, "< PREV", 0x00FFFFFF, 0, 2);

    /* Invoke */
    gfx_draw_beveled_rect(144, 480, 192, 50, 0x00781E28, 0x00D24650, 0x003C0F14);
    gfx_draw_string_centered(144, 480 + 17, 192, "INVOKE DEITY", 0x00FFD700, 0, 2);

    /* Next */
    gfx_draw_beveled_rect(346, 480, 110, 50, 0x00282D3C, 0x005A6482, 0x0014161E);
    gfx_draw_string_centered(346, 480 + 17, 110, "NEXT >", 0x00FFFFFF, 0, 2);

    /* Resume */
    gfx_draw_beveled_rect(120, 550, 240, 50, 0x001E283C, 0x00465A82, 0x000F141E);
    gfx_draw_string_centered(120, 550 + 17, 240, "RESUME DUEL", 0x00E6E6FA, 0, 2);
}

void Render_Flip(HWND hwnd) {
    if (!s_hdc_mem || !s_dib_bits) return;

    /* Fast 32-bit ARGB to 16-bit RGB565 conversion for Intel 2700G Marathon blitter */
    uint16_t *dst16 = (uint16_t *)s_dib_bits;
    const uint32_t *src32 = s_backbuffer;
    int total_pixels = SCREEN_WIDTH * SCREEN_HEIGHT;

    for (int i = 0; i < total_pixels; i++) {
        uint32_t c = src32[i];
        uint16_t r5 = (uint16_t)((c >> 19) & 0x1F);
        uint16_t g6 = (uint16_t)((c >> 10) & 0x3F);
        uint16_t b5 = (uint16_t)((c >> 3) & 0x1F);
        dst16[i] = (r5 << 11) | (g6 << 5) | b5;
    }

    HDC hdc = GetDC(hwnd);
    if (hdc) {
        BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, s_hdc_mem, 0, 0, SRCCOPY);
        ReleaseDC(hwnd, hdc);
    }
}
