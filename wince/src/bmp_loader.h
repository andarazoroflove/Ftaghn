#ifndef BMP_LOADER_H
#define BMP_LOADER_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL BMP_LoadToBuffer(const WCHAR *filepath, uint32_t *framebuffer, int dst_pitch);

#ifdef __cplusplus
}
#endif

#endif /* BMP_LOADER_H */
