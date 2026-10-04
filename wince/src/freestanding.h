#ifndef FREESTANDING_H
#define FREESTANDING_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int   memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char  *strcpy(char *dest, const char *src);
char  *strncpy(char *dest, const char *src, size_t n);
int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);
char  *strcat(char *dest, const char *src);
char  *strchr(const char *s, int c);
char  *strstr(const char *haystack, const char *needle);

int    tolower(int c);
int    toupper(int c);
int    atoi(const char *s);

int snprintf(char *buf, size_t size, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

int _snprintf(char *buf, size_t size, const char *fmt, ...);
int _vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

void ascii_to_wide(wchar_t *dest, const char *src, int max_chars);

#ifdef __cplusplus
}
#endif

#endif /* FREESTANDING_H */
