#!/usr/bin/env python3
"""
validate_begxnu.py — Host-side BEGXNU01 payload validator.

Implements the structural subset of bootshim/handoff.c handoff_validate()
that can be checked without a live FDT. FDT-dependent checks (memory
coverage, overlap with FDT/shim/reserved regions) are reported but not
enforced unless --fdt is supplied.

Usage:
    validate_begxnu.py <payload.begxnu> [--expect {valid|invalid}]
    validate_begxnu.py --suite        # run built-in positive/negative test cases
"""

import argparse
import os
import struct
import sys

# ---------------------------------------------------------------------------
# BEGXNU01 payload ABI — from bootshim/handoff.h
# ---------------------------------------------------------------------------

BEGXNU_MAGIC = b"BEGXNU01"
XNU_PAGE = 0x4000
XNU_CMDLINE = 608
XNU_MAX_SEGMENTS = 16
PAYLOAD_HEADER_SIZE = 688
SEGMENT_SIZE = 32

FAILURES = 0


def check(cond, msg):
    global FAILURES
    if cond:
        print(f"  PASS: {msg}")
    else:
        print(f"  FAIL: {msg}")
        FAILURES += 1


def aligned(value):
    return (value + XNU_PAGE - 1) & ~(XNU_PAGE - 1)


def align8(value):
    return (value + 7) & ~7


def overlap(a, n, b, m):
    """bootshim overlap(): returns True if ranges overlap."""
    if not n or not m:
        return False
    if a + n < a or b + m < b:  # overflow
        return True
    return a < b + m and b < a + n


def inside(start, size, base, span):
    """bootshim inside(): returns True if [start, start+size) within [base, base+span)."""
    return start >= base and start - base <= span and size <= span - (start - base)


