#ifndef RENDER_H
#define RENDER_H

#include <windows.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 240

#define BTN_NONE     0
#define BTN_NEW_GAME 1
#define BTN_DIFF     2
#define BTN_TOME     3
#define BTN_MUTE     4

#ifdef __cplusplus
extern "C" {
#endif

void Render_Init(HWND hwnd);
void Render_Cleanup(void);
void Render_Paint(HWND hwnd, HDC hdc);
void Render_LoadCurrentBackground(void);

BOOL Render_GetCellFromPoint(int x, int y, int *out_r, int *out_c);
int  Render_GetButtonClicked(int x, int y);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_H */
