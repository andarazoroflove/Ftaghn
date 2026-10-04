#include "render.h"
#include "game.h"
#include "sound.h"
#include "font.h"
#include "freestanding.h"

/* External Win32 GDI imports */
extern HDC     WINAPI CreateCompatibleDC(HDC hdc);
extern BOOL    WINAPI DeleteDC(HDC hdc);
extern HBITMAP WINAPI CreateDIBSection(HDC hdc, const BITMAPINFO *pbmi, UINT usage, void **ppvBits, HANDLE hSection, DWORD offset);
extern void*   WINAPI SelectObject(HDC hdc, void *hgdiobj);
extern BOOL    WINAPI DeleteObject(void *hObject);
extern BOOL    WINAPI BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
extern BOOL    WINAPI InvalidateRect(HWND hWnd, const RECT *lpRect, BOOL bErase);
extern BOOL    WINAPI UpdateWindow(HWND hWnd);

/* 16-bit RGB555 Color Palette for 4-Level Grayscale & Contrast */
#define RGB555(r, g, b)  ((uint16_t)(((((r) >> 3) & 0x1F) << 10) | ((((g) >> 3) & 0x1F) << 5) | (((b) >> 3) & 0x1F)))

#define COLOR_BLACK      RGB555(0, 0, 0)          /* 0x0000 */
#define COLOR_DARK_VOID  RGB555(16, 16, 20)       /* 0x0842 */
#define COLOR_DARK_SLATE RGB555(50, 50, 56)       /* 0x18C7 */
#define COLOR_MID_GRAY   RGB555(100, 100, 110)    /* 0x318C */
#define COLOR_ASH_GRAY   RGB555(160, 160, 170)    /* 0x5294 */
#define COLOR_PARCHMENT  RGB555(210, 210, 215)    /* 0x6B5A */
#define COLOR_WHITE      RGB555(255, 255, 255)    /* 0x7FFF */

static HWND      s_hwnd = NULL;
static HDC       s_memDC = NULL;
static HBITMAP   s_hBitmap = NULL;
static HBITMAP   s_hOldBitmap = NULL;
static uint16_t *s_backbuffer = NULL;

static BOOL      s_tome_open = FALSE;
static int       s_tome_page = 0;
#define TOME_PER_PAGE 4

static int s_cursor_r = 3;
static int s_cursor_c = 3;

/* Low-level graphics primitives */
static void PutPixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < NINO_SCREEN_W && y >= 0 && y < NINO_SCREEN_H) {
        s_backbuffer[y * NINO_SCREEN_W + x] = color;
    }
}

static void DrawHLine(int x1, int x2, int y, uint16_t color) {
    if (y < 0 || y >= NINO_SCREEN_H) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < 0) x1 = 0;
    if (x2 >= NINO_SCREEN_W) x2 = NINO_SCREEN_W - 1;
    uint16_t *p = &s_backbuffer[y * NINO_SCREEN_W + x1];
    for (int x = x1; x <= x2; x++) {
        *p++ = color;
    }
}

static void DrawVLine(int x, int y1, int y2, uint16_t color) {
    if (x < 0 || x >= NINO_SCREEN_W) return;
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    if (y1 < 0) y1 = 0;
    if (y2 >= NINO_SCREEN_H) y2 = NINO_SCREEN_H - 1;
    uint16_t *p = &s_backbuffer[y1 * NINO_SCREEN_W + x];
    for (int y = y1; y <= y2; y++) {
        *p = color;
        p += NINO_SCREEN_W;
    }
}

static void FillRect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > NINO_SCREEN_W) w = NINO_SCREEN_W - x;
    if (y + h > NINO_SCREEN_H) h = NINO_SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    for (int cy = y; cy < y + h; cy++) {
        uint16_t *p = &s_backbuffer[cy * NINO_SCREEN_W + x];
        for (int cx = 0; cx < w; cx++) {
            *p++ = color;
        }
    }
}

static void DrawBevelRect(int x, int y, int w, int h,
                          uint16_t fill_color, uint16_t hi_color, uint16_t lo_color) {
    FillRect(x, y, w, h, fill_color);
    DrawHLine(x, x + w - 1, y, hi_color);
    DrawVLine(x, y, y + h - 1, hi_color);
    DrawHLine(x, x + w - 1, y + h - 1, lo_color);
    DrawVLine(x + w - 1, y, y + h - 1, lo_color);
}

