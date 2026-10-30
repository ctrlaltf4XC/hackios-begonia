# KERNEL_VALIDATION.md

BEGONIA-XNU (Operation First Boot) — XNU 8796 kernel artifact validation.
Agent 7 — Release Validation Engineer.

Status: **ALL ITEMS PENDING — requires XNU kernel build on macOS.**

## 1. Scope and source baseline

| Item | Value |
|------|-------|
| XNU source tree | `/home/yu/BEGONIA-XNU/xnu/xnu-8796` |
| XNU commit | `1b191cb58250d0705d8a51287127505aa4bc0789` |
| XNU version | 8796.141.3 (iOS 16 generation) |
| Boot shim | `/home/yu/BEGONIA-XNU/bootshim/` (builds on Linux, V0 PASS) |
| Platform support | `/home/yu/BEGONIA-XNU/platform/begonia/` (minimum, in progress) |
| XNU build | NOT performed — no macOS runner available |
| XNU kernel artifacts | NONE present (no Mach-O, no kernel binary, no image) |
| `package/xnu.img` | 0-byte placeholder ("XNU 8796 placeholder (source not yet integrated)") |
| `package/MANIFEST` phase | `pre-XNU-integration` |

Per the mission charter (rule 10: never fabricate hardware test results; rule 9: every
implementation must include validation evidence), no artifact that does not exist is
validated here. Every XNU kernel validation item below is recorded as
**PENDING — requires XNU kernel build on macOS**.

## 2. Artifact validation checklist

Each item lists the requirement, the verification method, and the status. All XNU
kernel items are PENDING because no XNU kernel has been built; no artifact exists to
inspect.

| # | Requirement | Verification method | Status |
|---|-------------|---------------------|--------|
| K1 | Architecture: Mach-O, arm64 (MH_MAGIC_64 0xfeedfacf, cputype CPU_TYPE_ARM64) | `file`, `otool -h`, `readelf -h` on the linked kernel Mach-O | PENDING — no XNU build artifact |
| K2 | Mach-O format: no unsupported load commands, valid segment/__TEXT/__DATA layout | `otool -l`, compare against XNU 8796 linker expectations | PENDING — no XNU build artifact |
| K3 | CPU subtype: arm64 (CPU_SUBTYPE_ARM64_ALL or specific), not arm64e unless PAC is handled | `otool -h` cpusubtype field | PENDING — no XNU build artifact |
| K4 | Load commands: required segments present (PAGEZERO, __TEXT, __DATA, __LINKEDIT, etc.), LC_ENTRY_POINT or equivalent | `otool -l`, `nm` | PENDING — no XNU build artifact |
| K5 | Segment addresses: __TEXT virt/phys base consistent with bootshim `p->virt_base` / `p->phys_base` | Compare Mach-O segment VM addresses against payload header fields | PENDING — no XNU build artifact |
| K6 | Segment sizes: file_size <= memory_size per segment, total mem_size <= 0x60000000 and page-aligned | `otool -l` filesz/memsz vs payload header | PENDING — no XNU build artifact |
| K7 | Required symbols present: `start_first_cpu` (entry), plus XNU-exported symbols bootshim may reference | `nm` on the Mach-O, check `_start`/`start_first_cpu` | PENDING — no XNU build artifact |
| K8 | Kernel entry info: entry point is `entry_va`, relocated `entry_pa = phys_base + (entry_va - virt_base)` | Mach-O entry field vs payload header `entry_va` | PENDING — no XNU build artifact |
| K9 | Entry convention: XNU `start_first_cpu` receives x0 = boot_args physical address | Source inspection of `pexpert/arm64/` entry + first-boot UART | PENDING — no XNU build artifact |
| K10 | Artifact integrity: payload magic "BEGXNU01", struct xnu_payload 688 bytes, total_size == file size | Bootshim `handoff_validate()` checks magic/size on the packaged payload | PENDING — no XNU build artifact |
| K11 | SHA256 checksum: `bootshim/build.json` records sha256 of the shim Image; XNU payload needs its own sha256 entry | `sha256sum`, compare to manifest | PENDING — no XNU build artifact |
| K12 | Bootshim compatibility: XNU output matches bootshim loading requirements (magic, segments, bounds, entry coverage, kernel_end_pa alignment) | Run `handoff_validate()` logic against the packaged payload (host test or hardware) | PENDING — no XNU build artifact |

## 3. Bootshim compatibility analysis

