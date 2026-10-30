#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Package BEGONIA-XNU boot artifacts for temporary (fastboot boot) testing.
#
# Produces a directory suitable for `fastboot boot <kernel> <ramdisk>`:
#   package/bootshim/  -> boot shim Image (ARM64 Linux Image, "ARM\x64" magic)
#   package/xnu.img    -> XNU kernel image (placeholder until XNU source built)
#   package/boot.img   -> combined Android boot image (boot shim as kernel)
#   package/MANIFEST   -> artifact hashes and versions
#
# Rule 5: never modify critical boot partitions. This only creates files in
# package/ for temporary RAM-based testing.
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
pkg="$root/package"
rm -rf "$pkg"
mkdir -p "$pkg/bootshim"

echo "[1/3] Building boot shim..."
(cd "$root/bootshim" && ./build.sh)
cp "$root/bootshim/out/Image" "$pkg/bootshim/Image"
cp "$root/bootshim/out/build.json" "$pkg/bootshim/build.json"
cp "$root/bootshim/out/shim.elf" "$pkg/bootshim/shim.elf"

echo "[2/3] XNU placeholder..."
: > "$pkg/xnu.img"
echo "XNU 8796 placeholder (source not yet integrated)" > "$pkg/xnu.img.note"

echo "[3/3] Writing manifest..."
python3 - "$pkg" <<'PY'
import hashlib,json,os
from pathlib import Path
p=Path(sys.argv[1]) if False else Path(__import__('sys').argv[1])
def sha(pth):
    h=hashlib.sha256()
    with open(pth,'rb') as f:
        for c in iter(lambda:f.read(65536),b''): h.update(c)
    return h.hexdigest()
man={
 'project':'BEGONIA-XNU','target':'begonia/mt6785','phase':'pre-XNU-integration',
 'artifacts':[
   {'path':'bootshim/Image','kind':'boot-shim','sha256':sha(p/'bootshim'/'Image')},
   {'path':'bootshim/shim.elf','kind':'boot-shim-elf','sha256':sha(p/'bootshim'/'shim.elf')},
   {'path':'xnu.img','kind':'xnu-kernel-placeholder','sha256':sha(p/'xnu.img')},
 ],
 'hardware_tested':False,
 'xnu_boot_verified':False,
 'note':'fastboot boot -u <package>/bootshim/Image for temporary testing only'
}
(p/'MANIFEST').write_text(json.dumps(man,indent=2)+'\n')
print(json.dumps({'package':str(p),'manifest_sha256':hashlib.sha256((p/'MANIFEST').read_bytes()).hexdigest()}))
PY
echo ""
echo "PACKAGE: $pkg"
echo "  boot shim Image: $pkg/bootshim/Image"
echo "  manifest: $pkg/MANIFEST"
echo "  To test temporarily: fastboot boot $pkg/bootshim/Image"
