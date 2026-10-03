#include "sound.h"

static uint8_t s_is_muted = 0;

void Sound_Init(void) {
    NR52_REG = 0x80; /* Power on APU */
    NR50_REG = 0x77; /* Max stereo volume */
    NR51_REG = 0xFF; /* Route all 4 channels to Left and Right speakers */
}

void Sound_ToggleMute(void) {
    s_is_muted = !s_is_muted;
}

uint8_t Sound_IsMuted(void) {
    return s_is_muted;
}

void Sound_PlaySelect(void) {
    if (s_is_muted) return;
    NR10_REG = 0x00; /* No sweep */
    NR11_REG = 0x80; /* 50% duty */
    NR12_REG = 0x51; /* Quick volume fade */
    NR13_REG = 0xC0; /* Low freq byte */
    NR14_REG = 0x86; /* Trigger note ~1728 */
}

void Sound_PlayClone(void) {
    if (s_is_muted) return;
    NR10_REG = 0x14; /* Upward frequency sweep */
    NR11_REG = 0x80; /* 50% duty */
    NR12_REG = 0x62; /* Volume envelope */
    NR13_REG = 0x60;
    NR14_REG = 0x85; /* Trigger */
}

void Sound_PlayLeap(void) {
    if (s_is_muted) return;
    NR10_REG = 0x2A; /* Downward whoosh sweep */
    NR11_REG = 0x40; /* 25% duty */
    NR12_REG = 0x73; /* Volume decay */
    NR13_REG = 0x90;
    NR14_REG = 0x85; /* Trigger */
}

void Sound_PlayCapture(uint8_t count) {
    if (s_is_muted) return;
    /* Noise burst on Channel 4 */
    NR41_REG = 0x1F;
    if (count >= 3) {
        NR42_REG = 0x92; /* Louder punch for big capture */
        NR43_REG = 0x62;
    } else {
        NR42_REG = 0x62;
        NR43_REG = 0x53;
    }
    NR44_REG = 0x80; /* Trigger */

    /* Layer with low tone on Channel 2 */
    NR21_REG = 0x80;
    NR22_REG = 0x51;
    NR23_REG = 0x30;
    NR24_REG = 0x83;
}

void Sound_PlayVictory(void) {
    if (s_is_muted) return;
    NR10_REG = 0x12; /* Rising sweep */
    NR11_REG = 0x80;
    NR12_REG = 0x83;
    NR13_REG = 0x50;
    NR14_REG = 0x86;
}

void Sound_PlayDefeat(void) {
    if (s_is_muted) return;
    NR10_REG = 0x3A; /* Heavy downward sweep */
    NR11_REG = 0xC0;
    NR12_REG = 0x74;
    NR13_REG = 0x20;
    NR14_REG = 0x84;
}

