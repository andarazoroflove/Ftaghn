#ifndef FTAGHN_MAC68K_SOUND_H
#define FTAGHN_MAC68K_SOUND_H

#include <Types.h>

#define SND_ID_SELECT       1001
#define SND_ID_CLONE        1002
#define SND_ID_LEAP         1003
#define SND_ID_CAPTURE      1004
#define SND_ID_BIG_CAPTURE  1005
#define SND_ID_WIN          1006
#define SND_ID_WHISPER      1007
#define SND_ID_ANOMALY      1008
#define SND_ID_TICK         1009

void Sound_Init(void);
void Sound_Play(short snd_id);
void Sound_PlaySFX(const char *name);
void Sound_PlayBGM(const char *name);
void Sound_StopBGM(void);
void Sound_ToggleMute(void);
Boolean Sound_IsEnabled(void);
void Sound_Cleanup(void);

#endif /* FTAGHN_MAC68K_SOUND_H */
