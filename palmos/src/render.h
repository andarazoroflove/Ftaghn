#ifndef RENDER_H
#define RENDER_H

#include <PalmOS.h>
#include "game.h"

#ifdef __cplusplus
extern "C" {
#endif

Boolean Render_Init(WinHandle displayWin);
void    Render_Cleanup(void);
void    Render_DrawAll(void);

void    Render_SetTomeVisible(Boolean visible);
Boolean Render_IsTomeVisible(void);
void    Render_TomeScroll(int delta);

Boolean Render_HandleClick(Coord x, Coord y);
void    Render_MoveNavCursor(int dr, int dc);
void    Render_SelectNavCursor(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_H */