static void DrawFilledCircle(int cx, int cy, int radius, uint16_t color) {
    for (int dy = -radius; dy <= radius; dy++) {
        int r2 = radius * radius;
        int y2 = dy * dy;
        int dx = 0;
        while ((dx + 1) * (dx + 1) <= r2 - y2) {
            dx++;
        }
        DrawHLine(cx - dx, cx + dx, cy + dy, color);
    }
}

static void DrawCircleOutline(int cx, int cy, int radius, uint16_t color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        PutPixel(cx + x, cy + y, color);
        PutPixel(cx + y, cy + x, color);
        PutPixel(cx - y, cy + x, color);
        PutPixel(cx - x, cy + y, color);
        PutPixel(cx - x, cy - y, color);
        PutPixel(cx - y, cy - x, color);
        PutPixel(cx + y, cy - x, color);
        PutPixel(cx + x, cy - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

static void DrawChar(int x, int y, char c, uint16_t fg_color) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font8x8_basic[c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t line = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (line & (1 << col)) {
                PutPixel(x + col, y + row, fg_color);
            }
        }
    }
}

static void DrawText(int x, int y, const char *str, uint16_t color) {
    if (!str) return;
    while (*str) {
        DrawChar(x, y, *str++, color);
        x += 8;
    }
}

static void DrawTextCentered(int x, int y, int w, const char *str, uint16_t color) {
    if (!str) return;
    int len = (int)strlen(str);
    int text_w = len * 8;
    int cx = x + (w - text_w) / 2;
    if (cx < x) cx = x;
    DrawText(cx, y, str, color);
}

/* Board geometry */
static void GetBoardMetrics(int *out_ox, int *out_oy, int *out_csize) {
    int n = g_game.board_size;
    if (n >= 9) {
        *out_csize = 22;
        *out_ox = (NINO_SCREEN_W - (n * 22)) / 2;
        *out_oy = 6;
    } else {
        *out_csize = 28;
        *out_ox = (NINO_SCREEN_W - (n * 28)) / 2;
        *out_oy = 6;
    }
}

/* Piece rendering with high-contrast grayscale patterns */
static void DrawPiece(int cx, int cy, int radius, int cell_type) {
    if (radius < 4) radius = 4;

    if (cell_type == CELL_RED) {
        /* Player / Cultist Piece: Solid Bone White disk with central black slit eye */
        DrawFilledCircle(cx, cy, radius, COLOR_WHITE);
        DrawCircleOutline(cx, cy, radius, COLOR_PARCHMENT);
        /* Central pupil / horizontal slit */
        DrawHLine(cx - 3, cx + 3, cy, COLOR_BLACK);
        DrawHLine(cx - 2, cx + 2, cy - 1, COLOR_BLACK);
        DrawHLine(cx - 2, cx + 2, cy + 1, COLOR_BLACK);
    } else if (cell_type == CELL_BLUE) {
        /* AI / Elder Horror Piece: Textured dark slate disk with white crosshatch */
        DrawFilledCircle(cx, cy, radius, COLOR_DARK_SLATE);
        DrawCircleOutline(cx, cy, radius, COLOR_ASH_GRAY);
        DrawCircleOutline(cx, cy, radius - 2, COLOR_WHITE);
        /* Inverted cross symbol */
        DrawVLine(cx, cy - 4, cy + 5, COLOR_WHITE);
        DrawHLine(cx - 3, cx + 3, cy + 1, COLOR_WHITE);
    } else if (cell_type == CELL_OBSTACLE) {
        /* Monolith / Tentacle Barrier: Slate block with center dot */
        int hw = radius - 1;
        DrawBevelRect(cx - hw, cy - hw, hw * 2, hw * 2,
                      COLOR_MID_GRAY, COLOR_WHITE, COLOR_BLACK);
        DrawFilledCircle(cx, cy, 2, COLOR_BLACK);
        PutPixel(cx, cy, COLOR_WHITE);
    } else if (cell_type == CELL_PERM_OBSTACLE) {
        /* Rhan-Tegoth Ice Barrier: Diamond cross */
        int hw = radius - 1;
        DrawBevelRect(cx - hw, cy - hw, hw * 2, hw * 2,
                      COLOR_PARCHMENT, COLOR_WHITE, COLOR_DARK_SLATE);
        for (int d = -hw + 2; d <= hw - 2; d++) {
            PutPixel(cx + d, cy + d, COLOR_BLACK);
            PutPixel(cx + d, cy - d, COLOR_BLACK);
        }
    } else if (cell_type == CELL_PRESENT_OBSTACLE) {
        /* Santa Present: Box with ribbon */
        int hw = radius - 1;
        DrawBevelRect(cx - hw, cy - hw, hw * 2, hw * 2,
                      COLOR_ASH_GRAY, COLOR_WHITE, COLOR_BLACK);
        DrawVLine(cx, cy - hw, cy + hw - 1, COLOR_WHITE);
        DrawHLine(cx - hw, cx + hw - 1, cy, COLOR_WHITE);
    }
}

