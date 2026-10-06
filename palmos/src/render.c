#include "render.h"
#include "sound.h"
#include <StringMgr.h>

typedef enum {
    RENDER_MODE_160x160 = 0,
    RENDER_MODE_160x240 = 1,
    RENDER_MODE_320x480 = 2
} RenderModeType;

static RenderModeType s_render_mode = RENDER_MODE_160x240;
static Coord s_screen_w = 160;
static Coord s_screen_h = 240;
static Boolean s_is_lowres = false;

static WinHandle s_displayWin = NULL;
static WinHandle s_offscreenWin = NULL;

static Boolean s_tome_open = false;
static int s_tome_page = 0;
#define TOME_PER_PAGE_HI  6
#define TOME_PER_PAGE_MID 4
#define TOME_PER_PAGE_LO  3

/* 5-way D-Pad navigation cursor */
static int s_cursor_r = 3;
static int s_cursor_c = 3;

/* Color helpers */
static void SetDrawColor(UInt8 r, UInt8 g, UInt8 b) {
    RGBColorType rgb;
    rgb.index = 0;
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;
    WinSetForeColorRGB(&rgb, NULL);
}

static void SetTextColor(UInt8 r, UInt8 g, UInt8 b) {
    RGBColorType rgb;
    rgb.index = 0;
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;
    WinSetTextColorRGB(&rgb, NULL);
}

static void DrawFilledRect(Coord x, Coord y, Coord w, Coord h, UInt8 r, UInt8 g, UInt8 b, UInt16 cornerDiam) {
    RectangleType rect;
    rect.topLeft.x = x;
    rect.topLeft.y = y;
    rect.extent.x = w;
    rect.extent.y = h;
    SetDrawColor(r, g, b);
    WinDrawRectangle(&rect, cornerDiam);
}

static void DrawBeveledRect(Coord x, Coord y, Coord w, Coord h,
                           UInt8 fill_r, UInt8 fill_g, UInt8 fill_b,
                           UInt8 hi_r, UInt8 hi_g, UInt8 hi_b,
                           UInt8 lo_r, UInt8 lo_g, UInt8 lo_b) {
    DrawFilledRect(x, y, w, h, fill_r, fill_g, fill_b, 0);

    /* Top and Left highlight lines */
    SetDrawColor(hi_r, hi_g, hi_b);
    WinDrawLine(x, y, x + w - 1, y);
    WinDrawLine(x, y, x, y + h - 1);

    /* Bottom and Right shadow lines */
    SetDrawColor(lo_r, lo_g, lo_b);
    WinDrawLine(x, y + h - 1, x + w - 1, y + h - 1);
    WinDrawLine(x + w - 1, y, x + w - 1, y + h - 1);
}

