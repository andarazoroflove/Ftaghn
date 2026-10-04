#include "bmp_loader.h"
#include "freestanding.h"

#define MAX_SRC_PIXELS (1024 * 1024)

static uint32_t s_src_cache[MAX_SRC_PIXELS];

BOOL BMP_LoadScaled(const WCHAR *filepath, uint32_t *framebuffer, int dst_w, int dst_h, int dst_pitch) {
    if (!filepath || filepath[0] == L'\0' || !framebuffer) return FALSE;
    if (dst_w <= 0 || dst_h <= 0) return FALSE;

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

    int src_w = bih.biWidth;
    int src_h = (bih.biHeight < 0) ? -bih.biHeight : bih.biHeight;
    BOOL bottom_up = (bih.biHeight > 0);

    if (src_w <= 0 || src_h <= 0 || (src_w * src_h) > MAX_SRC_PIXELS) {
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

        /* Seek to bitmap bits if specified in header */
        if (bfh.bfOffBits > 0) {
            SetFilePointer(hFile, bfh.bfOffBits, NULL, FILE_BEGIN);
        }

        int row_stride = (src_w + 3) & ~3; /* 4-byte aligned rows */
        uint8_t row[2048];
        if (row_stride > (int)sizeof(row)) {
            CloseHandle(hFile);
            return FALSE;
        }

        for (int y = 0; y < src_h; y++) {
            int sy = bottom_up ? (src_h - 1 - y) : y;
            ReadFile(hFile, row, row_stride, &bytesRead, NULL);
            uint32_t *cache_row = s_src_cache + sy * src_w;
            for (int x = 0; x < src_w; x++) {
                cache_row[x] = lut[row[x]];
            }
        }
    } else if (bih.biBitCount == 24) {
        if (bfh.bfOffBits > 0) {
            SetFilePointer(hFile, bfh.bfOffBits, NULL, FILE_BEGIN);
        }

        int row_stride = ((src_w * 3) + 3) & ~3;
        uint8_t row[4096];
        if (row_stride > (int)sizeof(row)) {
            CloseHandle(hFile);
            return FALSE;
        }

        for (int y = 0; y < src_h; y++) {
            int sy = bottom_up ? (src_h - 1 - y) : y;
            ReadFile(hFile, row, row_stride, &bytesRead, NULL);
            uint32_t *cache_row = s_src_cache + sy * src_w;
            for (int x = 0; x < src_w; x++) {
                uint8_t b = row[x * 3 + 0];
                uint8_t g = row[x * 3 + 1];
                uint8_t r = row[x * 3 + 2];
                cache_row[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            }
        }
    } else {
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    /* Scale source image directly into destination buffer */
    for (int y = 0; y < dst_h; y++) {
        int sy = (y * src_h) / dst_h;
        if (sy >= src_h) sy = src_h - 1;
        uint32_t *src_row = s_src_cache + sy * src_w;
        uint32_t *dst_row = framebuffer + y * dst_pitch;

        for (int x = 0; x < dst_w; x++) {
            int sx = (x * src_w) / dst_w;
            if (sx >= src_w) sx = src_w - 1;
            dst_row[x] = src_row[sx];
        }
    }

    return TRUE;
}