/* Board Drawing */
static void DrawBoard(void) {
    int ox, oy, csize;
    int n = g_game.board_size;
    GetBoardMetrics(&ox, &oy, &csize);

    /* Outer board border */
    DrawBevelRect(ox - 3, oy - 3, (n * csize) + 6, (n * csize) + 6,
                  COLOR_BLACK, COLOR_ASH_GRAY, COLOR_BLACK);

    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            int cell_x = ox + (c * csize);
            int cell_y = oy + (r * csize);
            int cx = cell_x + (csize / 2);
            int cy = cell_y + (csize / 2);
            int pradius = (csize / 2) - 3;
            int cell_val = g_game.board[r][c];

            /* Alternating subtle dark checker pattern */
            uint16_t tile_fill = ((r + c) % 2 == 0) ? COLOR_DARK_SLATE : COLOR_DARK_VOID;
            DrawBevelRect(cell_x + 1, cell_y + 1, csize - 2, csize - 2,
                          tile_fill, COLOR_MID_GRAY, COLOR_BLACK);

            /* Valid Move Targets */
            if (g_game.has_selected) {
                int mtype = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
                if (mtype == MOVE_CLONE) {
                    /* Clone (Distance 1): Solid bone white diamond */
                    for (int d = -2; d <= 2; d++) {
                        int span = 2 - ((d < 0) ? -d : d);
                        DrawHLine(cx - span, cx + span, cy + d, COLOR_WHITE);
                    }
                } else if (mtype == MOVE_LEAP) {
                    /* Leap (Distance 2): Hollow white circle outline */
                    DrawCircleOutline(cx, cy, 4, COLOR_WHITE);
                    DrawCircleOutline(cx, cy, 5, COLOR_PARCHMENT);
                }
            }

            /* Pieces */
            if (cell_val != CELL_EMPTY) {
                DrawPiece(cx, cy, pradius, cell_val);
            }

            /* Selected Cell Highlight: Inverted flashing border */
            if (g_game.has_selected && r == g_game.selected_r && c == g_game.selected_c) {
                DrawCircleOutline(cx, cy, pradius + 2, COLOR_WHITE);
                DrawCircleOutline(cx, cy, pradius + 1, COLOR_PARCHMENT);
            }

            /* Hardware D-Pad / Nav Cursor */
            if (r == s_cursor_r && c == s_cursor_c) {
                /* 4-corner brackets */
                PutPixel(cell_x + 2, cell_y + 2, COLOR_WHITE);
                PutPixel(cell_x + 3, cell_y + 2, COLOR_WHITE);
                PutPixel(cell_x + 2, cell_y + 3, COLOR_WHITE);

                PutPixel(cell_x + csize - 3, cell_y + 2, COLOR_WHITE);
                PutPixel(cell_x + csize - 4, cell_y + 2, COLOR_WHITE);
                PutPixel(cell_x + csize - 3, cell_y + 3, COLOR_WHITE);

                PutPixel(cell_x + 2, cell_y + csize - 3, COLOR_WHITE);
                PutPixel(cell_x + 3, cell_y + csize - 3, COLOR_WHITE);
                PutPixel(cell_x + 2, cell_y + csize - 4, COLOR_WHITE);

                PutPixel(cell_x + csize - 3, cell_y + csize - 3, COLOR_WHITE);
                PutPixel(cell_x + csize - 4, cell_y + csize - 3, COLOR_WHITE);
                PutPixel(cell_x + csize - 3, cell_y + csize - 4, COLOR_WHITE);
            }
        }
    }
}

