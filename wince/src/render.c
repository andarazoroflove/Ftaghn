#include "render.h"
#include "game.h"
#include "bmp_loader.h"
#include "freestanding.h"

#undef SetRect
#define SetRect(prc, l, t, r, b) do { (prc)->left = (l); (prc)->top = (t); (prc)->right = (r); (prc)->bottom = (b); } while(0)

static HDC s_mem_dc = NULL;
static HBITMAP s_mem_bmp = NULL;
static HBITMAP s_old_bmp = NULL;
static void   *s_dib_bits = NULL;

static HBITMAP s_bg_bmp = NULL;
static char s_loaded_bg[MAX_PATH] = "";

static HFONT s_font_title = NULL;
static HFONT s_font_score = NULL;
static HFONT s_font_hud = NULL;
static HFONT s_font_btn = NULL;
static HFONT s_font_grim[GRIMOIRE_MAX] = { NULL };

static COLORREF s_grim_colors[GRIMOIRE_MAX] = {
    RGB(250, 204, 21),  /* Pos 0: Bright Gold */
    RGB(190, 190, 195), /* Pos 1: Silver */
    RGB(140, 140, 150), /* Pos 2: Gray */
    RGB(100, 100, 110), /* Pos 3: Dim Gray */
    RGB(65, 65, 75)     /* Pos 4: Dark Gray */
};

static HFONT CreateFontWCE(int height, int weight) {
    LOGFONTW lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = height;
    lf.lfWeight = weight;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpyW(lf.lfFaceName, L"Tahoma");
    HFONT hf = CreateFontIndirectW(&lf);
    if (!hf) {
        hf = (HFONT)GetStockObject(SYSTEM_FONT);
    }
    return hf;
}

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

void Render_LoadCurrentBackground(void) {
    if (g_game.current_bg_filename[0] == '\0') {
        return;
    }
    if (strcmp(s_loaded_bg, g_game.current_bg_filename) == 0 && s_bg_bmp != NULL) {
        return;
    }

    if (s_bg_bmp) {
        DeleteObject(s_bg_bmp);
        s_bg_bmp = NULL;
    }

    /* Resolve relative to executable path */
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

    s_bg_bmp = BMP_LoadFromFileW(full_bg_path, NULL, NULL);
    strncpy(s_loaded_bg, g_game.current_bg_filename, sizeof(s_loaded_bg) - 1);
    s_loaded_bg[sizeof(s_loaded_bg) - 1] = '\0';
}

