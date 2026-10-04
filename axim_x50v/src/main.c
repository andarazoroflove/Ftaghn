#include <windows.h>
#include "game.h"
#include "render.h"
#include "sound.h"
#include "freestanding.h"

#define TIMER_ID_SEC 1
#define TIMER_ID_AI  2
#define TIMER_ID_SND 3

#ifndef SHFS_SHOWTASKBAR
#define SHFS_SHOWTASKBAR    0x0001
#define SHFS_HIDETASKBAR    0x0002
#define SHFS_SHOWSIPBUTTON  0x0004
#define SHFS_HIDESIPBUTTON  0x0008
#define SHFS_SHOWSTARTICON  0x0010
#define SHFS_HIDESTARTICON  0x0020
#endif

#ifndef VK_ACTION
#define VK_ACTION 0x86
#endif

typedef BOOL (WINAPI *pfn_SHFullScreen)(HWND hwndRequester, DWORD dwState);

static HINSTANCE s_hInstance = NULL;
static HWND      s_hwndMain  = NULL;

/* Tome of Forbidden Knowledge Modal State */
static BOOL s_tome_open  = FALSE;
static int  s_tome_index = 0;

static void ApplyPocketPCFullScreen(HWND hwnd) {
    HMODULE hAyg = LoadLibraryW(L"aygshell.dll");
    if (hAyg) {
        pfn_SHFullScreen pfn = (pfn_SHFullScreen)GetProcAddressW(hAyg, L"SHFullScreen");
        if (pfn) {
            pfn(hwnd, SHFS_HIDETASKBAR | SHFS_HIDESIPBUTTON | SHFS_HIDESTARTICON);
        }
        FreeLibrary(hAyg);
    }
    SetWindowPos(hwnd, HWND_TOP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SWP_SHOWWINDOW);
}

static void UpdateScreen(HWND hwnd) {
    Sound_Poll();
    Render_DrawFrame();
    if (s_tome_open) {
        Render_DrawTomeModal(s_tome_index);
    }
    Render_Flip(hwnd);
}

