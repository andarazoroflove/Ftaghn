#include "sound.h"
#include "game.h"
#include <stdio.h>
#include <mmsystem.h>

static BOOL s_mci_open = FALSE;

void Sound_Init(void) {
    /* No special init needed for WinMM */
}

void Sound_PlaySFX(const char *rel_filename) {
    char full_path[MAX_PATH];
    if (g_game.is_muted) return;
    if (!rel_filename || rel_filename[0] == '\0') return;

    sprintf(full_path, "AUDIO\\%s", rel_filename);

    /* PlaySound with SND_ASYNC | SND_FILENAME | SND_NOWAIT is native to Windows 95 winmm.dll */
    PlaySoundA(full_path, NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT);
}

void Sound_PlayBGM(const char *rel_filename) {
    char cmd[512];
    char full_path[MAX_PATH];
    if (g_game.is_muted) return;
    if (!rel_filename || rel_filename[0] == '\0') return;

    Sound_StopBGM();

    sprintf(full_path, "AUDIO\\%s", rel_filename);

    sprintf(cmd, "open \"%s\" type waveaudio alias bgm", full_path);
    if (mciSendStringA(cmd, NULL, 0, NULL) == 0) {
        s_mci_open = TRUE;
        mciSendStringA("play bgm repeat", NULL, 0, NULL);
    } else {
        /* Fallback to PlaySound loop */
        PlaySoundA(full_path, NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);
    }
}

void Sound_StopBGM(void) {
    if (s_mci_open) {
        mciSendStringA("stop bgm", NULL, 0, NULL);
        mciSendStringA("close bgm", NULL, 0, NULL);
        s_mci_open = FALSE;
    }
    PlaySoundA(NULL, NULL, 0);
}

void Sound_ToggleMute(void) {
    g_game.is_muted = !g_game.is_muted;
    if (g_game.is_muted) {
        Sound_StopBGM();
    } else {
        Sound_PlayBGM("bg_music.wav");
    }
}

void Sound_Cleanup(void) {
    Sound_StopBGM();
}