void Render_Init(HWND hwnd) {
    HDC screen_dc = GetDC(hwnd);
    if (!screen_dc) return;

    s_mem_dc = CreateCompatibleDC(screen_dc);

    /* Allocate DIBSection backbuffer for Jornada 720 (640x240 16-bit 5:6:5 / 5:5:5) */
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

    s_mem_bmp = CreateDIBSection(screen_dc, (BITMAPINFO *)&bmi16, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
    if (!s_mem_bmp) {
        /* Fallback: bottom-up 16-bit 5:5:5 BI_RGB */
        BITMAPINFOHEADER bmi555;
        memset(&bmi555, 0, sizeof(bmi555));
        bmi555.biSize = sizeof(BITMAPINFOHEADER);
        bmi555.biWidth = SCREEN_WIDTH;
        bmi555.biHeight = SCREEN_HEIGHT;
        bmi555.biPlanes = 1;
        bmi555.biBitCount = 16;
        bmi555.biCompression = BI_RGB;
        s_mem_bmp = CreateDIBSection(screen_dc, (BITMAPINFO *)&bmi555, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
    }
    if (!s_mem_bmp) {
        /* Fallback: bottom-up 24-bit BI_RGB */
        BITMAPINFOHEADER bmi24;
        memset(&bmi24, 0, sizeof(bmi24));
        bmi24.biSize = sizeof(BITMAPINFOHEADER);
        bmi24.biWidth = SCREEN_WIDTH;
        bmi24.biHeight = SCREEN_HEIGHT;
        bmi24.biPlanes = 1;
        bmi24.biBitCount = 24;
        bmi24.biCompression = BI_RGB;
        s_mem_bmp = CreateDIBSection(screen_dc, (BITMAPINFO *)&bmi24, DIB_RGB_COLORS, &s_dib_bits, NULL, 0);
    }

    if (s_mem_dc && s_mem_bmp) {
        s_old_bmp = (HBITMAP)SelectObject(s_mem_dc, s_mem_bmp);
    }

    ReleaseDC(hwnd, screen_dc);

    /* Create Typography for 640x240 LCD */
    s_font_title = CreateFontWCE(15, FW_BOLD);
    s_font_score = CreateFontWCE(13, FW_BOLD);
    s_font_hud   = CreateFontWCE(11, FW_BOLD);
    s_font_btn   = CreateFontWCE(11, FW_BOLD);

    s_font_grim[0] = CreateFontWCE(12, FW_BOLD);
    s_font_grim[1] = CreateFontWCE(11, FW_NORMAL);
    s_font_grim[2] = CreateFontWCE(11, FW_NORMAL);
    s_font_grim[3] = CreateFontWCE(10, FW_NORMAL);
    s_font_grim[4] = CreateFontWCE(10, FW_NORMAL);

    if (g_game.current_bg_filename[0] != '\0') {
        Render_LoadCurrentBackground();
    }
}

void Render_Cleanup(void) {
    int i;
    if (s_mem_dc && s_old_bmp) {
        SelectObject(s_mem_dc, s_old_bmp);
        s_old_bmp = NULL;
    }
    if (s_mem_bmp) {
        DeleteObject(s_mem_bmp);
        s_mem_bmp = NULL;
    }
    if (s_mem_dc) {
        DeleteDC(s_mem_dc);
        s_mem_dc = NULL;
    }
    if (s_bg_bmp) {
        DeleteObject(s_bg_bmp);
        s_bg_bmp = NULL;
    }

    if (s_font_title) DeleteObject(s_font_title);
    if (s_font_score) DeleteObject(s_font_score);
    if (s_font_hud)   DeleteObject(s_font_hud);
    if (s_font_btn)   DeleteObject(s_font_btn);
    for (i = 0; i < GRIMOIRE_MAX; i++) {
        if (s_font_grim[i]) DeleteObject(s_font_grim[i]);
    }
}

static void DrawOrb(HDC hdc, int cx, int cy, int radius, COLORREF color, BOOL is_klf, BOOL is_gwb) {
    /* Shadow */
    HBRUSH shadow_br = CreateSolidBrush(RGB(10, 10, 15));
    HPEN null_pen = (HPEN)GetStockObject(NULL_PEN);
    HGDIOBJ old_br = SelectObject(hdc, shadow_br);
    HGDIOBJ old_pen = SelectObject(hdc, null_pen);

    Ellipse(hdc, cx - radius + 1, cy - radius + 2, cx + radius + 1, cy + radius + 2);

    /* Main body */
    HBRUSH body_br = CreateSolidBrush(color);
    SelectObject(hdc, body_br);
    Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);

    /* 3D Highlight Glint */
    int glint_r = radius / 3;
    if (glint_r < 2) glint_r = 2;
    HBRUSH glint_br;
    if (is_klf) {
        glint_br = CreateSolidBrush(RGB(255, 230, 120));
    } else {
        glint_br = CreateSolidBrush(RGB(255, 255, 255));
    }
    SelectObject(hdc, glint_br);
    Ellipse(hdc, cx - radius / 2, cy - radius / 2, cx - radius / 2 + glint_r * 2, cy - radius / 2 + glint_r * 2);

    if (is_gwb) {
        /* Golden Presidential Star in center */
        HBRUSH star_br = CreateSolidBrush(RGB(255, 215, 0));
        SelectObject(hdc, star_br);
        Ellipse(hdc, cx - 3, cy - 3, cx + 4, cy + 4);
        DeleteObject(star_br);
    }

    SelectObject(hdc, old_br);
    SelectObject(hdc, old_pen);
    DeleteObject(shadow_br);
    DeleteObject(body_br);
    DeleteObject(glint_br);
}

static void DrawButton(HDC hdc, int x, int y, int w, int h, const WCHAR *label, COLORREF bg_color, COLORREF text_color, HFONT font) {
    /* Button Body */
    HBRUSH bg_br = CreateSolidBrush(bg_color);
    HPEN border_pen = CreatePen(PS_SOLID, 1, RGB(70, 75, 95));
    HGDIOBJ old_br = SelectObject(hdc, bg_br);
    HGDIOBJ old_pen = SelectObject(hdc, border_pen);

    RoundRect(hdc, x, y, x + w, y + h, 6, 6);

    /* Top/Left 3D Bevel highlight */
    HPEN hi_pen = CreatePen(PS_SOLID, 1, RGB(110, 115, 140));
    SelectObject(hdc, hi_pen);
    MoveToEx(hdc, x + 2, y + h - 2, NULL);
    LineTo(hdc, x + 2, y + 2);
    LineTo(hdc, x + w - 2, y + 2);

    /* Text */
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, text_color);
    SelectObject(hdc, font);

    RECT rc;
    SetRect(&rc, x, y, x + w, y + h);
    DrawTextW(hdc, label, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, old_br);
    SelectObject(hdc, old_pen);
    DeleteObject(bg_br);
    DeleteObject(border_pen);
    DeleteObject(hi_pen);
}

