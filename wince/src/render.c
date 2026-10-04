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

static bool    s_dib_bottom_up = false;
static bool    s_is_555 = false;
static bool    s_is_24bpp = false;

static uint32_t s_bg_cache[320 * 240];
static char     s_loaded_bg[MAX_PATH] = "";
static bool     s_bg_loaded = false;

static const uint32_t s_grim_colors[GRIMOIRE_MAX] = {
    0x00FACC15, /* Pos 0: Bright Gold */
    0x00BEBEC3, /* Pos 1: Silver */
    0x008C8C96, /* Pos 2: Gray */
    0x0064646E, /* Pos 3: Dim Gray */
    0x0041414B  /* Pos 4: Dark Gray */
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

static void gfx_fill_circle(int cx, int cy, int radius, uint32_t color) {
    if (radius <= 0) return;
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        int py = cy + dy;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        int dy2 = dy * dy;
        /* Find horizontal extent for this dy */
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
                    gfx_fill_rect(x + gx * scale, y + gy * scale, scale, scale, col);
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
        if (*str == '\n') {
            y += 9 * scale;
            cur_x = x;
        } else {
            gfx_draw_char(cur_x, y, *str, fg, bg, scale);
            cur_x += step;
        }
        str++;
    }
}

static void gfx_draw_button(int x, int y, int w, int h, const char *label, uint32_t bg_col, uint32_t text_col, int scale) {
    /* Button Body */
    gfx_fill_rect(x, y, w, h, bg_col);

    /* 3D Border: Highlight top/left, Shadow bottom/right */
    gfx_fill_rect(x, y, w, 1, 0x006E7596);
    gfx_fill_rect(x, y, 1, h, 0x006E7596);
    gfx_fill_rect(x, y + h - 1, w, 1, 0x001B1E28);
    gfx_fill_rect(x + w - 1, y, 1, h, 0x001B1E28);

    /* Centered Text */
    if (label) {
        int len = (int)strlen(label);
        int text_w = len * 8 * scale;
        int text_h = 8 * scale;
        int tx = x + (w - text_w) / 2;
        int ty = y + (h - text_h) / 2;
        gfx_draw_string(tx, ty, label, text_col, 0, scale);
    }
}

static void gfx_draw_orb(int cx, int cy, int radius, uint32_t color, BOOL is_klf, BOOL is_gwb) {
    /* Drop shadow */
    gfx_fill_circle(cx + 1, cy + 2, radius, 0x000A0A0F);

    /* Base sphere */
    gfx_fill_circle(cx, cy, radius, color);

    /* 3D Highlight Glint */
    int glint_r = radius / 3;
    if (glint_r < 2) glint_r = 2;
    uint32_t glint_col = is_klf ? 0x00FFE678 : 0x00FFFFFF;
    gfx_fill_circle(cx - radius / 3, cy - radius / 3, glint_r, glint_col);

    if (is_gwb) {
        /* Golden presidential star dot */
        gfx_fill_circle(cx, cy, 3, 0x00FFD700);
    }
}

/* --------------------------------------------------------------------------
 * Layout Coordinates
 * -------------------------------------------------------------------------- */
static void GetBoardLayout(int *out_ox, int *out_oy, int *out_cell_size) {
    int n = g_game.board_size;
    if (n == 9) {
        *out_cell_size = 25;
        *out_ox = (320 - (25 * 9)) / 2; /* 47 */
        *out_oy = (240 - (25 * 9)) / 2; /* 7 */
    } else {
        *out_cell_size = 32;
        *out_ox = (320 - (32 * 7)) / 2; /* 48 */
        *out_oy = (240 - (32 * 7)) / 2; /* 8 */
    }
}

BOOL Render_GetCellFromPoint(int x, int y, int *out_r, int *out_c) {
    int ox, oy, cell_size;
    GetBoardLayout(&ox, &oy, &cell_size);
    int n = g_game.board_size;
    int bw = cell_size * n;
    int bh = cell_size * n;

    if (x >= ox && x < ox + bw && y >= oy && y < oy + bh) {
        *out_c = (x - ox) / cell_size;
        *out_r = (y - oy) / cell_size;
        return TRUE;
    }
    return FALSE;
}

