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

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == 0) ? (char *)s : NULL;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

int atoi(const char *s) {
    if (!s) return 0;
    while (*s == ' ' || *s == '\t') s++;
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return sign * val;
}

static void emit_char(char **buf, size_t *rem, size_t *total, char c) {
    if (buf && *buf) {
        if (*rem > 1) {
            **buf = c;
            (*buf)++;
            (*rem)--;
        }
    }
    (*total)++;
}

static void print_uint(char **buf, size_t *rem, size_t *total, uint64_t val, int base, int width, char pad, int uppercase) {
    char num_buf[65];
    int idx = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (val == 0) {
        num_buf[idx++] = '0';
    } else {
        while (val > 0) {
            num_buf[idx++] = digits[val % (uint64_t)base];
            val /= (uint64_t)base;
        }
    }

    while (idx < width) {
        num_buf[idx++] = pad;
    }

    for (int i = idx - 1; i >= 0; i--) {
        emit_char(buf, rem, total, num_buf[i]);
    }
}

static void print_int(char **buf, size_t *rem, size_t *total, int64_t val, int width, char pad) {
    if (val < 0) {
        emit_char(buf, rem, total, '-');
        val = -val;
        if (width > 0) width--;
    }
    print_uint(buf, rem, total, (uint64_t)val, 10, width, pad, 0);
}

int vsnprintf(char *out_buf, size_t max_size, const char *fmt, va_list args) {
    char *buf_ptr = out_buf;
    size_t rem = max_size;
    size_t total = 0;

    for (size_t i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            emit_char(out_buf ? &buf_ptr : NULL, &rem, &total, fmt[i]);
            continue;
        }

        i++;
        char pad = ' ';
        int width = 0;

        if (fmt[i] == '0') {
            pad = '0';
            i++;
        }

        while (fmt[i] >= '0' && fmt[i] <= '9') {
            width = width * 10 + (fmt[i] - '0');
            i++;
        }

        switch (fmt[i]) {
            case 'd':
            case 'i': {
                int val = va_arg(args, int);
                print_int(out_buf ? &buf_ptr : NULL, &rem, &total, val, width, pad);
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                print_uint(out_buf ? &buf_ptr : NULL, &rem, &total, val, 10, width, pad, 0);
                break;
            }
            case 'x': {
                unsigned int val = va_arg(args, unsigned int);
                print_uint(out_buf ? &buf_ptr : NULL, &rem, &total, val, 16, width, pad, 0);
                break;
            }
            case 'X': {
                unsigned int val = va_arg(args, unsigned int);
                print_uint(out_buf ? &buf_ptr : NULL, &rem, &total, val, 16, width, pad, 1);
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                emit_char(out_buf ? &buf_ptr : NULL, &rem, &total, c);
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                while (*s) {
                    emit_char(out_buf ? &buf_ptr : NULL, &rem, &total, *s++);
                }
                break;
            }
            case '%':
                emit_char(out_buf ? &buf_ptr : NULL, &rem, &total, '%');
                break;
            default:
                emit_char(out_buf ? &buf_ptr : NULL, &rem, &total, fmt[i]);
                break;
        }
    }

    if (out_buf && max_size > 0) {
        if (rem > 0) {
            *buf_ptr = '\0';
        } else {
            out_buf[max_size - 1] = '\0';
        }
    }

    return (int)total;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, 0x7FFFFFFF, fmt, args);
    va_end(args);
    return ret;
}

int _vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    return vsnprintf(buf, size, fmt, args);
}

int _snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}

void ascii_to_wide(wchar_t *dest, const char *src, int max_chars) {
    int i = 0;
    if (!dest || max_chars <= 0) return;
    if (!src) { dest[0] = L'\0'; return; }
    while (src[i] && i < max_chars - 1) {
        dest[i] = (wchar_t)(unsigned char)src[i];
        i++;
    }
    dest[i] = L'\0';
}
