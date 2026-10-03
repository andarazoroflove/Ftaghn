#include "sound.h"
#include <Sound.h>
#include <Resources.h>
#include <Traps.h>
#include <OSUtils.h>
#include <string.h>

#ifndef kOSTrapType
#define kOSTrapType 0
#define kToolboxTrapType 1
#endif

#ifndef _Unimplemented
#define _Unimplemented 0xA89F
#endif

static Boolean s_sound_enabled = true;
static Boolean s_sound_available = false;

static Boolean TrapAvailable(short theTrap) {
    TrapType tType = (theTrap & 0x0800) ? kToolboxTrapType : kOSTrapType;
    return (NGetTrapAddress(theTrap, tType) != NGetTrapAddress(_Unimplemented, kToolboxTrapType));
}

void Sound_Init(void) {
    s_sound_enabled = true;
    s_sound_available = TrapAvailable(_SoundDispatch);
}

void Sound_Play(short snd_id) {
    if (!s_sound_enabled) return;

    if (s_sound_available) {
        Handle h = GetResource('snd ', snd_id);
        if (h) {
            HLock(h);
            /* Synchronous play on 68k ensures memory is not released while playing */
            SndPlay(NULL, h, false);
            HUnlock(h);
            ReleaseResource(h);
        }
    } else {
        /* Fallback for pre-Sound Manager machines */
        SysBeep(1);
    }
}

void Sound_PlaySFX(const char *name) {
    if (!name || name[0] == '\0') return;

    if (strstr(name, "select")) {
        Sound_Play(SND_ID_SELECT);
    } else if (strstr(name, "enemy") || strstr(name, "leap")) {
        Sound_Play(SND_ID_LEAP);
    } else if (strstr(name, "place") || strstr(name, "clone")) {
        Sound_Play(SND_ID_CLONE);
    } else if (strstr(name, "player_big") || strstr(name, "big_capture")) {
        Sound_Play(SND_ID_BIG_CAPTURE);
    } else if (strstr(name, "capture")) {
        Sound_Play(SND_ID_CAPTURE);
    } else if (strstr(name, "win")) {
        Sound_Play(SND_ID_WIN);
    } else if (strstr(name, "whisper") || strstr(name, "special")) {
        Sound_Play(SND_ID_WHISPER);
    } else if (strstr(name, "anomaly")) {
        Sound_Play(SND_ID_ANOMALY);
    } else if (strstr(name, "tick")) {
        Sound_Play(SND_ID_TICK);
    }
}

void Sound_PlayBGM(const char *name) {
    Sound_PlaySFX(name);
}

void Sound_StopBGM(void) {
}

void Sound_ToggleMute(void) {
    s_sound_enabled = !s_sound_enabled;
}

Boolean Sound_IsEnabled(void) {
    return s_sound_enabled;
}

void Sound_Cleanup(void) {
}
