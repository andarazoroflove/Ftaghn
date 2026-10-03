#ifndef FTAGHN_SOUND_H
#define FTAGHN_SOUND_H

#include <windows.h>

void Sound_Init(void);
void Sound_PlaySFX(const char *rel_filename);
void Sound_PlayBGM(const char *rel_filename);
void Sound_StopBGM(void);
void Sound_ToggleMute(void);
void Sound_Cleanup(void);

#endif /* FTAGHN_SOUND_H */

