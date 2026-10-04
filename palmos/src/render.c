#include "render.h"
#include "sound.h"
#include <StringMgr.h>

#define SCREEN_W 320
#define SCREEN_H 480

static WinHandle s_displayWin = NULL;
static WinHandle s_offscreenWin = NULL;

static Boolean s_tome_open = false;
static int s_tome_page = 0;
#define TOME_PER_PAGE 6

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

static void SetBackColor(UInt8 r, UInt8 g, UInt8 b) {
    RGBColorType rgb;
    rgb.index = 0;
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;
    WinSetBackColorRGB(&rgb, NULL);
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
    s_displayWin = displayWin;

    if (s_offscreenWin) {
        WinDeleteWindow(s_offscreenWin, false);
        s_offscreenWin = NULL;
    }

    /* Create 320x480 offscreen buffer for flicker-free double buffering */
    s_offscreenWin = WinCreateOffscreenWindow(SCREEN_W, SCREEN_H, screenFormat, &err);
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
    int max_pages = (CHAR_MAX_COUNT + TOME_PER_PAGE - 1) / TOME_PER_PAGE;
    s_tome_page += delta;
    if (s_tome_page < 0) s_tome_page = 0;
    if (s_tome_page >= max_pages) s_tome_page = max_pages - 1;
}

/* Geometry calculations */
static void GetBoardMetrics(Coord *out_ox, Coord *out_oy, Coord *out_csize) {
    int n = g_game.board_size;
    if (n >= 9) {
        *out_csize = 26;
        *out_ox = (SCREEN_W - (n * 26)) / 2;
        *out_oy = 6;
    } else {
        *out_csize = 32;
        *out_ox = (SCREEN_W - (n * 32)) / 2;
        *out_oy = 8;
    }
}

