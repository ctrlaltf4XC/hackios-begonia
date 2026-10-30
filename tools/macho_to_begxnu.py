#!/usr/bin/env python3
"""
macho_to_begxnu.py — Host-side Mach-O 64-bit → BEGXNU01 payload packager.

Derives the BEGXNU01 payload format exclusively from bootshim/handoff.h and
bootshim/handoff.c (the BEGONIA-XNU boot shim source of truth). No payload
fields, offsets, alignment rules, or validation logic are invented.

Usage:
    macho_to_begxnu.py <input.macho> <output.begxnu> [options]

Options:
    --phys-base ADDR    Physical load address (default: 0x40000000)
    --cmdline STR       Boot command line (default: empty)
    --version N         Payload version (default: 1)
    --sha256 PATH       Also write SHA256 digest to PATH
"""

import argparse
import hashlib
import os
import struct
import sys

# ---------------------------------------------------------------------------
# BEGXNU01 payload ABI — from bootshim/handoff.h
# ---------------------------------------------------------------------------

BEGXNU_MAGIC = b"BEGXNU01"
XNU_PAGE = 0x4000          # 16K pages
XNU_CMDLINE = 608
XNU_MAX_SEGMENTS = 16
PAYLOAD_HEADER_SIZE = 688  # _Static_assert(sizeof(struct xnu_payload) == 688)
SEGMENT_SIZE = 32          # sizeof(struct xnu_segment)

# ---------------------------------------------------------------------------
# Mach-O 64-bit constants
# ---------------------------------------------------------------------------

MH_MAGIC_64_BE = 0xfeedfacf   # bytes: fe ed fa cf  (big-endian)
MH_CIGAM_64_LE = 0xcffaedfe   # bytes: cf fa ed fe  (little-endian, ARM64)
CPU_TYPE_ARM64 = 0x01000007
CPU_SUBTYPE_ARM64_ALL = 0x00000008
LC_SEGMENT_64 = 0x19

MACH_HEADER_64_SIZE = 32
SEGMENT_COMMAND_64_SIZE = 72   # minimum, nsects == 0

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def die(msg):
    print(f"macho_to_begxnu: error: {msg}", file=sys.stderr)
    sys.exit(1)


def aligned(value):
    """bootshim aligned(): round up to XNU_PAGE (16K)."""
    return (value + XNU_PAGE - 1) & ~(XNU_PAGE - 1)


def align8(value):
    return (value + 7) & ~7


def u32(data, off, little_endian):
    fmt = "<I" if little_endian else ">I"
    return struct.unpack_from(fmt, data, off)[0]


def u64(data, off, little_endian):
    fmt = "<Q" if little_endian else ">Q"
    return struct.unpack_from(fmt, data, off)[0]


def check_overflow_add(a, b, what):
    r = a + b
    if r < a or r < b:
        die(f"integer overflow computing {what}: 0x{a:x} + 0x{b:x}")
    return r


def check_overflow_mul(a, b, what):
    if a == 0 or b == 0:
        return 0
    r = a * b
    if r // a != b:
        die(f"integer overflow computing {what}: 0x{a:x} * 0x{b:x}")
    return r


# ---------------------------------------------------------------------------
# Mach-O parsing
# ---------------------------------------------------------------------------

