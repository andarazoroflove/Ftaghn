#ifndef FTAGHN_SOLARIS_RENDER_H
#define FTAGHN_SOLARIS_RENDER_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "game.h"

#define WINDOW_WIDTH  360
#define WINDOW_HEIGHT 440

#define BOARD_OFFSET_X 12
#define BOARD_OFFSET_Y 44
#define CELL_SIZE      48

void Render_Init(Display *dpy, Window win, int screen, int depth, Visual *visual, Colormap cmap);
void Render_Draw(Display *dpy, Window win);
void Render_Cleanup(Display *dpy);
int  Render_GetCellAt(int x, int y, int *out_r, int *out_c);

#endif /* FTAGHN_SOLARIS_RENDER_H */

