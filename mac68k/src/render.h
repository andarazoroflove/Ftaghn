#ifndef FTAGHN_MAC68K_RENDER_H
#define FTAGHN_MAC68K_RENDER_H

#include <Windows.h>
#include <Quickdraw.h>
#include "game.h"

#define WINDOW_WIDTH   500
#define WINDOW_HEIGHT  310

#define BOARD_OFFSET_X 14
#define BOARD_OFFSET_Y 22
#define CELL_SIZE      38

void Render_Init(WindowPtr win);
void Render_Draw(WindowPtr win);
void Render_Cleanup(void);
int  Render_GetCellAt(Point pt, int *out_r, int *out_c);

#endif /* FTAGHN_MAC68K_RENDER_H */