def parse_macho(path):
    with open(path, "rb") as f:
        data = f.read()

    size = len(data)
    if size < MACH_HEADER_64_SIZE:
        die(f"input too small for mach_header_64 ({size} bytes)")

    # Detect endianness from magic bytes
    magic_bytes = data[0:4]
    if magic_bytes == b"\xfe\xed\xfa\xcf":
        little_endian = False
        magic = MH_MAGIC_64_BE
    elif magic_bytes == b"\xcf\xfa\xed\xfe":
        little_endian = True
        magic = MH_CIGAM_64_LE
    else:
        die(f"unsupported Mach-O magic: {magic_bytes.hex()} (expected 0xfeedfacf or 0xcffaedfe)")

    cputype = u32(data, 4, little_endian)
    cpusubtype = u32(data, 8, little_endian)

    if cputype != CPU_TYPE_ARM64:
        die(f"unsupported CPU type: 0x{cputype:08x} (expected 0x{CPU_TYPE_ARM64:08x} ARM64)")
    if cpusubtype != CPU_SUBTYPE_ARM64_ALL:
        die(f"unsupported CPU subtype: 0x{cpusubtype:08x} (expected 0x{CPU_SUBTYPE_ARM64_ALL:08x})")

    filetype = u32(data, 12, little_endian)
    ncmds = u32(data, 16, little_endian)
    sizeofcmds = u32(data, 20, little_endian)
    flags = u32(data, 24, little_endian)
    reserved = u32(data, 28, little_endian)

    # Sanity-check load command region
    if sizeofcmds < ncmds * 8:
        die(f"sizeofcmds 0x{sizeofcmds:x} smaller than ncmds * 8")
    cmd_region_end = MACH_HEADER_64_SIZE + sizeofcmds
    if cmd_region_end > size:
        die(f"load commands extend beyond file (end 0x{cmd_region_end:x} > file 0x{size:x})")

    # Parse load commands, collect LC_SEGMENT_64
    segments = []
    off = MACH_HEADER_64_SIZE
    for i in range(ncmds):
        if off + 8 > cmd_region_end:
            die(f"truncated load command {i}")
        cmd = u32(data, off, little_endian)
        cmdsize = u32(data, off + 4, little_endian)
        if cmdsize < 8:
            die(f"load command {i} has invalid cmdsize {cmdsize}")
        if off + cmdsize > cmd_region_end:
            die(f"load command {i} extends beyond load command region")

        if cmd == LC_SEGMENT_64:
            if cmdsize < SEGMENT_COMMAND_64_SIZE:
                die(f"segment command {i} too small ({cmdsize} < {SEGMENT_COMMAND_64_SIZE})")
            segname = data[off + 8:off + 24].split(b"\x00")[0].decode("ascii", errors="replace")
            vmaddr = u64(data, off + 24, little_endian)
            vmsize = u64(data, off + 32, little_endian)
            fileoff = u64(data, off + 40, little_endian)
            filesize = u64(data, off + 48, little_endian)

            # Validate segment range within file
            if fileoff > size:
                die(f"segment '{segname}': file_offset 0x{fileoff:x} beyond file size 0x{size:x}")
            if filesize > size - fileoff:
                die(f"segment '{segname}': file_size 0x{filesize:x} extends beyond file")
            if filesize > vmsize:
                die(f"segment '{segname}': file_size 0x{filesize:x} > vmsize 0x{vmsize:x}")
            if vmsize == 0:
                die(f"segment '{segname}': zero vmsize")

            segments.append({
                "segname": segname,
                "vmaddr": vmaddr,
                "vmsize": vmsize,
                "fileoff": fileoff,
                "filesize": filesize,
            })

        off += cmdsize

    if not segments:
        die("no LC_SEGMENT_64 load commands found")

    # Sort segments by vmaddr for deterministic layout
    segments.sort(key=lambda s: s["vmaddr"])

    return {
        "little_endian": little_endian,
        "magic": magic,
        "filetype": filetype,
        "ncmds": ncmds,
        "sizeofcmds": sizeofcmds,
        "flags": flags,
        "segments": segments,
    }, data


# ---------------------------------------------------------------------------
# BEGXNU01 payload construction
# ---------------------------------------------------------------------------