int Render_GetButtonClicked(int x, int y) {
    if (y >= 190 && y <= 234) {
        if (x >= 326 && x <= 398) return BTN_NEW_GAME;
        if (x >= 402 && x <= 470) return BTN_DIFF;
        if (x >= 474 && x <= 560) return BTN_TOME;
        if (x >= 564 && x <= 634) return BTN_MUTE;
    }
    return BTN_NONE;
}

/* --------------------------------------------------------------------------
 * Background Management
 * -------------------------------------------------------------------------- */
static void LoadBackground(void) {
    if (g_game.current_bg_filename[0] == '\0') {
        for (int y = 0; y < 240; y++) {
            uint32_t c = (y < 120) ? 0x000F1224 : 0x000A0D18;
            for (int x = 0; x < 320; x++) s_bg_cache[y * 320 + x] = c;
        }
        s_bg_loaded = false;
        s_loaded_bg[0] = '\0';
    } else if (!s_bg_loaded || strcmp(s_loaded_bg, g_game.current_bg_filename) != 0) {
        WCHAR exe_path[MAX_PATH];
        DWORD len = GetModuleFileNameW(NULL, exe_path, MAX_PATH);
        WCHAR full_bg_path[MAX_PATH];

        if (len > 0) {
            int last_slash = -1;
            for (DWORD i = 0; i < len; i++) {
                if (exe_path[i] == L'\\' || exe_path[i] == L'/') last_slash = (int)i;
            }
            if (last_slash >= 0) exe_path[last_slash + 1] = L'\0';

            WCHAR wrel[MAX_PATH];
            ascii_to_wide(wrel, g_game.current_bg_filename, MAX_PATH);
            lstrcpyW(full_bg_path, exe_path);
            lstrcatW(full_bg_path, wrel);
        } else {
            WCHAR wrel[MAX_PATH];
            ascii_to_wide(wrel, g_game.current_bg_filename, MAX_PATH);
            lstrcpyW(full_bg_path, L"\\Storage Card\\Ftaghn\\");
            lstrcatW(full_bg_path, wrel);
        }

        if (BMP_LoadToBuffer(full_bg_path, s_bg_cache, 320)) {
            s_bg_loaded = true;
            strncpy(s_loaded_bg, g_game.current_bg_filename, sizeof(s_loaded_bg) - 1);
            s_loaded_bg[sizeof(s_loaded_bg) - 1] = '\0';
        } else {
            s_bg_loaded = false;
            s_loaded_bg[0] = '\0';
            for (int y = 0; y < 240; y++) {
                uint32_t c = (y < 120) ? 0x000F1224 : 0x000A0D18;
                for (int x = 0; x < 320; x++) s_bg_cache[y * 320 + x] = c;
            }
        }
    }

    /* Restore pristine background pixels to left half of backbuffer (0..319) */
    for (int y = 0; y < 240; y++) {
        memcpy(s_backbuffer + y * SCREEN_WIDTH, s_bg_cache + y * 320, 320 * sizeof(uint32_t));
    }
}

/* --------------------------------------------------------------------------
 * Frame Rendering
 * -------------------------------------------------------------------------- */
