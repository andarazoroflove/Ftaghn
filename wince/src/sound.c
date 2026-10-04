#include "sound.h"
#include "game.h"
#include "freestanding.h"

#define CE_SND_SYNC      0x00000000
#define CE_SND_ASYNC     0x00000001
#define CE_SND_NODEFAULT 0x00000002
#define CE_SND_MEMORY    0x00000004
#define CE_SND_LOOP      0x00000008
#define CE_SND_NOSTOP    0x00000010
#define CE_SND_NOWAIT    0x00002000
#define CE_SND_FILENAME  0x00020000

typedef BOOL (WINAPI *pfn_PlaySoundW)(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);

static pfn_PlaySoundW s_pfnPlaySoundW = NULL;
static HMODULE        s_hSoundModule  = NULL;
static WCHAR          s_audio_dir[MAX_PATH] = {0};
static BOOL           s_sound_inited = FALSE;

static char           s_current_bgm[64] = "bg_music.wav";
static BOOL           s_bgm_active = FALSE;
static BOOL           s_sfx_busy = FALSE;
static DWORD          s_sfx_end_tick = 0;

static BOOL Sound_FileExists(const WCHAR *path) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return TRUE;
    }
    return FALSE;
}

static int GetSoundDurationMs(const char *name) {
    if (!name) return 500;
    if (strstr(name, "select")) return 150;
    if (strstr(name, "place")) return 550;
    if (strstr(name, "player_big")) return 950;
    if (strstr(name, "enemyplace")) return 1900;
    if (strstr(name, "big_capture")) return 1500;
    if (strstr(name, "tick")) return 1050;
    if (strstr(name, "anomaly")) return 2200;
    if (strstr(name, "mumu")) return 2200;
    if (strstr(name, "player_special")) return 3500;
    if (strstr(name, "tkk")) return 3500;
    if (strstr(name, "doktor")) return 4200;
    if (strstr(name, "rhan")) return 4500;
    if (strstr(name, "dillinger")) return 5000;
    if (strstr(name, "xfiles")) return 5100;
    if (strstr(name, "santa")) return 5300;
    if (strstr(name, "win")) return 6500;
    if (strstr(name, "supernatural")) return 7300;
    if (strstr(name, "gwb")) return 10000;
    if (strstr(name, "nin")) return 30000;
    return 1000;
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
        s_sfx_busy = TRUE;
        s_sfx_end_tick = GetTickCount() + (DWORD)GetSoundDurationMs(rel_filename);
        /* Play immediately without SND_NOWAIT so move clicks and effects are never dropped */
        s_pfnPlaySoundW(full_path, NULL, CE_SND_ASYNC | CE_SND_FILENAME | CE_SND_NODEFAULT);
    }
}

void Sound_PlayBGM(const char *rel_filename) {
    if (!rel_filename || rel_filename[0] == '\0') return;

    strncpy(s_current_bgm, rel_filename, sizeof(s_current_bgm) - 1);
    s_current_bgm[sizeof(s_current_bgm) - 1] = '\0';
    s_bgm_active = TRUE;

    if (g_game.is_muted) return;
    if (!s_pfnPlaySoundW) return;

    /* If SFX is currently playing, let it finish; Sound_Poll will start BGM */
    if (s_sfx_busy && GetTickCount() < s_sfx_end_tick) {
        return;
    }

    WCHAR wfilename[64];
    ascii_to_wide(wfilename, rel_filename, 64);

    WCHAR full_path[MAX_PATH];
    lstrcpyW(full_path, s_audio_dir);
    lstrcatW(full_path, wfilename);

    if (Sound_FileExists(full_path)) {
        s_sfx_busy = FALSE;
        s_pfnPlaySoundW(full_path, NULL, CE_SND_ASYNC | CE_SND_FILENAME | CE_SND_LOOP | CE_SND_NODEFAULT);
    }
}

void Sound_Poll(void) {
    if (g_game.is_muted || !s_bgm_active || !s_pfnPlaySoundW) return;

    if (s_sfx_busy) {
        if (GetTickCount() >= s_sfx_end_tick) {
            s_sfx_busy = FALSE;
            /* SFX finished; resume background music loop */
            if (s_current_bgm[0] != '\0') {
                WCHAR wfilename[64];
                ascii_to_wide(wfilename, s_current_bgm, 64);

                WCHAR full_path[MAX_PATH];
                lstrcpyW(full_path, s_audio_dir);
                lstrcatW(full_path, wfilename);

                if (Sound_FileExists(full_path)) {
                    s_pfnPlaySoundW(full_path, NULL, CE_SND_ASYNC | CE_SND_FILENAME | CE_SND_LOOP | CE_SND_NODEFAULT);
                }
            }
        }
    }
}

void Sound_StopBGM(void) {
    s_bgm_active = FALSE;
    s_sfx_busy = FALSE;
    if (s_pfnPlaySoundW) {
        s_pfnPlaySoundW(NULL, NULL, 0);
    }
}

void Sound_ToggleMute(void) {
    g_game.is_muted = !g_game.is_muted;
    if (g_game.is_muted) {
        Sound_StopBGM();
    } else {
        Sound_PlayBGM(s_current_bgm[0] ? s_current_bgm : "bg_music.wav");
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