static BOOL HandleTomeClick(HWND hwnd, int x, int y) {
    if (!s_tome_open) return FALSE;

    /* Prev Button: X: 24..134, Y: 480..530 */
    if (x >= 24 && x <= 134 && y >= 480 && y <= 530) {
        s_tome_index = (s_tome_index - 1 + CHAR_MAX_COUNT) % CHAR_MAX_COUNT;
        Sound_PlaySFX("select.wav");
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Invoke Button: X: 144..336, Y: 480..530 */
    if (x >= 144 && x <= 336 && y >= 480 && y <= 530) {
        Game_ApplySecret((enum CharType)s_tome_index, CELL_RED);
        s_tome_open = FALSE;
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Next Button: X: 346..456, Y: 480..530 */
    if (x >= 346 && x <= 456 && y >= 480 && y <= 530) {
        s_tome_index = (s_tome_index + 1) % CHAR_MAX_COUNT;
        Sound_PlaySFX("select.wav");
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Resume Button: X: 120..360, Y: 550..600 */
    if (x >= 120 && x <= 360 && y >= 550 && y <= 600) {
        s_tome_open = FALSE;
        Sound_PlaySFX("select.wav");
        UpdateScreen(hwnd);
        return TRUE;
    }

    /* Click outside modal closes it */
    if (x < 10 || x > 470 || y < 10 || y > 630) {
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
            ApplyPocketPCFullScreen(hwnd);
            SetTimer(hwnd, TIMER_ID_SEC, 1000, NULL);
            SetTimer(hwnd, TIMER_ID_SND, 50, NULL);
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
                Sound_PlaySFX("place.wav");
                UpdateScreen(hwnd);
                return 0;
            } else if (btn == BTN_DIFF) {
                g_game.difficulty = (g_game.difficulty + 1) % 3;
                Sound_PlaySFX("select.wav");
                char dmsg[64];
                snprintf(dmsg, sizeof(dmsg), "AI Difficulty: %s",
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

            /* Check Board Cell Tap */
            int r, c;
            if (Render_GetCellFromPoint(x, y, &r, &c)) {
                Game_HandleClick(r, c);
                UpdateScreen(hwnd);

                /* If AI's turn, schedule comfortable 600ms response */
                if (g_game.current_player == CELL_BLUE && !g_game.game_over) {
                    SetTimer(hwnd, TIMER_ID_AI, 600, NULL);
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
                } else if (wParam == VK_LEFT || wParam == VK_UP) {
                    s_tome_index = (s_tome_index - 1 + CHAR_MAX_COUNT) % CHAR_MAX_COUNT;
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;
                } else if (wParam == VK_RIGHT || wParam == VK_DOWN) {
                    s_tome_index = (s_tome_index + 1) % CHAR_MAX_COUNT;
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;
                } else if (wParam == VK_RETURN || wParam == VK_ACTION) {
                    Game_ApplySecret((enum CharType)s_tome_index, CELL_RED);
                    s_tome_open = FALSE;
                    UpdateScreen(hwnd);
                    return 0;
                }
            }

            /* 5-Way Navigator D-Pad Controls */
            switch (wParam) {
                case VK_UP:
                    Render_MoveCursor(-1, 0);
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;
                case VK_DOWN:
                    Render_MoveCursor(1, 0);
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;
                case VK_LEFT:
                    Render_MoveCursor(0, -1);
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;
                case VK_RIGHT:
                    Render_MoveCursor(0, 1);
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;
                case VK_RETURN:
                case VK_SPACE:
                case VK_ACTION: {
                    int r, c;
                    Render_GetCursor(&r, &c);
                    Game_HandleClick(r, c);
                    UpdateScreen(hwnd);

                    if (g_game.current_player == CELL_BLUE && !g_game.game_over) {
                        SetTimer(hwnd, TIMER_ID_AI, 600, NULL);
                    }
                    return 0;
                }

                case VK_ESCAPE:
                    if (g_game.has_selected) {
                        g_game.has_selected = FALSE;
                        g_game.selected_r = -1;
                        g_game.selected_c = -1;
                        UpdateScreen(hwnd);
                    } else {
                        /* Escape exits game cleanly back to Pocket PC Today screen */
                        PostMessage(hwnd, WM_CLOSE, 0, 0);
                    }
                    return 0;

                case 'Q':
                case 'q':
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;

                case 'N':
                case 'n':
                    Game_ResetGame();
                    Sound_PlaySFX("place.wav");
                    UpdateScreen(hwnd);
                    return 0;

                case 'D':
                case 'd':
                    g_game.difficulty = (g_game.difficulty + 1) % 3;
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;

                case 'T':
                case 't':
                    s_tome_open = !s_tome_open;
                    s_tome_index = g_game.char_red;
                    Sound_PlaySFX("select.wav");
                    UpdateScreen(hwnd);
                    return 0;

                case 'M':
                case 'm':
                    Sound_ToggleMute();
                    UpdateScreen(hwnd);
                    return 0;
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
            } else if (wParam == TIMER_ID_SND) {
                Sound_Poll();
            }
            return 0;
        }

        case WM_CLOSE:
            KillTimer(hwnd, TIMER_ID_SEC);
            KillTimer(hwnd, TIMER_ID_AI);
            KillTimer(hwnd, TIMER_ID_SND);
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            Render_Cleanup();
            Sound_Cleanup();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    s_hInstance = hInstance;

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"FtaghnAximClass";
    wc.hCursor       = NULL;

    if (!RegisterClassW(&wc)) {
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        L"FtaghnAximClass",
        L"Ftaghn - Cosmic Horror Ataxx",
        WS_VISIBLE,
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        return 1;
    }

    if (!Render_Init(hwnd)) {
        DestroyWindow(hwnd);
        return 1;
    }

    Sound_Init();
    Game_Init();
    Sound_PlayBGM("bg_music.wav");

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    UpdateScreen(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