void Render_DrawFrame(void) {
    /* 1. Left Half: Background */
    LoadBackground();

    /* 2. Board Plate and Cells */
    int ox, oy, cell_size;
    GetBoardLayout(&ox, &oy, &cell_size);
    int n = g_game.board_size;
    int bw = cell_size * n;
    int bh = cell_size * n;

    /* Board plate border & floor */
    gfx_fill_rect(ox - 3, oy - 3, bw + 6, bh + 6, 0x000A0C12);
    gfx_draw_rect(ox - 3, oy - 3, bw + 6, bh + 6, 0x0050556E);

    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            int cx = ox + c * cell_size;
            int cy = oy + r * cell_size;

            /* Cell background & border */
            gfx_fill_rect(cx, cy, cell_size, cell_size, 0x0012141C);
            gfx_draw_rect(cx, cy, cell_size, cell_size, 0x00262A38);

            int val = g_game.board[r][c];

            /* Valid Move Highlights */
            if (g_game.has_selected && (val == CELL_EMPTY || val == CELL_PRESENT_OBSTACLE)) {
                int mtype = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
                int mid_x = cx + cell_size / 2;
                int mid_y = cy + cell_size / 2;
                if (mtype == MOVE_CLONE) {
                    /* Green Spore Dot */
                    gfx_fill_circle(mid_x, mid_y, 4, 0x0022C55E);
                } else if (mtype == MOVE_LEAP) {
                    /* Purple Warp Indicator */
                    gfx_fill_rect(mid_x - 4, mid_y - 4, 8, 8, 0x00A855F7);
                }
            }

            /* Selected Ring */
            if (g_game.has_selected && r == g_game.selected_r && c == g_game.selected_c) {
                gfx_draw_rect(cx + 1, cy + 1, cell_size - 2, cell_size - 2, 0x00FACC15);
                gfx_draw_rect(cx + 2, cy + 2, cell_size - 4, cell_size - 4, 0x00FACC15);
            }

            /* Stones & Obstacles */
            int mid_x = cx + cell_size / 2;
            int mid_y = cy + cell_size / 2;
            int orb_r = cell_size / 2 - 3;

            if (val == CELL_RED) {
                BOOL is_klf = (g_game.char_red == CHAR_KLF);
                BOOL is_gwb = (g_game.char_red == CHAR_GWB);
                gfx_draw_orb(mid_x, mid_y, orb_r, g_game.color_red, is_klf, is_gwb);
            } else if (val == CELL_BLUE) {
                BOOL is_klf = (g_game.char_blue == CHAR_KLF);
                gfx_draw_orb(mid_x, mid_y, orb_r, g_game.color_blue, is_klf, FALSE);
            } else if (val == CELL_OBSTACLE) {
                /* Monolith Stone */
                gfx_fill_rect(cx + 4, cy + 4, cell_size - 8, cell_size - 8, 0x00303440);
                gfx_draw_rect(cx + 4, cy + 4, cell_size - 8, cell_size - 8, 0x00505564);
            } else if (val == CELL_PERM_OBSTACLE) {
                /* Ice Block */
                gfx_fill_rect(cx + 3, cy + 3, cell_size - 6, cell_size - 6, 0x0087CEEB);
                gfx_draw_rect(cx + 3, cy + 3, cell_size - 6, cell_size - 6, 0x00FFFFFF);
            } else if (val == CELL_PRESENT_OBSTACLE) {
                /* Santa Gift Box */
                gfx_fill_rect(cx + 4, cy + 4, cell_size - 8, cell_size - 8, 0x00DC2626);
                gfx_fill_rect(mid_x - 1, cy + 4, 2, cell_size - 8, 0x00FACC15);
                gfx_fill_rect(cx + 4, mid_y - 1, cell_size - 8, 2, 0x00FACC15);
            }
        }
    }

    /* 3. Divider Line at x = 320 */
    gfx_fill_rect(320, 0, 1, SCREEN_HEIGHT, 0x002D3246);

    /* 4. Right Half: Background Plate */
    gfx_fill_rect(321, 0, SCREEN_WIDTH - 321, SCREEN_HEIGHT, 0x000E101A);

    /* --- Title Header (y: 5) --- */
    gfx_draw_string(344, 5, "FTAGHN : COSMIC HORROR ATAXX", 0x00FACC15, 0, 1);

    /* --- Score & Turn Row (y: 20 to 44) --- */
    char buf_red[32];
    snprintf(buf_red, sizeof(buf_red), "RED: %d", g_game.scores[CELL_RED]);
    gfx_draw_button(326, 20, 90, 24, buf_red, 0x00B91C1C, 0x00FFFFFF, 1);

    char buf_blue[32];
    snprintf(buf_blue, sizeof(buf_blue), "BLUE: %d", g_game.scores[CELL_BLUE]);
    gfx_draw_button(422, 20, 90, 24, buf_blue, 0x001D4ED8, 0x00FFFFFF, 1);

    char buf_turn[48];
    const char *turn_str = (g_game.current_player == CELL_RED) ? "CRIMSON" : "ABYSSAL";
    if (g_game.time_distortion == 0.0f) {
        snprintf(buf_turn, sizeof(buf_turn), "%s|PAUSE", turn_str);
    } else {
        int m = g_game.game_timer / 60;
        int s = g_game.game_timer % 60;
        snprintf(buf_turn, sizeof(buf_turn), "%s|%d:%02d", turn_str, m, s);
    }
    uint32_t turn_bg = (g_game.current_player == CELL_RED) ? 0x00461919 : 0x00192D50;
    gfx_draw_button(518, 20, 116, 24, buf_turn, turn_bg, 0x00FACC15, 1);

    /* --- The Grimoire Panel (y: 48 to 138) --- */
    gfx_fill_rect(326, 48, 308, 90, 0x000A0B13);
    gfx_draw_rect(326, 48, 308, 90, 0x002D3246);

    gfx_draw_string(332, 52, "--- THE GRIMOIRE (STATUS LOG) ---", 0x00B49650, 0, 1);

    int gy = 66;
    for (int i = 0; i < g_game.grimoire_count && i < GRIMOIRE_MAX; i++) {
        gfx_draw_string(332, gy, g_game.grimoire[i].text, s_grim_colors[i], 0, 1);
        gy += 14;
    }

    /* --- Active Deity / Secret Box (y: 142 to 184) --- */
    gfx_fill_rect(326, 142, 308, 42, 0x00121420);
    gfx_draw_rect(326, 142, 308, 42, 0x00373C50);

    const SecretInfo *sec = &g_secrets[g_game.char_red];
    char d_title[128];
    snprintf(d_title, sizeof(d_title), "Deity: %s (%s)", sec->name, sec->title);
    gfx_draw_string(332, 146, d_title, sec->color, 0, 1);
    gfx_draw_string(332, 162, sec->description, 0x00B4B9C8, 0, 1);

    /* --- Stylus Touch Action Buttons (y: 190 to 234) --- */
    gfx_draw_button(326, 190, 72, 42, "NEW GAME", 0x00282D41, 0x00FACC15, 1);

    const char *diff_str = (g_game.difficulty == DIFF_EASY) ? "AI:EASY" :
                           (g_game.difficulty == DIFF_MEDIUM) ? "AI:NORM" : "AI:HARD";
    gfx_draw_button(402, 190, 68, 42, diff_str, 0x00233755, 0x00FFFFFF, 1);

    gfx_draw_button(474, 190, 86, 42, "TOME/LORE", 0x004B235F, 0x00FFE664, 1);

    const char *sound_str = g_game.is_muted ? "SFX:OFF" : "SFX: ON";
    uint32_t sound_col = g_game.is_muted ? 0x00EF4444 : 0x0022C55E;
    gfx_draw_button(564, 190, 70, 42, sound_str, 0x00282D41, sound_col, 1);
}

