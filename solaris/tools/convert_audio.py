import os
import wave
import audioop

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SOLARIS_DIR = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(SOLARIS_DIR)
AUDIO_SRC = os.path.join(REPO_ROOT, "win95", "release", "AUDIO")
HEADER_OUT = os.path.join(SOLARIS_DIR, "src", "embedded_audio.h")

def wav_to_ulaw(wav_path):
    with wave.open(wav_path, 'rb') as w:
        nchannels = w.getnchannels()
        sampwidth = w.getsampwidth()
        framerate = w.getframerate()
        nframes = w.getnframes()
        raw = w.readframes(nframes)

    # Convert 8-bit unsigned PCM to 16-bit signed PCM
    if sampwidth == 1:
        raw_16 = audioop.bias(raw, 1, -128)
        raw_16 = audioop.lin2lin(raw_16, 1, 2)
    elif sampwidth == 2:
        raw_16 = raw
    else:
        print(f"Unsupported sample width: {sampwidth}")
        return None

    # Resample to 8000 Hz if needed
    if framerate != 8000:
        raw_8k, _ = audioop.ratecv(raw_16, 2, nchannels, framerate, 8000, None)
    else:
        raw_8k = raw_16

    # Convert to mono if stereo
    if nchannels > 1:
        raw_8k = audioop.tomono(raw_8k, 2, 0.5, 0.5)

    # Convert 16-bit linear to 8-bit u-law
    ulaw_data = audioop.lin2ulaw(raw_8k, 2)
    return ulaw_data

sound_files = [
    ("snd_select", os.path.join(AUDIO_SRC, "select.wav")),
    ("snd_clone", os.path.join(AUDIO_SRC, "place.wav")),
    ("snd_leap", os.path.join(AUDIO_SRC, "enemyplace.wav")),
    ("snd_capture", os.path.join(AUDIO_SRC, "big_capture.wav")),
    ("snd_big_capture", os.path.join(AUDIO_SRC, "player_big_capture.wav")),
    ("snd_win", os.path.join(AUDIO_SRC, "win.wav")),
    ("snd_whisper", os.path.join(AUDIO_SRC, "player_special.wav")),
    ("snd_anomaly", os.path.join(AUDIO_SRC, "anomaly.wav")),
    ("snd_tick", os.path.join(AUDIO_SRC, "tick.wav")),
]

with open(HEADER_OUT, "w") as f:
    f.write("/* Auto-generated Sun Solaris /dev/audio 8000 Hz u-law sound arrays */\n")
    f.write("#ifndef FTAGHN_EMBEDDED_AUDIO_H\n")
    f.write("#define FTAGHN_EMBEDDED_AUDIO_H\n\n")

    for var_name, wav_path in sound_files:
        if not os.path.exists(wav_path):
            print(f"Warning: {wav_path} not found")
            continue
        data = wav_to_ulaw(wav_path)
        if not data:
            continue
        f.write(f"/* {os.path.basename(wav_path)}: {len(data)} bytes @ 8kHz u-law */\n")
        f.write(f"static const unsigned char {var_name}[] = {{\n")
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"    {hex_vals},\n")
        f.write("};\n")
        f.write(f"static const unsigned int {var_name}_len = {len(data)};\n\n")
        print(f"Embedded {var_name}: {len(data)} bytes")

    f.write("#endif /* FTAGHN_EMBEDDED_AUDIO_H */\n")

print(f"\nWrote header: {HEADER_OUT}")