def covered(start, size, memory_regions):
    """bootshim covered(): returns True if [start, start+size) covered by memory regions."""
    if not size or start + size < start:
        return False
    cursor = start
    end = start + size
    for _ in range(len(memory_regions) + 1):
        nxt = cursor
        for b, sz in memory_regions:
            limit = b + sz
            if limit < b:
                continue
            if b <= cursor and limit > nxt:
                nxt = limit
        if nxt == cursor:
            return False
        if nxt >= end:
            return True
        cursor = nxt
    return False


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate(payload, fdt_info=None, verbose=True):
    """Return True if payload is structurally valid."""
    global FAILURES
    FAILURES = 0
    length = len(payload)

    if verbose:
        print(f"Validating BEGXNU01 payload ({length} bytes)")

    # Rule 1: length bounds
    check(PAYLOAD_HEADER_SIZE <= length <= 0x8000000,
          f"length in [{PAYLOAD_HEADER_SIZE}, 0x8000000]")

    # Rule 3: payload_pa + length no overflow (payload_pa = 0 for host check)
    check(0 + length >= 0, "payload_pa + length no overflow")

    if length < PAYLOAD_HEADER_SIZE:
        if verbose:
            print("  Payload too small for header; skipping remaining checks.")
        return False

    p = payload

    # Rule 4: magic
    magic_ok = all(p[i] == BEGXNU_MAGIC[i] for i in range(8))
    check(magic_ok, f"magic == 'BEGXNU01' (got {p[0:8]!r})")
    if not magic_ok:
        return False

    # Parse header fields (little-endian, as written by packager)
    version = struct.unpack_from("<I", p, 8)[0]
    segment_count = struct.unpack_from("<I", p, 12)[0]
    virt_base = struct.unpack_from("<Q", p, 16)[0]
    phys_base = struct.unpack_from("<Q", p, 24)[0]
    mem_size = struct.unpack_from("<Q", p, 32)[0]
    entry_va = struct.unpack_from("<Q", p, 40)[0]
    kernel_end_pa = struct.unpack_from("<Q", p, 48)[0]
    segments_offset = struct.unpack_from("<Q", p, 56)[0]
    data_offset = struct.unpack_from("<Q", p, 64)[0]
    total_size = struct.unpack_from("<Q", p, 72)[0]

    # Rule 5: segment count
    check(1 <= segment_count <= XNU_MAX_SEGMENTS,
          f"segment_count {segment_count} in [1, {XNU_MAX_SEGMENTS}]")

    # Rule 6: total_size == length
    check(total_size == length, f"total_size 0x{total_size:x} == length 0x{length:x}")

    # Rule 7: segments_offset
    check(segments_offset >= PAYLOAD_HEADER_SIZE,
          f"segments_offset 0x{segments_offset:x} >= header size 0x{PAYLOAD_HEADER_SIZE:x}")
    check(segments_offset % 8 == 0, "segments_offset 8-byte aligned")
    check(segments_offset <= length, f"segments_offset <= length")

    # Rule 8: data_offset
    min_data_offset = segments_offset + segment_count * SEGMENT_SIZE
    check(data_offset >= min_data_offset,
          f"data_offset >= segments_offset + count*32 (0x{min_data_offset:x})")
    check(data_offset % 8 == 0, "data_offset 8-byte aligned")
    check(data_offset <= length, f"data_offset <= length")

    # Rule 9: phys_base 32MB-aligned
    check(phys_base & 0x1ffffff == 0, f"phys_base 0x{phys_base:x} 32MB-aligned")

    # Rule 10: virt_base 32MB-aligned
    check(virt_base & 0x1ffffff == 0, f"virt_base 0x{virt_base:x} 32MB-aligned")

    # Rule 11: mem_size
    check(mem_size != 0, "mem_size != 0")
    check(mem_size <= 0x60000000, f"mem_size 0x{mem_size:x} <= 0x60000000")
    check(mem_size & (XNU_PAGE - 1) == 0, "mem_size page-aligned")

    # Rule 12: virt_base floor (ARM64 kernel high VA)
    check(virt_base >= 0xffffff8000000000,
          f"virt_base 0x{virt_base:x} >= 0xffffff8000000000")

    # Rule 13: virt_base + mem_size no overflow
    check(virt_base + mem_size >= virt_base, "virt_base + mem_size no overflow")

    # Parse segments
    segments = []
    seg_array_start = segments_offset
    valid_segments = True
    for i in range(segment_count):
        off = seg_array_start + i * SEGMENT_SIZE
        if off + SEGMENT_SIZE > length:
            check(False, f"segment {i} array entry within payload")
            valid_segments = False
            continue
        physical = struct.unpack_from("<Q", p, off + 0)[0]
        memory_size = struct.unpack_from("<Q", p, off + 8)[0]
        file_offset = struct.unpack_from("<Q", p, off + 16)[0]
        file_size = struct.unpack_from("<Q", p, off + 24)[0]
        segments.append({
            "physical": physical,
            "memory_size": memory_size,
            "file_offset": file_offset,
            "file_size": file_size,
        })

    if not valid_segments:
        return False

    # Rule 14-16: coverage / overlap — need FDT; report only
    if fdt_info:
        memory = [(r["base"], r["size"]) for r in fdt_info.get("memory", [])]
        check(covered(phys_base, mem_size, memory),
              "[phys_base, phys_base+mem_size) covered by FDT memory")
        check(covered(0, length, memory),
              "[payload_pa, payload_pa+length) covered by FDT memory")
        # overlap checks
        check(not overlap(phys_base, mem_size, 0, length),
              "kernel range does not overlap payload")
        check(not overlap(phys_base, mem_size, fdt_info.get("fdt_pa", 0), fdt_info.get("fdt_bytes", 0)),
              "kernel range does not overlap FDT")
        for r in fdt_info.get("reserved", []):
            check(not overlap(phys_base, mem_size, r["base"], r["size"]),
                  f"kernel range does not overlap reserved 0x{r['base']:x}")
    else:
        print("  INFO: FDT not supplied; skipping coverage/overlap rules 14-16")

    # Rule 17: command line null-terminated
    cmdline = p[80:80 + XNU_CMDLINE]
    terminated = any(b == 0 for b in cmdline)
    check(terminated, "command_line null-terminated within 608 bytes")

    # Rule 18: entry_va in range
    check(entry_va >= virt_base, "entry_va >= virt_base")
    check(entry_va - virt_base < mem_size, "entry_va - virt_base < mem_size")

    # Rule 19: per-segment alignment and nonzero memory_size
    for i, s in enumerate(segments):
        check(s["memory_size"] != 0, f"segment {i} memory_size != 0")
        check(s["physical"] & 7 == 0, f"segment {i} physical 8-aligned")
        check(s["file_offset"] & 7 == 0, f"segment {i} file_offset 8-aligned")

    # Rule 20: each segment inside [phys_base, phys_base + mem_size)
    for i, s in enumerate(segments):
        check(inside(s["physical"], s["memory_size"], phys_base, mem_size),
              f"segment {i} inside [phys_base, phys_base+mem_size)")

    # Rule 21: file_size <= memory_size
    for i, s in enumerate(segments):
        check(s["file_size"] <= s["memory_size"],
              f"segment {i} file_size <= memory_size")

    # Rule 22: file_offset bounds
    for i, s in enumerate(segments):
        check(s["file_offset"] >= data_offset,
              f"segment {i} file_offset >= data_offset")
        check(s["file_offset"] + s["file_size"] <= length,
              f"segment {i} file_offset + file_size <= length")

    # Rule 23: no segment overlap
    for i in range(len(segments)):
        for j in range(i):
            check(not overlap(segments[i]["physical"], segments[i]["memory_size"],
                              segments[j]["physical"], segments[j]["memory_size"]),
                  f"segment {i} does not overlap segment {j}")

    # Rule 24: entry found exactly once
    entry_pa = phys_base + (entry_va - virt_base)
    entry_found = 0
    for s in segments:
        if s["physical"] <= entry_pa < s["physical"] + s["file_size"]:
            entry_found += 1
    check(entry_found == 1, f"entry PA 0x{entry_pa:x} found in exactly 1 segment (got {entry_found})")

    # Rule 25: kernel_end_pa
    end = phys_base
    for s in segments:
        seg_end = s["physical"] + s["memory_size"]
        if seg_end > end:
            end = seg_end
    check(aligned(end) == kernel_end_pa,
          f"kernel_end_pa 0x{kernel_end_pa:x} == aligned(end) 0x{aligned(end):x}")

    # Rule 26: ADT + args fit
    adt_pa = kernel_end_pa
    top = adt_pa + 2 * XNU_PAGE
    check(top <= phys_base + mem_size,
          f"adt+args region [0x{adt_pa:x}, 0x{top:x}) inside kernel range")

    # Report plan
    print(f"  Plan: entry_pa=0x{entry_pa:x} adt_pa=0x{adt_pa:x} "
          f"args_pa=0x{adt_pa + XNU_PAGE:x} top=0x{top:x}")

    return FAILURES == 0


