#include "sound.h"
#include "game.h"
#include "freestanding.h"

#define CE_SND_SYNC      0x00000000
#define CE_SND_ASYNC     0x00000001
#define CE_SND_NODEFAULT 0x00000002
#define CE_SND_LOOP      0x00000008
#define CE_SND_NOWAIT    0x00002000
#define CE_SND_FILENAME  0x00020000

typedef BOOL (WINAPI *pfn_PlaySoundW)(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);

static pfn_PlaySoundW s_pfnPlaySoundW = NULL;
static HMODULE        s_hSoundModule  = NULL;
static WCHAR          s_audio_dir[MAX_PATH] = {0};
static BOOL           s_sound_inited = FALSE;

static BOOL Sound_FileExists(const WCHAR *path) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return TRUE;
    }
    return FALSE;
}

void Sound_Init(void) {
    if (s_sound_inited) return;
    s_sound_inited = TRUE;

    /* 1. Resolve audio folder directory relative to executable */
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

    /* 2. Dynamically bind PlaySoundW to avoid crashing if symbol is absent in COREDLL */
    /* Attempt 1: coredll.dll */
    s_hSoundModule = LoadLibraryW(L"coredll.dll");
    if (s_hSoundModule) {
        s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddressW(s_hSoundModule, L"PlaySoundW");
        if (!s_pfnPlaySoundW) {
            s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddressW(s_hSoundModule, L"PlaySound");
        }
        if (!s_pfnPlaySoundW) {
            s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddressW(s_hSoundModule, L"sndPlaySoundW");
        }
    }

    /* Attempt 2: waveapi.dll */
    if (!s_pfnPlaySoundW) {
        HMODULE hWave = LoadLibraryW(L"waveapi.dll");
        if (hWave) {
            s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddressW(hWave, L"PlaySoundW");
            if (!s_pfnPlaySoundW) {
                s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddressW(hWave, L"PlaySound");
            }
            if (s_pfnPlaySoundW) {
                s_hSoundModule = hWave;
            }
        }
    }

    /* Attempt 3: mmsystem.dll */
    if (!s_pfnPlaySoundW) {
        HMODULE hMM = LoadLibraryW(L"mmsystem.dll");
        if (hMM) {
            s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddressW(hMM, L"PlaySoundW");
            if (!s_pfnPlaySoundW) {
                s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddressW(hMM, L"sndPlaySoundW");
            }
            if (s_pfnPlaySoundW) {
                s_hSoundModule = hMM;
            }
        }
    }
}

void Sound_PlaySFX(const char *rel_filename) {
    if (g_game.is_muted) return;
    if (!s_pfnPlaySoundW) return;
    if (!rel_filename || rel_filename[0] == '\0') return;

    WCHAR wfilename[64];
    ascii_to_wide(wfilename, rel_filename, 64);

    WCHAR full_path[MAX_PATH];
    lstrcpyW(full_path, s_audio_dir);
    lstrcatW(full_path, wfilename);

    if (Sound_FileExists(full_path)) {
        s_pfnPlaySoundW(full_path, NULL, CE_SND_ASYNC | CE_SND_FILENAME | CE_SND_NOWAIT);
    }
}

void Sound_PlayBGM(const char *rel_filename) {
    if (g_game.is_muted) return;
    if (!s_pfnPlaySoundW) return;
    if (!rel_filename || rel_filename[0] == '\0') return;

    WCHAR wfilename[64];
    ascii_to_wide(wfilename, rel_filename, 64);

    WCHAR full_path[MAX_PATH];
    lstrcpyW(full_path, s_audio_dir);
    lstrcatW(full_path, wfilename);

    if (Sound_FileExists(full_path)) {
        s_pfnPlaySoundW(full_path, NULL, CE_SND_ASYNC | CE_SND_FILENAME | CE_SND_LOOP);
    }
}

void Sound_StopBGM(void) {
    if (s_pfnPlaySoundW) {
        s_pfnPlaySoundW(NULL, NULL, 0);
    }
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
    if (s_hSoundModule) {
        FreeLibrary(s_hSoundModule);
        s_hSoundModule = NULL;
    }
    s_pfnPlaySoundW = NULL;
}
