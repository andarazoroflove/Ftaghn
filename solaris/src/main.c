#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "game.h"
#include "render.h"
#include "sound.h"

int main(int argc, char *argv[]) {
    Display *dpy;
    Window win;
    int screen;
    int depth;
    Visual *visual;
    Colormap cmap;
    Atom wmDelete;
    XSizeHints size_hints;
    int running = 1;
    int x11_fd;
    struct timeval last_sec_tick, now;

    printf("====================================================\n");
    printf("  FTAGHN - Solaris SPARC Edition (X11 / CDE / OpenWin)\n");
    printf("  Target: SunOS 5.10 / Solaris 10 SPARC (V8+/V9)\n");
    printf("====================================================\n\n");

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printf("Error: Unable to open X11 display '%s'.\n", XDisplayName(NULL));
        printf("Ensure DISPLAY is set properly (e.g. export DISPLAY=:0.0)\n");
        return 1;
    }

    screen = DefaultScreen(dpy);
    depth  = DefaultDepth(dpy, screen);
    visual = DefaultVisual(dpy, screen);
    cmap   = DefaultColormap(dpy, screen);

    win = XCreateSimpleWindow(
        dpy,
        RootWindow(dpy, screen),
        100, 100,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        1,
        BlackPixel(dpy, screen),
        BlackPixel(dpy, screen)
    );

    /* Set window title and icon title */
    XStoreName(dpy, win, "FTAGHN - The Call of Ataxx (Solaris Edition)");
    XSetIconName(dpy, win, "FTAGHN");

    /* Lock window geometry for window managers (CDE dtwm / 4Dwm / mwm / twm) */
    memset(&size_hints, 0, sizeof(size_hints));
    size_hints.flags = PSize | PMinSize | PMaxSize;
    size_hints.width = WINDOW_WIDTH;
    size_hints.height = WINDOW_HEIGHT;
    size_hints.min_width = WINDOW_WIDTH;
    size_hints.min_height = WINDOW_HEIGHT;
    size_hints.max_width = WINDOW_WIDTH;
    size_hints.max_height = WINDOW_HEIGHT;
    XSetWMNormalHints(dpy, win, &size_hints);

    /* Handle window close gracefully via WM_DELETE_WINDOW */
    wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wmDelete, 1);

    /* Event mask: Expose, KeyPress, ButtonPress, StructureNotify */
    XSelectInput(dpy, win, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);

    /* Map (show) the window */
    XMapWindow(dpy, win);

    /* Initialize subsystems */
    Sound_Init();
    Render_Init(dpy, win, screen, depth, visual, cmap);
    Game_Init();

    /* Status banner on startup */
    Game_AddStatus("The stars align upon the Sun workstation...", FALSE);
    Game_AddStatus("Controls: [N]ew  [M]ute  [1-3]Diff  [Q]uit", FALSE);

    x11_fd = ConnectionNumber(dpy);
    gettimeofday(&last_sec_tick, NULL);

    while (running) {
        /* Process all pending X11 events */
        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            switch (ev.type) {
                case Expose:
                    if (ev.xexpose.count == 0) {
                        Render_Draw(dpy, win);
                    }
                    break;

                case ButtonPress:
                    if (ev.xbutton.button == Button1) {
                        int r, c;
                        if (Render_GetCellAt(ev.xbutton.x, ev.xbutton.y, &r, &c)) {
                            Game_HandleClick(r, c);
                            Render_Draw(dpy, win);
                        }
                    }
                    break;

                case KeyPress: {
                    KeySym sym = XLookupKeysym(&ev.xkey, 0);
                    if (sym == XK_Escape || sym == XK_q || sym == XK_Q) {
                        running = 0;
                    } else if (sym == XK_n || sym == XK_N || sym == XK_r || sym == XK_R) {
                        Game_ResetGame();
                        Game_AddStatus("The ritual begins anew.", FALSE);
                        Render_Draw(dpy, win);
                    } else if (sym == XK_m || sym == XK_M) {
                        Sound_ToggleMute();
                        Game_AddStatus(Sound_IsEnabled() ? "Audio restored to /dev/audio." : "Audio silenced.", TRUE);
                        Render_Draw(dpy, win);
                    } else if (sym == XK_1) {
                        g_game.difficulty = DIFF_EASY;
                        Game_AddStatus("Difficulty: Simple (Mortal)", TRUE);
                        Render_Draw(dpy, win);
                    } else if (sym == XK_2) {
                        g_game.difficulty = DIFF_MEDIUM;
                        Game_AddStatus("Difficulty: Cultist (Intermediate)", TRUE);
                        Render_Draw(dpy, win);
                    } else if (sym == XK_3) {
                        g_game.difficulty = DIFF_HARD;
                        Game_AddStatus("Difficulty: Elder Godlike", TRUE);
                        Render_Draw(dpy, win);
                    } else if (sym == XK_h || sym == XK_H || sym == XK_F1) {
                        Game_AddStatus("Keys: [N]ew [M]ute [1-3]Diff [Q]uit", TRUE);
                        Render_Draw(dpy, win);
                    }
                    break;
                }

                case ClientMessage:
                    if ((Atom)ev.xclient.data.l[0] == wmDelete) {
                        running = 0;
                    }
                    break;

                default:
                    break;
            }
        }

        /* 1-second timer tick */
        gettimeofday(&now, NULL);
        long elapsed_ms = (now.tv_sec - last_sec_tick.tv_sec) * 1000 +
                          (now.tv_usec - last_sec_tick.tv_usec) / 1000;
        if (elapsed_ms >= 1000) {
            last_sec_tick = now;
            Game_OnGameTimerTick();
            Game_OnTurnTimerTick();
            Render_Draw(dpy, win);
        }

        /* Sleep up to 20ms or until next X11 event arrives */
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(x11_fd, &rfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 20000; /* 20 ms (~50 fps polling) */
        select(x11_fd + 1, &rfds, NULL, NULL, &tv);
    }

    /* Clean exit */
    Render_Cleanup(dpy);
    Sound_Cleanup();
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    return 0;
}
