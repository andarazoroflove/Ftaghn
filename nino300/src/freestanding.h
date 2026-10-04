#ifndef FREESTANDING_H
#define FREESTANDING_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef int            int32_t;
typedef unsigned int   size_t;

typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_arg(v,l)   __builtin_va_arg(v,l)

#ifndef WINAPI
#define WINAPI
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

typedef int            BOOL;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;
typedef int            INT;
typedef long           LONG;
typedef unsigned short WCHAR;
typedef const WCHAR*   LPCWSTR;
typedef WCHAR*         LPWSTR;
typedef const char*    LPCSTR;
typedef char*          LPSTR;
typedef void*          HANDLE;
typedef void*          HWND;
typedef void*          HDC;
typedef void*          HBITMAP;
typedef void*          HBRUSH;
typedef void*          HFONT;
typedef void*          HMODULE;
typedef void*          HINSTANCE;
typedef void*          WNDPROC;
typedef DWORD          COLORREF;
typedef UINT           WPARAM;
typedef LONG           LPARAM;
typedef LONG           LRESULT;

#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define LOWORD(l)  ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)  ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define tolower(c) (((c)>='A' && (c)<='Z') ? ((c)+'a'-'A') : (c))
typedef unsigned long DWORD_PTR;
typedef unsigned short ATOM;
typedef unsigned int   UINT_PTR;

/* Windows Messages */
#define WM_CREATE           0x0001
#define WM_DESTROY          0x0002
#define WM_PAINT            0x000F
#define WM_CLOSE            0x0010
#define WM_QUIT             0x0012
#define WM_ERASEBKGND       0x0014
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_TIMER            0x0113
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_MOUSEMOVE        0x0200

/* Virtual Keys */
#define VK_ESCAPE           0x1B
#define VK_SPACE            0x20
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28
#define VK_RETURN           0x0D

/* Window Styles */
#define WS_VISIBLE          0x10000000L
#define WS_POPUP            0x80000000L
#define CS_HREDRAW          0x0002
#define CS_VREDRAW          0x0001
#define SW_SHOW             5
#define DIB_RGB_COLORS      0
#define SRCCOPY             0x00CC0020L
#define GENERIC_READ        0x80000000
#define FILE_SHARE_READ     0x00000001
#define OPEN_EXISTING       3
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
typedef long LONG_PTR;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *LPRECT;

typedef struct tagPAINTSTRUCT {
    HDC  hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *PMSG, *LPMSG;

typedef struct tagWNDCLASSW {
    UINT      style;
    WNDPROC   lpfnWndProc;
    int       cbClsExtra;
    int       cbWndExtra;
    HINSTANCE hInstance;
    void*     hIcon;
    void*     hCursor;
    HBRUSH    hbrBackground;
    LPCWSTR   lpszMenuName;
    LPCWSTR   lpszClassName;
} WNDCLASSW, *PWNDCLASSW, *LPWNDCLASSW;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

/* C Runtime functions */
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int   memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char  *strcpy(char *dest, const char *src);
char  *strncpy(char *dest, const char *src, size_t n);
char  *strcat(char *dest, const char *src);
int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);
char  *strstr(const char *haystack, const char *needle);

size_t wcslen(const WCHAR *s);
WCHAR *wcscpy(WCHAR *dest, const WCHAR *src);

int    sprintf(char *str, const char *format, ...);
int    snprintf(char *str, size_t size, const char *format, ...);
int    vsprintf(char *str, const char *format, va_list ap);
int    vsnprintf(char *str, size_t size, const char *format, va_list ap);

DWORD  WINAPI GetTickCount(void);

void  *malloc(size_t size);
void   free(void *ptr);

#endif /* FREESTANDING_H */
