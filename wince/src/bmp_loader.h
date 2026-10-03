#ifndef BMP_LOADER_H
#define BMP_LOADER_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

HBITMAP BMP_LoadFromFileW(const WCHAR *filepath, int *out_w, int *out_h);
HBITMAP BMP_LoadFromFileA(const char *filepath, int *out_w, int *out_h);

#ifdef __cplusplus
}
#endif

#endif /* BMP_LOADER_H */