/* Piece rendering */
static void DrawPiece(Coord cx, Coord cy, Int16 radius, int cell_type) {
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
        DrawFilledCircle(cx - radius / 3, cy - radius / 3, radius / 4);
    } else if (cell_type == CELL_BLUE) {
        /* Elder God AI Horror: cosmic sapphire orb */
        SetDrawColor(15, 45, 140);
        DrawFilledCircle(cx, cy, radius);

        SetDrawColor(50, 130, 245);
        DrawFilledCircle(cx, cy, radius - 1);

        SetDrawColor(200, 235, 255);
        DrawFilledCircle(cx - radius / 3, cy - radius / 3, radius / 4);
    } else if (cell_type == CELL_OBSTACLE) {
        /* Monolith / Tentacle Barrier */
        Int16 hw = radius - 1;
        DrawBeveledRect(cx - hw, cy - hw, hw * 2, hw * 2,
                       65, 70, 80, 110, 115, 130, 30, 32, 40);
        /* Red Eldritch Eye */
        SetDrawColor(220, 30, 30);
        DrawFilledCircle(cx, cy, 3);
        SetDrawColor(255, 200, 200);
        WinDrawPixel(cx, cy);
    } else if (cell_type == CELL_PERM_OBSTACLE) {
        /* Rhan-Tegoth Permanent Ice Block */
        Int16 hw = radius - 1;
        DrawBeveledRect(cx - hw, cy - hw, hw * 2, hw * 2,
                       120, 210, 240, 220, 245, 255, 60, 130, 170);
        SetDrawColor(255, 255, 255);
        WinDrawLine(cx - hw + 2, cy - hw + 2, cx + hw - 2, cy + hw - 2);
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
    DrawBeveledRect(ox - 3, oy - 3, (n * csize) + 6, (n * csize) + 6,
                   18, 20, 30, 70, 75, 95, 10, 10, 18);

    for (r = 0; r < n; r++) {
        for (c = 0; c < n; c++) {
            Coord cell_x = ox + (c * csize);
            Coord cell_y = oy + (r * csize);
            Coord cx = cell_x + (csize / 2);
            Coord cy = cell_y + (csize / 2);
            Int16 pradius = (csize / 2) - 3;
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
                    DrawFilledCircle(cx, cy, 4);
                } else if (mtype == MOVE_LEAP) {
                    /* Leap (Distance 2): Glowing amber target ring */
                    SetDrawColor(240, 190, 40);
                    DrawCircleOutline(cx, cy, 5);
                    DrawCircleOutline(cx, cy, 4);
                }
            }

            /* Draw Piece if present */
            if (cell_val != CELL_EMPTY) {
                DrawPiece(cx, cy, pradius, cell_val);
            }

            /* Selected Cell Highlight: Glowing pulsing ring */
            if (g_game.has_selected && r == g_game.selected_r && c == g_game.selected_c) {
                SetDrawColor(255, 230, 50);
                DrawCircleOutline(cx, cy, pradius + 2);
                DrawCircleOutline(cx, cy, pradius + 1);
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

/* HUD Banner & Scores */
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

/* Grimoire Log */
static void DrawGrimoire(void) {
    Coord gy = 280;
    Coord gh = 78;
    int i;

    DrawBeveledRect(8, gy, 304, gh, 16, 14, 22, 60, 50, 75, 8, 8, 12);
    DrawTextCentered("--- THE GRIMOIRE ---", 8, gy + 4, 304, 180, 160, 210, boldFont);

    for (i = 0; i < GRIMOIRE_MAX && i < g_game.grimoire_count; i++) {
        Coord line_y = gy + 19 + (i * 11);
        if (i == 0) {
            /* Latest entry: bright eldritch gold */
            DrawText(g_game.grimoire[i].text, 14, line_y, 255, 230, 120, boldFont);
        } else {
            /* Past entries: dimmed cosmic purple/grey */
            UInt8 lum = (UInt8)(180 - (i * 25));
            DrawText(g_game.grimoire[i].text, 14, line_y, lum, lum - 10, lum + 15, stdFont);
        }
    }
}

/* Active Deity Lore Card */
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

/* Stylus Buttons */
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

/* Full Screen Tome of Forbidden Lore Modal */
static void DrawTomeModal(void) {
    int max_pages = (CHAR_MAX_COUNT + TOME_PER_PAGE - 1) / TOME_PER_PAGE;
    int start_idx = s_tome_page * TOME_PER_PAGE;
    int i;
    char page_str[32];

    /* Backdrop */
    DrawFilledRect(0, 0, SCREEN_W, SCREEN_H, 14, 12, 18, 0);

    /* Header */
    DrawBeveledRect(6, 6, 308, 40, 28, 20, 36, 80, 60, 95, 12, 10, 18);
    DrawTextCentered("THE TOME OF FORBIDDEN LORE", 6, 10, 308, 255, 215, 0, boldFont);
    DrawTextCentered("Tap an Ancient One to awaken its cosmic power", 6, 26, 308, 200, 190, 210, stdFont);

    /* Entries (6 cards per page) */
    for (i = 0; i < TOME_PER_PAGE; i++) {
        int idx = start_idx + i;
        Coord ey = 50 + (i * 54);
        const SecretInfo *sec;
        char name_buf[80];

        if (idx >= CHAR_MAX_COUNT) break;
        sec = &g_secrets[idx];

        /* Card background */
        if (g_game.char_red == (enum CharType)idx) {
            /* Currently active: highlighted gold border */
            DrawBeveledRect(8, ey, 304, 50, 40, 30, 50, 255, 215, 0, 120, 100, 0);
        } else {
            DrawBeveledRect(8, ey, 304, 50, 24, 20, 30, 65, 55, 75, 12, 10, 16);
        }

        /* Deity Name & Title */
        StrPrintF(name_buf, "%s - %s", sec->name, sec->title);
        DrawText(name_buf, 16, ey + 4, 255, 220, 100, boldFont);

        /* Description (2 lines possible) */
        DrawText(sec->description, 16, ey + 18, 210, 210, 220, stdFont);
        if (StrLen(sec->description) > 42) {
            DrawText(sec->description + 42, 16, ey + 32, 190, 190, 200, stdFont);
        }
    }

    /* Page Navigation & Resume Button */
    DrawBeveledRect(10, 380, 80, 28, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered("< PREV", 10, 387, 80, 255, 255, 255, boldFont);

    StrPrintF(page_str, "Page %d of %d", s_tome_page + 1, max_pages);
    DrawTextCentered(page_str, 95, 387, 130, 220, 210, 160, boldFont);

    DrawBeveledRect(230, 380, 80, 28, 40, 45, 60, 90, 100, 130, 20, 22, 30);
    DrawTextCentered("NEXT >", 230, 387, 80, 255, 255, 255, boldFont);

    /* Close / Resume Button */
    DrawBeveledRect(50, 424, 220, 36, 80, 30, 40, 160, 70, 90, 40, 15, 20);
    DrawTextCentered("RESUME THE DUEL", 50, 434, 220, 255, 255, 255, boldFont);
}

void Render_DrawAll(void) {
    RectangleType fullRect;
    fullRect.topLeft.x = 0;
    fullRect.topLeft.y = 0;
    fullRect.extent.x = SCREEN_W;
    fullRect.extent.y = SCREEN_H;

    if (!s_offscreenWin || !s_displayWin) return;

    /* Direct all drawing to the offscreen buffer */
    WinSetDrawWindow(s_offscreenWin);

    if (s_tome_open) {
        DrawTomeModal();
    } else {
        /* Cosmic starfield backdrop */
        DrawFilledRect(0, 0, SCREEN_W, SCREEN_H, 12, 14, 22, 0);

        DrawBoard();
        DrawHUD();
        DrawGrimoire();
        DrawDeityBanner();
        DrawButtons();
    }

    /* Blit offscreen buffer to active display window in one pass */
    WinSetDrawWindow(s_displayWin);
    WinCopyRectangle(s_offscreenWin, s_displayWin, &fullRect, 0, 0, winPaint);
}

Boolean Render_HandleClick(Coord x, Coord y) {
    Coord ox, oy, csize;
    int n = g_game.board_size;

    if (s_tome_open) {
        int max_pages = (CHAR_MAX_COUNT + TOME_PER_PAGE - 1) / TOME_PER_PAGE;

        /* Check entry taps (6 cards) */
        if (y >= 50 && y < 50 + (TOME_PER_PAGE * 54) && x >= 8 && x < 312) {
            int card = (y - 50) / 54;
            int idx = (s_tome_page * TOME_PER_PAGE) + card;
            if (idx >= 0 && idx < CHAR_MAX_COUNT) {
                Game_ApplySecret((enum CharType)idx, CELL_RED);
                s_tome_open = false;
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

    /* Main Screen Buttons */
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