/* HUD Banner (Scores, Turn, Timer) */
static void DrawHUD(void) {
    char s_red[16], s_blue[16], s_status[32];
    int hy = 206;
    int hw = 230;
    int hh = 32;
    int hx = 5;

    DrawBevelRect(hx, hy, hw, hh, COLOR_DARK_VOID, COLOR_MID_GRAY, COLOR_BLACK);

    /* Left: Player 1 (Red) */
    DrawFilledCircle(hx + 12, hy + 12, 6, COLOR_WHITE);
    snprintf(s_red, sizeof(s_red), "P1:%d", g_game.scores[CELL_RED]);
    DrawText(hx + 24, hy + 8, s_red, COLOR_WHITE);
    DrawText(hx + 24, hy + 20, "CULT", COLOR_ASH_GRAY);

    /* Center: Turn Status & Timer */
    if (g_game.game_over) {
        DrawTextCentered(hx + 60, hy + 6, 110, "DUEL CONCLUDED", COLOR_WHITE);
    } else {
        if (g_game.current_player == CELL_RED) {
            DrawTextCentered(hx + 60, hy + 6, 110, "YOUR TURN", COLOR_WHITE);
        } else {
            DrawTextCentered(hx + 60, hy + 6, 110, "AI THINKING...", COLOR_PARCHMENT);
        }
    }

    if (g_game.time_distortion == 0) {
        strcpy(s_status, "TIME: STASIS");
    } else {
        int m = g_game.game_timer / 60;
        int s = g_game.game_timer % 60;
        snprintf(s_status, sizeof(s_status), "TIME: %d:%02d", m, s);
    }
    DrawTextCentered(hx + 60, hy + 19, 110, s_status, COLOR_ASH_GRAY);

    /* Right: AI (Blue) */
    snprintf(s_blue, sizeof(s_blue), "AI:%d", g_game.scores[CELL_BLUE]);
    DrawText(hx + hw - 60, hy + 8, s_blue, COLOR_WHITE);
    DrawText(hx + hw - 60, hy + 20, "ELDER", COLOR_ASH_GRAY);
    DrawFilledCircle(hx + hw - 12, hy + 12, 6, COLOR_DARK_SLATE);
    DrawCircleOutline(hx + hw - 12, hy + 12, 6, COLOR_WHITE);
}

/* Grimoire Status Chronicle */
static void DrawGrimoire(void) {
    int gy = 242;
    int gw = 230;
    int gh = 26;
    int gx = 5;

    DrawBevelRect(gx, gy, gw, gh, COLOR_DARK_SLATE, COLOR_ASH_GRAY, COLOR_BLACK);

    if (g_game.grimoire_count > 0) {
        DrawText(gx + 6, gy + 4, "--- THE GRIMOIRE ---", COLOR_PARCHMENT);
        DrawText(gx + 6, gy + 14, g_game.grimoire[0].text, COLOR_WHITE);
    } else {
        DrawText(gx + 6, gy + 4, "--- THE GRIMOIRE ---", COLOR_PARCHMENT);
        DrawText(gx + 6, gy + 14, "The stars align for battle...", COLOR_ASH_GRAY);
    }
}

/* Stylus Touch Toolbar */
static void DrawButtons(void) {
    const char *diff_names[] = { "MORTAL", "ELDER", "ANCIENT" };
    const char *snd_text = Sound_IsMuted() ? "SND:OFF" : "SND:ON";
    int by = 274;
    int bh = 38;

    /* [ NEW ] button */
    DrawBevelRect(5, by, 52, bh, COLOR_DARK_SLATE, COLOR_WHITE, COLOR_BLACK);
    DrawTextCentered(5, by + 14, 52, "NEW", COLOR_WHITE);

    /* [ DIFF ] button */
    DrawBevelRect(61, by, 54, bh, COLOR_DARK_SLATE, COLOR_WHITE, COLOR_BLACK);
    DrawTextCentered(61, by + 6, 54, "DIFF", COLOR_ASH_GRAY);
    DrawTextCentered(61, by + 20, 54, diff_names[g_game.difficulty % 3], COLOR_WHITE);

    /* [ TOME ] button */
    DrawBevelRect(119, by, 60, bh, COLOR_MID_GRAY, COLOR_WHITE, COLOR_BLACK);
    DrawTextCentered(119, by + 8, 60, "TOME OF", COLOR_WHITE);
    DrawTextCentered(119, by + 20, 60, "LORE", COLOR_WHITE);

    /* [ SND ] button */
    DrawBevelRect(183, by, 52, bh, COLOR_DARK_SLATE, COLOR_WHITE, COLOR_BLACK);
    DrawTextCentered(183, by + 14, 52, snd_text, COLOR_WHITE);
}

