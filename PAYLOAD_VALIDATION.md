# BEGXNU01 Payload Validation Rules and Test Cases

Derived from `bootshim/handoff.c` `handoff_validate()` and `handoff_boot()`.
All rules trace to bootshim source; none are invented.

## 1. Structural validation rules

These checks are applied in order, matching `handoff_validate()`:

| # | Rule | Failure result |
|---|------|---------------|
| 1 | Payload length: `688 <= length <= 0x8000000` | Reject |
| 2 | Payload pointer 8-byte aligned | Reject |
| 3 | `payload_pa + length` must not overflow | Reject |
| 4 | Magic bytes 0..7 equal `"BEGXNU01"` | Reject |
| 5 | `1 <= segment_count <= 16` | Reject |
| 6 | `total_size == length` | Reject |
| 7 | `segments_offset >= 688`, 8-aligned, `<= length` | Reject |
| 8 | `data_offset >= segments_offset + segment_count * 32`, 8-aligned, `<= length` | Reject |
| 9 | `phys_base & 0x1ffffff == 0` (32MB aligned) | Reject |
| 10 | `virt_base & 0x1ffffff == 0` (32MB aligned) | Reject |
| 11 | `mem_size != 0`, `<= 0x60000000`, 16K page-aligned | Reject |
| 12 | `virt_base >= 0xffffff8000000000ULL` | Reject |
| 13 | `virt_base + mem_size` must not overflow | Reject |
| 14 | `[phys_base, phys_base + mem_size)` covered by FDT memory | Reject |
| 15 | `[payload_pa, payload_pa + length)` covered by FDT memory | Reject |
| 16 | No overlap with payload, FDT, shim, reserved regions | Reject |
| 17 | Command line null-terminated within 608 bytes | Reject |
| 18 | `entry_va >= virt_base` and `entry_va - virt_base < mem_size` | Reject |
| 19 | Each segment: `memory_size != 0`, `physical` 8-aligned, `file_offset` 8-aligned | Reject |
| 20 | Each segment inside `[phys_base, phys_base + mem_size)` | Reject |
| 21 | `file_size <= memory_size` | Reject |
| 22 | `file_offset >= data_offset`, `file_offset + file_size <= length` | Reject |
| 23 | No two segments overlap physically | Reject |
| 24 | Entry PA inside exactly one segment's `[physical, physical + file_size)` | Reject |
| 25 | `aligned(max(physical + memory_size)) == kernel_end_pa` | Reject |
| 26 | `aligned(end) + 2 * XNU_PAGE` inside `[phys_base, phys_base + mem_size)` | Reject |

## 2. boot_args validation (written at boot)

Validated implicitly by `handoff_boot()` writing a 736-byte struct:

| Field | Constraint |
|-------|-----------|
| `revision`, `version` | Both == 2 |
| `device_tree` offset | 96 |
| `command_line` offset | 108 |
| `sizeof(struct xnu_boot_args)` | 736 |

## 3. Positive test case (valid payload)

Construct a minimal valid BEGXNU01 payload:

- Magic: `"BEGXNU01"`
- version: 1
- segment_count: 1
- virt_base: `0xffffff8000200000`
- phys_base: `0x40000000`
- mem_size: `0x20000000` (page-aligned, nonzero, <= 0x60000000)
- entry_va: `0xffffff8000200000` (== virt_base)
- kernel_end_pa: `0x400020000` (page-aligned end of segment)
- segments_offset: 688
- data_offset: 720 (688 + 1 * 32, 8-aligned)
- total_size: 720 + file_size
- command_line: empty string (first byte `\0`)
- One segment: physical=`0x40000000`, memory_size=`0x20000`, file_offset=720, file_size=`0x1000`

Expected: all 26 rules pass; entry found exactly once.

## 4. Negative test cases

### TC-NEG-01: Wrong magic
Magic bytes: `"BADMAGIC"` instead of `"BEGXNU01"`.
Expected: reject at rule 4.

### TC-NEG-02: Truncated header
Payload length < 688 bytes (e.g. 100 bytes).
Expected: reject at rule 1.

### TC-NEG-03: Invalid segment count
segment_count = 0 or segment_count = 17.
Expected: reject at rule 5.

### TC-NEG-04: Address overflow
`virt_base + mem_size` wraps (e.g. virt_base near 0xffffffffffffffff, mem_size large).
Expected: reject at rule 13.

Also `payload_pa + length` overflow.
Expected: reject at rule 3.

### TC-NEG-05: Misaligned payload
segments_offset not 8-aligned (e.g. 689), or data_offset not 8-aligned.
Expected: reject at rule 7 or 8.

### TC-NEG-06: Wrong architecture
virt_base < `0xffffff8000000000ULL` (not in ARM64 kernel high VA range).
Expected: reject at rule 12.

### TC-NEG-07: Segment out of range
Segment physical address outside `[phys_base, phys_base + mem_size)`.
Expected: reject at rule 20.

### TC-NEG-08: file_size > memory_size
Segment claims more file data than memory allocation.
Expected: reject at rule 21.

### TC-NEG-09: Segment data out of bounds
`file_offset + file_size > total_size`.
Expected: reject at rule 22.

### TC-NEG-10: Segment overlap
Two segments with overlapping physical ranges.
Expected: reject at rule 23.

### TC-NEG-11: Entry not found
Entry PA not inside any segment's `[physical, physical + file_size)` range.
Expected: reject at rule 24.

### TC-NEG-12: Entry found in multiple segments
Entry PA inside two segments' file ranges.
Expected: reject at rule 24 (`entry_found != 1`).

### TC-NEG-13: kernel_end_pa mismatch
`kernel_end_pa != aligned(max(physical + memory_size))`.
Expected: reject at rule 25.

### TC-NEG-14: mem_size invalid
mem_size = 0, or > 0x60000000, or not page-aligned.
Expected: reject at rule 11.

### TC-NEG-15: Command line not terminated
All 608 command_line bytes are nonzero.
Expected: reject at rule 17.

### TC-NEG-16: entry_va out of range
`entry_va < virt_base` or `entry_va - virt_base >= mem_size`.
Expected: reject at rule 18.

### TC-NEG-17: Length mismatch
`total_size != length`.
Expected: reject at rule 6.

### TC-NEG-18: segments_offset too small
`segments_offset < 688`.
Expected: reject at rule 7.

### TC-NEG-19: data_offset overlaps segment array
`data_offset < segments_offset + segment_count * 32`.
Expected: reject at rule 8.

### TC-NEG-20: ADT/args region out of range
`aligned(end) + 2 * XNU_PAGE` extends beyond `[phys_base, phys_base + mem_size)`.
Expected: reject at rule 26.

## 5. Validator tool behavior

`tools/validate_begxnu.py <payload> [--expect {valid|invalid}]`

- Exits 0 if payload matches expectation, 1 otherwise.
- Prints PASS/FAIL per rule.
- With `--expect valid`: all rules must pass.
- With `--expect invalid`: at least one rule must fail.

The validator also reports the computed `handoff_plan` (entry_pa, adt_pa, args_pa, top) for valid payloads.