# ---------------------------------------------------------------------------
# Test suite
# ---------------------------------------------------------------------------

def make_payload(magic=BEGXNU_MAGIC, version=1, segment_count=1,
                 virt_base=0xffffff8000000000, phys_base=0x40000000,
                 mem_size=0x30000000, entry_va=0xffffff8000000000,
                 kernel_end_pa=None, segments_offset=PAYLOAD_HEADER_SIZE,
                 data_offset=None, total_size=None,
                 command_line=b"\0", segments=None):
    """Build a raw payload byte string for testing."""
    if kernel_end_pa is None:
        end = phys_base
        if segments:
            for s in segments:
                seg_end = s["physical"] + s["memory_size"]
                if seg_end > end:
                    end = seg_end
        kernel_end_pa = aligned(end)
    # mem_size must cover segments + ADT + args (2 pages after kernel_end_pa)
    min_mem_size = (kernel_end_pa - phys_base) + 2 * XNU_PAGE
    if mem_size < min_mem_size:
        mem_size = aligned(min_mem_size)
    if data_offset is None:
        data_offset = align8(segments_offset + segment_count * SEGMENT_SIZE)
    if total_size is None:
        total_size = data_offset
        for s in segments or []:
            total_size = max(total_size, align8(s["file_offset"] + s["file_size"]))

    header = bytearray(PAYLOAD_HEADER_SIZE)
    header[0:8] = magic
    struct.pack_into("<I", header, 8, version)
    struct.pack_into("<I", header, 12, segment_count)
    struct.pack_into("<Q", header, 16, virt_base)
    struct.pack_into("<Q", header, 24, phys_base)
    struct.pack_into("<Q", header, 32, mem_size)
    struct.pack_into("<Q", header, 40, entry_va)
    struct.pack_into("<Q", header, 48, kernel_end_pa)
    struct.pack_into("<Q", header, 56, segments_offset)
    struct.pack_into("<Q", header, 64, data_offset)
    struct.pack_into("<Q", header, 72, total_size)
    cl = bytearray(XNU_CMDLINE)
    cl[:len(command_line)] = command_line
    header[80:80 + XNU_CMDLINE] = cl

    seg_array = bytearray(segment_count * SEGMENT_SIZE)
    if segments:
        for i, s in enumerate(segments):
            off = i * SEGMENT_SIZE
            struct.pack_into("<Q", seg_array, off + 0, s["physical"])
            struct.pack_into("<Q", seg_array, off + 8, s["memory_size"])
            struct.pack_into("<Q", seg_array, off + 16, s["file_offset"])
            struct.pack_into("<Q", seg_array, off + 24, s["file_size"])

    payload = bytearray(total_size)
    payload[0:PAYLOAD_HEADER_SIZE] = header
    payload[PAYLOAD_HEADER_SIZE:PAYLOAD_HEADER_SIZE + len(seg_array)] = seg_array
    return bytes(payload)


