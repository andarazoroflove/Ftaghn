#include "sound.h"
#include "embedded_audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

static int s_sound_enabled = 1;
static int s_audio_pipe[2] = { -1, -1 };
static pid_t s_audio_pid = -1;

void Sound_Init(void) {
    s_sound_enabled = 1;
    if (pipe(s_audio_pipe) == 0) {
        s_audio_pid = fork();
        if (s_audio_pid == 0) {
            /* Child audio player process */
            close(s_audio_pipe[1]);
            int audio_fd = open("/dev/audio", O_WRONLY);
            short id = 0;
            while (read(s_audio_pipe[0], &id, sizeof(short)) > 0) {
                if (audio_fd < 0) {
                    audio_fd = open("/dev/audio", O_WRONLY);
                }
                if (audio_fd >= 0) {
                    const unsigned char *data = NULL;
                    unsigned int len = 0;
                    switch (id) {
                        case SND_ID_SELECT:      data = snd_select; len = snd_select_len; break;
                        case SND_ID_CLONE:       data = snd_clone; len = snd_clone_len; break;
                        case SND_ID_LEAP:        data = snd_leap; len = snd_leap_len; break;
                        case SND_ID_CAPTURE:     data = snd_capture; len = snd_capture_len; break;
                        case SND_ID_BIG_CAPTURE: data = snd_big_capture; len = snd_big_capture_len; break;
                        case SND_ID_WIN:         data = snd_win; len = snd_win_len; break;
                        case SND_ID_WHISPER:     data = snd_whisper; len = snd_whisper_len; break;
                        case SND_ID_ANOMALY:     data = snd_anomaly; len = snd_anomaly_len; break;
                        case SND_ID_TICK:        data = snd_tick; len = snd_tick_len; break;
                    }
                    if (data && len > 0) {
                        write(audio_fd, data, len);
                    }
                }
            }
            if (audio_fd >= 0) close(audio_fd);
            close(s_audio_pipe[0]);
            _exit(0);
        } else if (s_audio_pid > 0) {
            /* Parent process */
            close(s_audio_pipe[0]);
        }
    }
}

void Sound_Play(short sfx_id) {
    if (!s_sound_enabled || s_audio_pipe[1] < 0) return;
    write(s_audio_pipe[1], &sfx_id, sizeof(short));
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

int Sound_IsEnabled(void) {
    return s_sound_enabled;
}

void Sound_Cleanup(void) {
    if (s_audio_pipe[1] >= 0) {
        close(s_audio_pipe[1]);
        s_audio_pipe[1] = -1;
    }
    if (s_audio_pid > 0) {
        kill(s_audio_pid, SIGTERM);
        waitpid(s_audio_pid, NULL, WNOHANG);
        s_audio_pid = -1;
    }
}

