#ifndef FTAGHN_MAC_RENDER_H
#define FTAGHN_MAC_RENDER_H

#include <Carbon.h>
#include "game.h"

#define WINDOW_WIDTH  360
#define WINDOW_HEIGHT 440

#define BOARD_OFFSET_X 12
#define BOARD_OFFSET_Y 44
#define CELL_SIZE      48

void Render_Init(WindowRef window);
void Render_Draw(WindowRef window);
void Render_Cleanup(void);
Boolean Render_GetCellAt(Point pt, int *out_r, int *out_c);

#endif /* FTAGHN_MAC_RENDER_H */

