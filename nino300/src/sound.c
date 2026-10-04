#include "sound.h"
#include "game.h"
#include "freestanding.h"

#define CE_SND_SYNC      0x00000000
#define CE_SND_ASYNC     0x00000001
#define CE_SND_NODEFAULT 0x00000002
#define CE_SND_MEMORY    0x00000004
#define CE_SND_LOOP      0x00000008
#define CE_SND_NOSTOP    0x00000010
#define CE_SND_FILENAME  0x00020000

typedef BOOL (WINAPI *pfn_PlaySoundW)(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);
typedef BOOL (WINAPI *pfn_MessageBeep)(UINT uType);

/* External Win32 imports */
extern HMODULE WINAPI LoadLibraryW(LPCWSTR lpLibFileName);
extern void*   WINAPI GetProcAddress(HMODULE hModule, LPCWSTR lpProcName);
extern BOOL    WINAPI FreeLibrary(HMODULE hLibModule);
extern HANDLE  WINAPI CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, void *lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
extern BOOL    WINAPI CloseHandle(HANDLE hObject);
extern DWORD   WINAPI GetTickCount(void);
extern DWORD   WINAPI GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);

static pfn_PlaySoundW  s_pfnPlaySoundW  = NULL;
static pfn_MessageBeep s_pfnMessageBeep = NULL;
static HMODULE         s_hSoundModule   = NULL;
static WCHAR           s_audio_dir[MAX_PATH] = {0};
static BOOL            s_sound_inited   = FALSE;

static char            s_current_bgm[64] = "bg_music.wav";
static BOOL            s_bgm_active     = FALSE;
static BOOL            s_sfx_busy       = FALSE;
static DWORD           s_sfx_end_tick   = 0;

static BOOL Sound_FileExists(const WCHAR *path) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return TRUE;
    }
    return FALSE;
}

static void ascii_to_wide(WCHAR *dest, const char *src, int max_chars) {
    int i = 0;
    while (src[i] && i < max_chars - 1) {
        dest[i] = (WCHAR)src[i];
        i++;
    }
    dest[i] = 0;
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
    if (strstr(name, "win")) return 6500;
    return 1000;
}

void Sound_Init(void) {
    if (s_sound_inited) return;

    /* Bind PlaySoundW and MessageBeep from coredll.dll */
    s_hSoundModule = LoadLibraryW(L"coredll.dll");
    if (s_hSoundModule) {
        s_pfnPlaySoundW = (pfn_PlaySoundW)GetProcAddress(s_hSoundModule, L"PlaySoundW");
        s_pfnMessageBeep = (pfn_MessageBeep)GetProcAddress(s_hSoundModule, L"MessageBeep");
    }

    /* Locate AUDIO directory adjacent to executable */
    WCHAR exe_path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exe_path, MAX_PATH);

    int last_slash = -1;
    for (int i = 0; exe_path[i] != 0; i++) {
        if (exe_path[i] == L'\\' || exe_path[i] == L'/') {
            last_slash = i;
        }
    }

    if (last_slash >= 0) {
        for (int i = 0; i <= last_slash; i++) {
            s_audio_dir[i] = exe_path[i];
        }
        s_audio_dir[last_slash + 1] = 0;
    } else {
        s_audio_dir[0] = L'\\';
        s_audio_dir[1] = 0;
    }

    /* Append AUDIO\ */
    const WCHAR *audio_sub = L"AUDIO\\";
    int dir_len = (int)wcslen(s_audio_dir);
    for (int i = 0; audio_sub[i] != 0; i++) {
        s_audio_dir[dir_len + i] = audio_sub[i];
    }
    s_audio_dir[dir_len + (int)wcslen(audio_sub)] = 0;

    s_sound_inited = TRUE;
}

void Sound_PlaySFX(const char *name) {
    if (g_game.is_muted || !name) return;
    if (!s_sound_inited) Sound_Init();

    if (s_pfnPlaySoundW) {
        WCHAR wfilename[64];
        ascii_to_wide(wfilename, name, 64);

        WCHAR full_path[MAX_PATH];
        wcscpy(full_path, s_audio_dir);
        int plen = (int)wcslen(full_path);
        for (int i = 0; wfilename[i] != 0; i++) {
            full_path[plen + i] = wfilename[i];
        }
        full_path[plen + (int)wcslen(wfilename)] = 0;

        if (Sound_FileExists(full_path)) {
            s_sfx_busy = TRUE;
            s_sfx_end_tick = GetTickCount() + (DWORD)GetSoundDurationMs(name);
            s_pfnPlaySoundW(full_path, NULL, CE_SND_ASYNC | CE_SND_FILENAME | CE_SND_NODEFAULT);
            return;
        }
    }

    /* Fallback tactile audio click via MessageBeep */
    if (s_pfnMessageBeep) {
        s_pfnMessageBeep(0);
    }
}

void Sound_PlayBGM(const char *name) {
    if (g_game.is_muted) return;
    if (!s_sound_inited) Sound_Init();

    if (name && name[0]) {
        strncpy(s_current_bgm, name, sizeof(s_current_bgm) - 1);
        s_current_bgm[sizeof(s_current_bgm) - 1] = '\0';
    }

    if (s_sfx_busy && GetTickCount() < s_sfx_end_tick) {
        s_bgm_active = TRUE;
        return;
    }

    if (s_pfnPlaySoundW && s_current_bgm[0]) {
        WCHAR wfilename[64];
        ascii_to_wide(wfilename, s_current_bgm, 64);

        WCHAR full_path[MAX_PATH];
        wcscpy(full_path, s_audio_dir);
        int plen = (int)wcslen(full_path);
        for (int i = 0; wfilename[i] != 0; i++) {
            full_path[plen + i] = wfilename[i];
        }
        full_path[plen + (int)wcslen(wfilename)] = 0;

        if (Sound_FileExists(full_path)) {
            s_pfnPlaySoundW(full_path, NULL, CE_SND_ASYNC | CE_SND_FILENAME | CE_SND_LOOP | CE_SND_NODEFAULT);
            s_bgm_active = TRUE;
        }
    }
}

void Sound_CheckResumeBGM(void) {
    if (!s_bgm_active || g_game.is_muted) return;

    if (s_sfx_busy) {
        if (GetTickCount() >= s_sfx_end_tick) {
            s_sfx_busy = FALSE;
            if (s_pfnPlaySoundW && s_current_bgm[0]) {
                WCHAR wfilename[64];
                ascii_to_wide(wfilename, s_current_bgm, 64);

                WCHAR full_path[MAX_PATH];
                wcscpy(full_path, s_audio_dir);
                int plen = (int)wcslen(full_path);
                for (int i = 0; wfilename[i] != 0; i++) {
                    full_path[plen + i] = wfilename[i];
                }
                full_path[plen + (int)wcslen(wfilename)] = 0;

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

BOOL Sound_IsMuted(void) {
    return g_game.is_muted;
}

void Sound_Cleanup(void) {
    Sound_StopBGM();
    if (s_hSoundModule) {
        FreeLibrary(s_hSoundModule);
        s_hSoundModule = NULL;
    }
    s_pfnPlaySoundW = NULL;
    s_pfnMessageBeep = NULL;
}
