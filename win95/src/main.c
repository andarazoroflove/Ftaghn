#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "game.h"
#include "render.h"
#include "sound.h"

#define TIMER_ID_SEC 1
#define TIMER_ID_TICK 2

static HINSTANCE s_hInstance = NULL;

static void UpdateMenuChecks(HWND hwnd) {
    HMENU menu = GetMenu(hwnd);
    if (!menu) return;

    /* Difficulty checks */
    CheckMenuItem(menu, ID_DIFF_EASY, (g_game.difficulty == DIFF_EASY) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_DIFF_MEDIUM, (g_game.difficulty == DIFF_MEDIUM) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_DIFF_HARD, (g_game.difficulty == DIFF_HARD) ? MF_CHECKED : MF_UNCHECKED);

    /* Obstacle checks */
    CheckMenuItem(menu, ID_OBS_NONE, (g_game.obstacle_setting == OBS_NONE) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_OBS_SOME, (g_game.obstacle_setting == OBS_SOME) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_OBS_MORE, (g_game.obstacle_setting == OBS_MORE) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_OBS_MADNESS, (g_game.obstacle_setting == OBS_MADNESS) ? MF_CHECKED : MF_UNCHECKED);

    /* Turn Timer checks */
    CheckMenuItem(menu, ID_TIMER_UNLIMITED, (g_game.turn_timer_setting == TIMER_UNLIMITED) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_TIMER_30, (g_game.turn_timer_setting == TIMER_30) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_TIMER_15, (g_game.turn_timer_setting == TIMER_15) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, ID_TIMER_5, (g_game.turn_timer_setting == TIMER_5) ? MF_CHECKED : MF_UNCHECKED);

    /* Mute check */
    CheckMenuItem(menu, ID_GAME_MUTE, g_game.is_muted ? MF_CHECKED : MF_UNCHECKED);
}

/* Options Dialog Procedure */
static INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemTextA(hDlg, IDC_RED_NAME, g_game.player_name_red);
            SetDlgItemTextA(hDlg, IDC_BLUE_NAME, g_game.player_name_blue);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                char red[64], blue[64];
                GetDlgItemTextA(hDlg, IDC_RED_NAME, red, sizeof(red));
                GetDlgItemTextA(hDlg, IDC_BLUE_NAME, blue, sizeof(blue));
                Game_SetNames(red, blue);
                EndDialog(hDlg, IDOK);
                return TRUE;
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

