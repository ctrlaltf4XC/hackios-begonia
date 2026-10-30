# BEGXNU01 Payload Format

Derived exclusively from `bootshim/handoff.h` and `bootshim/handoff.c`
(the BEGONIA-XNU boot shim source of truth). No fields, offsets, or
behaviors are invented.

## Source of truth

- `bootshim/handoff.h` — struct definitions, magic constant, static asserts
- `bootshim/handoff.c` — `handoff_validate()` validation rules and `handoff_boot()` segment-copy logic

## 1. Magic bytes

| Offset | Size | Value | Meaning |
|--------|------|-------|---------|
| 0x00   | 8    | `"BEGXNU01"` | Payload magic, validated byte-by-byte in `handoff_validate()` |

## 2. struct xnu_payload (688 bytes)

Defined in `bootshim/handoff.h:36-42`, enforced by `_Static_assert` at line 44.

| Offset | Size | Field | Type | Meaning |
|--------|------|-------|------|---------|
| 0x00   | 8    | `magic` | `char[8]` | `"BEGXNU01"` |
| 0x08   | 4    | `version` | `uint32_t` | Payload version |
| 0x0C   | 4    | `segment_count` | `uint32_t` | Number of segments (1..16) |
| 0x10   | 8    | `virt_base` | `uint64_t` | Kernel virtual base address |
| 0x18   | 8    | `phys_base` | `uint64_t` | Kernel physical base address |
| 0x20   | 8    | `mem_size` | `uint64_t` | Kernel memory region size |
| 0x28   | 8    | `entry_va` | `uint64_t` | Entry point virtual address |
| 0x30   | 8    | `kernel_end_pa` | `uint64_t` | Page-aligned end of all loaded segments |
| 0x38   | 8    | `segments_offset` | `uint64_t` | Offset to `struct xnu_segment` array |
| 0x40   | 8    | `data_offset` | `uint64_t` | Offset to segment raw data |
| 0x48   | 8    | `total_size` | `uint64_t` | Total payload file size |
| 0x50   | 608  | `command_line` | `char[608]` | Null-terminated boot command line |

Header size: 0x50 + 608 = 80 + 608 = **688 bytes**.

## 3. struct xnu_segment (32 bytes)

Defined in `bootshim/handoff.h:43`. One entry per segment, stored at `segments_offset`.

| Offset | Size | Field | Type | Meaning |
|--------|------|-------|------|---------|
| 0x00   | 8    | `physical` | `uint64_t` | Destination physical address |
| 0x08   | 8    | `memory_size` | `uint64_t` | Total memory to reserve (incl. zero-fill) |
| 0x10   | 8    | `file_offset` | `uint64_t` | Offset into payload file of segment data |
| 0x18   | 8    | `file_size` | `uint64_t` | Bytes of actual file data to copy |

Array size: `segment_count * 32` bytes.

## 4. Required alignment

| Item | Alignment | Source |
|------|-----------|--------|
| Payload pointer | 8 bytes | `((uintptr_t)payload & 7)` check |
| `segments_offset` | 8 bytes | `(p->segments_offset & 7)` check |
| `data_offset` | 8 bytes | `(p->data_offset & 7)` check |
| Segment `physical` | 8 bytes | `(s->physical & 7)` check |
| Segment `file_offset` | 8 bytes | `(s->file_offset & 7)` check |
| `mem_size` | 0x4000 (16K page) | `(p->mem_size & (XNU_PAGE - 1))` check |
| `kernel_end_pa` | 0x4000 (16K page) | `aligned(end)` rounds up to page |

`aligned(x) = (x + 0x3fff) & ~0x3fff`

## 5. Segment-loading behavior

The payload contains **separately represented segments**, not the raw Mach-O.
Each segment's data lives at `file_offset` within the payload file; the Mach-O
headers, load commands, and symbol tables are not included.

At boot (`handoff_boot()`, `bootshim/handoff.c:139-144`), for each segment:

1. `copy_bytes(s->physical, payload + s->file_offset, s->file_size)` — copy file data
2. `zero_bytes(s->physical + s->file_size, s->memory_size - s->file_size)` — zero-fill BSS

`memory_size` must be >= `file_size`. The difference is zero-filled at runtime.

Segments must not overlap each other (`overlap()` check on physical ranges).

## 6. Destination address validation rules

All checks in `handoff_validate()` (`bootshim/handoff.c:40-90`):

