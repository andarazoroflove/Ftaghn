#include <windows.h>
#include "game.h"
#include "render.h"
#include "sound.h"
#include "freestanding.h"

#undef SetRect
#define SetRect(prc, l, t, r, b) do { (prc)->left = (l); (prc)->top = (t); (prc)->right = (r); (prc)->bottom = (b); } while(0)

#define TIMER_ID_SEC 1
#define TIMER_ID_AI  2

static HINSTANCE s_hInstance = NULL;
static HWND s_hwndMain = NULL;

/* Tome of Forbidden Knowledge Modal State */
static BOOL s_tome_open = FALSE;
static int  s_tome_index = 0;

static HFONT s_tome_font_title = NULL;
static HFONT s_tome_font_name  = NULL;
static HFONT s_tome_font_desc  = NULL;
static HFONT s_tome_font_btn   = NULL;

static void LogDebug(const char *msg) {
    WCHAR path[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (len > 0) {
        int slash = -1;
        for (DWORD i = 0; i < len; i++) {
            if (path[i] == L'\\' || path[i] == L'/') slash = (int)i;
        }
        if (slash >= 0) path[slash + 1] = L'\0';
        lstrcatW(path, L"debug.txt");
        HANDLE hFile = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            SetFilePointer(hFile, 0, NULL, FILE_END);
            DWORD written = 0;
            DWORD l = (DWORD)strlen(msg);
            WriteFile(hFile, msg, l, &written, NULL);
            CloseHandle(hFile);
        }
    }
}

