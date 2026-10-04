#include "bmp_loader.h"
#include "freestanding.h"

BOOL BMP_LoadToBuffer(const WCHAR *filepath, uint32_t *framebuffer, int dst_pitch) {
    if (!filepath || filepath[0] == L'\0' || !framebuffer) return FALSE;

    HANDLE hFile = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    BITMAPFILEHEADER bfh;
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &bytesRead, NULL) || bytesRead != sizeof(BITMAPFILEHEADER)) {
        CloseHandle(hFile);
        return FALSE;
    }

    if (bfh.bfType != 0x4D42) { /* "BM" */
        CloseHandle(hFile);
        return FALSE;
    }

    BITMAPINFOHEADER bih;
    if (!ReadFile(hFile, &bih, sizeof(BITMAPINFOHEADER), &bytesRead, NULL) || bytesRead != sizeof(BITMAPINFOHEADER)) {
        CloseHandle(hFile);
        return FALSE;
    }

    int w = bih.biWidth;
    int h = (bih.biHeight < 0) ? -bih.biHeight : bih.biHeight;
    BOOL bottom_up = (bih.biHeight > 0);

    if (w != 320 || h != 240) {
        CloseHandle(hFile);
        return FALSE;
    }

    if (bih.biBitCount == 8) {
        RGBQUAD pal[256];
        DWORD pal_read = 0;
        int num_colors = (bih.biClrUsed > 0) ? (int)bih.biClrUsed : 256;
        if (num_colors > 256) num_colors = 256;
        ReadFile(hFile, pal, num_colors * sizeof(RGBQUAD), &pal_read, NULL);

        uint32_t lut[256];
        for (int i = 0; i < num_colors; i++) {
            lut[i] = ((uint32_t)pal[i].rgbRed << 16) | ((uint32_t)pal[i].rgbGreen << 8) | (uint32_t)pal[i].rgbBlue;
        }

        uint8_t row[320];
        for (int y = 0; y < 240; y++) {
            int dy = bottom_up ? (239 - y) : y;
            ReadFile(hFile, row, 320, &bytesRead, NULL);
            uint32_t *dst = framebuffer + dy * dst_pitch;
            for (int x = 0; x < 320; x++) {
                dst[x] = lut[row[x]];
            }
        }
        CloseHandle(hFile);
        return TRUE;
    } else if (bih.biBitCount == 24) {
        uint8_t row[320 * 3];
        for (int y = 0; y < 240; y++) {
            int dy = bottom_up ? (239 - y) : y;
            ReadFile(hFile, row, 320 * 3, &bytesRead, NULL);
            uint32_t *dst = framebuffer + dy * dst_pitch;
            for (int x = 0; x < 320; x++) {
                uint8_t b = row[x * 3 + 0];
                uint8_t g = row[x * 3 + 1];
                uint8_t r = row[x * 3 + 2];
                dst[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            }
        }
        CloseHandle(hFile);
        return TRUE;
    }

    CloseHandle(hFile);
    return FALSE;
}
