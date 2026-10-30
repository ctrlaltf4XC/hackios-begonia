# FIRST_KERNEL_ARTIFACT_REPORT.md

BEGONIA-XNU — Operation Kernel Forge
Agent 5: Artifact & Release Validation Engineer

| Field | Value |
|-------|-------|
| Repository | https://github.com/ctrlaltf4XC/hackios-begonia |
| Branch | ci/first-boot-validation |
| Report date | 2026-10-30T14:52:08Z |
| Sign-off status | BLOCKED — NOT EXECUTED (no kernel artifacts exist) |

---

## 1. Executive summary

No actual compiled XNU kernel artifacts (baseline or Begonia) exist in GitHub Actions for branch `ci/first-boot-validation`. Consequently, no Mach-O validation, SHA256 integrity verification, or bootshim compatibility assessment against real artifacts can be performed.

The only workflow present in the repository is `.github/workflows/build.yml`. Its single completed run (id 40135274393, `main`, 2026-10-24) produced two artifacts, neither of which is an XNU kernel:

| Artifact | Size | `file` type | XNU kernel Mach-O? |
|----------|------|-------------|---------------------|
| `loader` -> `loader.bin` | 12,550 B (extracted) | Linux kernel ARM64 boot executable Image | NO (boot shim) |
| `hackios-begonia-boot.img` | 176,128 B (extracted) | Android bootimg | NO (Android boot image) |

The `xnu.yml` workflow (jobs `xnu-baseline` / `xnu-begonia`) that would produce the `xnu-build-VMAPPLE` and `xnu-build-begonia` artifacts is **not present on GitHub** (workflows API returns total_count=1: only `build.yml`). No `xnu.yml` workflow run has ever occurred.

Root causes (documented in PROJECT_STATUS.md / BUILD.md):
- macOS 14 runner brownout active (Oct 30 14:00 – Oct 31 00:00 UTC); macos-14 retires Nov 2, 2026.
- XNU 8796 build requires a macOS host with Xcode 15.4 + KDK 13.5 (22G74); this host is Linux.
- PROJECT_STATUS success levels 3–6 (baseline build, Begonia build, artifact retrieval, bootshim compatibility) are all NOT EXECUTED.

Per the project charter rule "Do not validate artifacts that don't exist," this report documents non-existence rather than fabricating validation results.

---

## 2. Baseline kernel artifact (XNU 8796.141.3, unmodified)

### 2.1 Expected artifact (not produced)

| Field | Value |
|-------|-------|
| Workflow | `.github/workflows/xnu.yml`, job `xnu-baseline` |
| Machine config | VMAPPLE |
| Kernel config | DEVELOPMENT |
| XNU tag / commit | xnu-8796.141.3 / 1b191cb58250d0705d8a51287127505aa4bc0789 |
| Xcode / runner | Xcode 15.4 / macos-14 (macOS 13.5) |
| KDK | Kernel Debug Kit 13.5 build 22G74 |
| Build engine | blacktop/darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549 |
| Expected kernel path | `BEGONIA-XNU/tools/dxb/build/xnu.obj/kernel.development.vmapple` |
| Expected artifact name | `xnu-build-VMAPPLE` (build/, fakeroot/, checksums; 7-day retention) |
| Expected file format | Mach-O 64-bit (MH_MAGIC_64 `0xfeedfacf`), arm64 |
| Artifact path (actual) | **DOES NOT EXIST** |
| SHA256 (actual) | **N/A — no artifact** |

### 2.2 Mach-O validation results

| Check | Result | Basis |
|-------|--------|-------|
| File present | NOT EXECUTED | No artifact downloaded; no xnu.yml run |
| Mach-O magic | NOT EXECUTED | No artifact to inspect |
| ARM64 CPU type/subtype | NOT EXECUTED | No artifact to inspect |
| Load commands (LC_SEGMENT_64, etc.) | NOT EXECUTED | No artifact to inspect |
| Segment addresses & sizes | NOT EXECUTED | No artifact to inspect |
| Alignment | NOT EXECUTED | No artifact to inspect |
| Entry point (LC_MAIN / LC_UNIXTHREAD) | NOT EXECUTED | No artifact to inspect |
| Required symbols | NOT EXECUTED | No artifact to inspect |
| SHA256 integrity | NOT EXECUTED | No artifact to hash |