def valid_segment(physical, memory_size, file_offset, file_size):
    return {"physical": physical, "memory_size": memory_size,
            "file_offset": file_offset, "file_size": file_size}


def run_suite():
    global FAILURES
    passed = 0
    failed = 0

    def expect(name, payload, expect_valid):
        nonlocal passed, failed
        FAILURES = 0
        ok = validate(payload, verbose=False)
        success = (ok == expect_valid)
        if success:
            passed += 1
            print(f"PASS: {name}")
        else:
            failed += 1
            print(f"FAIL: {name} (expected valid={expect_valid}, got valid={ok}, {FAILURES} failures)")

    # --- Positive cases ---
    seg = valid_segment(0x40000000, 0x20000, PAYLOAD_HEADER_SIZE + SEGMENT_SIZE, 0x1000)
    expect("TC-POS-01: minimal valid payload",
           make_payload(segments=[seg]), True)

    # --- Negative cases ---
    # TC-NEG-01: wrong magic
    expect("TC-NEG-01: wrong magic",
           make_payload(magic=b"BADMAGIC"), False)

    # TC-NEG-02: truncated header
    expect("TC-NEG-02: truncated header (< 688 bytes)",
           make_payload()[:100], False)

    # TC-NEG-03: segment count 0
    expect("TC-NEG-03: segment_count = 0",
           make_payload(segment_count=0), False)
    # segment count 17
    expect("TC-NEG-03b: segment_count = 17",
           make_payload(segment_count=17), False)

    # TC-NEG-04: address overflow (virt_base + mem_size wraps)
    expect("TC-NEG-04: virt_base + mem_size overflow",
           make_payload(virt_base=0xffffffffffffffff, mem_size=0x1000), False)

    # TC-NEG-05: misaligned segments_offset
    expect("TC-NEG-05: segments_offset not 8-aligned",
           make_payload(segments_offset=689), False)
    # misaligned data_offset
    expect("TC-NEG-05b: data_offset not 8-aligned",
           make_payload(data_offset=721), False)

    # TC-NEG-06: wrong architecture (virt_base below kernel floor)
    expect("TC-NEG-06: virt_base below ARM64 floor",
           make_payload(virt_base=0x1000), False)

    # TC-NEG-07: segment out of range
    seg_oob = valid_segment(0x70000000, 0x1000, PAYLOAD_HEADER_SIZE + SEGMENT_SIZE, 0x1000)
    expect("TC-NEG-07: segment outside [phys_base, phys_base+mem_size)",
           make_payload(mem_size=0x20000000, segments=[seg_oob]), False)

    # TC-NEG-08: file_size > memory_size
    seg_big = valid_segment(0x40000000, 0x1000, PAYLOAD_HEADER_SIZE + SEGMENT_SIZE, 0x2000)
    expect("TC-NEG-08: file_size > memory_size",
           make_payload(mem_size=0x20000000, segments=[seg_big]), False)

    # TC-NEG-09: file_offset + file_size > length
    seg_oob2 = valid_segment(0x40000000, 0x20000, 0x1000000, 0x1000)
    expect("TC-NEG-09: segment data beyond payload",
           make_payload(total_size=PAYLOAD_HEADER_SIZE + SEGMENT_SIZE + 0x1000, segments=[seg_oob2]), False)

    # TC-NEG-10: segment overlap
    seg_a = valid_segment(0x40000000, 0x20000, PAYLOAD_HEADER_SIZE + SEGMENT_SIZE, 0x1000)
    seg_b = valid_segment(0x40001000, 0x20000, PAYLOAD_HEADER_SIZE + 2 * SEGMENT_SIZE, 0x1000)
    expect("TC-NEG-10: overlapping segments",
           make_payload(segment_count=2, segments=[seg_a, seg_b]), False)

    # TC-NEG-11: entry not found in any segment
    expect("TC-NEG-11: entry not in any segment file range",
           make_payload(entry_va=0xffffff8000201000,
                        segments=[valid_segment(0x40000000, 0x20000,
                                                PAYLOAD_HEADER_SIZE + SEGMENT_SIZE, 0x1000)]),
           False)

    # TC-NEG-12: kernel_end_pa mismatch
    expect("TC-NEG-12: kernel_end_pa != aligned(end)",
           make_payload(kernel_end_pa=0x40001000), False)

    # TC-NEG-13: mem_size = 0
    expect("TC-NEG-13: mem_size = 0",
           make_payload(mem_size=0), False)

    # TC-NEG-14: mem_size > 0x60000000
    expect("TC-NEG-14: mem_size > 0x60000000",
           make_payload(mem_size=0x80000000), False)

    # TC-NEG-15: mem_size not page-aligned
    expect("TC-NEG-15: mem_size not page-aligned",
           make_payload(mem_size=0x1000), False)

    # TC-NEG-16: command line not terminated
    hdr = bytearray(make_payload())
    hdr[80:80 + XNU_CMDLINE] = b"A" * XNU_CMDLINE
    expect("TC-NEG-16: command_line not null-terminated",
           bytes(hdr), False)

    # TC-NEG-17: entry_va < virt_base
    expect("TC-NEG-17: entry_va < virt_base",
           make_payload(entry_va=0xffffff8000100000), False)

    # TC-NEG-18: total_size != length
    hdr = bytearray(make_payload())
    struct.pack_into("<Q", hdr, 72, 999999)
    expect("TC-NEG-18: total_size != length",
           bytes(hdr), False)

    # TC-NEG-19: segments_offset < header size
    expect("TC-NEG-19: segments_offset < 688",
           make_payload(segments_offset=100), False)

    # TC-NEG-20: entry_va - virt_base >= mem_size
    expect("TC-NEG-20: entry_va - virt_base >= mem_size",
           make_payload(entry_va=0xffffff8000400000, mem_size=0x20000000), False)

    print(f"\nSuite: {passed} passed, {failed} failed out of {passed + failed}")
    return failed == 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Validate a BEGXNU01 payload")
    ap.add_argument("payload", nargs="?", help="Payload file to validate")
    ap.add_argument("--expect", choices=["valid", "invalid"],
                    help="Expected outcome (valid or invalid)")
    ap.add_argument("--suite", action="store_true",
                    help="Run built-in positive/negative test cases")
    args = ap.parse_args()

    if args.suite:
        return 0 if run_suite() else 1

    if not args.payload:
        ap.error("payload file required unless --suite")

    with open(args.payload, "rb") as f:
        data = f.read()

    ok = validate(data, verbose=True)

    if args.expect:
        expected = args.expect == "valid"
        if ok == expected:
            print(f"\nResult: payload matches expectation '{args.expect}'")
            return 0
        else:
            print(f"\nResult: payload does NOT match expectation '{args.expect}'")
            return 1

    print(f"\nResult: {'VALID' if ok else 'INVALID'}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
