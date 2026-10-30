#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build the Begonia (MT6785) XNU boot shim.
# Outputs: bootshim/out/shim.elf, bootshim/out/Image (raw binary), bootshim/out/build.json
set -euo pipefail
src=$(cd "$(dirname "$0")" && pwd)
out=${SHIM_OUT:-$src/out}
prefix=${CROSS_COMPILE:-aarch64-linux-gnu-}
cpu=${SHIM_CPU:-cortex-a55}
mkdir -p "$out"
flags=(-O2 -ffreestanding -nostdlib -fno-pic -mgeneral-regs-only -mstrict-align
       -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables
       -mcpu="$cpu" -Wall -Wextra -Werror -I"$src")
echo "[bootshim] prefix=$prefix cpu=$cpu out=$out"
"${prefix}gcc" "${flags[@]}" -c "$src/start.S"   -o "$out/start.o"
"${prefix}gcc" "${flags[@]}" -c "$src/shim.c"    -o "$out/shim.o"
"${prefix}gcc" "${flags[@]}" -c "$src/fdt.c"     -o "$out/fdt.o"
"${prefix}gcc" "${flags[@]}" -c "$src/adt.c"     -o "$out/adt.o"
"${prefix}gcc" "${flags[@]}" -c "$src/handoff.c" -o "$out/handoff.o"
# Reject absolute relocations (shim is position-independent / relocated).
if "${prefix}readelf" -r "$out/start.o" "$out/shim.o" "$out/fdt.o" "$out/adt.o" "$out/handoff.o" \
   2>/dev/null | grep -q 'R_AARCH64_ABS'; then
    echo 'Absolute C relocation is incompatible with a relocated shim' >&2; exit 1
fi
"${prefix}gcc" "${flags[@]}" -static -T "$src/shim.ld" -Wl,--build-id=none -Wl,-N \
    "$out/start.o" "$out/shim.o" "$out/fdt.o" "$out/adt.o" "$out/handoff.o" -o "$out/shim.elf"
"${prefix}objcopy" -O binary "$out/shim.elf" "$out/Image"
"${prefix}nm" "$out/shim.elf" > "$out/symbols.txt"
python3 - "$out" <<'PY'
from pathlib import Path
import hashlib, json, struct, sys
p=Path(sys.argv[1]); syms={}
for line in (p/'symbols.txt').read_text().splitlines():
    parts=line.split()
    if len(parts)==3: syms[parts[2]]=int(parts[0],16)
image=p/'Image'
with image.open('r+b') as f: f.truncate(syms.get('_end', len(image.read_bytes())))
b=image.read_bytes()
assert syms.get('_bss_end',0) <= syms.get('_stack_top',len(b)) == len(b), (syms, len(b))
import os
build={'kind':'begonia-xnu-shim','cpu':os.environ.get('SHIM_CPU','cortex-a55'),
       'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),
       'xnu_boot_verified':False,'hardware_tested':False}
(p/'build.json').write_text(json.dumps(build,indent=2)+'\n')
print(json.dumps({'image':str(image),'bytes':len(b),'hardware_tested':False}))
PY