### 2.3 Build metadata (baseline)

Build was never triggered. No build-logs, no checksums, no artifact were produced. See BUILD.md "Build environment blocker": baseline XNU build cannot be performed on this Linux host; requires macOS runner.

---

## 3. Begonia kernel artifact (XNU 8796.141.3 + Begonia platform)

### 3.1 Expected artifact (not produced)

| Field | Value |
|-------|-------|
| Workflow | `.github/workflows/xnu.yml`, job `xnu-begonia` (needs `xnu-baseline`) |
| Machine config | BEGONIA |
| Kernel config | DEVELOPMENT |
| XNU tag / commit | xnu-8796.141.3 / 1b191cb58250d0705d8a51287127505aa4bc0789 |
| Xcode / runner | Xcode 15.4 / macos-14 (macOS 13.5) |
| KDK | Kernel Debug Kit 13.5 build 22G74 |
| Build engine | blacktop/darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549 |
| Platform sources integrated | platform/begonia/BegoniaPlatformExpert.cpp, BegoniaCPU.cpp, begonia.h |
| Expected kernel path | `BEGONIA-XNU/tools/dxb/build/xnu.obj/kernel.development.begonia` |
| Expected artifact name | `xnu-build-begonia` (build/, fakeroot/; 7-day retention) |
| Expected file format | Mach-O 64-bit, arm64 |
| Artifact path (actual) | **DOES NOT EXIST** |
| SHA256 (actual) | **N/A — no artifact** |

### 3.2 Mach-O validation results

| Check | Result | Basis |
|-------|--------|-------|
| File present | NOT EXECUTED | No artifact downloaded; no xnu.yml run |
| Mach-O magic | NOT EXECUTED | No artifact to inspect |
| ARM64 CPU type/subtype | NOT EXECUTED | No artifact to inspect |
| Load commands | NOT EXECUTED | No artifact to inspect |
| Segment addresses & sizes | NOT EXECUTED | No artifact to inspect |
| Alignment | NOT EXECUTED | No artifact to inspect |
| Entry point | NOT EXECUTED | No artifact to inspect |
| Required symbols | NOT EXECUTED | No artifact to inspect |
| SHA256 integrity | NOT EXECUTED | No artifact to hash |

### 3.3 Build metadata (Begonia)

Build was never triggered (blocked behind `xnu-baseline`, which never ran). No platform integration build occurred. See PROJECT_STATUS success level 4: NOT EXECUTED.

---

## 4. Artifacts that DO exist (non-kernel — excluded from validation)

Downloaded from GitHub Actions run 40135274393 (`build.yml`, `main`, 2026-10-24) for due diligence. Confirmed NOT XNU kernel Mach-O artifacts; excluded from validation per "do not validate artifacts that don't exist" (these are not the baseline/Begonia kernel artifacts at all).

| Artifact | GitHub artifact id | Extracted path | SHA256 | `file` type | Role |
|----------|-------------------|----------------|--------|-------------|------|
| `loader` | 12576842618 | /tmp/kilo/artifacts/loader/loader.bin | 951ceca729ea845a8abdd5014762127bf6ddeccc2e18431b281d8d0c7e4f652f | Linux kernel ARM64 boot executable Image | Boot shim (Linux-verified), 37,477-byte class Image |
| `hackios-begonia-boot.img` | 12576487968 | /tmp/kilo/artifacts/hackios-begonia-boot.img/hackios-begonia-boot.img | 08364bad488eed9706afa9ec3a8a9d7b575502d177a7196d6bf31721ce6bc101 | Android bootimg (kernel @0x40080000, ramdisk @0x47c80000, page 2048) | Android boot image (not XNU) |
| `hackios-begonia-boot-raw.img` | 12576487968 | /tmp/kilo/artifacts/hackios-begonia-boot.img/hackios-begonia-boot-raw.img | 0ddb83480708e5bb79cbb48fb7469c5e59dd3e6933eb7de280b97520c3a1ed14 | Android bootimg (raw) | Duplicate of above |

