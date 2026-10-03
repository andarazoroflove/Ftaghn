import os
import sys
import subprocess
from PIL import Image

# Locate ffmpeg
ffmpeg_path = "ffmpeg"
try:
    subprocess.run([ffmpeg_path, "-version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
except Exception:
    # Try finding in User / Winget path
    import glob
    candidates = glob.glob(r"C:\Users\adam\AppData\Local\Microsoft\WinGet\Packages\Gyan.FFmpeg*\*\bin\ffmpeg.exe")
    if candidates:
        ffmpeg_path = candidates[0]
        print(f"Found ffmpeg at: {ffmpeg_path}")
    else:
        print("ERROR: ffmpeg not found!")
        sys.exit(1)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
WIN95_DIR = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.dirname(WIN95_DIR)

AUDIO_SRC = os.path.join(REPO_ROOT, "public", "audio")
AUDIO_DST = os.path.join(WIN95_DIR, "release", "AUDIO")
IMAGES_SRC = os.path.join(REPO_ROOT, "public", "images")
IMAGES_DST = os.path.join(WIN95_DIR, "release", "IMAGES")
BG_SRC = os.path.join(REPO_ROOT, "public", "images", "backgrounds")
BG_DST = os.path.join(WIN95_DIR, "release", "IMAGES", "BG")

os.makedirs(AUDIO_DST, exist_ok=True)
os.makedirs(IMAGES_DST, exist_ok=True)
os.makedirs(BG_DST, exist_ok=True)

# 1. Convert Audio files
audio_mapping = {
    "select.mp4": "select.wav",
    "place.mp4": "place.wav",
    "enemyplace.mp4": "enemyplace.wav",
    "win.mp4": "win.wav",
    "big_capture.mp4": "big_capture.wav",
    "player_big_capture.mp4": "player_big_capture.wav",
    "carcosa_whisper.mp3": "player_special.wav",
    "unseen_anomaly.mp3": "anomaly.wav",
    "timer_tick.mp3": "tick.wav",
    "gwb_theme.mp3": "gwb_theme.wav",
    "rhan.mp3": "rhan.wav",
    "bigmachine.mp3": "doktor.wav",
    "dillinger.mp3": "klf_dillinger.wav",
    "mumu.mp3": "klf_mumu.wav",
    "xfiles.mp3": "xfiles.wav",
    "reality.mp3": "tkk.wav",
    "carryon.mp3": "supernatural.wav",
    "happiness.mp3": "nin.wav",
    "hohoho.mp4": "santa.wav",
    "cosmic-horror-loop.mp3": "bg_music.wav"
}

print("--- Converting Audio to 8-bit PCM WAV (11025Hz Mono) ---")
for src_name, dst_name in audio_mapping.items():
    src_file = os.path.join(AUDIO_SRC, src_name)
    dst_file = os.path.join(AUDIO_DST, dst_name)
    if not os.path.exists(src_file):
        print(f"Warning: Audio file not found: {src_file}")
        continue
    
    # 8-bit unsigned PCM, 11025Hz, Mono
    cmd = [
        ffmpeg_path, "-y", "-i", src_file,
        "-ac", "1",
        "-ar", "11025",
        "-acodec", "pcm_u8",
        dst_file
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if res.returncode == 0:
        size_kb = os.path.getsize(dst_file) / 1024
        print(f" Converted: {src_name} -> {dst_name} ({size_kb:.1f} KB)")
    else:
        print(f" Error converting {src_name}: {res.stderr.decode('utf-8', errors='ignore')[:200]}")

print("\n--- Converting Images to 8-bit Paletted BMP ---")

def convert_to_8bit_bmp(src_path, dst_path, target_size=None):
    try:
        im = Image.open(src_path)
        # Handle RGBA transparent background by compositing over dark Lovecraftian slate #1a1a2e
        if im.mode in ('RGBA', 'LA') or (im.mode == 'P' and 'transparency' in im.info):
            im = im.convert('RGBA')
            bg = Image.new('RGB', im.size, (26, 26, 46))
            bg.paste(im, mask=im.split()[3])
            im = bg
        else:
            im = im.convert('RGB')
        
        if target_size:
            im = im.resize(target_size, Image.Resampling.LANCZOS)
        
        # Quantize to 256 colors
        im_8bit = im.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
        im_8bit.save(dst_path, format="BMP")
        return True
    except Exception as e:
        print(f" Error converting {src_path}: {e}")
        return False

# Convert Logo
logo_src = os.path.join(IMAGES_SRC, "ftaghn_logo.png")
logo_dst = os.path.join(IMAGES_DST, "logo.bmp")
if os.path.exists(logo_src):
    # Resize logo to fit nicely in 640 width (e.g. 300x75)
    convert_to_8bit_bmp(logo_src, logo_dst, target_size=(300, 75))
    print(f" Converted: ftaghn_logo.png -> logo.bmp ({os.path.getsize(logo_dst)/1024:.1f} KB)")

# Convert Runes texture
runes_src = os.path.join(IMAGES_SRC, "background_runes.png")
runes_dst = os.path.join(IMAGES_DST, "runes.bmp")
if os.path.exists(runes_src):
    convert_to_8bit_bmp(runes_src, runes_dst, target_size=(128, 128))
    print(f" Converted: background_runes.png -> runes.bmp ({os.path.getsize(runes_dst)/1024:.1f} KB)")

# Convert GWB dot overlay
gwb_src = os.path.join(IMAGES_SRC, "dots", "gwb.png")
gwb_dst = os.path.join(IMAGES_DST, "gwb_dot.bmp")
if os.path.exists(gwb_src):
    convert_to_8bit_bmp(gwb_src, gwb_dst, target_size=(32, 32))
    print(f" Converted: gwb.png -> gwb_dot.bmp ({os.path.getsize(gwb_dst)/1024:.1f} KB)")

# Convert Santa BG
santa_src = os.path.join(IMAGES_SRC, "bg_santa.png")
santa_dst = os.path.join(BG_DST, "bg_santa.bmp")
if os.path.exists(santa_src):
    convert_to_8bit_bmp(santa_src, santa_dst, target_size=(320, 240))
    print(f" Converted: bg_santa.png -> BG/bg_santa.bmp ({os.path.getsize(santa_dst)/1024:.1f} KB)")

# Convert all backgrounds
if os.path.exists(BG_SRC):
    for fname in os.listdir(BG_SRC):
        if fname.lower().endswith(".png") and not fname.startswith("._"):
            b_src = os.path.join(BG_SRC, fname)
            b_dst = os.path.join(BG_DST, os.path.splitext(fname)[0] + ".bmp")
            if convert_to_8bit_bmp(b_src, b_dst, target_size=(320, 240)):
                print(f" Converted BG: {fname} -> {os.path.basename(b_dst)} ({os.path.getsize(b_dst)/1024:.1f} KB)")


print("\nAsset conversion complete!")