/* --------------------------------------------------------------------------
 * Tome Modal Rendering
 * -------------------------------------------------------------------------- */
void Render_DrawTomeModal(int tome_index) {
    if (tome_index < 0 || tome_index >= CHAR_MAX_COUNT) return;

    /* Modal Background Box */
    gfx_fill_rect(40, 12, 560, 216, 0x00121420);
    gfx_draw_rect(40, 12, 560, 216, 0x00FACC15);

    /* Close 'X' Button in Top-Right */
    gfx_fill_rect(565, 18, 27, 22, 0x00B91C1C);
    gfx_draw_char(575, 23, 'X', 0x00FFFFFF, 0, 1);

    /* Modal Title Header */
    gfx_draw_string(140, 20, "=== TOME OF FORBIDDEN KNOWLEDGE ===", 0x00FACC15, 0, 1);

    char page_str[32];
    snprintf(page_str, sizeof(page_str), "Entry %d of %d", tome_index + 1, CHAR_MAX_COUNT);
    gfx_draw_string(270, 36, page_str, 0x00A0A5B9, 0, 1);

    const SecretInfo *sec = &g_secrets[tome_index];

    /* Deity Name Banner */
    char name_banner[128];
    snprintf(name_banner, sizeof(name_banner), "%s : %s", sec->name, sec->title);
    gfx_draw_string(60, 56, name_banner, sec->color, 0, 1);

    /* Description Box */
    gfx_fill_rect(55, 78, 530, 88, 0x000A0B12);
    gfx_draw_rect(55, 78, 530, 88, 0x0032374B);

    /* Word-wrapped description */
    const char *desc = sec->description;
    int line_y = 86;
    int line_x = 65;
    while (*desc && line_y < 160) {
        char word[64];
        int wlen = 0;
        while (*desc && *desc != ' ' && wlen < 63) {
            word[wlen++] = *desc++;
        }
        word[wlen] = '\0';
        if (*desc == ' ') desc++;

        if (line_x + wlen * 8 > 570) {
            line_y += 12;
            line_x = 65;
        }
        gfx_draw_string(line_x, line_y, word, 0x00DCDEF0, 0, 1);
        line_x += (wlen + 1) * 8;
    }

    /* Modal Action Buttons */
    gfx_draw_button(55, 178, 90, 40, "< PREV", 0x00282D41, 0x00FFFFFF, 1);
    gfx_draw_button(160, 178, 160, 40, "INVOKE SECRET", 0x00552369, 0x00FACC15, 1);
    gfx_draw_button(335, 178, 145, 40, "AUDIO PREVIEW", 0x00193755, 0x00B4DCFF, 1);
    gfx_draw_button(495, 178, 90, 40, "NEXT >", 0x00282D41, 0x00FFFFFF, 1);
}

