#ifndef RENDER_H
#define RENDER_H

#include "freestanding.h"

#define NINO_SCREEN_W 240
#define NINO_SCREEN_H 320

BOOL Render_Init(HWND hwnd);
void Render_Cleanup(void);
void Render_Paint(HDC hdc);
void Render_DrawAll(void);

void Render_SetTomeVisible(BOOL visible);
BOOL Render_IsTomeVisible(void);
void Render_TomeScroll(int delta);

BOOL Render_HandleClick(int x, int y);
void Render_MoveNavCursor(int dr, int dc);
void Render_SelectNavCursor(void);

#endif /* RENDER_H */