The bootshim (`bootshim/handoff.c`, `handoff_validate()`) imposes these hard requirements
on the XNU payload. A successfully linked Mach-O is **not** automatically compatible —
the payload must be repacked into the `struct xnu_payload` format and every field must
satisfy the checks below.

### Payload contract (bootshim `handoff.h:36-44`)

```
struct xnu_payload {   // 688 bytes, static-asserted
    char magic[8];     // "BEGXNU01"
    uint32_t version, segment_count;
    uint64_t virt_base, phys_base, mem_size, entry_va, kernel_end_pa;
    uint64_t segments_offset, data_offset, total_size;
    char command_line[608];
};                     // followed by segments[] then segment data
struct xnu_segment {   // per segment
    uint64_t physical, memory_size, file_offset, file_size;
};
```

### Bootshim validation checks (handoff.c:40-89) that XNU output must satisfy

1. Payload length: `sizeof(struct xnu_payload) <= length <= 0x8000000`, 8-byte aligned pointer
2. Magic == "BEGXNU01" (all 8 bytes)
3. `1 <= segment_count <= 16` (XNU_MAX_SEGMENTS)
4. `total_size == length`
5. `segments_offset >= sizeof(struct xnu_payload)`, 8-byte aligned, within length
6. `data_offset >= segments_offset + segment_count * sizeof(struct xnu_segment)`, 8-byte aligned, within length
7. `(virt_base & 0x1ffffff) == 0` and `(phys_base & 0x1ffffff) == 0` (32MB alignment)
8. `mem_size != 0`, `mem_size <= 0x60000000`, page-aligned (16K granule)
9. `virt_base >= 0xffffff8000000000ULL` (kernel VA range)
10. `[phys_base, phys_base + mem_size)` covered by live FDT memory regions (`covered()`)
11. Payload `[payload_pa, payload_pa + length)` covered by FDT memory
12. No overlap: kernel mem vs payload, vs FDT, vs reserved-memory, vs shim image
13. Command line NUL-terminated within 608 bytes
14. `virt_base <= entry_va < virt_base + mem_size`
15. Entry physical address `entry_pa = phys_base + (entry_va - virt_base)` covered by exactly one segment's `[physical, physical + file_size)`
16. `aligned(end of all segments) == kernel_end_pa` (16K alignment)
17. ADT at `aligned(end)`, boot_args at `adt_pa + XNU_PAGE`, top at `args_pa + XNU_PAGE`, all within `[phys_base, phys_base + mem_size)`

### Entry convention (handoff.c:168, BOOT_FLOW.md:79, XNU_BOOT_REQUIREMENTS.md:99-106)

The shim jumps to `plan.entry_pa` with:
- `x0 = plan.args_pa` (physical address of the 736-byte boot_args struct)
- MMU off, data + instruction caches off
- DAIF = 0xf (all exceptions masked)
- EL1

XNU's `start_first_cpu` must receive x0 = boot_args PA and bring up its own MMU/pmap.

### boot_args ABI (handoff.h:22-34, XNU_BOOT_REQUIREMENTS.md:13-38)

736 bytes, revision/version = 2:
- offset 0: revision, version (uint16)
- offset 4: virt_base, phys_base, mem_size, top_of_kernel_data (uint64 each)
- offset 36: video (6 x uint64, zeroed for minimum boot)
- offset 84: machine_type = 0x6785 (MT6785)
- offset 96: device_tree (uint64, ADT VA relative to kernel when MMU on)
- offset 104: device_tree_length (uint32)
- offset 108: command_line (char[608])
- offset 716: boot_flags (uint64)
- offset 724: mem_size_actual (uint64)
Static asserts: `sizeof == 736`, `offsetof(device_tree) == 96`, `offsetof(command_line) == 108`.

### Compatibility verdict

**UNDETERMINED (PENDING).** No XNU kernel has been built, so there is no Mach-O to
repackage into the `struct xnu_payload` format and no payload to run through
`handoff_validate()`. The bootshim side is complete and builds cleanly (V0 PASS per
BOOT_VALIDATION.md), but the XNU side cannot be validated until a macOS build produces
a kernel Mach-O that is packed into the payload format and checked against items K1-K12.

## 4. Mach-O format requirements for XNU 8796.141.3

These are the requirements the XNU 8796 kernel Mach-O must satisfy before it can be
packaged for the bootshim. None can be verified without a built artifact.

