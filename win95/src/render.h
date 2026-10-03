#ifndef FTAGHN_RENDER_H
#define FTAGHN_RENDER_H

#include <windows.h>

#define DEFAULT_WIDTH  352
#define DEFAULT_HEIGHT 450

void Render_Init(HWND hwnd);
void Render_Resize(HWND hwnd, int width, int height);
void Render_Cleanup(void);
void Render_Frame(HDC hdc, HWND hwnd);
void Render_BoardClick(int mouse_x, int mouse_y);
void Render_UpdateBackground(const char *filename);

#endif /* FTAGHN_RENDER_H */