/* Full-Screen Tome of Forbidden Lore Modal (240x320) */
static void DrawTomeModal(void) {
    int max_pages = (CHAR_MAX_COUNT + TOME_PER_PAGE - 1) / TOME_PER_PAGE;
    int start_idx = s_tome_page * TOME_PER_PAGE;
    char page_str[32];

    /* Backdrop */
    FillRect(0, 0, NINO_SCREEN_W, NINO_SCREEN_H, COLOR_BLACK);

    /* Header */
    DrawBevelRect(4, 4, 232, 22, COLOR_DARK_VOID, COLOR_ASH_GRAY, COLOR_BLACK);
    DrawText(10, 10, "THE TOME OF FORBIDDEN LORE", COLOR_WHITE);
    snprintf(page_str, sizeof(page_str), "%d/%d", s_tome_page + 1, max_pages);
    DrawText(200, 10, page_str, COLOR_PARCHMENT);

    /* 4 Deity Cards per page */
    for (int i = 0; i < TOME_PER_PAGE; i++) {
        int idx = start_idx + i;
        int cy = 29 + (i * 58);
        if (idx >= CHAR_MAX_COUNT) break;
        const SecretInfo *sec = &g_secrets[idx];

        if (g_game.char_red == (enum CharType)idx) {
            /* Active deity card: prominent white border */
            DrawBevelRect(5, cy, 230, 54, COLOR_MID_GRAY, COLOR_WHITE, COLOR_WHITE);
        } else {
            DrawBevelRect(5, cy, 230, 54, COLOR_DARK_VOID, COLOR_ASH_GRAY, COLOR_BLACK);
        }

        char title_buf[64];
        snprintf(title_buf, sizeof(title_buf), "%d. %s - %s", idx + 1, sec->name, sec->title);
        DrawText(10, cy + 4, title_buf, COLOR_WHITE);
        DrawText(10, cy + 18, sec->description, COLOR_PARCHMENT);
    }

    /* Footer Navigation Buttons */
    int fy = 272;
    int fh = 40;

    /* [< PREV] */
    DrawBevelRect(5, fy, 70, fh, COLOR_DARK_SLATE, COLOR_WHITE, COLOR_BLACK);
    DrawTextCentered(5, fy + 15, 70, "< PREV", COLOR_WHITE);

    /* [NEXT >] */
    DrawBevelRect(82, fy, 70, fh, COLOR_DARK_SLATE, COLOR_WHITE, COLOR_BLACK);
    DrawTextCentered(82, fy + 15, 70, "NEXT >", COLOR_WHITE);

    /* [RESUME] */
    DrawBevelRect(159, fy, 76, fh, COLOR_MID_GRAY, COLOR_WHITE, COLOR_BLACK);
    DrawTextCentered(159, fy + 15, 76, "RESUME", COLOR_WHITE);
}

BOOL Render_Init(HWND hwnd) {
    s_hwnd = hwnd;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = NINO_SCREEN_W;
    bmi.bmiHeader.biHeight = -NINO_SCREEN_H; /* Top-down DIB */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 16;
    bmi.bmiHeader.biCompression = 0; /* BI_RGB */

    s_memDC = CreateCompatibleDC(NULL);
    if (!s_memDC) return FALSE;

    s_hBitmap = CreateDIBSection(s_memDC, &bmi, DIB_RGB_COLORS, (void **)&s_backbuffer, NULL, 0);
    if (!s_hBitmap || !s_backbuffer) return FALSE;

    s_hOldBitmap = (HBITMAP)SelectObject(s_memDC, s_hBitmap);
    return TRUE;
}

void Render_Cleanup(void) {
    if (s_memDC && s_hOldBitmap) {
        SelectObject(s_memDC, s_hOldBitmap);
        s_hOldBitmap = NULL;
    }
    if (s_hBitmap) {
        DeleteObject(s_hBitmap);
        s_hBitmap = NULL;
    }
    if (s_memDC) {
        DeleteDC(s_memDC);
        s_memDC = NULL;
    }
    s_backbuffer = NULL;
}

void Render_DrawAll(void) {
    if (!s_backbuffer) return;

    if (s_tome_open) {
        DrawTomeModal();
    } else {
        FillRect(0, 0, NINO_SCREEN_W, NINO_SCREEN_H, COLOR_BLACK);
        DrawBoard();
        DrawHUD();
        DrawGrimoire();
        DrawButtons();
    }

    if (s_hwnd) {
        InvalidateRect(s_hwnd, NULL, FALSE);
        UpdateWindow(s_hwnd);
    }
}

