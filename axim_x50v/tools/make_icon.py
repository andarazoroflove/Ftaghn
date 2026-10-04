#!/usr/bin/env python3
import os
from PIL import Image, ImageDraw

def create_ico(target_path):
    os.makedirs(os.path.dirname(target_path), exist_ok=True)

    sizes = [(64, 64), (48, 48), (32, 32), (16, 16)]
    images = []

    for w, h in sizes:
        img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)

        # Dark cosmic outer circle
        draw.ellipse([1, 1, w - 2, h - 2], fill=(20, 16, 30, 255), outline=(120, 60, 180, 255))

        # Glowing crimson horror orb
        m = max(2, w // 8)
        draw.ellipse([m, m, w - m - 1, h - m - 1], fill=(220, 30, 30, 255), outline=(255, 80, 80, 255))

        # Arcane gold pupil / slit
        pw = max(2, w // 8)
        ph = max(4, h // 2)
        cx = w // 2
        cy = h // 2
        draw.ellipse([cx - pw, cy - ph // 2, cx + pw, cy + ph // 2], fill=(255, 215, 0, 255))

        # Specular glint
        gw = max(2, w // 6)
        draw.ellipse([m + 2, m + 2, m + 2 + gw, m + 2 + gw], fill=(255, 255, 255, 255))

        images.append(img)

    images[0].save(target_path, format="ICO", sizes=[(im.width, im.height) for im in images])
    print(f"Created multi-res icon: {target_path}")

if __name__ == "__main__":
    create_ico("axim_x50v/rsc/ftaghn.ico")