/* Tome of Forbidden Knowledge Dialog Procedure */
static INT_PTR CALLBACK TomeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            const char *tome_text =
                "=== TOME OF FORBIDDEN KNOWLEDGE ===\r\n\r\n"
                "Change Player 1 or Player 2 names in Options (F4) to invoke ancient powers:\r\n\r\n"
                "* Rhan-Tegoth / The Frozen Fear:\r\n"
                "  Player 1 Leap with 0 captures into cluster of >= 5 Red pieces freezes adjacent\r\n"
                "  Blue pieces into Permanent Obstacles (ice)!\r\n\r\n"
                "* Hastur / Unspeakable Oath:\r\n"
                "  Making 4 consecutive Clone moves summons an extra Red piece randomly.\r\n\r\n"
                "* Yog-Sothoth / Dimensional Toll:\r\n"
                "  A Leap capturing >= 2 pieces seals all 8 surrounding cells with Obstacles.\r\n\r\n"
                "* Ghroth / Ghroth's Orbit:\r\n"
                "  5% chance after Red turn to shuffle all pieces in a random 3x3 area.\r\n\r\n"
                "* Azathoth / Entropy:\r\n"
                "  When score >= 20, Clones convert 3 random empty cells into Obstacles.\r\n\r\n"
                "* Ithaqua / Cold Wind:\r\n"
                "  Leaping from an isolated square with 0 captures converts source square to Obstacle.\r\n\r\n"
                "* Abhoth / Annihilation:\r\n"
                "  Opponent moving adjacent to >= 6 Abhoth pieces gets their attacking piece annihilated!\r\n\r\n"
                "* Shoggoth / The Sprawl (AI):\r\n"
                "  10% chance each turn to execute up to 3 rapid Clone moves!\r\n\r\n"
                "* Nyarlathotep / Crawling Chaos (AI):\r\n"
                "  5% chance when clicking a piece to forcibly pick an adjacent piece instead.\r\n\r\n"
                "* Idha / The Sleeper:\r\n"
                "  Main Game Timer ceases completely (Time stops!).\r\n\r\n"
                "* Eihort / Dimensional Stasis:\r\n"
                "  Player 1 Eihort slows timer to 50%; Player 2 Eihort accelerates timer to 200%!\r\n\r\n"
                "* Shudde M'ell / Excessive Space:\r\n"
                "  Expands the board permanently to 9x9 when set before first move.\r\n\r\n"
                "* Cthulhu / Call of Cthulhu:\r\n"
                "  Occupying the center and all four corners awakens Cthulhu for an INSTANT WIN!\r\n\r\n"
                "=== MORTAL EASTER EGGS ===\r\n"
                "* George W. / W. : Presidential portrait overlay & theme sound.\r\n"
                "* Doktor Avalanche : Sisters of Mercy theme, black/silver pieces.\r\n"
                "* KLF / Justified : White pieces, Dillinger & MuMu themes.\r\n"
                "* Fox / Dana (X-Files) : Alien green glow, classic theme.\r\n"
                "* Groovie Mann (TKK) : Pink & black pieces, Sex on Wheelz theme.\r\n"
                "* Sam / Dean : Supernatural Hunter vs Witch (Rowena).\r\n"
                "* Trent (NIN) : Industrial deep blue, Happiness in Slavery theme.\r\n"
                "* Santa / Satan : Leaves Present Obstacles on every 4th move.\r\n";

            SetDlgItemTextA(hDlg, IDC_TOME_TEXT, tome_text);
            return TRUE;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

/* About Dialog Procedure */
static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_COMMAND) {
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
    }
    return FALSE;
}

