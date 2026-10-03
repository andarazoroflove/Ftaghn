#include <Carbon.h>
#include "game.h"
#include "render.h"
#include "sound.h"
#include <stdio.h>
#include <string.h>

#ifndef HiWord
#define HiWord(a) ((short)(((long)(a) >> 16) & 0xFFFF))
#endif
#ifndef LoWord
#define LoWord(a) ((short)((long)(a) & 0xFFFF))
#endif

/* Menu Resource IDs */
#define mApple      128
#define mFile       129
#define mDifficulty 130
#define mOptions    131

/* Menu Items */
#define iAbout      1

#define iNewGame    1
#define iReset      2
#define iQuit       4

#define iDiffEasy   1
#define iDiffMedium 2
#define iDiffHard   3

#define iOptSound   1

static WindowRef s_window = NULL;
static Boolean   s_running = true;
static UInt32    s_ai_trigger_tick = 0;
static UInt32    s_last_timer_tick = 0;

static void UpdateMenus(void) {
    MenuHandle m_diff = GetMenuHandle(mDifficulty);
    if (m_diff != NULL) {
        CheckMenuItem(m_diff, iDiffEasy,   (g_game.difficulty == DIFF_EASY));
        CheckMenuItem(m_diff, iDiffMedium, (g_game.difficulty == DIFF_MEDIUM));
        CheckMenuItem(m_diff, iDiffHard,   (g_game.difficulty == DIFF_HARD));
    }

    MenuHandle m_opt = GetMenuHandle(mOptions);
    if (m_opt != NULL) {
        CheckMenuItem(m_opt, iOptSound, Sound_IsEnabled());
    }
}

static void HandleMenuChoice(long choice) {
    short menu_id = HiWord(choice);
    short menu_item = LoWord(choice);
    if (menu_id == 0) return;

    switch (menu_id) {
        case mApple:
            if (menu_item == iAbout) {
                Alert(128, NULL);
            }
            break;

        case mFile:
            switch (menu_item) {
                case iNewGame:
                case iReset:
                    Game_ResetGame();
                    s_ai_trigger_tick = 0;
                    Render_Draw(s_window);
                    break;
                case iQuit:
                    s_running = false;
                    break;
            }
            break;

        case mDifficulty:
            switch (menu_item) {
                case iDiffEasy:
                    g_game.difficulty = DIFF_EASY;
                    Game_AddStatus("Difficulty set to Simple.", false);
                    break;
                case iDiffMedium:
                    g_game.difficulty = DIFF_MEDIUM;
                    Game_AddStatus("Difficulty set to Mortal.", false);
                    break;
                case iDiffHard:
                    g_game.difficulty = DIFF_HARD;
                    Game_AddStatus("Difficulty set to Elder Godlike!", false);
                    break;
            }
            UpdateMenus();
            Render_Draw(s_window);
            break;

        case mOptions:
            if (menu_item == iOptSound) {
                Sound_ToggleMute();
                UpdateMenus();
            }
            break;
    }
}

static void HandleMouseDown(EventRecord *event) {
    WindowPtr which_window;
    short part = FindWindow(event->where, &which_window);

    switch (part) {
        case inMenuBar: {
            long choice = MenuSelect(event->where);
            HandleMenuChoice(choice);
            HiliteMenu(0);
            break;
        }

        case inDrag: {
            BitMap screen_bits;
            GetQDGlobalsScreenBits(&screen_bits);
            DragWindow(which_window, event->where, &screen_bits.bounds);
            break;
        }

        case inGoAway:
            if (TrackGoAway(which_window, event->where)) {
                s_running = false;
            }
            break;

        case inContent:
            if (which_window == s_window) {
                SelectWindow(which_window);
                SetPortWindowPort(which_window);

                Point pt = event->where;
                GlobalToLocal(&pt);

                int r, c;
                if (Render_GetCellAt(pt, &r, &c)) {
                    int prev_player = g_game.current_player;
                    Game_HandleClick(r, c);
                    Render_Draw(s_window);

                    /* If player completed a move and handed turn to AI, schedule AI delay */
                    if (prev_player == CELL_RED && g_game.current_player == CELL_BLUE && !g_game.game_over) {
                        s_ai_trigger_tick = TickCount() + 18; /* ~300ms pause for visual pacing */
                    }
                }
            }
            break;
    }
}

static void HandleKeyDown(EventRecord *event) {
    char key = (char)(event->message & charCodeMask);
    if (event->modifiers & cmdKey) {
        long choice = MenuKey(key);
        HandleMenuChoice(choice);
        HiliteMenu(0);
    } else {
        /* Cancel selection on Escape / Space */
        if (key == 27 || key == ' ') {
            if (g_game.has_selected) {
                g_game.has_selected = false;
                Render_Draw(s_window);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    /* Initialize QuickDraw cursor */
    InitCursor();

    /* Set up Menu Bar from resource 128 */
    Handle mbar = GetNewMBar(128);
    if (mbar != NULL) {
        SetMenuBar(mbar);
        DrawMenuBar();
    }

    /* Initialize Game and Sound subsystems */
    Sound_Init();
    Game_Init();

    /* Center Window on Screen */
    BitMap screen_bits;
    GetQDGlobalsScreenBits(&screen_bits);
    short scr_w = screen_bits.bounds.right - screen_bits.bounds.left;
    short scr_h = screen_bits.bounds.bottom - screen_bits.bounds.top;
    short win_left = (scr_w - WINDOW_WIDTH) / 2;
    short win_top  = (scr_h - WINDOW_HEIGHT) / 2 + 15;
    if (win_left < 10) win_left = 10;
    if (win_top < 40) win_top = 40;

    Rect w_bounds;
    SetRect(&w_bounds, win_left, win_top, win_left + WINDOW_WIDTH, win_top + WINDOW_HEIGHT);

    s_window = NewCWindow(NULL, &w_bounds, "\pFTAGHN: Cosmic Horror Ataxx", true, documentProc, (WindowPtr)-1L, true, 0);
    if (s_window == NULL) {
        return 1;
    }

    SetPortWindowPort(s_window);
    Render_Init(s_window);
    UpdateMenus();
    Render_Draw(s_window);

    s_last_timer_tick = TickCount();

    /* Event Loop */
    EventRecord event;
    while (s_running) {
        Boolean got_event = WaitNextEvent(everyEvent, &event, 1, NULL);

        if (got_event) {
            switch (event.what) {
                case mouseDown:
                    HandleMouseDown(&event);
                    break;

                case keyDown:
                case autoKey:
                    HandleKeyDown(&event);
                    break;

                case updateEvt:
                    if ((WindowRef)event.message == s_window) {
                        BeginUpdate(s_window);
                        Render_Draw(s_window);
                        EndUpdate(s_window);
                    }
                    break;

                case kHighLevelEvent:
                    /* Carbon Quit / AppleEvents */
                    break;
            }
        }

        /* 1-second Game & Turn Timer Ticks */
        UInt32 now = TickCount();
        if (now - s_last_timer_tick >= 60) {
            s_last_timer_tick = now;
            Game_OnGameTimerTick();
            Game_OnTurnTimerTick();
            Render_Draw(s_window);
        }

        /* AI Turn Dispatch */
        if (!g_game.game_over && g_game.current_player == CELL_BLUE) {
            if (s_ai_trigger_tick != 0 && now >= s_ai_trigger_tick) {
                s_ai_trigger_tick = 0;
                Game_MakeAIMove();
                Render_Draw(s_window);
            }
        }
    }

    /* Clean exit */
    Render_Cleanup();
    Sound_Cleanup();
    DisposeWindow(s_window);

    return 0;
}