/* Fast integer circle filler */
static void DrawFilledCircle(Coord cx, Coord cy, Int16 radius) {
    Int16 dy;
    for (dy = -radius; dy <= radius; dy++) {
        Int32 r2 = (Int32)radius * radius;
        Int32 y2 = (Int32)dy * dy;
        Int16 dx = 0;
        while ((Int32)(dx + 1) * (dx + 1) <= r2 - y2) {
            dx++;
        }
        WinDrawLine(cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

/* Midpoint circle outline */
static void DrawCircleOutline(Coord cx, Coord cy, Int16 radius) {
    Int16 x = radius;
    Int16 y = 0;
    Int16 err = 0;

    while (x >= y) {
        WinDrawPixel(cx + x, cy + y);
        WinDrawPixel(cx + y, cy + x);
        WinDrawPixel(cx - y, cy + x);
        WinDrawPixel(cx - x, cy + y);
        WinDrawPixel(cx - x, cy - y);
        WinDrawPixel(cx - y, cy - x);
        WinDrawPixel(cx + y, cy - x);
        WinDrawPixel(cx + x, cy - y);

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

/* Text drawing helper */
static void DrawText(const char *str, Coord x, Coord y, UInt8 r, UInt8 g, UInt8 b, FontID font) {
    if (!str) return;
    FntSetFont(font);
    SetTextColor(r, g, b);
    SetDrawColor(r, g, b);
    WinDrawChars(str, StrLen(str), x, y);
}

static void DrawTextCentered(const char *str, Coord x, Coord y, Coord w, UInt8 r, UInt8 g, UInt8 b, FontID font) {
    Int16 tw;
    Int16 cx;
    if (!str) return;
    FntSetFont(font);
    tw = FntCharsWidth(str, StrLen(str));
    cx = x + (w - tw) / 2;
    if (cx < x) cx = x;
    SetTextColor(r, g, b);
    SetDrawColor(r, g, b);
    WinDrawChars(str, StrLen(str), cx, y);
}

Boolean Render_Init(WinHandle displayWin) {
    UInt16 err = 0;
    Coord w = 0, h = 0;
    s_displayWin = displayWin;

    WinGetDisplayExtent(&w, &h);
    if (w >= 300) {
        s_render_mode = RENDER_MODE_320x480;
        s_screen_w = 320;
        s_screen_h = (h >= 480) ? 480 : h;
        s_is_lowres = false;
    } else if (h >= 190) {
        s_render_mode = RENDER_MODE_160x240;
        s_screen_w = 160;
        s_screen_h = (h > 240) ? 240 : h;
        s_is_lowres = false;
    } else {
        s_render_mode = RENDER_MODE_160x160;
        s_screen_w = 160;
        s_screen_h = 160;
        s_is_lowres = true;
    }

    if (s_offscreenWin) {
        WinDeleteWindow(s_offscreenWin, false);
        s_offscreenWin = NULL;
    }

    /* Create offscreen buffer for flicker-free double buffering */
    s_offscreenWin = WinCreateOffscreenWindow(s_screen_w, s_screen_h, screenFormat, &err);
    if (!s_offscreenWin) {
        return false;
    }
    return true;
}

void Render_Cleanup(void) {
    if (s_offscreenWin) {
        WinDeleteWindow(s_offscreenWin, false);
        s_offscreenWin = NULL;
    }
}

void Render_SetTomeVisible(Boolean visible) {
    s_tome_open = visible;
    if (visible) {
        s_tome_page = 0;
    }
}

Boolean Render_IsTomeVisible(void) {
    return s_tome_open;
}

void Render_TomeScroll(int delta) {
    int per_page;
    int max_pages;
    if (s_render_mode == RENDER_MODE_160x160) {
        per_page = TOME_PER_PAGE_LO;
    } else if (s_render_mode == RENDER_MODE_160x240) {
        per_page = TOME_PER_PAGE_MID;
    } else {
        per_page = TOME_PER_PAGE_HI;
    }
    max_pages = (CHAR_MAX_COUNT + per_page - 1) / per_page;
    s_tome_page += delta;
    if (s_tome_page < 0) s_tome_page = 0;
    if (s_tome_page >= max_pages) s_tome_page = max_pages - 1;
}

/* Geometry calculations */
static void GetBoardMetrics(Coord *out_ox, Coord *out_oy, Coord *out_csize) {
    int n = g_game.board_size;
    if (s_render_mode == RENDER_MODE_160x160) {
        if (n >= 9) {
            *out_csize = 12;
            *out_ox = (160 - (n * 12)) / 2;
            *out_oy = 2;
        } else if (n >= 8) {
            *out_csize = 14;
            *out_ox = (160 - (n * 14)) / 2;
            *out_oy = 2;
        } else {
            *out_csize = 16;
            *out_ox = (160 - (n * 16)) / 2;
            *out_oy = 2;
        }
    } else if (s_render_mode == RENDER_MODE_160x240) {
        if (n >= 9) {
            *out_csize = 15;
            *out_ox = (160 - (n * 15)) / 2;
            *out_oy = 2;
        } else {
            *out_csize = 19;
            *out_ox = (160 - (n * 19)) / 2;
            *out_oy = 2;
        }
    } else {
        if (n >= 9) {
            *out_csize = 26;
            *out_ox = (s_screen_w - (n * 26)) / 2;
            *out_oy = 6;
        } else {
            *out_csize = 32;
            *out_ox = (s_screen_w - (n * 32)) / 2;
            *out_oy = 8;
        }
    }
}

/* Piece rendering */
static void DrawPiece(Coord cx, Coord cy, Int16 radius, int cell_type) {
    if (radius < 3) radius = 3;

    if (cell_type == CELL_RED) {
        /* Crimson Cult Horror: glowing 3D orb with specular highlight */
        COLORREF c = g_game.color_red;
        UInt8 r = (UInt8)((c >> 16) & 0xFF);
        UInt8 g = (UInt8)((c >> 8) & 0xFF);
        UInt8 b = (UInt8)(c & 0xFF);

        /* Outer shadow rim */
        SetDrawColor(r / 2, g / 2, b / 2);
        DrawFilledCircle(cx, cy, radius);

        /* Main body */
        SetDrawColor(r, g, b);
        DrawFilledCircle(cx, cy, radius - 1);

        /* Specular highlight */
        SetDrawColor(255, 220, 220);
        DrawFilledCircle(cx - radius / 3, cy - radius / 3, (radius > 5) ? (radius / 4) : 1);
    } else if (cell_type == CELL_BLUE) {
        /* Elder God AI Horror: cosmic sapphire orb */
        SetDrawColor(15, 45, 140);
        DrawFilledCircle(cx, cy, radius);

        SetDrawColor(50, 130, 245);
        DrawFilledCircle(cx, cy, radius - 1);

        SetDrawColor(200, 235, 255);
        DrawFilledCircle(cx - radius / 3, cy - radius / 3, (radius > 5) ? (radius / 4) : 1);
    } else if (cell_type == CELL_OBSTACLE) {
        /* Monolith / Tentacle Barrier */
        Int16 hw = radius - 1;
        DrawBeveledRect(cx - hw, cy - hw, hw * 2, hw * 2,
                       65, 70, 80, 110, 115, 130, 30, 32, 40);
        /* Red Eldritch Eye */
        SetDrawColor(220, 30, 30);
        DrawFilledCircle(cx, cy, (hw >= 4) ? 2 : 1);
        SetDrawColor(255, 200, 200);
        WinDrawPixel(cx, cy);
    } else if (cell_type == CELL_PERM_OBSTACLE) {
        /* Rhan-Tegoth Permanent Ice Block */
        Int16 hw = radius - 1;
        DrawBeveledRect(cx - hw, cy - hw, hw * 2, hw * 2,
                       120, 210, 240, 220, 245, 255, 60, 130, 170);
        SetDrawColor(255, 255, 255);
        WinDrawLine(cx - hw + 1, cy - hw + 1, cx + hw - 1, cy + hw - 1);
    } else if (cell_type == CELL_PRESENT_OBSTACLE) {
        /* Santa Present Obstacle */
        Int16 hw = radius - 1;
        DrawBeveledRect(cx - hw, cy - hw, hw * 2, hw * 2,
                       200, 20, 20, 245, 80, 80, 100, 10, 10);
        /* Gold Ribbon Cross */
        SetDrawColor(255, 215, 0);
        WinDrawLine(cx, cy - hw, cx, cy + hw - 1);
        WinDrawLine(cx - hw, cy, cx + hw - 1, cy);
    }
}

/* Board drawing */
static void DrawBoard(void) {
    Coord ox, oy, csize;
    int n = g_game.board_size;
    int r, c;

    GetBoardMetrics(&ox, &oy, &csize);

    /* Board background frame */
    DrawBeveledRect(ox - 2, oy - 2, (n * csize) + 4, (n * csize) + 4,
                   18, 20, 30, 70, 75, 95, 10, 10, 18);

    for (r = 0; r < n; r++) {
        for (c = 0; c < n; c++) {
            Coord cell_x = ox + (c * csize);
            Coord cell_y = oy + (r * csize);
            Coord cx = cell_x + (csize / 2);
            Coord cy = cell_y + (csize / 2);
            Boolean is_hires = (s_render_mode == RENDER_MODE_320x480);
            Int16 pradius = (csize / 2) - (is_hires ? 3 : 2);
            int cell_val = g_game.board[r][c];

            /* Tile background with subtle checker tone */
            UInt8 bg_shade = ((r + c) % 2 == 0) ? 36 : 28;
            DrawBeveledRect(cell_x + 1, cell_y + 1, csize - 2, csize - 2,
                           bg_shade, bg_shade + 4, bg_shade + 10,
                           bg_shade + 20, bg_shade + 24, bg_shade + 30,
                           bg_shade - 14, bg_shade - 14, bg_shade - 10);

            /* Valid Move Highlights */
            if (g_game.has_selected) {
                int mtype = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
                if (mtype == MOVE_CLONE) {
                    /* Clone (Distance 1): Glowing green target dot */
                    SetDrawColor(50, 230, 100);
                    DrawFilledCircle(cx, cy, is_hires ? 4 : 2);
                } else if (mtype == MOVE_LEAP) {
                    /* Leap (Distance 2): Glowing amber target ring */
                    SetDrawColor(240, 190, 40);
                    DrawCircleOutline(cx, cy, is_hires ? 5 : ((csize >= 18) ? 4 : 3));
                    if (is_hires) DrawCircleOutline(cx, cy, 4);
                }
            }

            /* Draw Piece if present */
            if (cell_val != CELL_EMPTY) {
                DrawPiece(cx, cy, pradius, cell_val);
            }

            /* Selected Cell Highlight: Glowing pulsing ring */
            if (g_game.has_selected && r == g_game.selected_r && c == g_game.selected_c) {
                SetDrawColor(255, 230, 50);
                DrawCircleOutline(cx, cy, pradius + (is_hires ? 2 : 1));
                if (is_hires) DrawCircleOutline(cx, cy, pradius + 1);
            }

            /* 5-way D-Pad Cursor */
            if (r == s_cursor_r && c == s_cursor_c) {
                SetDrawColor(255, 255, 100);
                WinDrawLine(cell_x + 1, cell_y + 1, cell_x + csize - 2, cell_y + 1);
                WinDrawLine(cell_x + 1, cell_y + 1, cell_x + 1, cell_y + csize - 2);
                WinDrawLine(cell_x + 1, cell_y + csize - 2, cell_x + csize - 2, cell_y + csize - 2);
                WinDrawLine(cell_x + csize - 2, cell_y + 1, cell_x + csize - 2, cell_y + csize - 2);
            }
        }
    }
}

/* --- High-Res (320x480) Layout Components --- */

static void DrawHUD(void) {
    char s_red[32], s_blue[32], s_status[48];
    Coord hy = 238;

    DrawBeveledRect(8, hy, 304, 38, 20, 22, 34, 55, 60, 80, 10, 10, 16);

    /* Player Red (Left) */
    SetDrawColor(239, 68, 68);
    DrawFilledCircle(20, hy + 19, 6);
    StrPrintF(s_red, "RED: %d", g_game.scores[CELL_RED]);
    DrawText(s_red, 32, hy + 6, 255, 100, 100, boldFont);
    DrawText(g_game.player_name_red, 32, hy + 20, 200, 160, 160, stdFont);

    /* Turn & Timer Status (Center) */
    if (g_game.game_over) {
        DrawTextCentered("DUEL CONCLUDED", 100, hy + 6, 120, 255, 215, 0, boldFont);
    } else {
        if (g_game.current_player == CELL_RED) {
            DrawTextCentered("YOUR TURN", 100, hy + 6, 120, 255, 80, 80, boldFont);
        } else {
            DrawTextCentered("AI THINKING...", 100, hy + 6, 120, 100, 180, 255, boldFont);
        }
    }

    if (g_game.time_distortion == 0.0f) {
        StrCopy(s_status, "TIME: STASIS");
    } else {
        int m = g_game.game_timer / 60;
        int s = g_game.game_timer % 60;
        StrPrintF(s_status, "TIME: %d:%02d", m, s);
    }
    DrawTextCentered(s_status, 100, hy + 20, 120, 200, 200, 210, stdFont);

    /* AI Blue (Right) */
    SetDrawColor(59, 130, 246);
    DrawFilledCircle(300, hy + 19, 6);
    StrPrintF(s_blue, "BLUE: %d", g_game.scores[CELL_BLUE]);
    DrawText(s_blue, 230, hy + 6, 120, 180, 255, boldFont);
    DrawText("Elder AI", 238, hy + 20, 160, 180, 220, stdFont);
}

static void DrawGrimoire(void) {
    Coord gy = 280;
    Coord gh = 78;
    int i;

    DrawBeveledRect(8, gy, 304, gh, 16, 14, 22, 60, 50, 75, 8, 8, 12);
    DrawTextCentered("--- THE GRIMOIRE ---", 8, gy + 4, 304, 180, 160, 210, boldFont);

    for (i = 0; i < GRIMOIRE_MAX && i < g_game.grimoire_count; i++) {
        Coord line_y = gy + 19 + (i * 11);
        if (i == 0) {
            DrawText(g_game.grimoire[i].text, 14, line_y, 255, 230, 120, boldFont);
        } else {
            UInt8 lum = (UInt8)(180 - (i * 25));
            DrawText(g_game.grimoire[i].text, 14, line_y, lum, lum - 10, lum + 15, stdFont);
        }
    }
}

static void DrawDeityBanner(void) {
    Coord by = 362;
    Coord bh = 42;
    const SecretInfo *sec = &g_secrets[g_game.char_red];
    char title_buf[80];

    DrawBeveledRect(8, by, 304, bh, 22, 18, 28, 70, 60, 85, 10, 8, 14);

    StrPrintF(title_buf, "%s: %s", sec->name, sec->title);
    DrawText(title_buf, 14, by + 4, 255, 210, 80, boldFont);
    DrawText(sec->description, 14, by + 18, 210, 210, 220, stdFont);
}

static void DrawButtons(void) {
    const char *diff_names[] = { "AI: MORTAL", "AI: ELDER", "AI: ANCIENT" };
    const char *snd_text = Sound_IsMuted() ? "SOUND: OFF" : "SOUND: ON";

    /* Row 1 */
    DrawBeveledRect(8, 410, 148, 28, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered("NEW GAME", 8, 410 + 7, 148, 255, 255, 255, boldFont);

    DrawBeveledRect(164, 410, 148, 28, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered(diff_names[g_game.difficulty % 3], 164, 410 + 7, 148, 255, 255, 255, boldFont);

    /* Row 2 */
    DrawBeveledRect(8, 444, 148, 28, 65, 30, 80, 130, 70, 160, 30, 15, 40);
    DrawTextCentered("TOME OF LORE", 8, 444 + 7, 148, 255, 230, 120, boldFont);

    DrawBeveledRect(164, 444, 148, 28, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered(snd_text, 164, 444 + 7, 148, 255, 255, 255, boldFont);
}

static void DrawTomeModal(void) {
    int start_idx = s_tome_page * TOME_PER_PAGE_HI;
    int i;
    char page_str[32];

    /* Backdrop */
    DrawFilledRect(0, 0, 320, 480, 14, 12, 18, 0);

    /* Header */
    DrawBeveledRect(6, 6, 308, 40, 28, 20, 36, 80, 60, 95, 12, 10, 18);
    DrawTextCentered("THE TOME OF FORBIDDEN LORE", 6, 10, 308, 255, 215, 0, boldFont);
    DrawTextCentered("Tap an Ancient One to awaken its cosmic power", 6, 26, 308, 200, 190, 210, stdFont);

    /* Entries (6 cards per page) */
    for (i = 0; i < TOME_PER_PAGE_HI; i++) {
        int idx = start_idx + i;
        Coord ey = 50 + (i * 54);
        const SecretInfo *sec;
        char name_buf[80];

        if (idx >= CHAR_MAX_COUNT) break;
        sec = &g_secrets[idx];

        if (g_game.char_red == (enum CharType)idx) {
            DrawBeveledRect(8, ey, 304, 50, 40, 30, 50, 255, 215, 0, 120, 100, 0);
        } else {
            DrawBeveledRect(8, ey, 304, 50, 24, 20, 30, 65, 55, 75, 12, 10, 16);
        }

        StrPrintF(name_buf, "%d. %s - %s", idx + 1, sec->name, sec->title);
        DrawText(name_buf, 14, ey + 6, 255, 215, 80, boldFont);
        DrawText(sec->description, 14, ey + 24, 210, 210, 220, stdFont);
    }

    /* Footer Pagination */
    StrPrintF(page_str, "PAGE %d OF %d", s_tome_page + 1, (CHAR_MAX_COUNT + TOME_PER_PAGE_HI - 1) / TOME_PER_PAGE_HI);
    DrawTextCentered(page_str, 100, 386, 120, 255, 255, 255, boldFont);

    /* Pagination buttons */
    DrawBeveledRect(10, 380, 80, 28, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered("< PREV", 10, 387, 80, 255, 255, 255, stdFont);

    DrawBeveledRect(230, 380, 80, 28, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered("NEXT >", 230, 387, 80, 255, 255, 255, stdFont);

    /* Close / Resume Button */
    DrawBeveledRect(50, 424, 220, 36, 80, 30, 40, 160, 70, 90, 40, 15, 20);
    DrawTextCentered("RESUME THE DUEL", 50, 434, 220, 255, 255, 255, boldFont);
}

/* --- HVGA (160x240 Palm T|X / T3 / LifeDrive) Layout Components --- */

static void DrawHUD_160x240(void) {
    char s_red[16], s_blue[16], s_status[32];
    Coord hy = 138;
    Coord hw = 156;
    Coord hh = 22;
    Coord hx = 2;

    DrawBeveledRect(hx, hy, hw, hh, 20, 22, 34, 55, 60, 80, 10, 10, 16);

    /* Red score (Left) */
    SetDrawColor(239, 68, 68);
    DrawFilledCircle(hx + 6, hy + 6, 3);
    StrPrintF(s_red, "R:%d", g_game.scores[CELL_RED]);
    DrawText(s_red, hx + 12, hy + 2, 255, 120, 120, boldFont);
    DrawText(g_secrets[g_game.char_red].name, hx + 3, hy + 12, 210, 160, 160, stdFont);

    /* Turn & Timer Status (Center) */
    if (g_game.game_over) {
        DrawTextCentered("DUEL OVER", hx + 38, hy + 2, 80, 255, 215, 0, boldFont);
    } else if (g_game.current_player == CELL_BLUE) {
        DrawTextCentered("AI THINKING", hx + 38, hy + 2, 80, 100, 180, 255, boldFont);
    } else {
        DrawTextCentered("YOUR TURN", hx + 38, hy + 2, 80, 255, 90, 90, boldFont);
    }

    if (g_game.time_distortion == 0.0f) {
        StrCopy(s_status, "STASIS");
    } else {
        int m = g_game.game_timer / 60;
        int s = g_game.game_timer % 60;
        StrPrintF(s_status, "%d:%02d", m, s);
    }
    DrawTextCentered(s_status, hx + 38, hy + 12, 80, 200, 200, 215, stdFont);

    /* Blue score (Right) */
    StrPrintF(s_blue, "%d:B", g_game.scores[CELL_BLUE]);
    DrawText(s_blue, hx + hw - 33, hy + 2, 120, 180, 255, boldFont);
    SetDrawColor(59, 130, 246);
    DrawFilledCircle(hx + hw - 6, hy + 6, 3);
    DrawText("Elder AI", hx + hw - 36, hy + 12, 160, 180, 220, stdFont);
}

static void DrawGrimoire_160x240(void) {
    Coord gy = 162;
    Coord gw = 156;
    Coord gh = 34;
    Coord gx = 2;
    int i;

    DrawBeveledRect(gx, gy, gw, gh, 16, 14, 22, 60, 50, 75, 8, 8, 12);
    DrawTextCentered("--- THE GRIMOIRE ---", gx, gy + 2, gw, 180, 160, 210, boldFont);

    for (i = 0; i < 2 && i < g_game.grimoire_count; i++) {
        Coord line_y = gy + 12 + (i * 10);
        if (i == 0) {
            DrawText(g_game.grimoire[i].text, gx + 4, line_y, 255, 230, 120, stdFont);
        } else {
            DrawText(g_game.grimoire[i].text, gx + 4, line_y, 160, 155, 185, stdFont);
        }
    }
}

static void DrawButtons_160x240(void) {
    const char *diff_names[] = { "MORT", "ELDR", "ANCT" };
    const char *snd_text = Sound_IsMuted() ? "S:OFF" : "S:ON";
    Coord by = 198;
    Coord bh = 24;

    /* [NEW] button: x = 2..38 */
    DrawBeveledRect(2, by, 36, bh, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered("NEW", 2, by + 6, 36, 255, 255, 255, boldFont);

    /* [DIF] button: x = 41..78 */
    DrawBeveledRect(41, by, 37, bh, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered(diff_names[g_game.difficulty % 3], 41, by + 6, 37, 255, 255, 255, boldFont);

    /* [LORE] button: x = 81..119 */
    DrawBeveledRect(81, by, 38, bh, 65, 30, 80, 130, 70, 160, 30, 15, 40);
    DrawTextCentered("LORE", 81, by + 6, 38, 255, 230, 120, boldFont);

    /* [SND] button: x = 122..158 */
    DrawBeveledRect(122, by, 36, bh, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered(snd_text, 122, by + 6, 36, 255, 255, 255, boldFont);
}

static void DrawTomeModal_160x240(void) {
    int max_pages = (CHAR_MAX_COUNT + TOME_PER_PAGE_MID - 1) / TOME_PER_PAGE_MID;
    int start_idx = s_tome_page * TOME_PER_PAGE_MID;
    int i;
    char page_str[24];
    Coord by = 194;
    Coord bh = 24;

    /* Backdrop */
    DrawFilledRect(0, 0, 160, s_screen_h, 14, 12, 18, 0);

    /* Header */
    DrawBeveledRect(2, 2, 156, 17, 28, 20, 36, 80, 60, 95, 12, 10, 18);
    DrawText("TOME OF LORE", 6, 4, 255, 215, 0, boldFont);
    StrPrintF(page_str, "%d/%d", s_tome_page + 1, max_pages);
    DrawText(page_str, 128, 4, 200, 200, 220, stdFont);

    /* 4 Cards per page */
    for (i = 0; i < TOME_PER_PAGE_MID; i++) {
        int idx = start_idx + i;
        Coord cy = 22 + (i * 42);
        const SecretInfo *sec;
        char title_buf[64];

        if (idx >= CHAR_MAX_COUNT) break;
        sec = &g_secrets[idx];

        if (g_game.char_red == (enum CharType)idx) {
            DrawBeveledRect(3, cy, 154, 40, 40, 30, 50, 255, 215, 0, 120, 100, 0);
        } else {
            DrawBeveledRect(3, cy, 154, 40, 24, 20, 30, 65, 55, 75, 12, 10, 16);
        }

        StrPrintF(title_buf, "%d. %s - %s", idx + 1, sec->name, sec->title);
        DrawText(title_buf, 6, cy + 3, 255, 215, 80, boldFont);
        DrawText(sec->description, 6, cy + 16, 200, 200, 210, stdFont);
    }

    /* Navigation buttons */
    /* [< PREV] */
    DrawBeveledRect(3, by, 46, bh, 35, 40, 55, 80, 90, 120, 16, 18, 24);
    DrawTextCentered("< PREV", 3, by + 6, 46, 220, 220, 230, boldFont);

    /* [NEXT >] */
    DrawBeveledRect(52, by, 46, bh, 35, 40, 55, 80, 90, 120, 16, 18, 24);
    DrawTextCentered("NEXT >", 52, by + 6, 46, 220, 220, 230, boldFont);

    /* [DONE] */
    DrawBeveledRect(101, by, 56, bh, 70, 35, 45, 140, 70, 90, 35, 18, 22);
    DrawTextCentered("DONE", 101, by + 6, 56, 255, 230, 140, boldFont);
}

/* --- Low-Res (160x160 Palm Z22) Layout Components --- */

static void DrawHUDLowRes(void) {
    char s_red[16], s_blue[16], s_status[32];
    Coord hy = 117;
    Coord hw = 156;
    Coord hh = 16;
    Coord hx = 2;

    DrawBeveledRect(hx, hy, hw, hh, 20, 22, 34, 55, 60, 80, 10, 10, 16);

    /* Red score */
    SetDrawColor(239, 68, 68);
    DrawFilledCircle(hx + 7, hy + 8, 3);
    StrPrintF(s_red, "R:%d", g_game.scores[CELL_RED]);
    DrawText(s_red, hx + 13, hy + 2, 255, 120, 120, stdFont);

    /* Center status */
    if (g_game.game_over) {
        DrawTextCentered("DUEL OVER", hx + 38, hy + 2, 80, 255, 215, 0, stdFont);
    } else if (g_game.current_player == CELL_BLUE) {
        DrawTextCentered("AI THINKING", hx + 38, hy + 2, 80, 100, 180, 255, stdFont);
    } else {
        int m = g_game.game_timer / 60;
        int s = g_game.game_timer % 60;
        StrPrintF(s_status, "T: %d:%02d", m, s);
        DrawTextCentered(s_status, hx + 38, hy + 2, 80, 220, 220, 230, stdFont);
    }

    /* Blue score */
    StrPrintF(s_blue, "%d:B", g_game.scores[CELL_BLUE]);
    DrawText(s_blue, hx + hw - 36, hy + 2, 120, 180, 255, stdFont);
    SetDrawColor(59, 130, 246);
    DrawFilledCircle(hx + hw - 7, hy + 8, 3);
}

static void DrawButtonsLowRes(void) {
    const char *diff_names[] = { "MORT", "ELDR", "ANCT" };
    const char *snd_text = Sound_IsMuted() ? "S:OFF" : "S:ON";
    Coord by = 136;
    Coord bh = 22;

    /* [NEW] button */
    DrawBeveledRect(2, by, 36, bh, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered("NEW", 2, by + 4, 36, 255, 255, 255, stdFont);

    /* [DIF] button */
    DrawBeveledRect(40, by, 36, bh, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered(diff_names[g_game.difficulty % 3], 40, by + 4, 36, 255, 255, 255, stdFont);

    /* [LORE] button */
    DrawBeveledRect(78, by, 44, bh, 65, 30, 80, 130, 70, 160, 30, 15, 40);
    DrawTextCentered("LORE", 78, by + 4, 44, 255, 230, 120, stdFont);

    /* [SND] button */
    DrawBeveledRect(124, by, 34, bh, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered(snd_text, 124, by + 4, 34, 255, 255, 255, stdFont);
}

static void DrawTomeModalLowRes(void) {
    int max_pages = (CHAR_MAX_COUNT + TOME_PER_PAGE_LO - 1) / TOME_PER_PAGE_LO;
    int start_idx = s_tome_page * TOME_PER_PAGE_LO;
    int i;
    char page_str[24];
    Coord by = 135;
    Coord bh = 22;

    /* Backdrop */
    DrawFilledRect(0, 0, 160, 160, 14, 12, 18, 0);

    /* Header */
    DrawBeveledRect(2, 2, 156, 15, 28, 20, 36, 80, 60, 95, 12, 10, 18);
    DrawText("TOME OF LORE", 6, 3, 255, 215, 0, boldFont);
    StrPrintF(page_str, "%d/%d", s_tome_page + 1, max_pages);
    DrawText(page_str, 128, 3, 200, 200, 220, stdFont);

    /* 3 Cards per page */
    for (i = 0; i < TOME_PER_PAGE_LO; i++) {
        int idx = start_idx + i;
        Coord cy = 19 + (i * 38);
        const SecretInfo *sec;
        char title_buf[48];

        if (idx >= CHAR_MAX_COUNT) break;
        sec = &g_secrets[idx];

        if (g_game.char_red == (enum CharType)idx) {
            DrawBeveledRect(3, cy, 154, 36, 40, 30, 50, 255, 215, 0, 120, 100, 0);
        } else {
            DrawBeveledRect(3, cy, 154, 36, 24, 20, 30, 65, 55, 75, 12, 10, 16);
        }

        StrPrintF(title_buf, "%d. %s", idx + 1, sec->name);
        DrawText(title_buf, 8, cy + 3, 255, 215, 80, boldFont);
        DrawText(sec->description, 8, cy + 18, 200, 200, 210, stdFont);
    }

    /* Bottom navigation buttons */

    /* [< PREV] */
    DrawBeveledRect(3, by, 48, bh, 35, 40, 55, 80, 90, 120, 16, 18, 24);
    DrawTextCentered("< PREV", 3, by + 5, 48, 220, 220, 230, stdFont);

    /* [NEXT >] */
    DrawBeveledRect(54, by, 48, bh, 35, 40, 55, 80, 90, 120, 16, 18, 24);
    DrawTextCentered("NEXT >", 54, by + 5, 48, 220, 220, 230, stdFont);

    /* [DONE] */
    DrawBeveledRect(105, by, 52, bh, 70, 35, 45, 140, 70, 90, 35, 18, 22);
    DrawTextCentered("DONE", 105, by + 5, 52, 255, 230, 140, boldFont);
}

/* Master Draw Routine */
void Render_DrawAll(void) {
    RectangleType fullRect;
    fullRect.topLeft.x = 0;
    fullRect.topLeft.y = 0;
    fullRect.extent.x = s_screen_w;
    fullRect.extent.y = s_screen_h;

    if (!s_offscreenWin || !s_displayWin) return;

    /* Direct all drawing to the offscreen buffer */
    WinSetDrawWindow(s_offscreenWin);

    if (s_tome_open) {
        if (s_render_mode == RENDER_MODE_160x160) {
            DrawTomeModalLowRes();
        } else if (s_render_mode == RENDER_MODE_160x240) {
            DrawTomeModal_160x240();
        } else {
            DrawTomeModal();
        }
    } else {
        if (s_render_mode == RENDER_MODE_160x160) {
            /* Cosmic starfield backdrop */
            DrawFilledRect(0, 0, 160, 160, 12, 14, 22, 0);
            DrawBoard();
            DrawHUDLowRes();
            DrawButtonsLowRes();
        } else if (s_render_mode == RENDER_MODE_160x240) {
            /* Cosmic starfield backdrop */
            DrawFilledRect(0, 0, s_screen_w, s_screen_h, 12, 14, 22, 0);
            DrawBoard();
            DrawHUD_160x240();
            DrawGrimoire_160x240();
            DrawButtons_160x240();
        } else {
            /* Cosmic starfield backdrop */
            DrawFilledRect(0, 0, 320, 480, 12, 14, 22, 0);
            DrawBoard();
            DrawHUD();
            DrawGrimoire();
            DrawDeityBanner();
            DrawButtons();
        }
    }

    /* Blit offscreen buffer to active display window in one pass */
    WinSetDrawWindow(s_displayWin);
    WinCopyRectangle(s_offscreenWin, s_displayWin, &fullRect, 0, 0, winPaint);
}

Boolean Render_HandleClick(Coord x, Coord y) {
    Coord ox, oy, csize;
    int n = g_game.board_size;

    if (s_render_mode == RENDER_MODE_160x240) {
        if (s_tome_open) {
            /* 4 cards per page on 160x240 */
            if (y >= 22 && y < 22 + (4 * 42) && x >= 3 && x < 157) {
                int card = (y - 22) / 42;
                int idx = (s_tome_page * TOME_PER_PAGE_MID) + card;
                if (idx >= 0 && idx < CHAR_MAX_COUNT) {
                    Game_ApplySecret((enum CharType)idx, CELL_RED);
                    s_tome_open = false;
                    Sound_PlaySFX("select.wav");
                    Render_DrawAll();
                    return true;
                }
            }
            /* Prev Button */
            if (x >= 3 && x < 49 && y >= 194 && y < 218) {
                Render_TomeScroll(-1);
                Sound_PlaySFX("select.wav");
                Render_DrawAll();
                return true;
            }
            /* Next Button */
            if (x >= 52 && x < 98 && y >= 194 && y < 218) {
                Render_TomeScroll(1);
                Sound_PlaySFX("select.wav");
                Render_DrawAll();
                return true;
            }
            /* Done Button */
            if (x >= 101 && x < 157 && y >= 194 && y < 218) {
                s_tome_open = false;
                Sound_PlaySFX("select.wav");
                Render_DrawAll();
                return true;
            }
            return true;
        }

        /* Main Screen Buttons (160x240): y = 198..222 */
        /* [NEW] */
        if (x >= 2 && x < 39 && y >= 198 && y < 224) {
            Sound_PlaySFX("place.wav");
            Game_ResetGame();
            Render_DrawAll();
            return true;
        }

        /* [DIF] */
        if (x >= 41 && x < 79 && y >= 198 && y < 224) {
            g_game.difficulty = (g_game.difficulty + 1) % 3;
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return true;
        }

        /* [LORE] */
        if (x >= 81 && x < 120 && y >= 198 && y < 224) {
            Sound_PlaySFX("select.wav");
            Render_SetTomeVisible(true);
            Render_DrawAll();
            return true;
        }

        /* [SND] */
        if (x >= 122 && x < 158 && y >= 198 && y < 224) {
            Sound_ToggleMute();
            if (!Sound_IsMuted()) {
                Sound_PlayTone(600, 30, 40);
            }
            Render_DrawAll();
            return true;
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
            return true;
        }

        return false;
    }

    if (s_render_mode == RENDER_MODE_160x160) {
        if (s_tome_open) {
            /* 3 cards per page on 160x160 */
            if (y >= 19 && y < 19 + (3 * 38) && x >= 3 && x < 157) {
                int card = (y - 19) / 38;
                int idx = (s_tome_page * 3) + card;
                if (idx >= 0 && idx < CHAR_MAX_COUNT) {
                    Game_ApplySecret((enum CharType)idx, CELL_RED);
                    s_tome_open = false;
                    Sound_PlaySFX("select.wav");
                    Render_DrawAll();
                    return true;
                }
            }
            /* Prev Button */
            if (x >= 3 && x < 51 && y >= 135 && y < 158) {
                Render_TomeScroll(-1);
                Sound_PlaySFX("select.wav");
                Render_DrawAll();
                return true;
            }
            /* Next Button */
            if (x >= 54 && x < 102 && y >= 135 && y < 158) {
                Render_TomeScroll(1);
                Sound_PlaySFX("select.wav");
                Render_DrawAll();
                return true;
            }
            /* Done Button */
            if (x >= 105 && x < 157 && y >= 135 && y < 158) {
                s_tome_open = false;
                Sound_PlaySFX("select.wav");
                Render_DrawAll();
                return true;
            }
            return true;
        }

        /* Low-res buttons at y: 136..158 */
        /* [NEW] */
        if (x >= 2 && x < 38 && y >= 136 && y < 158) {
            Sound_PlaySFX("place.wav");
            Game_ResetGame();
            Render_DrawAll();
            return true;
        }

        /* [DIF] */
        if (x >= 40 && x < 76 && y >= 136 && y < 158) {
            g_game.difficulty = (g_game.difficulty + 1) % 3;
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return true;
        }

        /* [LORE] */
        if (x >= 78 && x < 122 && y >= 136 && y < 158) {
            Sound_PlaySFX("select.wav");
            Render_SetTomeVisible(true);
            Render_DrawAll();
            return true;
        }

        /* [SND] */
        if (x >= 124 && x < 158 && y >= 136 && y < 158) {
            Sound_ToggleMute();
            if (!Sound_IsMuted()) {
                Sound_PlayTone(600, 30, 40);
            }
            Render_DrawAll();
            return true;
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
            return true;
        }

        return false;
    }

    /* Otherwise RENDER_MODE_320x480 */
    if (s_tome_open) {
        /* Check entry taps (6 cards) on 320x480 */
        if (y >= 50 && y < 50 + (TOME_PER_PAGE_HI * 54) && x >= 8 && x < 312) {
            int card = (y - 50) / 54;
            int idx = (s_tome_page * TOME_PER_PAGE_HI) + card;
            if (idx >= 0 && idx < CHAR_MAX_COUNT) {
                Game_ApplySecret((enum CharType)idx, CELL_RED);
                s_tome_open = false;
                Sound_PlaySFX("select.wav");
                Render_DrawAll();
                return true;
            }
        }

        /* Prev Button */
        if (x >= 10 && x < 90 && y >= 380 && y < 408) {
            Render_TomeScroll(-1);
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return true;
        }

        /* Next Button */
        if (x >= 230 && x < 310 && y >= 380 && y < 408) {
            Render_TomeScroll(1);
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return true;
        }

        /* Resume Button */
        if (x >= 50 && x < 270 && y >= 424 && y < 460) {
            s_tome_open = false;
            Sound_PlaySFX("select.wav");
            Render_DrawAll();
            return true;
        }

        return true;
    }

    /* Main Screen Buttons (320x480) */
    /* Button 1: NEW GAME */
    if (x >= 8 && x < 156 && y >= 410 && y < 438) {
        Sound_PlaySFX("place.wav");
        Game_ResetGame();
        Render_DrawAll();
        return true;
    }

    /* Button 2: AI DIFFICULTY */
    if (x >= 164 && x < 312 && y >= 410 && y < 438) {
        g_game.difficulty = (g_game.difficulty + 1) % 3;
        Sound_PlaySFX("select.wav");
        Render_DrawAll();
        return true;
    }

    /* Button 3: TOME OF LORE */
    if (x >= 8 && x < 156 && y >= 444 && y < 472) {
        Sound_PlaySFX("select.wav");
        Render_SetTomeVisible(true);
        Render_DrawAll();
        return true;
    }

    /* Button 4: SOUND TOGGLE */
    if (x >= 164 && x < 312 && y >= 444 && y < 472) {
        Sound_ToggleMute();
        if (!Sound_IsMuted()) {
            Sound_PlayTone(600, 30, 40);
        }
        Render_DrawAll();
        return true;
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
        return true;
    }

    return false;
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
