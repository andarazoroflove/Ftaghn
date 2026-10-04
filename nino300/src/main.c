#include "freestanding.h"
#include "game.h"
#include "render.h"
#include "sound.h"

/* Win32 API functions from coredll.dll */
extern ATOM      WINAPI RegisterClassW(const WNDCLASSW *lpWndClass);
extern HWND      WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, void *hMenu, HINSTANCE hInstance, void *lpParam);
extern LRESULT   WINAPI DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern BOOL      WINAPI ShowWindow(HWND hWnd, int nCmdShow);
extern BOOL      WINAPI UpdateWindow(HWND hWnd);
extern BOOL      WINAPI GetMessageW(MSG *lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
extern BOOL      WINAPI TranslateMessage(const MSG *lpMsg);
extern LRESULT   WINAPI DispatchMessageW(const MSG *lpMsg);
extern void      WINAPI PostQuitMessage(int nExitCode);
extern BOOL      WINAPI DestroyWindow(HWND hWnd);
extern HDC       WINAPI BeginPaint(HWND hWnd, PAINTSTRUCT *lpPaint);
extern BOOL      WINAPI EndPaint(HWND hWnd, const PAINTSTRUCT *lpPaint);
extern UINT_PTR  WINAPI SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, void *lpTimerFunc);
extern BOOL      WINAPI KillTimer(HWND hWnd, UINT_PTR uIDEvent);
extern void      WINAPI Sleep(DWORD dwMilliseconds);

static const WCHAR s_szClassName[] = L"FtaghnNinoClass";
static const WCHAR s_szTitle[]     = L"Ftaghn: Cosmic Horror Ataxx (Philips Nino 300)";

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            Render_Init(hWnd);
            Sound_Init();
            Game_Init();
            SetTimer(hWnd, 1, 1000, NULL);
            Render_DrawAll();
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            Render_Paint(hdc);
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_ERASEBKGND:
            return 1; /* Suppress GDI background clear for flicker-free rendering */

        case WM_LBUTTONDOWN: {
            int x = (int)LOWORD(lParam);
            int y = (int)HIWORD(lParam);
            if (Render_HandleClick(x, y)) {
                if (!g_game.game_over && g_game.current_player == CELL_BLUE) {
                    Render_DrawAll();
                    Sleep(200);
                    Game_MakeAIMove();
                    Render_DrawAll();
                }
            }
            break;
        }

        case WM_KEYDOWN: {
            /* Clean exit on Escape or Application Exit button */
            if (wParam == VK_ESCAPE || wParam == 0x1B) {
                DestroyWindow(hWnd);
                break;
            }

            /* Hardware Rocker Up/Down and Arrow keys */
            if (wParam == VK_UP) {
                Render_MoveNavCursor(-1, 0);
                break;
            }
            if (wParam == VK_DOWN) {
                Render_MoveNavCursor(1, 0);
                break;
            }
            if (wParam == VK_LEFT) {
                Render_MoveNavCursor(0, -1);
                break;
            }
            if (wParam == VK_RIGHT) {
                Render_MoveNavCursor(0, 1);
                break;
            }

            /* Action / Enter / Space button */
            if (wParam == VK_RETURN || wParam == VK_SPACE || wParam == 0x86) {
                Render_SelectNavCursor();
                if (!g_game.game_over && g_game.current_player == CELL_BLUE) {
                    Render_DrawAll();
                    Sleep(200);
                    Game_MakeAIMove();
                    Render_DrawAll();
                }
                break;
            }
            break;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                if (!g_game.game_over) {
                    if (g_game.current_player == CELL_BLUE) {
                        Game_MakeAIMove();
                        Render_DrawAll();
                    } else {
                        Game_OnGameTimerTick();
                        Game_OnTurnTimerTick();
                        Render_DrawAll();
                    }
                }
                Sound_CheckResumeBGM();
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            KillTimer(hWnd, 1);
            Render_Cleanup();
            Sound_Cleanup();
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = NULL;
    wc.lpszClassName = s_szClassName;

    if (!RegisterClassW(&wc)) {
        return 0;
    }

    /* Create full-screen window for Palm-size PC (240x320) */
    HWND hWnd = CreateWindowExW(
        0,
        s_szClassName,
        s_szTitle,
        WS_VISIBLE | WS_POPUP,
        0, 0,
        NINO_SCREEN_W, NINO_SCREEN_H,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow ? nCmdShow : SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

/* Helper called by crt_mips.S */
int WinMainStartupHelper(void) {
    extern HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName);
    HINSTANCE hInst = (HINSTANCE)GetModuleHandleW(NULL);
    return WinMain(hInst, NULL, (LPWSTR)L"", SW_SHOW);
}
