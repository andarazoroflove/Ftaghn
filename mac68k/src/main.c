#include <Types.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Devices.h>
#include <OSUtils.h>
#include <ToolUtils.h>
#include <Traps.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "render.h"
#include "sound.h"

#define MENU_APPLE      128
#define MENU_FILE       129
#define MENU_DIFF       130
#define MENU_OPTIONS    131

#define ITEM_ABOUT      1

#define ITEM_NEW        1
#define ITEM_RESET      2
#define ITEM_QUIT       4

#define ITEM_EASY       1
#define ITEM_MEDIUM     2
#define ITEM_HARD       3

#define ITEM_SOUND      1

#ifndef kOSTrapType
#define kOSTrapType 0
#define kToolboxTrapType 1
#endif

#ifndef _Unimplemented
#define _Unimplemented 0xA89F
#endif

#ifndef _WaitNextEvent
#define _WaitNextEvent 0xA860
#endif

#ifndef osEvt
#define osEvt 15
#endif

#ifndef suspendResumeMessage
#define suspendResumeMessage 0x01
#endif

#ifndef resumeFlag
#define resumeFlag 0x01
#endif

static Boolean   s_running = true;
static WindowPtr s_win = NULL;
static Boolean   s_hasWNE = false;

static Boolean TrapAvailable(short theTrap) {
    TrapType tType = (theTrap & 0x0800) ? kToolboxTrapType : kOSTrapType;
    return (NGetTrapAddress(theTrap, tType) != NGetTrapAddress(_Unimplemented, kToolboxTrapType));
}

static void UpdateMenus(void) {
    MenuHandle hDiff = GetMenuHandle(MENU_DIFF);
    if (hDiff) {
        CheckItem(hDiff, ITEM_EASY,   g_game.difficulty == DIFF_EASY);
        CheckItem(hDiff, ITEM_MEDIUM, g_game.difficulty == DIFF_MEDIUM);
        CheckItem(hDiff, ITEM_HARD,   g_game.difficulty == DIFF_HARD);
    }
    MenuHandle hOpt = GetMenuHandle(MENU_OPTIONS);
    if (hOpt) {
        CheckItem(hOpt, ITEM_SOUND, Sound_IsEnabled());
    }
}

static void HandleMenuChoice(long choice) {
    short menuId = HiWord(choice);
    short item   = LoWord(choice);

    if (menuId == 0) return;

    switch (menuId) {
        case MENU_APPLE:
            if (item == ITEM_ABOUT) {
                Alert(128, NULL);
            } else {
                Str255 daName;
                GetMenuItemText(GetMenuHandle(MENU_APPLE), item, daName);
                OpenDeskAcc(daName);
            }
            break;

        case MENU_FILE:
            switch (item) {
                case ITEM_NEW:
                case ITEM_RESET:
                    Game_ResetGame();
                    Game_AddStatus("The ritual begins anew.", false);
                    Render_Draw(s_win);
                    break;
                case ITEM_QUIT:
                    s_running = false;
                    break;
            }
            break;

        case MENU_DIFF:
            if (item == ITEM_EASY) {
                g_game.difficulty = DIFF_EASY;
                Game_AddStatus("Difficulty: Simple (Mortal)", true);
            } else if (item == ITEM_MEDIUM) {
                g_game.difficulty = DIFF_MEDIUM;
                Game_AddStatus("Difficulty: Cultist (Intermediate)", true);
            } else if (item == ITEM_HARD) {
                g_game.difficulty = DIFF_HARD;
                Game_AddStatus("Difficulty: Elder Godlike", true);
            }
            UpdateMenus();
            Render_Draw(s_win);
            break;

        case MENU_OPTIONS:
            if (item == ITEM_SOUND) {
                Sound_ToggleMute();
                UpdateMenus();
                Game_AddStatus(Sound_IsEnabled() ? "Audio enabled." : "Audio silenced.", true);
                Render_Draw(s_win);
            }
            break;
    }

    HiliteMenu(0);
}

