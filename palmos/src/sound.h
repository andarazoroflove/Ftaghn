#ifndef SOUND_H
#define SOUND_H

#include <PalmOS.h>

#ifdef __cplusplus
extern "C" {
#endif

void Sound_Init(void);
void Sound_PlaySFX(const char *name);
void Sound_PlayTone(UInt16 freq, UInt16 duration_ms, UInt16 amp);
void Sound_PlayFanfare(void);
void Sound_PlayAnomaly(void);
void Sound_ToggleMute(void);
Boolean Sound_IsMuted(void);
void Sound_Cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* SOUND_H */
