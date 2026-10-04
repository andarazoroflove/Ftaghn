#ifndef SOUND_H
#define SOUND_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void Sound_Init(void);
void Sound_Poll(void);
void Sound_PlaySFX(const char *rel_filename);
void Sound_PlayBGM(const char *rel_filename);
void Sound_StopBGM(void);
void Sound_ToggleMute(void);
void Sound_Cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* SOUND_H */
