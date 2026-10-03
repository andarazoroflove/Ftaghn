cmake_minimum_required(VERSION 3.10)
project(Ftaghn C)

set(REZ_INCLUDE_PATH ${REZ_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/rsrc)

add_application(Ftaghn
    TYPE APPL
    CREATOR FTAG
    src/main.c
    src/game.c
    src/render.c
    src/sound.c
    rsrc/Ftaghn.r
)