int main(void) {
    /* Initialize Macintosh 68k Toolbox */
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();

    s_hasWNE = TrapAvailable(_WaitNextEvent);

    /* Setup Menu Bar */
    Handle mbar = GetNewMBar(128);
    if (mbar) {
        SetMenuBar(mbar);
        DisposeHandle(mbar);
        AppendResMenu(GetMenuHandle(MENU_APPLE), 'DRVR');
        DrawMenuBar();
    }

    /* Create Main Document Window */
    s_win = GetNewWindow(128, NULL, (WindowPtr)-1);
    if (!s_win) return 1;

    ShowWindow(s_win);
    SelectWindow(s_win);
    SetPort(s_win);

    /* Initialize Subsystems */
    Sound_Init();
    Render_Init(s_win);
    Game_Init();
    UpdateMenus();

    Game_AddStatus("Cosmic horror stirs within the Macintosh SE...", false);
    Game_AddStatus("Keys: [N]ew  [M]ute  [1-3]Diff  [Q]uit", false);
    Render_Draw(s_win);

    unsigned long last_sec_tick = TickCount();

    while (s_running) {
        EventRecord event;
        Boolean gotEvent;

        if (s_hasWNE) {
            gotEvent = WaitNextEvent(everyEvent, &event, 2, NULL);
        } else {
            SystemTask();
            gotEvent = GetNextEvent(everyEvent, &event);
        }

        if (gotEvent) {
            switch (event.what) {
                case mouseDown: {
                    WindowPtr whichWin;
                    short part = FindWindow(event.where, &whichWin);
                    switch (part) {
                        case inMenuBar:
                            HandleMenuChoice(MenuSelect(event.where));
                            break;

                        case inSysWindow:
                            SystemClick(&event, whichWin);
                            break;

                        case inContent:
                            if (whichWin != FrontWindow()) {
                                SelectWindow(whichWin);
                            } else {
                                SetPort(whichWin);
                                Point pt = event.where;
                                GlobalToLocal(&pt);
                                int r, c;
                                if (Render_GetCellAt(pt, &r, &c)) {
                                    Game_HandleClick(r, c);
                                    Render_Draw(whichWin);
                                }
                            }
                            break;

                        case inDrag:
                            DragWindow(whichWin, event.where, &qd.screenBits.bounds);
                            break;

                        case inGoAway:
                            if (TrackGoAway(whichWin, event.where)) {
                                s_running = false;
                            }
                            break;
                    }
                    break;
                }

                case keyDown:
                case autoKey: {
                    char ch = (char)(event.message & charCodeMask);
                    if (event.modifiers & cmdKey) {
                        HandleMenuChoice(MenuKey(ch));
                    } else {
                        if (ch == 'q' || ch == 'Q' || ch == 27) { /* 27 = Escape */
                            s_running = false;
                        } else if (ch == 'n' || ch == 'N' || ch == 'r' || ch == 'R') {
                            Game_ResetGame();
                            Game_AddStatus("The ritual begins anew.", false);
                            Render_Draw(s_win);
                        } else if (ch == 'm' || ch == 'M') {
                            Sound_ToggleMute();
                            UpdateMenus();
                            Game_AddStatus(Sound_IsEnabled() ? "Audio enabled." : "Audio silenced.", true);
                            Render_Draw(s_win);
                        } else if (ch == '1') {
                            g_game.difficulty = DIFF_EASY;
                            UpdateMenus();
                            Game_AddStatus("Difficulty: Simple (Mortal)", true);
                            Render_Draw(s_win);
                        } else if (ch == '2') {
                            g_game.difficulty = DIFF_MEDIUM;
                            UpdateMenus();
                            Game_AddStatus("Difficulty: Cultist (Intermediate)", true);
                            Render_Draw(s_win);
                        } else if (ch == '3') {
                            g_game.difficulty = DIFF_HARD;
                            UpdateMenus();
                            Game_AddStatus("Difficulty: Elder Godlike", true);
                            Render_Draw(s_win);
                        }
                    }
                    break;
                }

                case updateEvt:
                    if ((WindowPtr)event.message == s_win) {
                        BeginUpdate(s_win);
                        Render_Draw(s_win);
                        EndUpdate(s_win);
                    }
                    break;

                case osEvt:
                    /* MultiFinder Suspend / Resume event */
                    if ((event.message >> 24) == suspendResumeMessage) {
                        if (event.message & resumeFlag) {
                            /* Resumed: redraw window */
                            Render_Draw(s_win);
                        }
                    }
                    break;
            }
        }

        /* 1-Second Timer Tick (60 ticks/sec on Macintosh) */
        unsigned long now = TickCount();
        if (now - last_sec_tick >= 60) {
            last_sec_tick = now;
            Game_OnGameTimerTick();
            Game_OnTurnTimerTick();
            Render_Draw(s_win);
        }
    }

    /* Clean exit */
    Render_Cleanup();
    Sound_Cleanup();
    DisposeWindow(s_win);

    return 0;
}