/* --------------------------------------------------------------------------
 * Double-Buffering Initialization and Screen Flip
 * -------------------------------------------------------------------------- */
void Render_Init(HWND hwnd) {
    HDC screen_dc = GetDC(hwnd);
    if (!screen_dc) return;

    s_hdc_mem = CreateCompatibleDC(screen_dc);

    /* Allocate DIBSection (identical to CEssh): Top-down 16bpp 5:6:5 BI_BITFIELDS */
    struct {
        BITMAPINFOHEADER bmiHeader;
        DWORD bmiColors[3];
    } bmi16;
    memset(&bmi16, 0, sizeof(bmi16));
    bmi16.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi16.bmiHeader.biWidth = SCREEN_WIDTH;
    bmi16.bmiHeader.biHeight = -SCREEN_HEIGHT; /* Top-down */
    bmi16.bmiHeader.biPlanes = 1;
    bmi16.bmiHeader.biBitCount = 16;
    bmi16.bmiHeader.biCompression = BI_BITFIELDS;
    bmi16.bmiColors[0] = 0xF800; /* Red */
    bmi16.bmiColors[1] = 0x07E0; /* Green */
    bmi16.bmiColors[2] = 0x001F; /* Blue */

    s_hbm_dib = CreateDIBSection(screen_dc, (BITMAPINFO *)&bmi16, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
    if (s_hbm_dib && s_dib_bits) {
        s_dib_bottom_up = false;
        s_is_555 = false;
    } else {
        /* Fallback: Bottom-up 16bpp 5:6:5 */
        bmi16.bmiHeader.biHeight = SCREEN_HEIGHT;
        s_hbm_dib = CreateDIBSection(screen_dc, (BITMAPINFO *)&bmi16, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
        if (s_hbm_dib && s_dib_bits) {
            s_dib_bottom_up = true;
            s_is_555 = false;
        } else {
            /* Fallback: Bottom-up 16bpp 5:5:5 BI_RGB */
            BITMAPINFOHEADER bmi555;
            memset(&bmi555, 0, sizeof(bmi555));
            bmi555.biSize = sizeof(BITMAPINFOHEADER);
            bmi555.biWidth = SCREEN_WIDTH;
            bmi555.biHeight = SCREEN_HEIGHT;
            bmi555.biPlanes = 1;
            bmi555.biBitCount = 16;
            bmi555.biCompression = BI_RGB;

            s_hbm_dib = CreateDIBSection(screen_dc, (BITMAPINFO *)&bmi555, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
            if (s_hbm_dib && s_dib_bits) {
                s_dib_bottom_up = true;
                s_is_555 = true;
            } else {
                /* Fallback: Bottom-up 24bpp */
                BITMAPINFOHEADER bmi24;
                memset(&bmi24, 0, sizeof(bmi24));
                bmi24.biSize = sizeof(BITMAPINFOHEADER);
                bmi24.biWidth = SCREEN_WIDTH;
                bmi24.biHeight = SCREEN_HEIGHT;
                bmi24.biPlanes = 1;
                bmi24.biBitCount = 24;
                bmi24.biCompression = BI_RGB;
                s_hbm_dib = CreateDIBSection(screen_dc, (BITMAPINFO *)&bmi24, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
                if (s_hbm_dib && s_dib_bits) {
                    s_dib_bottom_up = true;
                    s_is_24bpp = true;
                }
            }
        }
    }

    if (s_hbm_dib && s_hdc_mem) {
        s_hbm_old = (HBITMAP)SelectObject(s_hdc_mem, s_hbm_dib);
    }

    ReleaseDC(hwnd, screen_dc);
}

void Render_Flip(HWND hwnd) {
    if (!hwnd || !s_hdc_mem || !s_dib_bits) return;

    /* Convert 32bpp backbuffer to DIBSection memory */
    if (s_is_24bpp) {
        uint8_t *dst = (uint8_t *)s_dib_bits;
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            int dy = s_dib_bottom_up ? (SCREEN_HEIGHT - 1 - y) : y;
            uint8_t *row = dst + (dy * SCREEN_WIDTH * 3);
            const uint32_t *src = s_backbuffer + (y * SCREEN_WIDTH);
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                uint32_t c = src[x];
                row[x * 3 + 0] = (uint8_t)(c & 0xFF);         /* B */
                row[x * 3 + 1] = (uint8_t)((c >> 8) & 0xFF);  /* G */
                row[x * 3 + 2] = (uint8_t)((c >> 16) & 0xFF); /* R */
            }
        }
    } else {
        uint16_t *dst = (uint16_t *)s_dib_bits;
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            int dy = s_dib_bottom_up ? (SCREEN_HEIGHT - 1 - y) : y;
            uint16_t *row = dst + (dy * SCREEN_WIDTH);
            const uint32_t *src = s_backbuffer + (y * SCREEN_WIDTH);
            if (s_is_555) {
                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    uint32_t c = src[x];
                    uint16_t r = (uint16_t)((c >> 19) & 0x1F);
                    uint16_t g = (uint16_t)((c >> 11) & 0x1F);
                    uint16_t b = (uint16_t)((c >> 3) & 0x1F);
                    row[x] = (r << 10) | (g << 5) | b;
                }
            } else {
                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    uint32_t c = src[x];
                    uint16_t r = (uint16_t)((c >> 19) & 0x1F);
                    uint16_t g = (uint16_t)((c >> 10) & 0x3F);
                    uint16_t b = (uint16_t)((c >> 3) & 0x1F);
                    row[x] = (r << 11) | (g << 5) | b;
                }
            }
        }
    }

    /* Blit to screen DC */
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, s_hdc_mem, 0, 0, SRCCOPY);
        ReleaseDC(hwnd, hdc);
    }
}

void Render_Cleanup(void) {
    if (s_hdc_mem && s_hbm_old) {
        SelectObject(s_hdc_mem, s_hbm_old);
        s_hbm_old = NULL;
    }
    if (s_hbm_dib) {
        DeleteObject(s_hbm_dib);
        s_hbm_dib = NULL;
    }
    if (s_hdc_mem) {
        DeleteDC(s_hdc_mem);
        s_hdc_mem = NULL;
    }
    s_dib_bits = NULL;
}