/* Main Window Procedure */
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            Game_Init();
            Render_Init(hwnd);
            Sound_Init();
            UpdateMenuChecks(hwnd);
            SetTimer(hwnd, TIMER_ID_SEC, 1000, NULL);
            SetTimer(hwnd, TIMER_ID_TICK, 100, NULL);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_GAME_NEW:
                    Game_ResetGame();
                    UpdateMenuChecks(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;

                case ID_GAME_OPTIONS:
                    if (DialogBoxA(s_hInstance, MAKEINTRESOURCEA(IDD_OPTIONS), hwnd, OptionsDlgProc) == IDOK) {
                        UpdateMenuChecks(hwnd);
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    break;

                case ID_GAME_MUTE:
                    Sound_ToggleMute();
                    UpdateMenuChecks(hwnd);
                    break;

                case ID_GAME_RESET_DEFAULTS:
                    strcpy(g_game.player_name_red, "Aforgomon");
                    strcpy(g_game.player_name_blue, "Xexanoth");
                    g_game.difficulty = DIFF_MEDIUM;
                    g_game.obstacle_setting = OBS_NONE;
                    g_game.turn_timer_setting = TIMER_UNLIMITED;
                    Game_ResetGame();
                    UpdateMenuChecks(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;

                case ID_GAME_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;

                case ID_DIFF_EASY:
                    g_game.difficulty = DIFF_EASY;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_DIFF_MEDIUM:
                    g_game.difficulty = DIFF_MEDIUM;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_DIFF_HARD:
                    g_game.difficulty = DIFF_HARD;
                    UpdateMenuChecks(hwnd);
                    break;

                case ID_OBS_NONE:
                    g_game.obstacle_setting = OBS_NONE;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_OBS_SOME:
                    g_game.obstacle_setting = OBS_SOME;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_OBS_MORE:
                    g_game.obstacle_setting = OBS_MORE;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_OBS_MADNESS:
                    g_game.obstacle_setting = OBS_MADNESS;
                    UpdateMenuChecks(hwnd);
                    break;

                case ID_TIMER_UNLIMITED:
                    g_game.turn_timer_setting = TIMER_UNLIMITED;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_TIMER_30:
                    g_game.turn_timer_setting = TIMER_30;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_TIMER_15:
                    g_game.turn_timer_setting = TIMER_15;
                    UpdateMenuChecks(hwnd);
                    break;
                case ID_TIMER_5:
                    g_game.turn_timer_setting = TIMER_5;
                    UpdateMenuChecks(hwnd);
                    break;

                case ID_HELP_RULES:
                    MessageBoxA(hwnd,
                        "HOW TO PLAY FTAGHN (ATAXX):\n\n"
                        "- Red pieces start at top-left and bottom-right corners.\n"
                        "- Blue pieces start at top-right and bottom-left corners.\n\n"
                        "MOVES:\n"
                        "1. GROW (CLONE): Move 1 space in any direction (horizontal, vertical, or diagonal).\n"
                        "   This clones a new piece into the square; your original piece stays!\n\n"
                        "2. JUMP (LEAP): Move 2 spaces in any direction.\n"
                        "   Your original piece leaps to the new square, leaving the old square empty.\n\n"
                        "CONVERSIONS:\n"
                        "Any opponent pieces adjacent to your landing square are immediately captured and converted to your color!\n\n"
                        "WINNING:\n"
                        "The player with the most pieces when the board is filled, or when the timer expires, wins!",
                        "Rules of Ftaghn", MB_OK | MB_ICONINFORMATION);
                    break;

                case ID_HELP_TOME:
                    DialogBoxA(s_hInstance, MAKEINTRESOURCEA(IDD_TOME), hwnd, TomeDlgProc);
                    break;

                case ID_HELP_ABOUT:
                    DialogBoxA(s_hInstance, MAKEINTRESOURCEA(IDD_ABOUT), hwnd, AboutDlgProc);
                    break;
            }
            return 0;

        case WM_LBUTTONDOWN:
            Render_BoardClick(LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_F2) {
                SendMessage(hwnd, WM_COMMAND, ID_GAME_NEW, 0);
            } else if (wParam == VK_F3) {
                SendMessage(hwnd, WM_COMMAND, ID_GAME_MUTE, 0);
            } else if (wParam == VK_F4) {
                SendMessage(hwnd, WM_COMMAND, ID_GAME_OPTIONS, 0);
            } else if (wParam == VK_F1) {
                SendMessage(hwnd, WM_COMMAND, ID_HELP_TOME, 0);
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ID_SEC) {
                Game_OnGameTimerTick();
                Game_OnTurnTimerTick();
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Render_Frame(hdc, hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SIZE:
            Render_Resize(hwnd, LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_ERASEBKGND:
            return 1; /* Suppress background erase to eliminate flicker */

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ID_SEC);
            KillTimer(hwnd, TIMER_ID_TICK);
            Render_Cleanup();
            Sound_Cleanup();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    RECT rect;

    s_hInstance = hInstance;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "FtaghnWin95Class";
    wc.lpszMenuName = MAKEINTRESOURCEA(IDR_MAIN_MENU);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class!", "Error", MB_ICONERROR);
        return 1;
    }

    rect.left = 0;
    rect.top = 0;
    rect.right = DEFAULT_WIDTH;
    rect.bottom = DEFAULT_HEIGHT;
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);

    hwnd = CreateWindowA(
        "FtaghnWin95Class",
        "Ftaghn - Cosmic Horror Ataxx (Windows 95 Edition)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBoxA(NULL, "Failed to create main window!", "Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}