void Render_Paint(HWND hwnd, HDC hdc) {
    (void)hwnd;
    if (!s_mem_dc) return;

    Render_LoadCurrentBackground();

    /* =========================================================================
     * 1. LEFT HALF: BACKGROUND & 7x7 ATAXX BOARD (0 to 320, 0 to 240)
     * ========================================================================= */
    if (s_bg_bmp) {
        HDC img_dc = CreateCompatibleDC(hdc);
        HGDIOBJ old_img = SelectObject(img_dc, s_bg_bmp);
        BitBlt(s_mem_dc, 0, 0, 320, 240, img_dc, 0, 0, SRCCOPY);
        SelectObject(img_dc, old_img);
        DeleteDC(img_dc);
    } else {
        HBRUSH bg_br = CreateSolidBrush(RGB(20, 20, 32));
        RECT lrc;
        SetRect(&lrc, 0, 0, 320, 240);
        FillRect(s_mem_dc, &lrc, bg_br);
        DeleteObject(bg_br);
    }

    int ox, oy, cell_size;
    GetBoardLayout(&ox, &oy, &cell_size);
    int n = g_game.board_size;
    int bw = cell_size * n;
    int bh = cell_size * n;

    /* Board background plate */
    HBRUSH plate_br = CreateSolidBrush(RGB(10, 12, 18));
    HPEN plate_pen = CreatePen(PS_SOLID, 2, RGB(80, 85, 110));
    HGDIOBJ old_br = SelectObject(s_mem_dc, plate_br);
    HGDIOBJ old_pen = SelectObject(s_mem_dc, plate_pen);

    Rectangle(s_mem_dc, ox - 3, oy - 3, ox + bw + 4, oy + bh + 4);

    SelectObject(s_mem_dc, old_br);
    SelectObject(s_mem_dc, old_pen);
    DeleteObject(plate_br);
    DeleteObject(plate_pen);

    /* Render Cells & Stones */
    HPEN grid_pen = CreatePen(PS_SOLID, 1, RGB(38, 42, 56));
    HBRUSH cell_br = CreateSolidBrush(RGB(18, 20, 28));

    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            int cx = ox + c * cell_size;
            int cy = oy + r * cell_size;

            /* Cell Floor */
            old_br = SelectObject(s_mem_dc, cell_br);
            old_pen = SelectObject(s_mem_dc, grid_pen);
            Rectangle(s_mem_dc, cx, cy, cx + cell_size + 1, cy + cell_size + 1);

            int val = g_game.board[r][c];

            /* Valid Move Highlights */
            if (g_game.has_selected && (val == CELL_EMPTY || val == CELL_PRESENT_OBSTACLE)) {
                int mtype = Game_GetValidMoveType(g_game.selected_r, g_game.selected_c, r, c);
                if (mtype == MOVE_CLONE) {
                    /* Green Spore Dot */
                    HBRUSH clone_br = CreateSolidBrush(RGB(34, 197, 94));
                    SelectObject(s_mem_dc, clone_br);
                    int mid_x = cx + cell_size / 2;
                    int mid_y = cy + cell_size / 2;
                    Ellipse(s_mem_dc, mid_x - 4, mid_y - 4, mid_x + 5, mid_y + 5);
                    DeleteObject(clone_br);
                } else if (mtype == MOVE_LEAP) {
                    /* Purple Warp Indicator */
                    HBRUSH leap_br = CreateSolidBrush(RGB(168, 85, 247));
                    SelectObject(s_mem_dc, leap_br);
                    int mid_x = cx + cell_size / 2;
                    int mid_y = cy + cell_size / 2;
                    RoundRect(s_mem_dc, mid_x - 4, mid_y - 4, mid_x + 5, mid_y + 5, 2, 2);
                    DeleteObject(leap_br);
                }
            }

            /* Selected Ring */
            if (g_game.has_selected && r == g_game.selected_r && c == g_game.selected_c) {
                HPEN sel_pen = CreatePen(PS_SOLID, 2, RGB(250, 204, 21));
                SelectObject(s_mem_dc, sel_pen);
                SelectObject(s_mem_dc, GetStockObject(NULL_BRUSH));
                Rectangle(s_mem_dc, cx + 1, cy + 1, cx + cell_size, cy + cell_size);
                DeleteObject(sel_pen);
            }

            /* Pieces & Obstacles */
            int mid_x = cx + cell_size / 2;
            int mid_y = cy + cell_size / 2;
            int orb_r = cell_size / 2 - 3;

            if (val == CELL_RED) {
                BOOL is_klf = (g_game.char_red == CHAR_KLF);
                BOOL is_gwb = (g_game.char_red == CHAR_GWB);
                DrawOrb(s_mem_dc, mid_x, mid_y, orb_r, g_game.color_red, is_klf, is_gwb);
            } else if (val == CELL_BLUE) {
                BOOL is_klf = (g_game.char_blue == CHAR_KLF);
                DrawOrb(s_mem_dc, mid_x, mid_y, orb_r, g_game.color_blue, is_klf, FALSE);
            } else if (val == CELL_OBSTACLE) {
                /* Monolith Stone */
                HBRUSH obs_br = CreateSolidBrush(RGB(48, 52, 64));
                HPEN obs_pen = CreatePen(PS_SOLID, 1, RGB(80, 85, 100));
                SelectObject(s_mem_dc, obs_br);
                SelectObject(s_mem_dc, obs_pen);
                RoundRect(s_mem_dc, cx + 4, cy + 4, cx + cell_size - 4, cy + cell_size - 4, 4, 4);
                DeleteObject(obs_br);
                DeleteObject(obs_pen);
            } else if (val == CELL_PERM_OBSTACLE) {
                /* Permanent Ice Block */
                HBRUSH ice_br = CreateSolidBrush(RGB(135, 206, 235));
                HPEN ice_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
                SelectObject(s_mem_dc, ice_br);
                SelectObject(s_mem_dc, ice_pen);
                RoundRect(s_mem_dc, cx + 3, cy + 3, cx + cell_size - 3, cy + cell_size - 3, 3, 3);
                DeleteObject(ice_br);
                DeleteObject(ice_pen);
            } else if (val == CELL_PRESENT_OBSTACLE) {
                /* Santa Holiday Present Box */
                HBRUSH gift_br = CreateSolidBrush(RGB(220, 38, 38));
                HPEN gift_pen = CreatePen(PS_SOLID, 1, RGB(250, 204, 21));
                SelectObject(s_mem_dc, gift_br);
                SelectObject(s_mem_dc, gift_pen);
                Rectangle(s_mem_dc, cx + 4, cy + 4, cx + cell_size - 4, cy + cell_size - 4);
                DeleteObject(gift_br);
                DeleteObject(gift_pen);
            }
        }
    }

    SelectObject(s_mem_dc, old_br);
    SelectObject(s_mem_dc, old_pen);
    DeleteObject(grid_pen);
    DeleteObject(cell_br);

    /* =========================================================================
     * 2. DIVIDER LINE (x = 320)
     * ========================================================================= */
    HPEN div_pen = CreatePen(PS_SOLID, 2, RGB(45, 50, 70));
    old_pen = SelectObject(s_mem_dc, div_pen);
    MoveToEx(s_mem_dc, 320, 0, NULL);
    LineTo(s_mem_dc, 320, 240);
    SelectObject(s_mem_dc, old_pen);
    DeleteObject(div_pen);

    /* =========================================================================
     * 3. RIGHT HALF: SCOREBOARD, GRIMOIRE, DEITY & TOUCH BUTTONS (320 to 640)
     * ========================================================================= */
    HBRUSH r_bg = CreateSolidBrush(RGB(14, 16, 26));
    RECT r_rc;
    SetRect(&r_rc, 321, 0, 640, 240);
    FillRect(s_mem_dc, &r_rc, r_bg);
    DeleteObject(r_bg);

    /* --- Title Header (y: 4 to 20) --- */
    SetBkMode(s_mem_dc, TRANSPARENT);
    SetTextColor(s_mem_dc, RGB(250, 204, 21));
    SelectObject(s_mem_dc, s_font_title);
    RECT title_rc;
    SetRect(&title_rc, 326, 3, 634, 21);
    DrawTextW(s_mem_dc, L"FTAGHN : COSMIC HORROR ATAXX", -1, &title_rc, DT_CENTER | DT_SINGLELINE);

    /* --- Score & Turn Row (y: 23 to 50) --- */
    /* Red Score Badge */
    WCHAR wscore_red[32];
    wsprintfW(wscore_red, L"RED: %d", g_game.scores[CELL_RED]);
    DrawButton(s_mem_dc, 326, 23, 90, 24, wscore_red, RGB(185, 28, 28), RGB(255, 255, 255), s_font_score);

    /* Blue Score Badge */
    WCHAR wscore_blue[32];
    wsprintfW(wscore_blue, L"BLUE: %d", g_game.scores[CELL_BLUE]);
    DrawButton(s_mem_dc, 422, 23, 90, 24, wscore_blue, RGB(29, 78, 216), RGB(255, 255, 255), s_font_score);

    /* Turn & Timer Box */
    WCHAR wturn_timer[48];
    const WCHAR *turn_str = (g_game.current_player == CELL_RED) ? L"CRIMSON" : L"ABYSSAL";
    if (g_game.time_distortion == 0.0f) {
        wsprintfW(wturn_timer, L"%s | TIME: STOP", turn_str);
    } else {
        int m = g_game.game_timer / 60;
        int s = g_game.game_timer % 60;
        wsprintfW(wturn_timer, L"%s | %d:%02d", turn_str, m, s);
    }
    COLORREF turn_bg = (g_game.current_player == CELL_RED) ? RGB(70, 25, 25) : RGB(25, 45, 80);
    DrawButton(s_mem_dc, 518, 23, 116, 24, wturn_timer, turn_bg, RGB(250, 204, 21), s_font_hud);

    /* --- The Grimoire Panel (y: 52 to 140) --- */
    HPEN grim_pen = CreatePen(PS_SOLID, 1, RGB(45, 50, 70));
    HBRUSH grim_bg = CreateSolidBrush(RGB(10, 11, 19));
    old_pen = SelectObject(s_mem_dc, grim_pen);
    old_br = SelectObject(s_mem_dc, grim_bg);

    RoundRect(s_mem_dc, 326, 52, 634, 140, 4, 4);

    SelectObject(s_mem_dc, old_pen);
    SelectObject(s_mem_dc, old_br);
    DeleteObject(grim_pen);
    DeleteObject(grim_bg);

    /* Grimoire Title */
    SetTextColor(s_mem_dc, RGB(180, 150, 80));
    SelectObject(s_mem_dc, s_font_hud);
    RECT gtitle_rc;
    SetRect(&gtitle_rc, 332, 54, 628, 68);
    DrawTextW(s_mem_dc, L"--- THE GRIMOIRE (STATUS LOG) ---", -1, &gtitle_rc, DT_LEFT | DT_SINGLELINE);

    /* Render Grimoire Lines (Fading Top-to-Bottom) */
    int gy = 70;
    for (int i = 0; i < g_game.grimoire_count && i < GRIMOIRE_MAX; i++) {
        WCHAR wline[128];
        ascii_to_wide(wline, g_game.grimoire[i].text, 128);

        SetTextColor(s_mem_dc, s_grim_colors[i]);
        SelectObject(s_mem_dc, s_font_grim[i]);

        RECT line_rc;
        SetRect(&line_rc, 332, gy, 628, gy + 14);
        DrawTextW(s_mem_dc, wline, -1, &line_rc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        gy += 14;
    }

    /* --- Active Deity / Secret Box (y: 144 to 184) --- */
    HPEN d_pen = CreatePen(PS_SOLID, 1, RGB(55, 60, 80));
    HBRUSH d_bg = CreateSolidBrush(RGB(18, 20, 32));
    old_pen = SelectObject(s_mem_dc, d_pen);
    old_br = SelectObject(s_mem_dc, d_bg);

    RoundRect(s_mem_dc, 326, 144, 634, 185, 4, 4);

    SelectObject(s_mem_dc, old_pen);
    SelectObject(s_mem_dc, old_br);
    DeleteObject(d_pen);
    DeleteObject(d_bg);

    /* Deity Title Line */
    WCHAR wdeity[128];
    const SecretInfo *sec = &g_secrets[g_game.char_red];
    wsprintfW(wdeity, L"Deity: %S (%S)", sec->name, sec->title);
    SetTextColor(s_mem_dc, sec->color);
    SelectObject(s_mem_dc, s_font_hud);
    RECT d_rc;
    SetRect(&d_rc, 332, 147, 628, 162);
    DrawTextW(s_mem_dc, wdeity, -1, &d_rc, DT_LEFT | DT_SINGLELINE);

    /* Deity Description Line */
    WCHAR wdesc[128];
    ascii_to_wide(wdesc, sec->description, 128);
    SetTextColor(s_mem_dc, RGB(180, 185, 200));
    SelectObject(s_mem_dc, s_font_hud);
    RECT desc_rc;
    SetRect(&desc_rc, 332, 165, 628, 182);
    DrawTextW(s_mem_dc, wdesc, -1, &desc_rc, DT_LEFT | DT_SINGLELINE);

    /* --- Stylus Touch Action Buttons (y: 190 to 234) --- */
    /* 1. New Game Button */
    DrawButton(s_mem_dc, 326, 190, 72, 42, L"NEW GAME", RGB(40, 45, 65), RGB(250, 204, 21), s_font_btn);

    /* 2. Difficulty Button */
    const WCHAR *diff_str = (g_game.difficulty == DIFF_EASY) ? L"AI: MORTAL" :
                            (g_game.difficulty == DIFF_MEDIUM) ? L"AI: ELDER" : L"AI: ANCIENT";
    DrawButton(s_mem_dc, 402, 190, 68, 42, diff_str, RGB(35, 55, 85), RGB(255, 255, 255), s_font_btn);

    /* 3. Tome / Secrets Button */
    DrawButton(s_mem_dc, 474, 190, 86, 42, L"TOME / LORE", RGB(75, 35, 95), RGB(255, 230, 100), s_font_btn);

    /* 4. Audio Mute Button */
    const WCHAR *sound_str = g_game.is_muted ? L"SFX: OFF" : L"SFX: ON";
    COLORREF sound_col = g_game.is_muted ? RGB(239, 68, 68) : RGB(34, 197, 94);
    DrawButton(s_mem_dc, 564, 190, 70, 42, sound_str, RGB(40, 45, 65), sound_col, s_font_btn);

    /* Blit composite frame to screen */
    BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, s_mem_dc, 0, 0, SRCCOPY);
}
