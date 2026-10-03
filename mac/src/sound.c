#include "sound.h"
#include <string.h>

static Boolean s_sound_enabled = true;

void Sound_Init(void) {
    s_sound_enabled = true;
}

void Sound_Play(short res_id) {
    if (!s_sound_enabled) return;

    Handle snd_handle = GetResource('snd ', res_id);
    if (snd_handle != NULL) {
        HLock(snd_handle);
        /* SndPlay with async = true */
        SndPlay(NULL, snd_handle, true);
        HUnlock(snd_handle);
        ReleaseResource(snd_handle);
    }
}

void Sound_PlaySFX(const char *rel_filename) {
    if (!rel_filename || rel_filename[0] == '\0') return;

    if (strstr(rel_filename, "select")) {
        Sound_Play(SND_ID_SELECT);
    } else if (strstr(rel_filename, "enemy") || strstr(rel_filename, "leap")) {
        Sound_Play(SND_ID_LEAP);
    } else if (strstr(rel_filename, "place") || strstr(rel_filename, "clone")) {
        Sound_Play(SND_ID_CLONE);
    } else if (strstr(rel_filename, "player_big") || strstr(rel_filename, "big_capture")) {
        Sound_Play(SND_ID_BIG_CAPTURE);
    } else if (strstr(rel_filename, "capture")) {
        Sound_Play(SND_ID_CAPTURE);
    } else if (strstr(rel_filename, "win")) {
        Sound_Play(SND_ID_WIN);
    } else if (strstr(rel_filename, "whisper") || strstr(rel_filename, "special")) {
        Sound_Play(SND_ID_WHISPER);
    } else if (strstr(rel_filename, "anomaly")) {
        Sound_Play(SND_ID_ANOMALY);
    } else if (strstr(rel_filename, "tick")) {
        Sound_Play(SND_ID_TICK);
    }
}

void Sound_PlayBGM(const char *rel_filename) {
    /* For BGM on classic Mac, trigger theme sound if available */
    Sound_PlaySFX(rel_filename);
}

void Sound_StopBGM(void) {
    /* No active streaming channel to halt */
}

void Sound_ToggleMute(void) {
    s_sound_enabled = !s_sound_enabled;
}

Boolean Sound_IsEnabled(void) {
    return s_sound_enabled;
}

void Sound_Cleanup(void) {
    /* Release any allocated channels if applicable */
}