void Render_Paint(HDC hdc) {
    if (s_memDC && hdc) {
        BitBlt(hdc, 0, 0, NINO_SCREEN_W, NINO_SCREEN_H, s_memDC, 0, 0, SRCCOPY);
    }
}

void Render_SetTomeVisible(BOOL visible) {
    s_tome_open = visible;
    if (visible) {
        s_tome_page = 0;
    }
}

BOOL Render_IsTomeVisible(void) {
    return s_tome_open;
}

void Render_TomeScroll(int delta) {
    int max_pages = (CHAR_MAX_COUNT + TOME_PER_PAGE - 1) / TOME_PER_PAGE;
    s_tome_page += delta;
    if (s_tome_page < 0) s_tome_page = 0;
    if (s_tome_page >= max_pages) s_tome_page = max_pages - 1;
}

BOOL Render_HandleClick(int x, int y) {
    int ox, oy, csize;
    int n = g_game.board_size;

    if (s_tome_open) {
        /* Check 4 cards */
        for (int i = 0; i < TOME_PER_PAGE; i++) {
            int cy = 29 + (i * 58);
            if (y >= cy && y < cy + 54 && x >= 5 && x < 235) {
                int idx = (s_tome_page * TOME_PER_PAGE) + i;
                if (idx >= 0 && idx < CHAR_MAX_COUNT) {
                    Game_ApplySecret((enum CharType)idx, CELL_RED);
                    s_tome_open = FALSE;
                    Sound_PlaySFX("select.wav");
                    Render_DrawAll();
                    return TRUE;
                }
            }
        }

        /* Prev button */
        if (x >= 5 && x < 75 && y >= 272 && y < 312) {
            Render_TomeScroll(-1);
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return TRUE;
        }

        /* Next button */
        if (x >= 82 && x < 152 && y >= 272 && y < 312) {
            Render_TomeScroll(1);
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return TRUE;
        }

        /* Resume button */
        if (x >= 159 && x < 235 && y >= 272 && y < 312) {
            s_tome_open = FALSE;
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return TRUE;
        }

        return TRUE;
    }

    /* Toolbar Buttons */
    /* [ NEW ] */
    if (x >= 5 && x < 57 && y >= 274 && y < 312) {
        Sound_PlaySFX("place.wav");
        Game_ResetGame();
        Render_DrawAll();
        return TRUE;
    }

    /* [ DIFF ] */
    if (x >= 61 && x < 115 && y >= 274 && y < 312) {
        g_game.difficulty = (g_game.difficulty + 1) % 3;
        Sound_PlaySFX("select.wav");
        Render_DrawAll();
        return TRUE;
    }

    /* [ TOME ] */
    if (x >= 119 && x < 179 && y >= 274 && y < 312) {
        Sound_PlaySFX("select.wav");
        Render_SetTomeVisible(TRUE);
        Render_DrawAll();
        return TRUE;
    }

    /* [ SND ] */
    if (x >= 183 && x < 235 && y >= 274 && y < 312) {
        Sound_ToggleMute();
        Render_DrawAll();
        return TRUE;
    }

    /* Board Click */
    GetBoardMetrics(&ox, &oy, &csize);
    if (x >= ox && x < ox + (n * csize) && y >= oy && y < oy + (n * csize)) {
        int col = (x - ox) / csize;
        int row = (y - oy) / csize;
        s_cursor_r = row;
        s_cursor_c = col;
        Game_HandleClick(row, col);
        Render_DrawAll();
        return TRUE;
    }

    return FALSE;
}

void Render_MoveNavCursor(int dr, int dc) {
    int n = g_game.board_size;
    s_cursor_r += dr;
    s_cursor_c += dc;
    if (s_cursor_r < 0) s_cursor_r = 0;
    if (s_cursor_r >= n) s_cursor_r = n - 1;
    if (s_cursor_c < 0) s_cursor_c = 0;
    if (s_cursor_c >= n) s_cursor_c = n - 1;
    Sound_PlaySFX("select.wav");
    Render_DrawAll();
}

void Render_SelectNavCursor(void) {
    Game_HandleClick(s_cursor_r, s_cursor_c);
    Render_DrawAll();
}