| Check | Rule |
|-------|------|
| Length | `sizeof(struct xnu_payload) <= length <= 0x8000000` |
| Magic | All 8 bytes equal `"BEGXNU01"` |
| Segment count | `1 <= segment_count <= 16` (`XNU_MAX_SEGMENTS`) |
| Total size | `total_size == length` |
| Segments offset | `>= sizeof(struct xnu_payload)`, 8-aligned, `<= length` |
| Data offset | `>= segments_offset + segment_count * 32`, 8-aligned, `<= length` |
| Phys base alignment | `phys_base & 0x1ffffff == 0` (32MB aligned) |
| Virt base alignment | `virt_base & 0x1ffffff == 0` (32MB aligned) |
| Mem size | Non-zero, `<= 0x60000000` (1.5GB), 16K page-aligned |
| Virt base floor | `virt_base >= 0xffffff8000000000ULL` |
| Virt overflow | `virt_base + mem_size` must not wrap |
| Memory coverage | `[phys_base, phys_base + mem_size)` covered by FDT memory regions |
| Payload coverage | `[payload_pa, payload_pa + length)` covered by FDT memory regions |
| No overlap w/ payload | `[phys_base, phys_base + mem_size)` vs `[payload_pa, payload_pa + length)` |
| No overlap w/ FDT | `[phys_base, phys_base + mem_size)` vs `[fdt_pa, fdt_pa + fdt->bytes)` |
| No overlap w/ shim | `[phys_base, phys_base + mem_size)` vs `[shim_start, shim_end)` |
| No overlap w/ reserved | vs each FDT reserved-memory region |
| Segment in range | Each segment inside `[phys_base, phys_base + mem_size)` |
| Segment file bounds | `file_size <= memory_size`, `file_offset >= data_offset`, `file_offset + file_size <= length` |
| No segment overlap | No two segments' physical ranges overlap |
| Command line | Must be null-terminated within 608 bytes |
| Entry in range | `entry_va >= virt_base` and `entry_va - virt_base < mem_size` |
| Entry found | Entry PA inside exactly one segment's `[physical, physical + file_size)` |
| Kernel end | `aligned(max(physical + memory_size)) == kernel_end_pa` |
| ADT/args fit | `aligned(end) + 2 * XNU_PAGE` inside `[phys_base, phys_base + mem_size)` |

## 7. Entry address handling

- `entry_va` must be within `[virt_base, virt_base + mem_size)`
- Physical entry: `entry_pa = phys_base + (entry_va - virt_base)`
- The entry PA must fall inside exactly one segment's `[physical, physical + file_size)` range (`entry_found == 1`)
- Boot jumps to `entry_pa` with `x0 = boot_args physical address`

## 8. boot_args struct layout (736 bytes, revision/version 2)

Defined in `bootshim/handoff.h:22-31`, enforced by `_Static_assert` at line 32.

| Offset | Size | Field | Type | Value |
|--------|------|-------|------|-------|
| 0x00   | 2    | `revision` | `uint16_t` | 2 |
| 0x02   | 2    | `version` | `uint16_t` | 2 |
| 0x08   | 8    | `virt_base` | `uint64_t` | from payload |
| 0x10   | 8    | `phys_base` | `uint64_t` | from payload |
| 0x18   | 8    | `mem_size` | `uint64_t` | from payload |
| 0x20   | 8    | `top_of_kernel_data` | `uint64_t` | `plan->top` |
| 0x28   | 8    | `video.base` | `uint64_t` | 0 |
| 0x30   | 8    | `video.display` | `uint64_t` | 0 |
| 0x38   | 8    | `video.rowbytes` | `uint64_t` | 0 |
| 0x40   | 8    | `video.width` | `uint64_t` | 0 |
| 0x48   | 8    | `video.height` | `uint64_t` | 0 |
| 0x50   | 8    | `video.depth` | `uint64_t` | 0 |
| 0x58   | 4    | `machine_type` | `uint32_t` | 0x6785 |
| 0x5C   | 4    | (padding) | | |
| 0x60   | 8    | `device_tree` | `uint64_t` | `virt_base + (adt_pa - phys_base)` |
| 0x68   | 4    | `device_tree_length` | `uint32_t` | ADT size |
| 0x6C   | 4    | (padding) | | |
| 0x6C   | 608  | `command_line` | `char[608]` | copied from payload |
| 0x2CC  | 8    | `boot_flags` | `uint64_t` | 0 |
| 0x2D4  | 8    | `mem_size_actual` | `uint64_t` | `mem_size` |
| —      | 4    | (trailing padding) | | to 736 |

Static asserts: `device_tree` at offset 96 (0x60), `command_line` at offset 108 (0x6C).

## 9. Memory plan (`handoff_plan`)

Computed by `handoff_validate()`:

| Field | Value |
|-------|-------|
| `entry_pa` | `phys_base + (entry_va - virt_base)` |
| `adt_pa` | `aligned(end)` where `end = max(physical + memory_size)` |
| `args_pa` | `adt_pa + XNU_PAGE` |
| `top` | `args_pa + XNU_PAGE` |

ADT and boot_args occupy `[adt_pa, adt_pa + 2 * XNU_PAGE)` and must lie within `[phys_base, phys_base + mem_size)`.