static HFONT CreateFontSafe(int height, int weight) {
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

static void InitTomeFonts(void) {
    s_tome_font_title = CreateFontSafe(15, FW_BOLD);
    s_tome_font_name  = CreateFontSafe(14, FW_BOLD);
    s_tome_font_desc  = CreateFontSafe(12, FW_NORMAL);
    s_tome_font_btn   = CreateFontSafe(11, FW_BOLD);
}

static void CleanupTomeFonts(void) {
    if (s_tome_font_title) DeleteObject(s_tome_font_title);
    if (s_tome_font_name)  DeleteObject(s_tome_font_name);
    if (s_tome_font_desc)  DeleteObject(s_tome_font_desc);
    if (s_tome_font_btn)   DeleteObject(s_tome_font_btn);
}

static void DrawTomeModal(HDC hdc) {
    if (!s_tome_open) return;

    /* Background Panel */
    HBRUSH bg_br = CreateSolidBrush(RGB(18, 20, 32));
    HPEN gold_pen = CreatePen(PS_SOLID, 2, RGB(250, 204, 21));
    HGDIOBJ old_br = SelectObject(hdc, bg_br);
    HGDIOBJ old_pen = SelectObject(hdc, gold_pen);

    RoundRect(hdc, 40, 12, 600, 228, 8, 8);

    SelectObject(hdc, old_br);
    SelectObject(hdc, old_pen);
    DeleteObject(bg_br);
    DeleteObject(gold_pen);

    /* Close 'X' Button in Top-Right Corner */
    HBRUSH x_br = CreateSolidBrush(RGB(185, 28, 28));
    old_br = SelectObject(hdc, x_br);
    Rectangle(hdc, 565, 18, 592, 40);
    SelectObject(hdc, old_br);
    DeleteObject(x_br);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, s_tome_font_title);
    RECT x_rc;
    SetRect(&x_rc, 565, 18, 592, 40);
    DrawTextW(hdc, L"X", -1, &x_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    /* Modal Header */
    SetTextColor(hdc, RGB(250, 204, 21));
    RECT th_rc;
    SetRect(&th_rc, 50, 18, 555, 38);
    DrawTextW(hdc, L"=== TOME OF FORBIDDEN KNOWLEDGE ===", -1, &th_rc, DT_CENTER | DT_SINGLELINE);

    /* Secret Index indicator */
    WCHAR wpage[32];
    wsprintfW(wpage, L"Entry %d of %d", s_tome_index + 1, CHAR_MAX_COUNT);
    SetTextColor(hdc, RGB(160, 165, 185));
    SelectObject(hdc, s_tome_font_btn);
    RECT p_rc;
    SetRect(&p_rc, 50, 42, 590, 56);
    DrawTextW(hdc, wpage, -1, &p_rc, DT_CENTER | DT_SINGLELINE);

    const SecretInfo *sec = &g_secrets[s_tome_index];

    /* Deity Name & Title Banner */
    WCHAR wsec_name[128];
    wsprintfW(wsec_name, L"%S : %S", sec->name, sec->title);
    SetTextColor(hdc, sec->color);
    SelectObject(hdc, s_tome_font_name);
    RECT name_rc;
    SetRect(&name_rc, 55, 62, 585, 82);
    DrawTextW(hdc, wsec_name, -1, &name_rc, DT_CENTER | DT_SINGLELINE);

    /* Description Box */
    HBRUSH desc_br = CreateSolidBrush(RGB(10, 11, 18));
    HPEN desc_pen = CreatePen(PS_SOLID, 1, RGB(50, 55, 75));
    old_br = SelectObject(hdc, desc_br);
    old_pen = SelectObject(hdc, desc_pen);

    RoundRect(hdc, 55, 86, 585, 168, 6, 6);

    SelectObject(hdc, old_br);
    SelectObject(hdc, old_pen);
    DeleteObject(desc_br);
    DeleteObject(desc_pen);

    WCHAR wsec_desc[256];
    ascii_to_wide(wsec_desc, sec->description, 256);
    SetTextColor(hdc, RGB(220, 225, 240));
    SelectObject(hdc, s_tome_font_desc);
    RECT d_rc;
    SetRect(&d_rc, 65, 94, 575, 160);
    DrawTextW(hdc, wsec_desc, -1, &d_rc, DT_LEFT | DT_WORDBREAK);

    /* Navigation & Action Buttons */
    /* [ PREV ] */
    HBRUSH btn_br = CreateSolidBrush(RGB(40, 45, 65));
    HPEN b_pen = CreatePen(PS_SOLID, 1, RGB(80, 85, 110));
    old_br = SelectObject(hdc, btn_br);
    old_pen = SelectObject(hdc, b_pen);

    RoundRect(hdc, 55, 178, 145, 218, 4, 4);
    /* [ INVOKE ] */
    HBRUSH inv_br = CreateSolidBrush(RGB(85, 35, 105));
    SelectObject(hdc, inv_br);
    RoundRect(hdc, 160, 178, 320, 218, 4, 4);
    /* [ SOUND PREVIEW ] */
    HBRUSH snd_br = CreateSolidBrush(RGB(25, 55, 85));
    SelectObject(hdc, snd_br);
    RoundRect(hdc, 335, 178, 480, 218, 4, 4);
    /* [ NEXT ] */
    SelectObject(hdc, btn_br);
    RoundRect(hdc, 495, 178, 585, 218, 4, 4);

    SelectObject(hdc, old_br);
    SelectObject(hdc, old_pen);
    DeleteObject(btn_br);
    DeleteObject(inv_br);
    DeleteObject(snd_br);
    DeleteObject(b_pen);

    SelectObject(hdc, s_tome_font_btn);

    RECT b1; SetRect(&b1, 55, 178, 145, 218);
    SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextW(hdc, L"< PREV", -1, &b1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT b2; SetRect(&b2, 160, 178, 320, 218);
    SetTextColor(hdc, RGB(250, 204, 21));
    DrawTextW(hdc, L"INVOKE SECRET", -1, &b2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT b3; SetRect(&b3, 335, 178, 480, 218);
    SetTextColor(hdc, RGB(180, 220, 255));
    DrawTextW(hdc, L"AUDIO PREVIEW", -1, &b3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT b4; SetRect(&b4, 495, 178, 585, 218);
    SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextW(hdc, L"NEXT >", -1, &b4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static BOOL HandleTomeClick(HWND hwnd, int x, int y) {
    if (!s_tome_open) return FALSE;

    /* Close Button */
    if (x >= 565 && x <= 592 && y >= 18 && y <= 40) {
        s_tome_open = FALSE;
        InvalidateRect(hwnd, NULL, FALSE);
        return TRUE;
    }

    /* Prev Button */
    if (x >= 55 && x <= 145 && y >= 178 && y <= 218) {
        s_tome_index = (s_tome_index - 1 + CHAR_MAX_COUNT) % CHAR_MAX_COUNT;
        InvalidateRect(hwnd, NULL, FALSE);
        return TRUE;
    }

    /* Invoke Button */
    if (x >= 160 && x <= 320 && y >= 178 && y <= 218) {
        Game_ApplySecret((enum CharType)s_tome_index, CELL_RED);
        s_tome_open = FALSE;
        InvalidateRect(hwnd, NULL, FALSE);
        return TRUE;
    }

    /* Audio Preview Button */
    if (x >= 335 && x <= 480 && y >= 178 && y <= 218) {
        Sound_PlaySFX(g_secrets[s_tome_index].sound_file);
        return TRUE;
    }

    /* Next Button */
    if (x >= 495 && x <= 585 && y >= 178 && y <= 218) {
        s_tome_index = (s_tome_index + 1) % CHAR_MAX_COUNT;
        InvalidateRect(hwnd, NULL, FALSE);
        return TRUE;
    }

    /* Click outside modal closes it */
    if (x < 40 || x > 600 || y < 12 || y > 228) {
        s_tome_open = FALSE;
        InvalidateRect(hwnd, NULL, FALSE);
        return TRUE;
    }

    return TRUE; /* Consumed inside modal */
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            s_hwndMain = hwnd;
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Render_Paint(hwnd, hdc);
            if (s_tome_open) {
                DrawTomeModal(hdc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (s_tome_open) {
                if (HandleTomeClick(hwnd, x, y)) return 0;
            }

            /* Check Stylus Touch Buttons */
            int btn = Render_GetButtonClicked(x, y);
            if (btn == BTN_NEW_GAME) {
                Game_ResetGame();
                Sound_PlaySFX("select.wav");
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else if (btn == BTN_DIFF) {
                g_game.difficulty = (g_game.difficulty + 1) % 3;
                Sound_PlaySFX("select.wav");
                char dmsg[64];
                snprintf(dmsg, sizeof(dmsg), "AI Difficulty set to: %s",
                    (g_game.difficulty == DIFF_EASY ? "Mortal (Fast)" :
                     g_game.difficulty == DIFF_MEDIUM ? "Elder (Positional)" : "Ancient One (Tactical)"));
                Game_AddStatus(dmsg, FALSE);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else if (btn == BTN_TOME) {
                s_tome_open = TRUE;
                s_tome_index = g_game.char_red;
                Sound_PlaySFX("select.wav");
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else if (btn == BTN_MUTE) {
                Sound_ToggleMute();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            /* Check Board Cell Click */
            int r, c;
            if (Render_GetCellFromPoint(x, y, &r, &c)) {
                Game_HandleClick(r, c);
                InvalidateRect(hwnd, NULL, FALSE);

                /* If AI's turn, schedule fast AI trigger */
                if (g_game.current_player == CELL_BLUE && !g_game.game_over) {
                    SetTimer(hwnd, TIMER_ID_AI, 250, NULL);
                }
            }
            return 0;
        }

        case WM_KEYDOWN: {
            if (s_tome_open) {
                if (wParam == VK_ESCAPE) {
                    s_tome_open = FALSE;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                } else if (wParam == VK_LEFT) {
                    s_tome_index = (s_tome_index - 1 + CHAR_MAX_COUNT) % CHAR_MAX_COUNT;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                } else if (wParam == VK_RIGHT) {
                    s_tome_index = (s_tome_index + 1) % CHAR_MAX_COUNT;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                } else if (wParam == VK_RETURN) {
                    Game_ApplySecret((enum CharType)s_tome_index, CELL_RED);
                    s_tome_open = FALSE;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }

            switch (wParam) {
                case 'N':
                case 'n':
                    Game_ResetGame();
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                case 'D':
                case 'd':
                    g_game.difficulty = (g_game.difficulty + 1) % 3;
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                case 'T':
                case 't':
                case 'S':
                case 's':
                    s_tome_open = !s_tome_open;
                    s_tome_index = g_game.char_red;
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                case 'M':
                case 'm':
                    Sound_ToggleMute();
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                case VK_ESCAPE:
                    if (g_game.has_selected) {
                        g_game.has_selected = FALSE;
                        g_game.selected_r = -1;
                        g_game.selected_c = -1;
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    break;
            }
            return 0;
        }

        case WM_TIMER: {
            if (wParam == TIMER_ID_SEC) {
                Game_OnGameTimerTick();
                Game_OnTurnTimerTick();
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (wParam == TIMER_ID_AI) {
                KillTimer(hwnd, TIMER_ID_AI);
                if (g_game.current_player == CELL_BLUE && !g_game.game_over) {
                    Game_MakeAIMove();
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            return 0;
        }

        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID_SEC);
            KillTimer(hwnd, TIMER_ID_AI);
            CleanupTomeFonts();
            Render_Cleanup();
            Sound_Cleanup();
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    s_hInstance = hInstance;

    LogDebug("=== Ftaghn Ataxx Jornada 720 Starting ===\r\n");

    /* 1. Initialize logic & sound subsystems before creating window */
    Game_Init();
    LogDebug("[1/5] Game_Init OK\r\n");

    Sound_Init();
    LogDebug("[2/5] Sound_Init OK\r\n");

    InitTomeFonts();
    LogDebug("[3/5] InitTomeFonts OK\r\n");

    /* 2. Register Window Class */
    const wchar_t szClassName[] = L"FtaghnAtaxxCE";
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = szClassName;

    if (!RegisterClassW(&wc)) {
        DWORD err = GetLastError();
        if (err != 1410) { /* 1410 = ERROR_CLASS_ALREADY_EXISTS */
            WCHAR errBuf[80];
            wsprintfW(errBuf, L"RegisterClass failed: err=%lu", err);
            LogDebug("FATAL: RegisterClass failed\r\n");
            MessageBoxW(NULL, errBuf, L"Ftaghn Error", MB_OK);
            return 1;
        }
    }
    LogDebug("[4/5] RegisterClass OK\r\n");

    /* 3. Create Fullscreen 640x240 Window (Try WS_EX_TOPMOST first) */
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        szClassName,
        L"Ftaghn: Cosmic Horror Ataxx",
        WS_POPUP | WS_VISIBLE,
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        LogDebug("Retrying CreateWindowEx without WS_EX_TOPMOST...\r\n");
        hwnd = CreateWindowExW(
            0,
            szClassName,
            L"Ftaghn: Cosmic Horror Ataxx",
            WS_POPUP | WS_VISIBLE,
            0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
            NULL, NULL, hInstance, NULL
        );
    }

    if (!hwnd) {
        DWORD err = GetLastError();
        WCHAR errBuf[80];
        wsprintfW(errBuf, L"CreateWindowEx failed: err=%lu", err);
        LogDebug("FATAL: CreateWindowEx failed\r\n");
        MessageBoxW(NULL, errBuf, L"Ftaghn Error", MB_OK);
        return 1;
    }
    s_hwndMain = hwnd;
    LogDebug("[5/5] CreateWindowEx OK\r\n");

    /* 4. Initialize double-buffered renderer with window DC */
    Render_Init(hwnd);
    LogDebug("Render_Init OK\r\n");

    /* 5. Show and refresh window */
    ShowWindow(hwnd, (nShowCmd == 0 ? SW_SHOW : nShowCmd));
    UpdateWindow(hwnd);
    LogDebug("ShowWindow and UpdateWindow OK\r\n");

    /* 6. Start game loop timer & background audio */
    SetTimer(hwnd, TIMER_ID_SEC, 1000, NULL);
    Sound_PlayBGM("bg_music.wav");
    LogDebug("Audio & Timer started\r\n");

    /* 7. Main message dispatch loop */
    LogDebug("Entering message loop...\r\n");
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    LogDebug("Message loop exited cleanly.\r\n");

    return (int)msg.wParam;
}
