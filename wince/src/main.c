#include <windows.h>
#include "game.h"
#include "render.h"
#include "sound.h"
#include "freestanding.h"

#define TIMER_ID_SEC 1
#define TIMER_ID_AI  2

static HINSTANCE s_hInstance = NULL;
static HWND      s_hwndMain  = NULL;

/* Tome of Forbidden Knowledge Modal State */
static BOOL s_tome_open  = FALSE;
static int  s_tome_index = 0;

static void LogDebug(const char *msg) {
    DWORD written = 0;
    DWORD len = (DWORD)strlen(msg);

    /* 1. Root RAM store: \ftaghn_debug.txt */
    HANDLE hRoot = CreateFileW(L"\\ftaghn_debug.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hRoot != INVALID_HANDLE_VALUE) {
        SetFilePointer(hRoot, 0, NULL, FILE_END);
        WriteFile(hRoot, msg, len, &written, NULL);
        CloseHandle(hRoot);
    }

    /* 2. Application directory: debug.txt */
    WCHAR path[MAX_PATH];
    DWORD plen = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (plen > 0) {
        int slash = -1;
        for (DWORD i = 0; i < plen; i++) {
            if (path[i] == L'\\' || path[i] == L'/') slash = (int)i;
        }
        if (slash >= 0) path[slash + 1] = L'\0';
        lstrcatW(path, L"debug.txt");
        HANDLE hApp = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hApp != INVALID_HANDLE_VALUE) {
            SetFilePointer(hApp, 0, NULL, FILE_END);
            WriteFile(hApp, msg, len, &written, NULL);
            CloseHandle(hApp);
        }
    }
}

static void UpdateScreen(HWND hwnd) {
    Render_DrawFrame();
    if (s_tome_open) {
        Render_DrawTomeModal(s_tome_index);
    }
    Render_Flip(hwnd);
}

