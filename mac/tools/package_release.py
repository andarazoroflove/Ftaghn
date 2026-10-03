import os
import shutil

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MAC_DIR = os.path.dirname(SCRIPT_DIR)
BUILD_DIR = os.path.join(MAC_DIR, "build")
RELEASE_DIR = os.path.join(MAC_DIR, "release")

os.makedirs(RELEASE_DIR, exist_ok=True)

# 1. Copy Classic Mac OS artifacts
src_dsk = os.path.join(BUILD_DIR, "Ftaghn.dsk")
dst_dsk = os.path.join(RELEASE_DIR, "FTAGHN.DSK")
if os.path.exists(src_dsk):
    shutil.copyfile(src_dsk, dst_dsk)
    print(f"Copied {os.path.basename(dst_dsk)} ({os.path.getsize(dst_dsk):,} bytes)")

src_bin = os.path.join(BUILD_DIR, "Ftaghn.bin")
dst_bin = os.path.join(RELEASE_DIR, "FTAGHN.BIN")
if os.path.exists(src_bin):
    shutil.copyfile(src_bin, dst_bin)
    print(f"Copied {os.path.basename(dst_bin)} ({os.path.getsize(dst_bin):,} bytes)")

src_appl = os.path.join(BUILD_DIR, "Ftaghn.APPL")
dst_appl = os.path.join(RELEASE_DIR, "FTAGHN.APPL")
if os.path.exists(src_appl):
    shutil.copyfile(src_appl, dst_appl)
    print(f"Copied {os.path.basename(dst_appl)} ({os.path.getsize(dst_appl):,} bytes)")

# Copy AppleDouble resource files if present
src_ad = os.path.join(BUILD_DIR, "%Ftaghn.ad")
dst_ad = os.path.join(RELEASE_DIR, "%FTAGHN.ad")
if os.path.exists(src_ad):
    shutil.copyfile(src_ad, dst_ad)

# 2. Construct Mac OS X Application Bundle (.app)
bundle_dir = os.path.join(RELEASE_DIR, "Ftaghn.app")
contents_dir = os.path.join(bundle_dir, "Contents")
macos_dir = os.path.join(contents_dir, "MacOS")
resources_dir = os.path.join(contents_dir, "Resources")

os.makedirs(macos_dir, exist_ok=True)
os.makedirs(resources_dir, exist_ok=True)

# Copy PEF binary to MacOS/Ftaghn
src_pef = os.path.join(BUILD_DIR, "Ftaghn.pef")
dst_pef = os.path.join(macos_dir, "Ftaghn")
if os.path.exists(src_pef):
    shutil.copyfile(src_pef, dst_pef)
    os.chmod(dst_pef, 0o755)
    print(f"Bundled executable: {dst_pef}")

# Copy resource fork to Resources/Ftaghn.rsrc
src_rsrc = os.path.join(BUILD_DIR, "Ftaghn.r.rsrc.bin")
dst_rsrc = os.path.join(resources_dir, "Ftaghn.rsrc")
if os.path.exists(src_rsrc):
    shutil.copyfile(src_rsrc, dst_rsrc)
    print(f"Bundled resources: {dst_rsrc}")

# Write PkgInfo (APPLFTAG)
pkginfo_path = os.path.join(contents_dir, "PkgInfo")
with open(pkginfo_path, "wb") as f:
    f.write(b"APPLFTAG")

# Write Info.plist
plist_path = os.path.join(contents_dir, "Info.plist")
plist_content = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>Ftaghn</string>
	<key>CFBundleGetInfoString</key>
	<string>Ftaghn: Cosmic Horror Ataxx (Carbon Retro Edition)</string>
	<key>CFBundleIconFile</key>
	<string>Ftaghn</string>
	<key>CFBundleIdentifier</key>
	<string>com.retro.ftaghn</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>Ftaghn</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleSignature</key>
	<string>FTAG</string>
	<key>CFBundleVersion</key>
	<string>1.0</string>
	<key>CSResourcesFileMapped</key>
	<true/>
</dict>
</plist>
"""

with open(plist_path, "w", encoding="utf-8") as f:
    f.write(plist_content)

print(f"Generated Mac OS X bundle at: {bundle_dir}")

