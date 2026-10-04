#!/usr/bin/env python3
import os
from PIL import Image, ImageDraw

def create_icons(rsc_dir):
    os.makedirs(rsc_dir, exist_ok=True)

    # 1. Large 8-bpp Color Icon (44x44)
    img_lg8 = Image.new("P", (44, 44), 0)
    # Palette: 0 is dark purple/black, 1 is crimson, 2 is bright red, 3 is gold, etc.
    palette = [0]*768
    palette[0:3] = [12, 10, 20]      # 0: BG dark void
    palette[3:6] = [180, 20, 20]     # 1: Crimson body
    palette[6:9] = [230, 45, 45]     # 2: Bright red
    palette[9:12] = [255, 215, 0]    # 3: Arcane gold
    palette[12:15] = [255, 255, 200] # 4: Specular highlight
    palette[15:18] = [70, 20, 90]    # 5: Tentacle shadow
    palette[18:21] = [140, 40, 180]  # 6: Cosmic purple
    img_lg8.putpalette(palette)

    draw_lg8 = ImageDraw.Draw(img_lg8)
    # Background circle
    draw_lg8.ellipse([2, 2, 41, 41], fill=5, outline=6)
    # Glowing eldritch eye / orb in center
    draw_lg8.ellipse([8, 8, 35, 35], fill=1, outline=2)
    # Inner slit / gold pupil
    draw_lg8.ellipse([18, 12, 25, 31], fill=3)
    draw_lg8.ellipse([20, 16, 23, 27], fill=0)
    # Specular glint
    draw_lg8.ellipse([11, 11, 16, 16], fill=4)
    img_lg8.save(os.path.join(rsc_dir, "icon_lg_8.bmp"), format="BMP")

    # 2. Large 1-bpp Monochrome Icon (22x22)
    img_lg1 = Image.new("1", (22, 22), 0)
    draw_lg1 = ImageDraw.Draw(img_lg1)
    draw_lg1.ellipse([1, 1, 20, 20], fill=1)
    draw_lg1.ellipse([3, 3, 18, 18], fill=0)
    draw_lg1.ellipse([9, 5, 12, 16], fill=1)
    img_lg1.save(os.path.join(rsc_dir, "icon_lg_1.bmp"), format="BMP")

    # 3. Small 8-bpp Color Icon (30x18)
    img_sm8 = Image.new("P", (30, 18), 0)
    img_sm8.putpalette(palette)
    draw_sm8 = ImageDraw.Draw(img_sm8)
    draw_sm8.ellipse([6, 1, 23, 16], fill=1, outline=2)
    draw_sm8.ellipse([13, 4, 16, 13], fill=3)
    draw_sm8.ellipse([8, 3, 11, 6], fill=4)
    img_sm8.save(os.path.join(rsc_dir, "icon_sm_8.bmp"), format="BMP")

    # 4. Small 1-bpp Monochrome Icon (15x9)
    img_sm1 = Image.new("1", (15, 9), 0)
    draw_sm1 = ImageDraw.Draw(img_sm1)
    draw_sm1.ellipse([3, 0, 11, 8], fill=1)
    draw_sm1.ellipse([6, 2, 8, 6], fill=0)
    img_sm1.save(os.path.join(rsc_dir, "icon_sm_1.bmp"), format="BMP")

    print("Icons successfully created in:", rsc_dir)

if __name__ == "__main__":
    create_icons("palmos/rsc")
