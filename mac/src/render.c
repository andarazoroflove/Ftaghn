#include "render.h"
#include "sound.h"
#include <stdio.h>
#include <string.h>

static GWorldPtr s_offscreen = NULL;
static Rect      s_bounds;

static inline RGBColor MakeRGB(int r, int g, int b) {
    RGBColor c;
    c.red   = (unsigned short)((r << 8) | r);
    c.green = (unsigned short)((g << 8) | g);
    c.blue  = (unsigned short)((b << 8) | b);
    return c;
}

static inline void SetForeRGB(int r, int g, int b) {
    RGBColor c = MakeRGB(r, g, b);
    RGBForeColor(&c);
}

static inline void SetBackRGB(int r, int g, int b) {
    RGBColor c = MakeRGB(r, g, b);
    RGBBackColor(&c);
}

void Render_Init(WindowRef window) {
    SetRect(&s_bounds, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (s_offscreen == NULL) {
        NewGWorld(&s_offscreen, 32, &s_bounds, NULL, NULL, 0);
    }
}

void Render_Cleanup(void) {
    if (s_offscreen != NULL) {
        DisposeGWorld(s_offscreen);
        s_offscreen = NULL;
    }
}

Boolean Render_GetCellAt(Point pt, int *out_r, int *out_c) {
    int x = pt.h - BOARD_OFFSET_X;
    int y = pt.v - BOARD_OFFSET_Y;
    if (x < 0 || y < 0) return false;

    int c = x / CELL_SIZE;
    int r = y / CELL_SIZE;

    if (r >= 0 && r < g_game.board_size && c >= 0 && c < g_game.board_size) {
        *out_r = r;
        *out_c = c;
        return true;
    }
    return false;
}

static void DrawCenteredText(const char *text, Rect *r, short font, short size, short face) {
    TextFont(font);
    TextSize(size);
    TextFace(face);

    int len = strlen(text);
    short text_w = TextWidth((Ptr)text, 0, len);
    FontInfo fi;
    GetFontInfo(&fi);

    short x = r->left + (r->right - r->left - text_w) / 2;
    short y = r->top + (r->bottom - r->top + fi.ascent - fi.descent) / 2;

    MoveTo(x, y);
    DrawText((Ptr)text, 0, len);
}

static void DrawPiece(int r, int c, int cell_type) {
    Rect cell_rect;
    SetRect(&cell_rect,
            BOARD_OFFSET_X + c * CELL_SIZE,
            BOARD_OFFSET_Y + r * CELL_SIZE,
            BOARD_OFFSET_X + (c + 1) * CELL_SIZE,
            BOARD_OFFSET_Y + (r + 1) * CELL_SIZE);

    Rect piece_rect = cell_rect;
    InsetRect(&piece_rect, 6, 6);

    if (cell_type == CELL_RED) {
        /* Crimson Spawn - Tentacled Eldritch Beast */
        /* Outer tentacle flares */
        SetForeRGB(136, 17, 17);
        Rect tentacle = cell_rect;
        InsetRect(&tentacle, 3, 3);
        PaintOval(&tentacle);

        /* Main crimson body */
        SetForeRGB(220, 38, 38);
        PaintOval(&piece_rect);

        /* Highlight ring */
        SetForeRGB(239, 68, 68);
        FrameOval(&piece_rect);

        /* Inner eldritch core */
        Rect core_rect = piece_rect;
        InsetRect(&core_rect, 7, 7);
        SetForeRGB(255, 112, 67);
        PaintOval(&core_rect);

        /* Dark abyss pupil */
        Rect pupil = core_rect;
        InsetRect(&pupil, 5, 4);
        SetForeRGB(26, 5, 5);
        PaintOval(&pupil);

        /* Pinpoint glint */
        Rect glint;
        SetRect(&glint, pupil.left + 2, pupil.top + 2, pupil.left + 4, pupil.top + 4);
        SetForeRGB(255, 255, 255);
        PaintOval(&glint);

    } else if (cell_type == CELL_BLUE) {
        /* Abyssal Watcher - Cosmic Eye */
        /* Outer dark aura */
        SetForeRGB(26, 35, 126);
        Rect aura = cell_rect;
        InsetRect(&aura, 3, 3);
        PaintOval(&aura);

        /* Main sapphire body */
        SetForeRGB(37, 99, 235);
        PaintOval(&piece_rect);

        /* Highlight ring */
        SetForeRGB(96, 165, 250);
        FrameOval(&piece_rect);

        /* Inner cyan iris */
        Rect iris = piece_rect;
        InsetRect(&iris, 7, 7);
        SetForeRGB(0, 229, 255);
        PaintOval(&iris);

        /* Vertical eldritch eye slit */
        Rect slit;
        SetRect(&slit, iris.left + 6, iris.top + 2, iris.right - 6, iris.bottom - 2);
        SetForeRGB(5, 11, 20);
        PaintOval(&slit);

        /* Celestial star reflection */
        Rect glint;
        SetRect(&glint, iris.left + 4, iris.top + 4, iris.left + 7, iris.top + 7);
        SetForeRGB(255, 255, 255);
        PaintOval(&glint);

    } else if (cell_type == CELL_OBSTACLE || cell_type == CELL_PERM_OBSTACLE) {
        /* Obsidian Monolith Block */
        Rect mono_rect = cell_rect;
        InsetRect(&mono_rect, 3, 3);

        /* Slate base */
        SetForeRGB(38, 50, 56);
        PaintRoundRect(&mono_rect, 6, 6);

        /* Top & left light bevel */
        SetForeRGB(69, 90, 100);
        MoveTo(mono_rect.left, mono_rect.bottom - 2);
        LineTo(mono_rect.left, mono_rect.top);
        LineTo(mono_rect.right - 2, mono_rect.top);

        /* Bottom & right shadow bevel */
        SetForeRGB(16, 23, 26);
        MoveTo(mono_rect.right - 1, mono_rect.top + 1);
        LineTo(mono_rect.right - 1, mono_rect.bottom - 1);
        LineTo(mono_rect.left + 1, mono_rect.bottom - 1);

        /* Ancient purple arcane rune in center */
        SetForeRGB(186, 104, 200);
        short cx = (mono_rect.left + mono_rect.right) / 2;
        short cy = (mono_rect.top + mono_rect.bottom) / 2;
        MoveTo(cx - 6, cy); LineTo(cx + 6, cy);
        MoveTo(cx, cy - 6); LineTo(cx, cy + 6);
        MoveTo(cx - 3, cy - 3); LineTo(cx + 3, cy + 3);
        MoveTo(cx - 3, cy + 3); LineTo(cx + 3, cy - 3);
    }
}

static void DrawHUD(void) {
    Rect hud_rect;
    SetRect(&hud_rect, 0, 0, WINDOW_WIDTH, BOARD_OFFSET_Y);

    /* Background */
    SetForeRGB(18, 19, 28);
    PaintRect(&hud_rect);

    /* Bottom border separator */
    SetForeRGB(45, 49, 66);
    MoveTo(0, BOARD_OFFSET_Y - 1);
    LineTo(WINDOW_WIDTH, BOARD_OFFSET_Y - 1);

    /* Red Score Pill */
    Rect red_pill;
    SetRect(&red_pill, 12, 8, 90, 36);
    SetForeRGB(220, 38, 38);
    PaintRoundRect(&red_pill, 8, 8);
    SetForeRGB(255, 112, 67);
    FrameRoundRect(&red_pill, 8, 8);

    char red_str[32];
    sprintf(red_str, "YOU: %02d", g_game.scores[CELL_RED]);
    SetForeRGB(255, 255, 255);
    DrawCenteredText(red_str, &red_pill, kFontIDMonaco, 12, bold);

    /* Center Title / Turn Status */
    Rect center_rect;
    SetRect(&center_rect, 94, 6, WINDOW_WIDTH - 94, 38);

    const char *status_str = "FTAGHN";
    if (g_game.game_over) {
        status_str = g_game.winner_msg;
        SetForeRGB(250, 204, 21);
    } else if (g_game.current_player == CELL_RED) {
        status_str = "YOUR TURN (RED)";
        SetForeRGB(255, 112, 67);
    } else {
        status_str = "AI PONDERS...";
        SetForeRGB(96, 165, 250);
    }
    DrawCenteredText(status_str, &center_rect, applFont, 12, bold);

    /* Blue Score Pill */
    Rect blue_pill;
    SetRect(&blue_pill, WINDOW_WIDTH - 90, 8, WINDOW_WIDTH - 12, 36);
    SetForeRGB(37, 99, 235);
    PaintRoundRect(&blue_pill, 8, 8);
    SetForeRGB(96, 165, 250);
    FrameRoundRect(&blue_pill, 8, 8);

    char blue_str[32];
    sprintf(blue_str, "AI: %02d", g_game.scores[CELL_BLUE]);
    SetForeRGB(255, 255, 255);
    DrawCenteredText(blue_str, &blue_pill, kFontIDMonaco, 12, bold);
}

static void DrawBoard(void) {
    int r, c;

    /* Board background */
    Rect board_bg;
    SetRect(&board_bg,
            BOARD_OFFSET_X, BOARD_OFFSET_Y,
            BOARD_OFFSET_X + g_game.board_size * CELL_SIZE,
            BOARD_OFFSET_Y + g_game.board_size * CELL_SIZE);

    SetForeRGB(24, 26, 38);
    PaintRect(&board_bg);

    /* Draw grid cells */
    for (r = 0; r < g_game.board_size; r++) {
        for (c = 0; c < g_game.board_size; c++) {
            Rect cell_rect;
            SetRect(&cell_rect,
                    BOARD_OFFSET_X + c * CELL_SIZE,
                    BOARD_OFFSET_Y + r * CELL_SIZE,
                    BOARD_OFFSET_X + (c + 1) * CELL_SIZE,
                    BOARD_OFFSET_Y + (r + 1) * CELL_SIZE);

            /* Cell background tile */
            SetForeRGB(30, 32, 46);
            PaintRect(&cell_rect);

            /* Grid border */
            SetForeRGB(45, 49, 66);
            FrameRect(&cell_rect);

            /* Cell content */
            int cell_type = g_game.board[r][c];
            if (cell_type != CELL_EMPTY) {
                DrawPiece(r, c, cell_type);
            }
        }
    }

    /* Outer board border */
    SetForeRGB(90, 95, 120);
    FrameRect(&board_bg);

    /* Draw selection highlight & valid move indicators */
    if (g_game.has_selected && !g_game.game_over && g_game.current_player == CELL_RED) {
        int sr = g_game.selected_r;
        int sc = g_game.selected_c;

        /* Highlight selected cell with golden aura */
        Rect sel_rect;
        SetRect(&sel_rect,
                BOARD_OFFSET_X + sc * CELL_SIZE + 1,
                BOARD_OFFSET_Y + sr * CELL_SIZE + 1,
                BOARD_OFFSET_X + (sc + 1) * CELL_SIZE - 1,
                BOARD_OFFSET_Y + (sr + 1) * CELL_SIZE - 1);

        SetForeRGB(245, 158, 11);
        FrameRoundRect(&sel_rect, 6, 6);
        InsetRect(&sel_rect, 1, 1);
        FrameRoundRect(&sel_rect, 4, 4);

        /* Highlight valid clone and leap moves */
        for (r = 0; r < g_game.board_size; r++) {
            for (c = 0; c < g_game.board_size; c++) {
                int move_type = Game_GetValidMoveType(sr, sc, r, c);
                if (move_type != MOVE_NONE) {
                    Rect target_rect;
                    SetRect(&target_rect,
                            BOARD_OFFSET_X + c * CELL_SIZE,
                            BOARD_OFFSET_Y + r * CELL_SIZE,
                            BOARD_OFFSET_X + (c + 1) * CELL_SIZE,
                            BOARD_OFFSET_Y + (r + 1) * CELL_SIZE);

                    short cx = (target_rect.left + target_rect.right) / 2;
                    short cy = (target_rect.top + target_rect.bottom) / 2;

                    if (move_type == MOVE_CLONE) {
                        /* Emerald Spore / Arcane Circle (Distance 1) */
                        Rect dot;
                        SetRect(&dot, cx - 6, cy - 6, cx + 6, cy + 6);
                        SetForeRGB(16, 185, 129);
                        PaintOval(&dot);
                        SetForeRGB(110, 231, 183);
                        FrameOval(&dot);
                    } else if (move_type == MOVE_LEAP) {
                        /* Amethyst Diamond (Distance 2) */
                        Rect dot;
                        SetRect(&dot, cx - 7, cy - 7, cx + 7, cy + 7);
                        SetForeRGB(139, 92, 246);
                        PaintRoundRect(&dot, 4, 4);
                        SetForeRGB(196, 181, 253);
                        FrameRoundRect(&dot, 4, 4);
                    }
                }
            }
        }
    }
}

static void DrawGrimoire(void) {
    Rect grim_rect;
    SetRect(&grim_rect,
            BOARD_OFFSET_X, BOARD_OFFSET_Y + g_game.board_size * CELL_SIZE + 8,
            BOARD_OFFSET_X + g_game.board_size * CELL_SIZE, WINDOW_HEIGHT - 8);

    /* Background panel */
    SetForeRGB(16, 17, 24);
    PaintRoundRect(&grim_rect, 6, 6);

    /* Border */
    SetForeRGB(45, 49, 66);
    FrameRoundRect(&grim_rect, 6, 6);

    /* Grimoire messages */
    int i;
    int y = grim_rect.top + 14;
    int max_lines = 3;
    if (g_game.grimoire_count < max_lines) {
        max_lines = g_game.grimoire_count;
    }

    TextFont(applFont);
    TextSize(11);

    for (i = 0; i < max_lines; i++) {
        if (i == 0) {
            SetForeRGB(250, 204, 21); /* Gold/Yellow for newest */
            TextFace(bold);
        } else if (i == 1) {
            SetForeRGB(180, 180, 190); /* Light slate */
            TextFace(normal);
        } else {
            SetForeRGB(110, 110, 125); /* Faint gray */
            TextFace(normal);
        }

        const char *msg = g_game.grimoire[i].text;
        short text_w = TextWidth((Ptr)msg, 0, strlen(msg));
        short x = grim_rect.left + (grim_rect.right - grim_rect.left - text_w) / 2;
        if (x < grim_rect.left + 6) x = grim_rect.left + 6;

        MoveTo(x, y);
        DrawText((Ptr)msg, 0, strlen(msg));
        y += 14;
    }
}

void Render_Draw(WindowRef window) {
    if (s_offscreen == NULL) {
        Render_Init(window);
        if (s_offscreen == NULL) return;
    }

    CGrafPtr orig_port;
    GDHandle orig_device;
    GetGWorld(&orig_port, &orig_device);

    SetGWorld((CGrafPtr)s_offscreen, NULL);
    PixMapHandle pix_map = GetGWorldPixMap(s_offscreen);
    LockPixels(pix_map);

    /* Overall window background */
    SetForeRGB(18, 19, 28);
    PaintRect(&s_bounds);

    /* Subcomponents */
    DrawHUD();
    DrawBoard();
    DrawGrimoire();

    /* Blit offscreen buffer to window */
    CGrafPtr win_port = GetWindowPort(window);
    SetGWorld(win_port, NULL);

    BitMap *src_bm = GetPortBitMapForCopyBits((CGrafPtr)s_offscreen);
    BitMap *dst_bm = GetPortBitMapForCopyBits(win_port);
    CopyBits(src_bm, dst_bm, &s_bounds, &s_bounds, srcCopy, NULL);

    UnlockPixels(pix_map);
    SetGWorld(orig_port, orig_device);
}
