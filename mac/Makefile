# Ftaghn (Cosmic Horror Ataxx) - Carbon Classic Mac OS 9 & OS X Edition
# Built using Retro68 GCC 16.1.0 (PowerPC Carbon CFM)

.PHONY: all build sounds package clean

all: build package

sounds:
	python tools/wav_to_snd.py

build:
	wsl -d Debian -u root -e bash /mnt/c/Users/adam/code/ataxx/mac/tools/docker_run.sh bash -c "mkdir -p /src/build && cd /src/build && cmake .. --toolchain /Retro68-build/toolchain/powerpc-apple-macos/cmake/retrocarbon.toolchain.cmake && make"

package:
	python tools/package_release.py
	python tools/create_disk_images.py

clean:
	rm -rf build release/Ftaghn.app release/*.DSK release/*.BIN release/*.APPL release/*.IMG release/*.ad

