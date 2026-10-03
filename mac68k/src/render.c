#include "render.h"
#include <Fonts.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <stdio.h>
#include <string.h>

#ifndef monaco
#define monaco 4
#endif

static BitMap    s_offBitmap;
static GrafPort  s_offPort;
static Ptr       s_offBits = NULL;
static Boolean   s_ready = false;

void Render_Init(WindowPtr win) {
    short rowBytes = ((WINDOW_WIDTH + 15) / 16) * 2; /* 64 bytes */
    long totalBytes = (long)rowBytes * WINDOW_HEIGHT;

    s_offBits = NewPtrClear(totalBytes);
    if (!s_offBits) return;

    s_offBitmap.baseAddr = s_offBits;
    s_offBitmap.rowBytes = rowBytes;
    SetRect(&s_offBitmap.bounds, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    OpenPort(&s_offPort);
    s_offPort.portBits = s_offBitmap;
    s_offPort.portRect = s_offBitmap.bounds;
    SetPort(&s_offPort);
    ClipRect(&s_offBitmap.bounds);
    RectRgn(s_offPort.visRgn, &s_offBitmap.bounds);

    s_ready = true;
}

void Render_Cleanup(void) {
    if (s_ready) {
        ClosePort(&s_offPort);
        if (s_offBits) {
            DisposePtr(s_offBits);
            s_offBits = NULL;
        }
        s_ready = false;
    }
}

int Render_GetCellAt(Point pt, int *out_r, int *out_c) {
    int cx = pt.h - BOARD_OFFSET_X;
    int cy = pt.v - BOARD_OFFSET_Y;
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

static void DrawPiece(int r, int c, int cell_type) {
    Rect cellRect;
    int x1 = BOARD_OFFSET_X + c * CELL_SIZE;
    int y1 = BOARD_OFFSET_Y + r * CELL_SIZE;
    SetRect(&cellRect, x1, y1, x1 + CELL_SIZE, y1 + CELL_SIZE);

    int cx = x1 + CELL_SIZE / 2;
    int cy = y1 + CELL_SIZE / 2;

    if (cell_type == CELL_RED) {
        /* Crimson Spawn: Occult Black Eye with White Iris & Black Pupil */
        Rect pieceRect = cellRect;
        InsetRect(&pieceRect, 4, 4);
        PaintOval(&pieceRect);

        Rect irisRect = pieceRect;
        InsetRect(&irisRect, 7, 7);
        EraseOval(&irisRect);

        Rect pupilRect = irisRect;
        InsetRect(&pupilRect, 4, 4);
        PaintOval(&pupilRect);

    } else if (cell_type == CELL_BLUE) {
        /* Abyssal Watcher: Dark Gray Dithered Horror with White Horizontal Slit */
        Rect pieceRect = cellRect;
        InsetRect(&pieceRect, 4, 4);
        FillOval(&pieceRect, &qd.dkGray);
        FrameOval(&pieceRect);

        Rect slitRect;
        SetRect(&slitRect, cx - 8, cy - 2, cx + 8, cy + 2);
        EraseRect(&slitRect);
        FrameRect(&slitRect);

    } else if (cell_type == CELL_OBSTACLE || cell_type == CELL_PERM_OBSTACLE) {
        /* Obsidian Monolith: Textured Stone Block with Etched Rune Cross */
        Rect blockRect = cellRect;
        InsetRect(&blockRect, 4, 4);
        FillRect(&blockRect, &qd.ltGray);
        FrameRect(&blockRect);

        /* 3D bevel shadow */
        MoveTo(blockRect.right - 1, blockRect.top);
        LineTo(blockRect.right - 1, blockRect.bottom - 1);
        LineTo(blockRect.left, blockRect.bottom - 1);

        /* Etched rune cross */
        MoveTo(cx - 7, cy);
        LineTo(cx + 7, cy);
        MoveTo(cx, cy - 7);
        LineTo(cx, cy + 7);
        MoveTo(cx - 4, cy - 4);
        LineTo(cx + 4, cy + 4);
        MoveTo(cx - 4, cy + 4);
        LineTo(cx + 4, cy - 4);
    }
}

static void DrawBoard(void) {
    int r, c;
    int board_px = g_game.board_size * CELL_SIZE;

    /* Outer border frame */
    Rect boardOuter;
    SetRect(&boardOuter, BOARD_OFFSET_X - 2, BOARD_OFFSET_Y - 2,
            BOARD_OFFSET_X + board_px + 2, BOARD_OFFSET_Y + board_px + 2);
    FrameRect(&boardOuter);

    /* Grid tiles */
    for (r = 0; r < g_game.board_size; r++) {
        for (c = 0; c < g_game.board_size; c++) {
            Rect tileRect;
            int tx = BOARD_OFFSET_X + c * CELL_SIZE;
            int ty = BOARD_OFFSET_Y + r * CELL_SIZE;
            SetRect(&tileRect, tx, ty, tx + CELL_SIZE, ty + CELL_SIZE);

            EraseRect(&tileRect);
            FrameRect(&tileRect);

            int cell_type = g_game.board[r][c];
            if (cell_type != CELL_EMPTY) {
                DrawPiece(r, c, cell_type);
            }
        }
    }

    /* Move highlights if a piece is selected */
    if (g_game.has_selected && !g_game.game_over && g_game.current_player == CELL_RED) {
        int sr = g_game.selected_r;
        int sc = g_game.selected_c;

        /* Selection aura: Invert outer border */
        Rect selRect;
        int sx = BOARD_OFFSET_X + sc * CELL_SIZE;
        int sy = BOARD_OFFSET_Y + sr * CELL_SIZE;
        SetRect(&selRect, sx + 2, sy + 2, sx + CELL_SIZE - 2, sy + CELL_SIZE - 2);
        FrameRect(&selRect);
        InsetRect(&selRect, 1, 1);
        FrameRect(&selRect);

        for (r = 0; r < g_game.board_size; r++) {
            for (c = 0; c < g_game.board_size; c++) {
                int move_type = Game_GetValidMoveType(sr, sc, r, c);
                if (move_type != MOVE_NONE) {
                    int mcx = BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2;
                    int mcy = BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2;

                    if (move_type == MOVE_CLONE) {
                        /* Spore clone move: Small filled black circle */
                        Rect dotRect;
                        SetRect(&dotRect, mcx - 4, mcy - 4, mcx + 4, mcy + 4);
                        PaintOval(&dotRect);
                    } else if (move_type == MOVE_LEAP) {
                        /* Void leap move: Hollow square */
                        Rect sqRect;
                        SetRect(&sqRect, mcx - 5, mcy - 5, mcx + 5, mcy + 5);
                        FrameRect(&sqRect);
                    }
                }
            }
        }
    }
}

static void DrawHUD(void) {
    char str[128];
    int len;

    /* Panel bounds */
    int px1 = 296;
    int px2 = 486;

    TextFont(applFont); /* Geneva */
    TextSize(10);
    TextFace(bold);

    /* Player (You) Score Box */
    Rect redBox;
    SetRect(&redBox, px1, 18, px2, 48);
    EraseRect(&redBox);
    FrameRect(&redBox);

    /* Draw mini icon of Crimson Spawn inside box */
    Rect miniRed;
    SetRect(&miniRed, px1 + 8, 24, px1 + 24, 40);
    PaintOval(&miniRed);
    Rect miniRedInner = miniRed;
    InsetRect(&miniRedInner, 4, 4);
    EraseOval(&miniRedInner);

    sprintf(str, "YOU:  %02d  (Crimson)", g_game.scores[CELL_RED]);
    len = strlen(str);
    MoveTo(px1 + 32, 37);
    DrawText(str, 0, len);

    /* AI (Watcher) Score Box */
    Rect blueBox;
    SetRect(&blueBox, px1, 54, px2, 84);
    EraseRect(&blueBox);
    FrameRect(&blueBox);

    /* Draw mini icon of Abyssal Watcher inside box */
    Rect miniBlue;
    SetRect(&miniBlue, px1 + 8, 60, px1 + 24, 76);
    FillOval(&miniBlue, &qd.dkGray);
    FrameOval(&miniBlue);
    Rect miniBlueSlit;
    SetRect(&miniBlueSlit, px1 + 12, 67, px1 + 20, 69);
    EraseRect(&miniBlueSlit);

    sprintf(str, "AI:   %02d  (Abyssal)", g_game.scores[CELL_BLUE]);
    len = strlen(str);
    MoveTo(px1 + 32, 73);
    DrawText(str, 0, len);

    /* Turn Banner */
    Rect bannerBox;
    SetRect(&bannerBox, px1, 90, px2, 118);

    if (g_game.game_over) {
        PaintRect(&bannerBox);
        TextFace(bold);
        sprintf(str, "%s", g_game.winner_msg);
        len = strlen(str);
        short textW = TextWidth(str, 0, len);
        int center_x = px1 + (px2 - px1 - textW) / 2;
        ForeColor(whiteColor);
        MoveTo(center_x, 109);
        DrawText(str, 0, len);
        ForeColor(blackColor);
    } else if (g_game.current_player == CELL_RED) {
        PaintRect(&bannerBox);
        strcpy(str, "YOUR TURN (RED)");
        len = strlen(str);
        short textW = TextWidth(str, 0, len);
        int center_x = px1 + (px2 - px1 - textW) / 2;
        ForeColor(whiteColor);
        MoveTo(center_x, 109);
        DrawText(str, 0, len);
        ForeColor(blackColor);
    } else {
        FillRect(&bannerBox, &qd.ltGray);
        FrameRect(&bannerBox);
        strcpy(str, "AI IS THINKING...");
        len = strlen(str);
        short textW = TextWidth(str, 0, len);
        int center_x = px1 + (px2 - px1 - textW) / 2;
        MoveTo(center_x, 109);
        DrawText(str, 0, len);
    }

    /* Game Timer & Difficulty */
    TextFace(normal);
    TextSize(9);
    const char *diff_str = (g_game.difficulty == DIFF_EASY) ? "Simple" :
                          (g_game.difficulty == DIFF_MEDIUM) ? "Mortal" : "Elder Godlike";
    sprintf(str, "Time: %03ds  |  AI: %s", g_game.game_timer, diff_str);
    len = strlen(str);
    MoveTo(px1 + 4, 134);
    DrawText(str, 0, len);

    /* Grimoire Lore Box (Last 4 messages) */
    Rect grimBox;
    SetRect(&grimBox, px1, 144, px2, 266);
    EraseRect(&grimBox);
    FrameRect(&grimBox);

    /* Grimoire Title Header */
    Rect titleRect;
    SetRect(&titleRect, px1 + 40, 138, px2 - 40, 150);
    EraseRect(&titleRect);
    strcpy(str, "-- Grimoire --");
    len = strlen(str);
    short gTitleW = TextWidth(str, 0, len);
    MoveTo(px1 + (px2 - px1 - gTitleW) / 2, 148);
    DrawText(str, 0, len);

    /* Grimoire text lines */
    TextFont(monaco); /* Monaco 9pt */
    TextSize(9);
    int max_lines = (g_game.grimoire_count < 4) ? g_game.grimoire_count : 4;
    int line_y = 168;
    int i;
    for (i = 0; i < max_lines; i++) {
        if (i == 0) {
            TextFace(bold);
        } else {
            TextFace(normal);
        }
        const char *msg = g_game.grimoire[i].text;
        len = strlen(msg);
        MoveTo(px1 + 6, line_y);
        DrawText((Ptr)msg, 0, len);
        line_y += 24;
    }

    /* Control Keys Footer */
    TextFont(applFont);
    TextSize(9);
    TextFace(normal);
    strcpy(str, "[N]ew  [M]ute  [1-3]Diff  [Q]uit");
    len = strlen(str);
    short kw = TextWidth(str, 0, len);
    MoveTo(px1 + (px2 - px1 - kw) / 2, 286);
    DrawText((Ptr)str, 0, len);
}

void Render_Draw(WindowPtr win) {
    if (!s_ready || !win) return;

    /* Render offscreen in s_offPort */
    SetPort(&s_offPort);

    /* Erase entire background */
    EraseRect(&s_offBitmap.bounds);

    /* Draw Board and HUD */
    DrawBoard();
    DrawHUD();

    /* CopyBits to on-screen window */
    SetPort(win);
    CopyBits(&s_offBitmap, &win->portBits, &s_offBitmap.bounds, &win->portRect, srcCopy, NULL);
}
