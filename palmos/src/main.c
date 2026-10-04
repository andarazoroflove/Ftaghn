#include <PalmOS.h>
#include <PenInputMgr.h>
#include "game.h"
#include "render.h"
#include "sound.h"

#define MainForm 1000

#ifndef chrEscape
#define chrEscape 0x001B
#endif
#ifndef vchrRockerUp
#define vchrRockerUp 0x0132
#define vchrRockerDown 0x0133
#define vchrRockerLeft 0x0134
#define vchrRockerRight 0x0135
#define vchrRockerCenter 0x0136
#endif
#ifndef vchrNavChange
#define vchrNavChange 0x0803
#define navBitUp 0x0001
#define navBitDown 0x0002
#define navBitLeft 0x0004
#define navBitRight 0x0008
#define navBitSelect 0x0010
#endif

static Boolean MainFormHandleEvent(EventType *eventP) {
    Boolean handled = false;

    switch (eventP->eType) {
        case frmOpenEvent: {
            FormType *frm = FrmGetActiveForm();
            FrmDrawForm(frm);
            Render_Init(WinGetDisplayWindow());
            Render_DrawAll();
            handled = true;
            break;
        }

        case frmUpdateEvent:
            Render_DrawAll();
            handled = true;
            break;

        case winDisplayChangedEvent:
            Render_DrawAll();
            handled = true;
            break;

        case penDownEvent: {
            Coord x = eventP->screenX;
            Coord y = eventP->screenY;
            if (Render_HandleClick(x, y)) {
                if (!g_game.game_over && g_game.current_player == CELL_BLUE) {
                    SysTaskDelay(SysTicksPerSecond() / 4);
                    Game_MakeAIMove();
                    Render_DrawAll();
                }
                handled = true;
            }
            break;
        }

        case keyDownEvent: {
            WChar chr = eventP->data.keyDown.chr;
            UInt16 keyCode = eventP->data.keyDown.keyCode;

            /* Escape / Hard Power / Home exits cleanly */
            if (chr == chrEscape || chr == vchrHard1 || chr == vchrHardPower) {
                EventType stopEvt;
                MemSet(&stopEvt, sizeof(EventType), 0);
                stopEvt.eType = appStopEvent;
                EvtAddEventToQueue(&stopEvt);
                handled = true;
                break;
            }

            /* 5-way D-Pad Rocker */
            if (chr == vchrRockerUp || chr == vchrPageUp) {
                Render_MoveNavCursor(-1, 0);
                handled = true;
                break;
            }
            if (chr == vchrRockerDown || chr == vchrPageDown) {
                Render_MoveNavCursor(1, 0);
                handled = true;
                break;
            }
            if (chr == vchrRockerLeft) {
                Render_MoveNavCursor(0, -1);
                handled = true;
                break;
            }
            if (chr == vchrRockerRight) {
                Render_MoveNavCursor(0, 1);
                handled = true;
                break;
            }
            if (chr == vchrRockerCenter) {
                Render_SelectNavCursor();
                if (!g_game.game_over && g_game.current_player == CELL_BLUE) {
                    SysTaskDelay(SysTicksPerSecond() / 4);
                    Game_MakeAIMove();
                    Render_DrawAll();
                }
                handled = true;
                break;
            }

            /* PalmOne 5-way navigation change */
            if (chr == vchrNavChange) {
                if (keyCode & navBitUp) Render_MoveNavCursor(-1, 0);
                else if (keyCode & navBitDown) Render_MoveNavCursor(1, 0);
                else if (keyCode & navBitLeft) Render_MoveNavCursor(0, -1);
                else if (keyCode & navBitRight) Render_MoveNavCursor(0, 1);
                else if (keyCode & navBitSelect) {
                    Render_SelectNavCursor();
                    if (!g_game.game_over && g_game.current_player == CELL_BLUE) {
                        SysTaskDelay(SysTicksPerSecond() / 4);
                        Game_MakeAIMove();
                        Render_DrawAll();
                    }
                }
                handled = true;
                break;
            }
            break;
        }

        default:
            break;
    }

    return handled;
}

static Boolean AppHandleEvent(EventType *eventP) {
    UInt16 formId;
    FormType *frm;

    if (eventP->eType == frmLoadEvent) {
        formId = eventP->data.frmLoad.formID;
        frm = FrmInitForm(formId);
        FrmSetActiveForm(frm);
        FrmSetEventHandler(frm, MainFormHandleEvent);
        return true;
    }
    return false;
}

static void AppEventLoop(void) {
    UInt16 error;
    EventType event;

    do {
        /* Poll every 1 second or until user input */
        EvtGetEvent(&event, SysTicksPerSecond());

        if (event.eType == nilEvent) {
            /* Idle tick: AI move or countdown timer */
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
            continue;
        }

        if (!SysHandleEvent(&event)) {
            if (!MenuHandleEvent(0, &event, &error)) {
                if (!AppHandleEvent(&event)) {
                    FrmDispatchEvent(&event);
                }
            }
        }
    } while (event.eType != appStopEvent);
}

static UInt32 s_pin_version = 0;

static Err AppStart(void) {
    /* Set native coordinate system (320x480 on Palm T|X, 160x160 on Palm Z22) */
    WinSetCoordinateSystem(kCoordinatesNative);

    /* Check if Dynamic Input Area (PIN) is supported before manipulating it */
    s_pin_version = 0;
    if (FtrGet(pinCreator, pinFtrAPIVersion, &s_pin_version) == errNone && s_pin_version != 0) {
        /* Close dynamic input area to unlock full height on devices with virtual Graffiti (T|X, T3, LifeDrive) */
        PINSetInputAreaState(pinInputAreaClosed);
        /* Hide control bar for full immersion */
        StatHide();
    }

    Sound_Init();
    Game_Init();

    FrmGotoForm(MainForm);
    return errNone;
}

static void AppStop(void) {
    FrmCloseAllForms();
    Render_Cleanup();
    Sound_Cleanup();

    /* Restore soft graffiti area and control bar on exit if supported */
    if (s_pin_version != 0) {
        PINSetInputAreaState(pinInputAreaOpen);
        StatShow();
    }
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags) {
    (void)cmdPBP;
    (void)launchFlags;

    if (cmd == sysAppLaunchCmdNormalLaunch) {
        Err err = AppStart();
        if (err == errNone) {
            AppEventLoop();
            AppStop();
        }
    }
    return 0;
}