| Requirement | Detail | Status |
|-------------|--------|--------|
| Magic | MH_MAGIC_64 (0xfeedfacf) | PENDING |
| CPU type | CPU_TYPE_ARM64 | PENDING |
| CPU subtype | CPU_SUBTYPE_ARM64_ALL (PAC considerations) | PENDING |
| File type | MH_EXECUTE | PENDING |
| Entry point | LC_MAIN / entry offset; `start_first_cpu` | PENDING |
| Segments | __TEXT (RX), __DATA (RW), __LINKEDIT, etc. | PENDING |
| Section alignment | 16K granule (XNU_PAGE) | PENDING |
| No R_AARCH64_ABS relocations in final linked image (bootshim build.sh rejects these for the shim; XNU relocates itself) | PENDING |
| virt_base >= 0xffffff8000000000 (kernel VA range) | PENDING |
| __TEXT file size <= __TEXT memory size; __DATA zero-fill accounted | PENDING |

## 5. Entry point contract

Defined by `bootshim/handoff.c:168` and `XNU_BOOT_REQUIREMENTS.md:96-106`:

- Jump target: `entry_pa = phys_base + (entry_va - virt_base)` (computed by `handoff_validate`, line 69)
- Register x0: physical address of boot_args (`plan.args_pa`)
- MMU: off
- Data cache: off
- Instruction cache: off
- DAIF: 0xf (all exceptions masked)
- Exception level: EL1
- Stack: set by shim; XNU `start_first_cpu` establishes its own stack

XNU `start_first_cpu` (in `pexpert/arm64/`) is expected to be the entry symbol and must
interpret x0 as a physical pointer to the 736-byte `boot_args` struct. XNU then
re-interprets `device_tree` as a VA once its MMU is on.

Status: **PENDING** — requires XNU build + first-boot UART evidence of
`XNU_BEGONIA_KERNEL_ENTRY` (BOOT_VALIDATION.md V4).

## 6. Checksum validation procedure

The bootshim build already demonstrates the pattern (`bootshim/build.sh:29-44`):

1. Build the artifact (shim Image, or XNU payload image).
2. Compute `sha256` of the raw binary bytes.
3. Record the digest in `build.json` (or the package MANIFEST).
4. On each boot, recompute and compare to detect corruption/tampering.

For the XNU kernel, once built:
- Package the kernel Mach-O into `struct xnu_payload` format (magic "BEGXNU01", segments, command line).
- Compute `sha256` of the packaged payload.
- Record in `package/MANIFEST` under the xnu-kernel artifact entry (replacing the current
  placeholder SHA256 `e3b0c44...` which is the digest of the empty 0-byte `xnu.img`).
- Bootshim `handoff_validate()` re-verifies `total_size == length` and magic at boot;
  the SHA256 is an out-of-band integrity check recorded before handoff.

Status: **PENDING** — no XNU payload exists to checksum.

## 7. Artifact status

| Artifact | Path | Status |
|----------|------|--------|
| Boot shim Image | `bootshim/out/Image` | BUILT (37349 bytes, V0 PASS) |
| Boot shim ELF | `bootshim/out/shim.elf` | BUILT (41752 bytes) |
| Boot shim build.json | `bootshim/out/build.json` | BUILT (sha256 recorded) |
| XNU kernel Mach-O | (none) | **PENDING — requires macOS build** |
| XNU packaged payload | (none) | **PENDING — requires Mach-O + packing** |
| XNU kernel image (`package/xnu.img`) | `package/xnu.img` | 0-byte placeholder |
| XNU payload SHA256 | (none) | **PENDING** |

Overall kernel artifact status: **PENDING — requires XNU kernel build on macOS.**

## 8. Sign-off

| Role | Decision | Status |
|------|----------|--------|
| Release Validation Engineer (Agent 7) | All XNU kernel validation items PENDING | RECORDED |
| XNU kernel build | Not performed — no macOS runner | BLOCKED |
| Bootshim compatibility | Cannot be determined without a built XNU payload | PENDING |
| First boot (V4/V5) | Not started — depends on XNU build + platform/begonia | NOT STARTED |

**Sign-off: NOT ISSUED.** KERNEL_VALIDATION.md records requirements and the current
blocked state. No XNU kernel artifact exists to validate; every kernel item is
PENDING pending an XNU 8796 build on macOS (per `XNU_SOURCE_ACQUISITION.md` and
`XNU_BOOT_REQUIREMENTS.md:131-133`, XNU builds on macOS via `darwin-xnu-build`;
Linux build needs clang + cctools or equivalent).
