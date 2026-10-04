#ifndef SOUND_H
#define SOUND_H

#include "freestanding.h"

void Sound_Init(void);
void Sound_Cleanup(void);
void Sound_PlaySFX(const char *name);
void Sound_PlayBGM(const char *name);
void Sound_StopBGM(void);
void Sound_ToggleMute(void);
BOOL Sound_IsMuted(void);
void Sound_CheckResumeBGM(void);

#endif /* SOUND_H */
