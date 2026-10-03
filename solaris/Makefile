# ====================================================================
# FTAGHN - Solaris SPARC Edition Makefile
# Target: SunOS 5.10 / Solaris 10 (SPARC V8+ / SPARC V9)
#
# Supported Compilers:
#   - Sun Freeware GCC (/usr/sfw/bin/gcc)
#   - GNU C Compiler (gcc)
#   - Sun Studio C Compiler (cc)
# ====================================================================

# Auto-detect compiler if not specified
CC ?= gcc
ifeq ($(shell which $(CC) 2>/dev/null),)
    ifneq ($(wildcard /usr/sfw/bin/gcc),)
        CC = /usr/sfw/bin/gcc
    else
        CC = cc
    endif
endif

SRC_DIR = src
BIN_32  = ftaghn
BIN_64  = ftaghn64

SRCS    = $(SRC_DIR)/main.c \
          $(SRC_DIR)/game.c \
          $(SRC_DIR)/render.c \
          $(SRC_DIR)/sound.c

# X11 & Solaris system paths
X11_INC = -I/usr/openwin/include
X11_LIB = -L/usr/openwin/lib -R/usr/openwin/lib
X11_LIB64 = -L/usr/openwin/lib/sparcv9 -R/usr/openwin/lib/sparcv9 -L/usr/lib/64 -R/usr/lib/64

COMMON_CFLAGS = -O2 -I$(SRC_DIR) $(X11_INC)
COMMON_LIBS   = -lX11 -lsocket -lnsl -lm

all: $(BIN_32)

# 32-bit SPARC (V8+)
$(BIN_32): $(SRCS)
	$(CC) $(COMMON_CFLAGS) -m32 $(X11_LIB) $(SRCS) -o $@ $(COMMON_LIBS)
	@echo "Built 32-bit SPARC binary: $(BIN_32)"

# 64-bit SPARC (V9 / UltraSPARC)
$(BIN_64): $(SRCS)
	$(CC) $(COMMON_CFLAGS) -m64 $(X11_LIB64) $(SRCS) -o $@ $(COMMON_LIBS)
	@echo "Built 64-bit SPARC V9 binary: $(BIN_64)"

both: $(BIN_32) $(BIN_64)

clean:
	rm -f $(BIN_32) $(BIN_64) *.o $(SRC_DIR)/*.o

.PHONY: all both clean

