#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
RELEASE_DIR="${BASE_DIR}/release"
SRC_DIR="${BASE_DIR}/src"
TOOLS_DIR="${BASE_DIR}/tools"
STUBS_DIR="${TOOLS_DIR}/stubs"

mkdir -p "${STUBS_DIR}/32" "${STUBS_DIR}/64"
mkdir -p "${RELEASE_DIR}"

echo "=== 1. Generating Solaris ABI Stub Libraries ==="

cat << 'EOF' > "${STUBS_DIR}/stubs_libc.c"
void close(void) {}
void _exit(void) {}
void exit(void) {}
void fork(void) {}
void gettimeofday(void) {}
void kill(void) {}
void memcpy(void) {}
void memmove(void) {}
void memset(void) {}
void open(void) {}
void pipe(void) {}
void printf(void) {}
void puts(void) {}
void rand(void) {}
void read(void) {}
void select(void) {}
void sprintf(void) {}
void srand(void) {}
void strcmp(void) {}
void strlen(void) {}
void strncpy(void) {}
void strstr(void) {}
void time(void) {}
void tolower(void) {}
void waitpid(void) {}
void write(void) {}
EOF

cat << 'EOF' > "${STUBS_DIR}/stubs_x11.c"
void XAllocColor(void) {}
void XCloseDisplay(void) {}
void XCopyArea(void) {}
void XCreateGC(void) {}
void XCreatePixmap(void) {}
void XCreateSimpleWindow(void) {}
void XDestroyWindow(void) {}
void XDisplayName(void) {}
void XDrawLine(void) {}
void XDrawRectangle(void) {}
void XDrawString(void) {}
void XFillArc(void) {}
void XFillRectangle(void) {}
void XFlush(void) {}
void XFreeFont(void) {}
void XFreeGC(void) {}
void XFreePixmap(void) {}
void XInternAtom(void) {}
void XLoadQueryFont(void) {}
void XLookupKeysym(void) {}
void XMapWindow(void) {}
void XNextEvent(void) {}
void XOpenDisplay(void) {}
void XPending(void) {}
void XSelectInput(void) {}
void XSetFont(void) {}
void XSetForeground(void) {}
void XSetIconName(void) {}
void XSetWMNormalHints(void) {}
void XSetWMProtocols(void) {}
void XStoreName(void) {}
void XTextWidth(void) {}
EOF

# Build 32-bit stubs
sparc64-linux-gnu-gcc -m32 -shared -fPIC -fno-builtin \
    -Wl,-soname,libc.so.1 -Wl,--hash-style=sysv \
    -o "${STUBS_DIR}/32/libc.so.1" "${STUBS_DIR}/stubs_libc.c"
ln -sf libc.so.1 "${STUBS_DIR}/32/libc.so"

sparc64-linux-gnu-gcc -m32 -shared -fPIC -fno-builtin \
    -Wl,-soname,libX11.so.4 -Wl,--hash-style=sysv \
    -o "${STUBS_DIR}/32/libX11.so.4" "${STUBS_DIR}/stubs_x11.c"
ln -sf libX11.so.4 "${STUBS_DIR}/32/libX11.so"

# Build 64-bit stubs
sparc64-linux-gnu-gcc -m64 -shared -fPIC -fno-builtin \
    -Wl,-soname,libc.so.1 -Wl,--hash-style=sysv \
    -o "${STUBS_DIR}/64/libc.so.1" "${STUBS_DIR}/stubs_libc.c"
ln -sf libc.so.1 "${STUBS_DIR}/64/libc.so"

sparc64-linux-gnu-gcc -m64 -shared -fPIC -fno-builtin \
    -Wl,-soname,libX11.so.4 -Wl,--hash-style=sysv \
    -o "${STUBS_DIR}/64/libX11.so.4" "${STUBS_DIR}/stubs_x11.c"
ln -sf libX11.so.4 "${STUBS_DIR}/64/libX11.so"

echo "=== 2. Compiling 32-bit SPARC (V8) Release Executable ==="
sparc64-linux-gnu-gcc -m32 -fno-pie -no-pie -O2 -mcpu=v8 -I"${SRC_DIR}" -c \
    "${SRC_DIR}/main.c" \
    "${SRC_DIR}/game.c" \
    "${SRC_DIR}/render.c" \
    "${SRC_DIR}/sound.c" \
    "${SRC_DIR}/crt1_sparc32.s"

sparc64-linux-gnu-gcc -m32 -fno-pie -no-pie -nostdlib -Wl,-e,_start \
    -Wl,--dynamic-linker,/usr/lib/ld.so.1 \
    -Wl,-rpath,/usr/openwin/lib:/usr/lib \
    -Wl,--hash-style=sysv \
    -L"${STUBS_DIR}/32" \
    crt1_sparc32.o main.o game.o render.o sound.o \
    -lX11 -lc -lgcc \
    -o "${RELEASE_DIR}/ftaghn"

rm -f crt1_sparc32.o main.o game.o render.o sound.o

echo "=== 3. Compiling 64-bit SPARC (V9 / UltraSPARC) Release Executable ==="
sparc64-linux-gnu-gcc -m64 -fno-pie -no-pie -O2 -mcpu=v9 -I"${SRC_DIR}" -c \
    "${SRC_DIR}/main.c" \
    "${SRC_DIR}/game.c" \
    "${SRC_DIR}/render.c" \
    "${SRC_DIR}/sound.c" \
    "${SRC_DIR}/crt1_sparc64.s"

sparc64-linux-gnu-gcc -m64 -fno-pie -no-pie -nostdlib -Wl,-e,_start \
    -Wl,--dynamic-linker,/usr/lib/sparcv9/ld.so.1 \
    -Wl,-rpath,/usr/openwin/lib/sparcv9:/usr/lib/64 \
    -Wl,--hash-style=sysv \
    -L"${STUBS_DIR}/64" \
    crt1_sparc64.o main.o game.o render.o sound.o \
    -lX11 -lc -lgcc \
    -o "${RELEASE_DIR}/ftaghn64"

rm -f crt1_sparc64.o main.o game.o render.o sound.o

echo "=== 4. Stripping Debug Symbols ==="
sparc64-linux-gnu-strip --strip-unneeded "${RELEASE_DIR}/ftaghn"
sparc64-linux-gnu-strip --strip-unneeded "${RELEASE_DIR}/ftaghn64"

echo "=== 5. Verification ==="
ls -la "${RELEASE_DIR}/ftaghn" "${RELEASE_DIR}/ftaghn64"
echo ""
echo "--- 32-bit ELF Header ---"
sparc64-linux-gnu-readelf -h "${RELEASE_DIR}/ftaghn"
echo ""
echo "--- 32-bit Dynamic Section ---"
sparc64-linux-gnu-readelf -d "${RELEASE_DIR}/ftaghn"
echo ""
echo "--- 64-bit ELF Header ---"
sparc64-linux-gnu-readelf -h "${RELEASE_DIR}/ftaghn64"
echo ""
echo "--- 64-bit Dynamic Section ---"
sparc64-linux-gnu-readelf -d "${RELEASE_DIR}/ftaghn64"

echo "=== Cross-Compilation Complete! ==="

