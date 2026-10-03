# Makefile for Ftaghn - Nintendo Game Boy Color Edition
# Target: Nintendo Game Boy Color (CGB), GBA, Analogue Pocket, Emulators

GBDK_HOME = tools/gbdk
LCC = $(GBDK_HOME)/bin/lcc.exe

CFLAGS = -Wm-yc -Wm-ynFTAGHN

SRC_DIR = src
OUT_DIR = release

SRCS = $(SRC_DIR)/tiles.c $(SRC_DIR)/sound.c $(SRC_DIR)/game.c $(SRC_DIR)/main.c
OBJS = $(SRC_DIR)/tiles.o $(SRC_DIR)/sound.o $(SRC_DIR)/game.o $(SRC_DIR)/main.o

TARGET = $(OUT_DIR)/FTAGHN.GBC

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LCC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(LCC) -c -o $@ $<

clean:
	-cmd /c del /f /q src\*.o src\*.lst src\*.asm src\*.ihx src\*.sym release\FTAGHN.GBC 2>nul

.PHONY: all clean