Artifact retention: expire 2027-01-22 (not expired). Neither begins with Mach-O magic (`0xfeedface` / `0xfeedfacf` / `0xcafebabe`), so neither can be validated as an XNU kernel.

---

## 5. Bootshim loading requirements (reference contract)

Source: `bootshim/handoff.c`, `bootshim/handoff.h`, `BOOT_FLOW.md`, `XNU_BOOT_REQUIREMENTS.md`.

The bootshim (`handoff_boot` / `handoff_validate`) does **not** consume a raw Mach-O. It consumes a wrapped payload with magic `"BEGXNU01"`.

### 5.1 Payload ABI (`struct xnu_payload`, 688 bytes)

| Field | Offset | Size | Meaning |
|-------|--------|------|---------|
| magic[8] | 0 | 8 | Must equal `"BEGXNU01"` |
| version | 8 | 4 | — |
| segment_count | 12 | 4 | 1..16 |
| virt_base | 16 | 8 | kernel virtual base (>= 0xffffff8000000000, 16MiB-aligned) |
| phys_base | 24 | 8 | kernel physical base (16MiB-aligned) |
| mem_size | 32 | 8 | page-aligned (16K), >0, <= 0x60000000 |
| entry_va | 40 | 8 | entry virtual address |
| kernel_end_pa | 48 | 8 | end physical address (aligned(end) == kernel_end_pa) |
| segments_offset | 56 | 8 | offset to segment table (8-aligned) |
| data_offset | 64 | 8 | offset to segment data (8-aligned) |
| total_size | 72 | 8 | == payload length |
| command_line[608] | 80 | 608 | NUL-terminated |

### 5.2 Segment ABI (`struct xnu_segment`)

`{ physical, memory_size, file_offset, file_size }`, each 8 bytes. physical & file_offset 8-aligned; memory_size > 0; file_size <= memory_size; no overlap; entry covered by exactly one segment.

### 5.3 Entry convention

- x0 = physical address of `boot_args` (736 bytes, revision/version 2)
- MMU off, data + instruction caches off
- DAIF = 0xf (all masked)
- EL1 (or EL2->EL1 via bootshim `enter_el1`)
- `entry_pa = phys_base + (entry_va - virt_base)`
- Page size: 16K (XNU_PAGE = 0x4000)

### 5.4 `handoff_validate` rejection conditions (handoff.c:44-88)

Rejects if: wrong magic, segment_count out of range, total_size != length, unaligned pointers, virt_base/phys_base not 16MiB-aligned, mem_size invalid, virt_base < 0xffffff8000000000, phys/mem not covered by FDT memory, overlap with FDT/reserved/shim, command line not NUL-terminated, entry outside [virt_base, virt_base+mem_size), entry not covered by exactly one segment, `aligned(kernel_end) != kernel_end_pa`.

---

## 6. Bootshim compatibility assessment

### 6.1 Can the bootshim load a Mach-O directly?

**NO.** A Mach-O kernel cannot be loaded directly by the bootshim.

Evidence: `handoff_validate` (bootshim/handoff.c:49-50) requires the first 8 bytes of the payload to equal `"BEGXNU01"`. A 64-bit Mach-O begins with `MH_MAGIC_64` (`0xfeedfacf`, little-endian bytes `cf fa ed fe`) or `MH_MAGIC` (`0xfeedface`). `"BEGXNU01"` != Mach-O magic, so `handoff_validate` returns -1 and `handoff_boot` emits `XNU_PAYLOAD_PLAN_REJECTED` before any segment is copied.

### 6.2 Required loader adaptation

**YES — loader adaptation is required.** To make the XNU Mach-O consumable by the bootshim, an intermediate loader/packer step must:

