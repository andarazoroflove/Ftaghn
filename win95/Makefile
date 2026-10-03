# Makefile for Ftaghn - Windows 95 Edition
# Target: Intel 80486, Windows 95, 12MB RAM

CC = gcc
WINDRES = windres

CFLAGS = -march=i486 -mtune=i486 -O2 -fno-stack-protector -fomit-frame-pointer -mwindows -Wall
LDFLAGS = -nostartfiles -mwindows -Wl,--subsystem,windows:4.0 -Wl,--major-os-version,4 -Wl,--minor-os-version,0 -Wl,--major-subsystem-version,4 -Wl,--minor-subsystem-version,0 -s
LIBS = -lmsvcrt -lwinmm -lgdi32 -luser32 -lkernel32

SRC_DIR = src
OUT_DIR = release

SRCS = $(SRC_DIR)/crt_startup.c $(SRC_DIR)/main.c $(SRC_DIR)/game.c $(SRC_DIR)/render.c $(SRC_DIR)/sound.c
OBJS = $(SRC_DIR)/crt_startup.o $(SRC_DIR)/main.o $(SRC_DIR)/game.o $(SRC_DIR)/render.o $(SRC_DIR)/sound.o $(SRC_DIR)/resource.o

TARGET = $(OUT_DIR)/FTAGHN.EXE

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

$(SRC_DIR)/resource.o: $(SRC_DIR)/resource.rc $(SRC_DIR)/resource.h
	$(WINDRES) -i $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-cmd /c del /f /q "src\*.o" "release\FTAGHN.EXE" "release\FTAGHN_TEST.EXE" 2>nul

.PHONY: all clean

