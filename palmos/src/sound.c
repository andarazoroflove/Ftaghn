#include "sound.h"
#include <SoundMgr.h>
#include <StringMgr.h>

static Boolean s_muted = false;

void Sound_Init(void) {
    s_muted = false;
}

void Sound_Cleanup(void) {
    /* Nothing required */
}

void Sound_ToggleMute(void) {
    s_muted = !s_muted;
}

Boolean Sound_IsMuted(void) {
    return s_muted;
}

void Sound_PlayTone(UInt16 freq, UInt16 duration_ms, UInt16 amp) {
    SndCommandType cmd;
    if (s_muted) return;

    if (amp > sndMaxAmp) amp = sndMaxAmp;
    cmd.cmd = sndCmdFreqDurationAmp;
    cmd.reserved = 0;
    cmd.param1 = freq;
    cmd.param2 = duration_ms;
    cmd.param3 = amp;

    SndDoCmd(0, &cmd, 0); /* 0 = wait/block for short duration */
}

void Sound_PlayFanfare(void) {
    if (s_muted) return;
    Sound_PlayTone(523, 70, 50); /* C5 */
    Sound_PlayTone(659, 70, 55); /* E5 */
    Sound_PlayTone(784, 90, 60); /* G5 */
    Sound_PlayTone(1046, 180, 64); /* C6 */
}

void Sound_PlayAnomaly(void) {
    if (s_muted) return;
    Sound_PlayTone(880, 40, 48);
    Sound_PlayTone(740, 40, 48);
    Sound_PlayTone(587, 50, 52);
    Sound_PlayTone(440, 80, 55);
}

void Sound_PlaySFX(const char *name) {
    if (s_muted || !name) return;

    if (StrStr(name, "select") != NULL) {
        Sound_PlayTone(1400, 15, 35);
    } else if (StrStr(name, "player_big_capture") != NULL) {
        Sound_PlayTone(523, 35, 50);
        Sound_PlayTone(659, 40, 55);
        Sound_PlayTone(784, 60, 60);
    } else if (StrStr(name, "big_capture") != NULL) {
        Sound_PlayTone(392, 40, 55);
        Sound_PlayTone(293, 45, 55);
        Sound_PlayTone(196, 70, 60);
    } else if (StrStr(name, "enemyplace") != NULL) {
        Sound_PlayTone(330, 40, 45);
    } else if (StrStr(name, "place") != NULL) {
        Sound_PlayTone(587, 30, 45);
    } else if (StrStr(name, "win") != NULL) {
        Sound_PlayFanfare();
    } else if (StrStr(name, "anomaly") != NULL || StrStr(name, "rhan") != NULL) {
        Sound_PlayAnomaly();
    } else if (StrStr(name, "tick") != NULL) {
        Sound_PlayTone(1100, 10, 30);
    } else if (StrStr(name, "player_special") != NULL) {
        Sound_PlayTone(440, 40, 50);
        Sound_PlayTone(554, 40, 55);
        Sound_PlayTone(659, 70, 60);
    } else {
        /* General tactile feedback */
        Sound_PlayTone(440, 25, 40);
    }
}