1. Parse the Mach-O load commands (`LC_SEGMENT_64`, `LC_MAIN` or `LC_UNIXTHREAD` for entry).
2. Extract virt_base, phys_base, entry_va, mem_size, kernel_end_pa from the Mach-O segments/entry.
3. Build the `struct xnu_payload` header with magic `"BEGXNU01"` and a `struct xnu_segment` table.
4. Copy each Mach-O segment's file data into the payload at `data_offset`, with `file_offset`/`file_size`/`physical`/`memory_size` per the segment ABI.
5. Set `command_line`, NUL-terminate it.
6. Ensure `aligned(kernel_end) == kernel_end_pa` and all alignment/bounds/coverage/no-overlap rules pass.

This adaptation must happen before `handoff_boot`; the bootshim itself has no Mach-O parser.

### 6.3 Assessment per artifact

| Artifact | Exists? | Mach-O? | Bootshim direct load? | Verdict |
|----------|---------|---------|----------------------|---------|
| Baseline kernel | NO | N/A | N/A | Cannot assess — no artifact |
| Begonia kernel | NO | N/A | N/A | Cannot assess — no artifact |

General verdict (applies to any XNU Mach-O once built): **needs loader adaptation** (Mach-O → BEGXNU01 payload packer). Not directly loadable.

---

## 7. Artifact integrity verification

| Artifact | Expected SHA256 source | Actual SHA256 | Match? | Status |
|----------|----------------------|---------------|--------|--------|
| Baseline kernel (`xnu-build-VMAPPLE`) | Would be in `kernel.development.vmapple.sha256` / artifact checksum step | N/A | N/A | Artifact missing |
| Begonia kernel (`xnu-build-begonia`) | Would be in `kernel.development.begonia.sha256` / `steps.checksums.outputs.kernel_checksum` | N/A | N/A | Artifact missing |

Integrity verification cannot be performed because neither artifact was produced. SHA256 values recorded above are for the non-kernel `loader` and `hackios-begonia-boot.img` artifacts only, and are not kernel integrity evidence.

---

## 8. Sign-off status

| Item | Status |
|------|--------|
| Baseline kernel artifact retrieved | NOT EXECUTED (no xnu.yml run / artifact) |
| Begonia kernel artifact retrieved | NOT EXECUTED (no xnu.yml run / artifact) |
| Baseline Mach-O validation | NOT EXECUTED (no artifact) |
| Begonia Mach-O validation | NOT EXECUTED (no artifact) |
| SHA256 integrity verification | NOT EXECUTED (no artifact) |
| Bootshim direct-load assessment | DETERMINED — Mach-O NOT directly loadable; needs loader adaptation (reasoned from handoff.c, not from artifact) |
| Overall sign-off | **BLOCKED — NOT EXECUTED** |

### Conditions to unblock

1. macOS brownout window must pass (after 2026-10-31T00:00:00Z) and before macos-14 retires (2026-11-02), OR secure a self-hosted macOS Sonoma 14 runner (BUILD.md Option C).
2. `gh auth` must remain valid (`repo` + `workflow` scopes; currently valid for ctrlaltf4XC).
3. The `xnu.yml` workflow must be present on the `ci/first-boot-validation` branch (currently only `build.yml` exists on GitHub) and dispatched for `xnu-baseline` (VMAPPLE) then `xnu-begonia` (BEGONIA).
4. On success, download `xnu-build-VMAPPLE` (`kernel.development.vmapple`) and `xnu-build-begonia` (`kernel.development.begonia`) artifacts, then perform the Mach-O checks in sections 2.2 / 3.2 and re-run this assessment against actual artifacts.

---

**Prepared by:** Agent 5 — Artifact & Release Validation Engineer
**Evidence:** GitHub Actions API (`gh api repos/ctrlaltf4XC/hackios-begonia/actions/...`), `gh run download`, `file`, `sha256sum`; bootshim contract from `bootshim/handoff.c:44-90`, `bootshim/handoff.h`, `BOOT_FLOW.md`, `XNU_BOOT_REQUIREMENTS.md`.
