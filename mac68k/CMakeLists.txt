cmake_minimum_required(VERSION 3.10)
project(FtaghnSE C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -mcpu=68000 -Wall")

set(REZ_INCLUDE_PATH ${REZ_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/rsrc)

add_application(FtaghnSE
    TYPE APPL
    CREATOR FTAG
    src/main.c
    src/game.c
    src/render.c
    src/sound.c
    rsrc/FtaghnSE.r
)

