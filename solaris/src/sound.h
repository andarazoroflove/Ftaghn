#ifndef FTAGHN_SOLARIS_SOUND_H
#define FTAGHN_SOLARIS_SOUND_H

#define SND_ID_SELECT       1
#define SND_ID_CLONE        2
#define SND_ID_LEAP         3
#define SND_ID_CAPTURE      4
#define SND_ID_BIG_CAPTURE  5
#define SND_ID_WIN          6
#define SND_ID_WHISPER      7
#define SND_ID_ANOMALY      8
#define SND_ID_TICK         9

void Sound_Init(void);
void Sound_Play(short sfx_id);
void Sound_PlaySFX(const char *name);
void Sound_PlayBGM(const char *name);
void Sound_StopBGM(void);
void Sound_ToggleMute(void);
int  Sound_IsEnabled(void);
void Sound_Cleanup(void);

#endif /* FTAGHN_SOLARIS_SOUND_H */