static BOOL HandleTomeClick(HWND hwnd, int x, int y) {
    if (!s_tome_open) return FALSE;

    /* Close Button (top-right X: 565..592, 18..40) */
    if (x >= 565 && x <= 592 && y >= 18 && y <= 40) {
        s_tome_open = FALSE;
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Prev Button: 55..145, 178..218 */
    if (x >= 55 && x <= 145 && y >= 178 && y <= 218) {
        s_tome_index = (s_tome_index - 1 + CHAR_MAX_COUNT) % CHAR_MAX_COUNT;
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Invoke Button: 160..320, 178..218 */
    if (x >= 160 && x <= 320 && y >= 178 && y <= 218) {
        Game_ApplySecret((enum CharType)s_tome_index, CELL_RED);
        s_tome_open = FALSE;
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Audio Preview Button: 335..480, 178..218 */
    if (x >= 335 && x <= 480 && y >= 178 && y <= 218) {
        Sound_PlaySFX(g_secrets[s_tome_index].sound_file);
        return TRUE;
    }

    /* Next Button: 495..585, 178..218 */
    if (x >= 495 && x <= 585 && y >= 178 && y <= 218) {
        s_tome_index = (s_tome_index + 1) % CHAR_MAX_COUNT;
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Click outside modal closes it: 40..600, 12..228 */
    if (x < 40 || x > 600 || y < 12 || y > 228) {
        s_tome_open = FALSE;
        UpdateScreen(hwnd);
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
            BeginPaint(hwnd, &ps);
            UpdateScreen(hwnd);
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
                UpdateScreen(hwnd);
                return 0;
            } else if (btn == BTN_DIFF) {
                g_game.difficulty = (g_game.difficulty + 1) % 3;
                Sound_PlaySFX("select.wav");
                char dmsg[64];
                snprintf(dmsg, sizeof(dmsg), "AI Difficulty set to: %s",
                    (g_game.difficulty == DIFF_EASY ? "Mortal (Fast)" :
                     g_game.difficulty == DIFF_MEDIUM ? "Elder (Positional)" : "Ancient One (Tactical)"));
                Game_AddStatus(dmsg, FALSE);
                UpdateScreen(hwnd);
                return 0;
            } else if (btn == BTN_TOME) {
                s_tome_open = TRUE;
                s_tome_index = g_game.char_red;
                Sound_PlaySFX("select.wav");
                UpdateScreen(hwnd);
                return 0;
            } else if (btn == BTN_MUTE) {
                Sound_ToggleMute();
                UpdateScreen(hwnd);
                return 0;
            }

            /* Check Board Cell Click */
            int r, c;
            if (Render_GetCellFromPoint(x, y, &r, &c)) {
                Game_HandleClick(r, c);
                UpdateScreen(hwnd);

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
                    UpdateScreen(hwnd);
                    return 0;
                } else if (wParam == VK_LEFT) {
                    s_tome_index = (s_tome_index - 1 + CHAR_MAX_COUNT) % CHAR_MAX_COUNT;
                    UpdateScreen(hwnd);
                    return 0;
                } else if (wParam == VK_RIGHT) {
                    s_tome_index = (s_tome_index + 1) % CHAR_MAX_COUNT;
                    UpdateScreen(hwnd);
                    return 0;
                } else if (wParam == VK_RETURN) {
                    Game_ApplySecret((enum CharType)s_tome_index, CELL_RED);
                    s_tome_open = FALSE;
                    UpdateScreen(hwnd);
                    return 0;
                }
            }

            switch (wParam) {
                case 'N':
                case 'n':
                    Game_ResetGame();
                    UpdateScreen(hwnd);
                    break;
                case 'D':
                case 'd':
                    g_game.difficulty = (g_game.difficulty + 1) % 3;
                    UpdateScreen(hwnd);
                    break;
                case 'T':
                case 't':
                case 'S':
                case 's':
                    s_tome_open = !s_tome_open;
                    s_tome_index = g_game.char_red;
                    UpdateScreen(hwnd);
                    break;
                case 'M':
                case 'm':
                    Sound_ToggleMute();
                    UpdateScreen(hwnd);
                    break;
                case VK_ESCAPE:
                    if (g_game.has_selected) {
                        g_game.has_selected = FALSE;
                        g_game.selected_r = -1;
                        g_game.selected_c = -1;
                        UpdateScreen(hwnd);
                    }
                    break;
            }
            return 0;
        }

        case WM_TIMER: {
            if (wParam == TIMER_ID_SEC) {
                Game_OnGameTimerTick();
                Game_OnTurnTimerTick();
                UpdateScreen(hwnd);
            } else if (wParam == TIMER_ID_AI) {
                KillTimer(hwnd, TIMER_ID_AI);
                if (g_game.current_player == CELL_BLUE && !g_game.game_over) {
                    Game_MakeAIMove();
                    UpdateScreen(hwnd);
                }
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID_SEC);
            KillTimer(hwnd, TIMER_ID_AI);
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

    LogDebug("=====================================================\r\n");
    LogDebug("  FTAGHN: COSMIC HORROR ATAXX - HP Jornada 720\r\n");
    LogDebug("  Pure ARMv4 StrongARM SA-1110 (CeGCC Freestanding)\r\n");
    LogDebug("=====================================================\r\n");

    /* 1. Initialize logic & sound subsystems */
    Game_Init();
    LogDebug("[1/4] Game_Init OK\r\n");

    Sound_Init();
    LogDebug("[2/4] Sound_Init OK\r\n");

    /* 2. Register Window Class */
    const wchar_t szClassName[] = L"FtaghnAtaxxCE";
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = NULL; /* Pure software rendering; no GDI background brush */
    wc.lpszClassName = szClassName;

    if (!RegisterClassW(&wc)) {
        DWORD err = GetLastError();
        if (err != 1410) { /* 1410 = ERROR_CLASS_ALREADY_EXISTS */
            LogDebug("FATAL: RegisterClass failed\r\n");
            MessageBoxW(NULL, L"RegisterClass failed", L"Ftaghn Error", MB_OK);
            return 1;
        }
    }
    LogDebug("[3/4] RegisterClass OK\r\n");

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
        LogDebug("FATAL: CreateWindowEx failed\r\n");
        MessageBoxW(NULL, L"CreateWindowEx failed", L"Ftaghn Error", MB_OK);
        return 1;
    }
    s_hwndMain = hwnd;
    LogDebug("[4/4] CreateWindowEx OK\r\n");

    /* 4. Initialize double-buffered renderer */
    Render_Init(hwnd);
    LogDebug("Render_Init OK\r\n");

    /* 5. Show and render initial frame */
    ShowWindow(hwnd, (nShowCmd == 0 ? SW_SHOW : nShowCmd));
    UpdateWindow(hwnd);
    UpdateScreen(hwnd);
    LogDebug("ShowWindow and initial UpdateScreen OK\r\n");

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
