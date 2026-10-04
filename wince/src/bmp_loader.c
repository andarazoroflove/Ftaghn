#include "bmp_loader.h"
#include "freestanding.h"

HBITMAP BMP_LoadFromFileW(const WCHAR *filepath, int *out_w, int *out_h) {
    if (!filepath || filepath[0] == L'\0') return NULL;

    HANDLE hFile = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    BITMAPFILEHEADER bfh;
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &bytesRead, NULL) || bytesRead != sizeof(BITMAPFILEHEADER)) {
        CloseHandle(hFile);
        return NULL;
    }

    if (bfh.bfType != 0x4D42) { /* "BM" in little-endian */
        CloseHandle(hFile);
        return NULL;
    }

    BITMAPINFOHEADER bih;
    if (!ReadFile(hFile, &bih, sizeof(BITMAPINFOHEADER), &bytesRead, NULL) || bytesRead != sizeof(BITMAPINFOHEADER)) {
        CloseHandle(hFile);
        return NULL;
    }

    int width = bih.biWidth;
    int height = (bih.biHeight < 0) ? -bih.biHeight : bih.biHeight;
    int bitCount = bih.biBitCount;

    if (width <= 0 || width > 1024 || height <= 0 || height > 1024) {
        CloseHandle(hFile);
        return NULL;
    }

    int numColors = 0;
    if (bitCount <= 8) {
        numColors = (bih.biClrUsed > 0) ? (int)bih.biClrUsed : (1 << bitCount);
        if (numColors > 256) numColors = 256;
    }

    /* Fixed stack buffer for BITMAPINFO + palette (up to 256 colors) */
    uint8_t bmi_buf[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
    memset(bmi_buf, 0, sizeof(bmi_buf));
    BITMAPINFO *pbmi = (BITMAPINFO *)bmi_buf;

    pbmi->bmiHeader = bih;
    if (numColors > 0) {
        ReadFile(hFile, pbmi->bmiColors, numColors * sizeof(RGBQUAD), &bytesRead, NULL);
    }

    SetFilePointer(hFile, bfh.bfOffBits, NULL, FILE_BEGIN);

    void *pBits = NULL;
    HDC hdc = GetDC(NULL);
    HBITMAP hBitmap = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (hdc) ReleaseDC(NULL, hdc);

    if (hBitmap && pBits) {
        DWORD rowStride = ((width * bitCount + 31) / 32) * 4;
        DWORD totalSize = rowStride * height;
        ReadFile(hFile, pBits, totalSize, &bytesRead, NULL);
        if (out_w) *out_w = width;
        if (out_h) *out_h = height;
    } else if (hBitmap) {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }

    CloseHandle(hFile);
    return hBitmap;
}

HBITMAP BMP_LoadFromFileA(const char *filepath, int *out_w, int *out_h) {
    if (!filepath || filepath[0] == '\0') return NULL;
    WCHAR wpath[MAX_PATH];
    ascii_to_wide(wpath, filepath, MAX_PATH);
    return BMP_LoadFromFileW(wpath, out_w, out_h);
}
