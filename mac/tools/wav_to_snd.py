import os
import struct
import wave

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MAC_DIR = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(MAC_DIR)
AUDIO_DIR = os.path.join(REPO_ROOT, "win95", "release", "AUDIO")
RSRC_DIR = os.path.join(MAC_DIR, "rsrc")
SND_R_FILE = os.path.join(RSRC_DIR, "sounds.r")

def wav_to_snd_bytes(wav_path):
    with wave.open(wav_path, 'rb') as w:
        nchannels = w.getnchannels()
        sampwidth = w.getsampwidth()
        framerate = w.getframerate()
        nframes = w.getnframes()
        raw_data = w.readframes(nframes)
    
    if sampwidth != 1 or nchannels != 1:
        print(f"Warning: {wav_path} is {nchannels}ch, {sampwidth}B. Expected 1ch, 1B (8-bit mono). Skipping.")
        return None
    
    # Calculate sample rate in 16.16 fixed point
    rate_fixed = int(framerate) << 16
    
    # Format 1 Snd Resource Header
    header1 = struct.pack(">HHHIHHHI", 1, 1, 5, 0x00000080, 1, 0x8051, 0, 20)
    
    # Standard Sound Header
    sound_header = struct.pack(">IIIIIbb", 0, nframes, rate_fixed, 0, 0, 0, 60)
    
    blob = header1 + sound_header + raw_data
    if (len(blob) % 2) != 0:
        blob += b'\x80'
    return blob

sound_defs = [
    (1001, "Select", os.path.join(AUDIO_DIR, "select.wav")),
    (1002, "Clone", os.path.join(AUDIO_DIR, "place.wav")),
    (1003, "Leap", os.path.join(AUDIO_DIR, "enemyplace.wav")),
    (1004, "Capture", os.path.join(AUDIO_DIR, "big_capture.wav")),
    (1005, "BigCapture", os.path.join(AUDIO_DIR, "player_big_capture.wav")),
    (1006, "Win", os.path.join(AUDIO_DIR, "win.wav")),
    (1007, "Whisper", os.path.join(AUDIO_DIR, "player_special.wav")),
    (1008, "Anomaly", os.path.join(AUDIO_DIR, "anomaly.wav")),
    (1009, "Tick", os.path.join(AUDIO_DIR, "tick.wav")),
]

with open(SND_R_FILE, "w") as f:
    f.write("/* Auto-generated Macintosh 'snd ' resources */\n\n")
    for res_id, res_name, wav_path in sound_defs:
        if not os.path.exists(wav_path):
            print(f"Missing audio: {wav_path}")
            continue
        blob = wav_to_snd_bytes(wav_path)
        if not blob:
            continue
        f.write(f'data \'snd \' ({res_id}, "{res_name}") {{\n')
        # Write hex lines of 32 bytes (64 hex chars)
        for i in range(0, len(blob), 32):
            chunk = blob[i:i+32]
            hex_str = chunk.hex().upper()
            spaced_hex = " ".join(hex_str[j:j+4] for j in range(0, len(hex_str), 4))
            f.write(f'    $"{spaced_hex}"\n')
        f.write("};\n\n")
        print(f"Generated 'snd ' {res_id} ({res_name}): {len(blob)} bytes")

print(f"\nWrote all 'snd ' resources to {SND_R_FILE}")

