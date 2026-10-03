import os
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MAC_DIR = os.path.dirname(SCRIPT_DIR)
RELEASE_DIR = os.path.join(MAC_DIR, "release")
BUILD_DIR = os.path.join(MAC_DIR, "build")

def run_in_docker(cmd):
    full_cmd = [
        "wsl", "-d", "Debian", "-u", "root", "-e", "bash",
        "/mnt/c/Users/adam/code/ataxx/mac/tools/docker_run.sh",
        "bash", "-c", cmd
    ]
    res = subprocess.run(full_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Error running: {cmd}\nOutput: {res.stdout}\nStderr: {res.stderr}")
    return res.returncode == 0

print("--- Creating 1.44 MB Standard Floppy Disk Image (FTAGHN.IMG) ---")
cmd_floppy = """
set -e
dd if=/dev/zero of=/src/release/FTAGHN.IMG bs=1024 count=1440 status=none
hformat -l "FTAGHN" /src/release/FTAGHN.IMG
hmount /src/release/FTAGHN.IMG
hcopy -m /src/build/Ftaghn.bin :Ftaghn
hcopy -t /src/release/README.TXT :README.TXT
hattrib -t TEXT -c ttxt :README.TXT
echo "FTAGHN.IMG Contents:"
hls -l
humount /src/release/FTAGHN.IMG
"""
if run_in_docker(cmd_floppy):
    print(f"Created FTAGHN.IMG ({os.path.getsize(os.path.join(RELEASE_DIR, 'FTAGHN.IMG')):,} bytes)")

print("\n--- Creating 5 MB Hard Disk Image (FTAGHN_HD.IMG) ---")
cmd_hd = """
set -e
dd if=/dev/zero of=/src/release/FTAGHN_HD.IMG bs=1024 count=5120 status=none
hformat -l "FTAGHN HD" /src/release/FTAGHN_HD.IMG
hmount /src/release/FTAGHN_HD.IMG
hcopy -m /src/build/Ftaghn.bin :Ftaghn
hcopy -t /src/release/README.TXT :README.TXT
hattrib -t TEXT -c ttxt :README.TXT
echo "FTAGHN_HD.IMG Contents:"
hls -l
humount /src/release/FTAGHN_HD.IMG
"""
if run_in_docker(cmd_hd):
    print(f"Created FTAGHN_HD.IMG ({os.path.getsize(os.path.join(RELEASE_DIR, 'FTAGHN_HD.IMG')):,} bytes)")