def build_payload(macho, phys_base, cmdline, version):
    segments = macho["segments"]
    seg_count = len(segments)

    if seg_count < 1 or seg_count > XNU_MAX_SEGMENTS:
        die(f"segment count {seg_count} outside [1, {XNU_MAX_SEGMENTS}]")

    # virt_base = lowest vmaddr among segments (kernel high VA)
    virt_base = segments[0]["vmaddr"]
    if virt_base < 0xffffff8000000000:
        die(f"virt_base 0x{virt_base:x} below ARM64 kernel floor 0xffffff8000000000")

    # phys_base must be 32MB-aligned per bootshim rule (phys_base & 0x1ffffff == 0)
    if phys_base & 0x1ffffff:
        die(f"phys_base 0x{phys_base:x} not 32MB-aligned (0x1ffffff mask)")
    if phys_base & (XNU_PAGE - 1):
        die(f"phys_base 0x{phys_base:x} not 16K page-aligned")

    # Compute per-segment physical addresses and check for overflow
    for s in segments:
        delta = s["vmaddr"] - virt_base
        s["physical"] = check_overflow_add(phys_base, delta, "segment physical address")
        if s["physical"] < phys_base:
            die(f"segment '{s['segname']}': physical 0x{s['physical']:x} below phys_base")

    # Lay out segment data sequentially, 8-aligned
    data_offset = align8(PAYLOAD_HEADER_SIZE + seg_count * SEGMENT_SIZE)
    cursor = data_offset
    for s in segments:
        s["file_offset"] = cursor
        cursor = align8(cursor + s["filesize"])

    total_size = cursor

    # end = max(physical + memory_size) across all segments
    end = phys_base
    for s in segments:
        seg_end = check_overflow_add(s["physical"], s["vmsize"], "segment end")
        if seg_end > end:
            end = seg_end

    kernel_end_pa = aligned(end)

    # mem_size must cover [phys_base, kernel_end_pa) plus the ADT +
    # boot_args region (2 pages after kernel_end_pa), per handoff_validate
    # rule: inside(adt_pa, 2 * XNU_PAGE, phys_base, mem_size)
    mem_size = aligned((kernel_end_pa - phys_base) + 2 * XNU_PAGE)
    if mem_size == 0:
        die("zero mem_size after layout")
    if mem_size > 0x60000000:
        die(f"mem_size 0x{mem_size:x} exceeds 0x60000000 (1.5GB)")

    # entry_va = virt_base (entry at start of first segment, XNU convention)
    entry_va = virt_base
    if entry_va < virt_base or entry_va - virt_base >= mem_size:
        die("entry_va outside [virt_base, virt_base + mem_size)")

    # entry_found: entry must be inside exactly one segment's file range
    entry_pa = check_overflow_add(phys_base, entry_va - virt_base, "entry physical address")
    entry_found = 0
    for s in segments:
        if s["physical"] <= entry_pa < s["physical"] + s["filesize"]:
            entry_found += 1
    if entry_found != 1:
        die(f"entry PA 0x{entry_pa:x} found in {entry_found} segments (expected exactly 1)")

    # kernel_end_pa must equal aligned(end)
    if kernel_end_pa != aligned(end):
        die("kernel_end_pa mismatch (internal)")

    # ADT + args region must fit inside [phys_base, phys_base + mem_size)
    adt_pa = kernel_end_pa
    args_pa = adt_pa + XNU_PAGE
    top = args_pa + XNU_PAGE
    if top > phys_base + mem_size:
        die("ADT + boot_args region exceeds [phys_base, phys_base + mem_size)")

    # Encode command line (must be null-terminated within XNU_CMDLINE)
    cmdline_bytes = cmdline.encode("utf-8", errors="replace")
    if len(cmdline_bytes) >= XNU_CMDLINE:
        die(f"command line too long ({len(cmdline_bytes)} >= {XNU_CMDLINE})")
    command_line = bytearray(XNU_CMDLINE)
    command_line[:len(cmdline_bytes)] = cmdline_bytes

    # Build payload header
    header = bytearray(PAYLOAD_HEADER_SIZE)
    header[0:8] = BEGXNU_MAGIC
    struct.pack_into("<I", header, 8, version)           # version
    struct.pack_into("<I", header, 12, seg_count)        # segment_count
    struct.pack_into("<Q", header, 16, virt_base)        # virt_base
    struct.pack_into("<Q", header, 24, phys_base)        # phys_base
    struct.pack_into("<Q", header, 32, mem_size)         # mem_size
    struct.pack_into("<Q", header, 40, entry_va)         # entry_va
    struct.pack_into("<Q", header, 48, kernel_end_pa)    # kernel_end_pa
    struct.pack_into("<Q", header, 56, PAYLOAD_HEADER_SIZE)  # segments_offset
    struct.pack_into("<Q", header, 64, data_offset)      # data_offset
    struct.pack_into("<Q", header, 72, total_size)       # total_size
    header[80:80 + XNU_CMDLINE] = command_line

    # Build segment array
    segment_array = bytearray(seg_count * SEGMENT_SIZE)
    for i, s in enumerate(segments):
        off = i * SEGMENT_SIZE
        struct.pack_into("<Q", segment_array, off + 0, s["physical"])
        struct.pack_into("<Q", segment_array, off + 8, s["vmsize"])      # memory_size
        struct.pack_into("<Q", segment_array, off + 16, s["file_offset"])
        struct.pack_into("<Q", segment_array, off + 24, s["filesize"])

    # Assemble payload: header + segment array + segment data (placeholders)
    payload = bytearray(total_size)
    payload[0:PAYLOAD_HEADER_SIZE] = header
    payload[PAYLOAD_HEADER_SIZE:PAYLOAD_HEADER_SIZE + seg_count * SEGMENT_SIZE] = segment_array

    # Copy segment data from Mach-O file into payload at file_offset
    with open(macho["_path"], "rb") as f:
        macho_data = f.read()
    for s in segments:
        payload[s["file_offset"]:s["file_offset"] + s["filesize"]] = \
            macho_data[s["fileoff"]:s["fileoff"] + s["filesize"]]

    return bytes(payload), {
        "virt_base": virt_base,
        "phys_base": phys_base,
        "mem_size": mem_size,
        "entry_va": entry_va,
        "entry_pa": entry_pa,
        "kernel_end_pa": kernel_end_pa,
        "segment_count": seg_count,
        "total_size": total_size,
        "segments": segments,
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description="Pack a Mach-O 64-bit ARM64 kernel into a BEGXNU01 payload")
    ap.add_argument("input", help="Input Mach-O file")
    ap.add_argument("output", help="Output BEGXNU01 payload file")
    ap.add_argument("--phys-base", type=lambda x: int(x, 0), default=0x40000000,
                    help="Physical load address (default: 0x40000000)")
    ap.add_argument("--cmdline", default="",
                    help="Boot command line (default: empty)")
    ap.add_argument("--version", type=int, default=1,
                    help="Payload version (default: 1)")
    ap.add_argument("--sha256", help="Write SHA256 digest to this file")
    args = ap.parse_args()

    if not os.path.isfile(args.input):
        die(f"input file not found: {args.input}")

    macho, _ = parse_macho(args.input)
    macho["_path"] = args.input

    payload, info = build_payload(macho, args.phys_base, args.cmdline, args.version)

    with open(args.output, "wb") as f:
        f.write(payload)

    digest = hashlib.sha256(payload).hexdigest()
    print(f"wrote {args.output}: {info['total_size']} bytes, "
          f"{info['segment_count']} segments, "
          f"virt=0x{info['virt_base']:x} phys=0x{info['phys_base']:x} "
          f"entry=0x{info['entry_pa']:x} sha256={digest}")

    if args.sha256:
        with open(args.sha256, "w") as f:
            f.write(digest + "\n")
        print(f"sha256 written to {args.sha256}")


if __name__ == "__main__":
    main()
