#include "freestanding.h"

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n > 0 && *src) {
        *d++ = *src++;
        n--;
    }
    while (n > 0) {
        *d++ = '\0';
        n--;
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strstr(const char *haystack, const char *needle) {
    size_t nlen;
    if (!*needle) return (char *)haystack;
    nlen = strlen(needle);
    while (*haystack) {
        if (strncmp(haystack, needle, nlen) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }
    return NULL;
}

size_t wcslen(const WCHAR *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

WCHAR *wcscpy(WCHAR *dest, const WCHAR *src) {
    WCHAR *d = dest;
    while ((*d++ = *src++));
    return dest;
}

/* Minimal snprintf implementation */
static void fmt_int(char **buf, size_t *rem, long val, int width, char pad) {
    char tmp[32];
    int i = 0;
    unsigned long uval;
    int neg = 0;

    if (val < 0) {
        neg = 1;
        uval = (unsigned long)(-val);
    } else {
        uval = (unsigned long)val;
    }

    if (uval == 0) tmp[i++] = '0';
    while (uval > 0) {
        tmp[i++] = (char)('0' + (uval % 10));
        uval /= 10;
    }
    if (neg) tmp[i++] = '-';

    while (i < width && *rem > 1) {
        *(*buf)++ = pad;
        (*rem)--;
        width--;
    }
    while (i > 0 && *rem > 1) {
        *(*buf)++ = tmp[--i];
        (*rem)--;
    }
}

static void fmt_str(char **buf, size_t *rem, const char *s) {
    if (!s) s = "(null)";
    while (*s && *rem > 1) {
        *(*buf)++ = *s++;
        (*rem)--;
    }
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    char *buf = str;
    size_t rem = size;

    if (!str || size == 0) return 0;

    while (*format && rem > 1) {
        if (*format != '%') {
            *buf++ = *format++;
            rem--;
            continue;
        }

        format++; /* Skip '%' */
        int width = 0;
        char pad = ' ';
        if (*format == '0') {
            pad = '0';
            format++;
        }
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }

        if (*format == 'd' || *format == 'i') {
            fmt_int(&buf, &rem, va_arg(ap, int), width, pad);
        } else if (*format == 's') {
            fmt_str(&buf, &rem, va_arg(ap, const char *));
        } else if (*format == 'c') {
            if (rem > 1) {
                *buf++ = (char)va_arg(ap, int);
                rem--;
            }
        } else if (*format == '%') {
            *buf++ = '%';
            rem--;
        }
        format++;
    }

    *buf = '\0';
    return (int)(buf - str);
}

int snprintf(char *str, size_t size, const char *format, ...) {
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *str, const char *format, ...) {
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vsnprintf(str, 1024, format, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char *str, const char *format, va_list ap) {
    return vsnprintf(str, 1024, format, ap);
}

/* Static heap allocator (64 KB heap for game structures) */
static unsigned char s_heap[64 * 1024];
static size_t        s_heap_ptr = 0;

void *malloc(size_t size) {
    /* Align to 8 bytes */
    size = (size + 7) & ~7;
    if (s_heap_ptr + size > sizeof(s_heap)) return NULL;
    void *p = &s_heap[s_heap_ptr];
    s_heap_ptr += size;
    return p;
}

void free(void *ptr) {
    (void)ptr;
    /* Static bump allocator: no free needed for fixed game lifecycle */
}
