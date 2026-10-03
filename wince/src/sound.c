#include "sound.h"
#include "game.h"
#include "freestanding.h"

static WCHAR s_audio_dir[MAX_PATH] = {0};

void Sound_Init(void) {
    WCHAR exe_path[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    if (len > 0) {
        int last_slash = -1;
        for (DWORD i = 0; i < len; i++) {
            if (exe_path[i] == L'\\' || exe_path[i] == L'/') {
                last_slash = (int)i;
            }
        }
        if (last_slash >= 0) {
            exe_path[last_slash + 1] = L'\0';
        }
        lstrcpyW(s_audio_dir, exe_path);
        lstrcatW(s_audio_dir, L"AUDIO\\");
    } else {
        lstrcpyW(s_audio_dir, L"\\Storage Card\\Ftaghn\\AUDIO\\");
    }
}

void Sound_PlaySFX(const char *rel_filename) {
    if (g_game.is_muted) return;
    if (!rel_filename || rel_filename[0] == '\0') return;

    WCHAR wfilename[64];
    MultiByteToWideChar(CP_ACP, 0, rel_filename, -1, wfilename, 64);

    WCHAR full_path[MAX_PATH];
    lstrcpyW(full_path, s_audio_dir);
    lstrcatW(full_path, wfilename);

    PlaySoundW(full_path, NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT);
}

void Sound_PlayBGM(const char *rel_filename) {
    if (g_game.is_muted) return;
    if (!rel_filename || rel_filename[0] == '\0') return;

    WCHAR wfilename[64];
    MultiByteToWideChar(CP_ACP, 0, rel_filename, -1, wfilename, 64);

    WCHAR full_path[MAX_PATH];
    lstrcpyW(full_path, s_audio_dir);
    lstrcatW(full_path, wfilename);

    PlaySoundW(full_path, NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);
}

void Sound_StopBGM(void) {
    PlaySoundW(NULL, NULL, 0);
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
