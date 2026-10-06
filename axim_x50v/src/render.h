#ifndef RENDER_H
#define RENDER_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#define SCREEN_MAX_W 640
#define SCREEN_MAX_H 640

#define BTN_NONE     0
#define BTN_NEW_GAME 1
#define BTN_DIFF     2
#define BTN_TOME     3
#define BTN_MUTE     4

#ifdef __cplusplus
extern "C" {
#endif

BOOL Render_Init(HWND hwnd);
void Render_Cleanup(void);
void Render_DrawFrame(void);
void Render_DrawTomeModal(int selected_secret);
void Render_Flip(HWND hwnd);

int  Render_GetWidth(void);
int  Render_GetHeight(void);
BOOL Render_IsQVGA(void);

BOOL Render_GetCellFromPoint(int x, int y, int *out_r, int *out_c);
int  Render_GetButtonClicked(int x, int y);

void Render_SetCursor(int r, int c);
void Render_MoveCursor(int dr, int dc);
void Render_GetCursor(int *out_r, int *out_c);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_H */
