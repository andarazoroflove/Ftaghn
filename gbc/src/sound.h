#ifndef FTAGHN_GBC_SOUND_H
#define FTAGHN_GBC_SOUND_H

#include <gb/gb.h>

void Sound_Init(void);
void Sound_ToggleMute(void);
uint8_t Sound_IsMuted(void);

void Sound_PlaySelect(void);
void Sound_PlayClone(void);
void Sound_PlayLeap(void);
void Sound_PlayCapture(uint8_t count);
void Sound_PlayVictory(void);
void Sound_PlayDefeat(void);

#endif /* FTAGHN_GBC_SOUND_H */

