# BENCHMARK_RESULTS.md — XNU 8796 Build Performance Optimization Plan & Benchmark Framework

Owner: Agent 5 — Performance Engineer, Operation First Boot
Repository: /home/yu/BEGONIA-XNU
Build engine: tools/darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549
Current environment: Linux x86_64 (no macOS/Xcode/KDK available)

## Status — ALL RESULTS ESTIMATED

This host is Linux x86_64. The reference `darwin-xnu-build/build.sh` requires a macOS host with
Xcode 15.x, mig, kmutil, hdiutil, and the Kernel Debug Kit (KDK). A baseline XNU build cannot be
performed here. **Every result in this document is ESTIMATED — pending macOS runner execution.**
No durations, CPU utilization, RAM consumption, translation unit counts, or linker times have been
measured. No value in any results table is a measurement; all are placeholders marked ESTIMATED.

## 1. Benchmark Framework Description

The benchmark framework measures the performance of the optimized XNU 8796.141.3 build pipeline
defined in `.github/workflows/xnu.yml` and `BUILD.md`. It runs on GitHub-hosted macOS 14 runners
(Xcode 15.4, KDK 13.5 build 22G74) and exercises the build engine `tools/darwin-xnu-build` at commit
`663906528d1cd5ea8fde7de6f16de73db9511549`.

### Framework components

| Component | Source | Purpose |
|-----------|--------|---------|
| Build engine | `tools/darwin-xnu-build` @ `6639065` | dxb build.sh (pinned, never modified) |
| CI workflow | `.github/workflows/xnu.yml` | Triggers builds, detects parallelism, manages cache |
| Build orchestrator | `build.sh` | Top-level target dispatch (bootshim/xnu/platform/all/clean) |
| Build config doc | `BUILD.md` | Optimization objectives, parallelism, cache strategy |
| Benchmark plan | `BENCHMARK.md` | Scenarios B1-B5, metrics, results template |
| This document | `BENCHMARK_RESULTS.md` | Plan + results placeholder framework (Agent 5) |

### Execution model

Benchmarks are executed via `workflow_dispatch` on `macos-14` with explicit inputs:
`kernel_config`, `machine_config`, `xnu_build_jobs`, `build_kc`, `keep_going`, `force_clean`.
The `xnu-baseline` job builds with `MACHINE_CONFIG=VMAPPLE`; the `xnu-begonia` job builds with
`MACHINE_CONFIG=BEGONIA` and depends on `xnu-baseline` succeeding. Begonia patches are never applied
during the baseline job.

### Environment blocker (documented, not a fabrication)

This Linux x86_64 host lacks: Xcode 15.x (`xcodebuild`, `xcrun`, `mig`, `kmutil`, `hdiutil`),
`DEVELOPER_DIR`, KDK headers/libs, iPhoneOS SDK, and the macOS toolchain. `build.sh xnu` exits 1
with a documented message. Success criterion "Baseline XNU build succeeds" is pending a macOS build
host. See `BUILD.md` section "Build environment blocker".

## 2. Scenario Definitions (B1-B5)

| Scenario | Description | Trigger | Expected behavior |
|----------|-------------|---------|-------------------|
| B1 | Clean baseline build | Empty cache, `force_clean=true` or first run | Full compilation from scratch; no cache restore; all TUs compile |
| B2 | Unchanged rebuild | Cache hit, no source changes | Cache restore; make reports "nothing to be done"; near-instant |
| B3 | Single-file source modification | Touch one `.c` file | Cache restore; only affected object(s) recompile; minimal |
| B4 | Multiple-file source modification | Touch several `.c`/`.h` files | Cache restore; moderate set of objects recompile |
| B5 | Begonia platform patch rebuild | `platform/begonia/` sources added | `xnu-begonia` job; platform sources compiled into kernel |

### Expected behaviors per scenario

- **B1**: Longest duration; CPU utilization near job count; peak RAM at max for job count; cache
  save duration reflects full object tree size; linker duration reflects full link.
- **B2**: Shortest duration; near-zero CPU; minimal RAM; cache restore dominant; no TUs compiled;
  linker duration ~0 (nothing to link); kernel SHA-256 matches B1.
- **B3**: Short duration; small TU compile count; cache restore for unchanged objects; linker
  duration small; kernel SHA-256 differs from B1.
- **B4**: Moderate duration; moderate TU compile count; linker duration moderate; kernel SHA-256
  differs from B1.
- **B5**: Duration between B1 and B4 depending on platform source count; additional TU compiles for
  BegoniaPlatformExpert.cpp, BegoniaCPU.cpp; kernel SHA-256 differs from baseline VMAPPLE build.

## 3. Job Count Comparison Matrix (4 / 6 / 8)

Benchmarks run each scenario at job counts 4, 6, and 8 to characterize parallelism scaling. All
entries below are ESTIMATED — pending macOS runner execution.

| Jobs | B1 Clean Duration (sec) | B2 Incremental Duration (sec) | Speedup vs 4x (B1) | ESTIMATED/MEASURED |
|------|-------------------------|-------------------------------|--------------------|--------------------|
| 4 | ESTIMATED — pending macOS runner execution | ESTIMATED — pending macOS runner execution | 1.0x | ESTIMATED |
| 6 | ESTIMATED — pending macOS runner execution | ESTIMATED — pending macOS runner execution | ESTIMATED — pending macOS runner execution | ESTIMATED |
| 8 | ESTIMATED — pending macOS runner execution | ESTIMATED — pending macOS runner execution | ESTIMATED — pending macOS runner execution | ESTIMATED |

### Job count selection inputs (workflow_dispatch)

| Input | Default | Description |
|-------|---------|-------------|
| `xnu_build_jobs` | (empty = auto) | Parallel compilation jobs; empty triggers auto-detection |

## 4. Metrics Collection Methodology

| Metric | Source / capture method | Unit | ESTIMATED/MEASURED |
|--------|------------------------|------|--------------------|
| Build duration | `steps.xnu-build.outputs.build_duration_sec` (`date +%s` around make) | seconds | ESTIMATED |
| Cache restore duration | `steps.cache-restore.outputs.duration_sec` | seconds | ESTIMATED |
| Cache save duration | `steps.cache-save.outputs.duration_sec` | seconds | ESTIMATED |
| Compilation job count | `steps.detect-parallelism.outputs.build_jobs` | count | ESTIMATED |
| Kernel SHA-256 | `steps.checksums.outputs.kernel_checksum` | hex digest | ESTIMATED |
| CPU utilization | Runner metrics (GitHub) | % | ESTIMATED |
| Peak RAM consumption | Runner metrics (GitHub) | GB | ESTIMATED |
| Cache size | `actions/cache` stored size | GB/MB | ESTIMATED |
| TUs compiled | Build log object count (`.o` files produced) | count | ESTIMATED |
| TUs reused | Cache-hit objects skipped by make | count | ESTIMATED |
| Linker duration | Link phase of `make install` | seconds | ESTIMATED |

### Measurement placeholders (all ESTIMATED)

Each scenario table below uses the value `ESTIMATED — pending macOS runner execution` for every
metric. No number is invented. A measured result replaces this placeholder only after execution on a
macOS runner.

## 5. Results Tables (All ESTIMATED)

### Scenario B1 — Clean baseline build (empty cache, full compilation)

| Metric | Value |
|--------|-------|
| Build duration (sec) | ESTIMATED — pending macOS runner execution |
| Compilation jobs | ESTIMATED — pending macOS runner execution |
| CPU utilization (%) | ESTIMATED — pending macOS runner execution |
| Peak RAM (GB) | ESTIMATED — pending macOS runner execution |
| Cache restore (sec) | N/A (clean — no cache) |
| Cache save (sec) | ESTIMATED — pending macOS runner execution |
| Cache size (GB) | ESTIMATED — pending macOS runner execution |
| TUs compiled | ESTIMATED — pending macOS runner execution |
| TUs reused | ESTIMATED — pending macOS runner execution |
| Linker duration (sec) | ESTIMATED — pending macOS runner execution |
| Kernel SHA-256 | ESTIMATED — pending macOS runner execution |
| Xcode version | 15.4 |
| KDK version | 22G74 |
| XNU commit | 1b191cb58250d0705d8a51287127505aa4bc0789 |
| BUILD_LTO | 0 (enforced) |
| Status | ESTIMATED — pending macOS runner execution |

### Scenario B2 — Second unchanged build (incremental, cache hit)

| Metric | Value |
|--------|-------|
| Build duration (sec) | ESTIMATED — pending macOS runner execution |
| Compilation jobs | ESTIMATED — pending macOS runner execution |
| CPU utilization (%) | ESTIMATED — pending macOS runner execution |
| Peak RAM (GB) | ESTIMATED — pending macOS runner execution |
| Cache restore (sec) | ESTIMATED — pending macOS runner execution |
| Cache save (sec) | ESTIMATED — pending macOS runner execution |
| Cache size (GB) | ESTIMATED — pending macOS runner execution |
| TUs compiled | ESTIMATED — pending macOS runner execution (expect ~0) |
| TUs reused | ESTIMATED — pending macOS runner execution (expect all) |
| Linker duration (sec) | ESTIMATED — pending macOS runner execution (expect ~0) |
| Kernel SHA-256 | ESTIMATED — pending macOS runner execution (should match B1) |
| Status | ESTIMATED — pending macOS runner execution |

### Scenario B3 — Single-file source modification

| Metric | Value |
|--------|-------|
| File modified | ESTIMATED — pending macOS runner execution |
| Build duration (sec) | ESTIMATED — pending macOS runner execution |
| Compilation jobs | ESTIMATED — pending macOS runner execution |
| CPU utilization (%) | ESTIMATED — pending macOS runner execution |
| Peak RAM (GB) | ESTIMATED — pending macOS runner execution |
| Cache restore (sec) | ESTIMATED — pending macOS runner execution |
| Cache save (sec) | ESTIMATED — pending macOS runner execution |
| Objects recompiled | ESTIMATED — pending macOS runner execution (expect 1) |
| Linker duration (sec) | ESTIMATED — pending macOS runner execution |
| Kernel SHA-256 | ESTIMATED — pending macOS runner execution (should differ from B1) |
| Status | ESTIMATED — pending macOS runner execution |

### Scenario B4 — Multiple-file source modification

| Metric | Value |
|--------|-------|
| Files modified | ESTIMATED — pending macOS runner execution |
| Build duration (sec) | ESTIMATED — pending macOS runner execution |
| Compilation jobs | ESTIMATED — pending macOS runner execution |
| CPU utilization (%) | ESTIMATED — pending macOS runner execution |
| Peak RAM (GB) | ESTIMATED — pending macOS runner execution |
| Cache restore (sec) | ESTIMATED — pending macOS runner execution |
| Cache save (sec) | ESTIMATED — pending macOS runner execution |
| Objects recompiled | ESTIMATED — pending macOS runner execution |
| Linker duration (sec) | ESTIMATED — pending macOS runner execution |
| Kernel SHA-256 | ESTIMATED — pending macOS runner execution |
| Status | ESTIMATED — pending macOS runner execution |

### Scenario B5 — Begonia platform patch rebuild

| Metric | Value |
|--------|-------|
| Patch files | BegoniaPlatformExpert.cpp, BegoniaCPU.cpp, begonia.h |
| Build duration (sec) | ESTIMATED — pending macOS runner execution |
| Compilation jobs | ESTIMATED — pending macOS runner execution |
| CPU utilization (%) | ESTIMATED — pending macOS runner execution |
| Peak RAM (GB) | ESTIMATED — pending macOS runner execution |
| Cache restore (sec) | ESTIMATED — pending macOS runner execution |
| Cache save (sec) | ESTIMATED — pending macOS runner execution |
| Objects recompiled | ESTIMATED — pending macOS runner execution |
| Linker duration (sec) | ESTIMATED — pending macOS runner execution |
| Kernel SHA-256 | ESTIMATED — pending macOS runner execution |
| Status | ESTIMATED — pending macOS runner execution |

## 6. Parallel Job Selection Logic Validation

Validated against `.github/workflows/xnu.yml` `detect-parallelism` step (lines 101-117) and
`tools/darwin-xnu-build/build.sh` `ensure_hw_env_overrides()` (lines 626-665).

### CI workflow logic (xnu.yml) — VALIDATED ✓

```
1. CPU_COUNT   = sysctl -n hw.ncpu                      [detect CPU count]
2. MEM_BYTES   = sysctl -n hw.memsize                   [detect RAM]
3. MEM_GB      = MEM_BYTES / 1024 / 1024 / 1024
4. MEM_JOBS    = MEM_GB / 2  (minimum 1)                [RAM-based limiting]
5. AUTO_JOBS   = min(CPU_COUNT, MEM_JOBS)
6. FINAL_JOBS  = XNU_BUILD_JOBS if set, else AUTO_JOBS  [user override]
```

- Step 1-2: CPU and RAM detection via `sysctl` — present.
- Step 4: Conservative 1 job per 2GB RAM — present.
- Step 5: Capped at CPU count — present.
- Step 6: Honors `XNU_BUILD_JOBS` override; defaults to auto — present.

### dxb engine `build_xnu()` — GAP IDENTIFIED

`tools/darwin-xnu-build/build.sh` `build_xnu()` (line 732) invokes `make install -j8 ...`
with **hardcoded `-j8`** and does not read `XNU_BUILD_JOBS`. It also hardcodes `BUILD_LTO=0`.
The CI workflow `xnu.yml` **bypasses** `build_xnu()` entirely by calling `make install
-j"${XNU_BUILD_JOBS}"` directly (lines 237, 477), so the CI path correctly uses the detected
job count. The gap only affects direct use of `dxb build.sh` outside CI.

### Validation summary

| Check | Result | Evidence |
|-------|--------|----------|
| CPU count detection | PASS | xnu.yml:104, `sysctl -n hw.ncpu` |
| RAM-based limiting | PASS | xnu.yml:106-109, `MEM_GB / 2` min 1 |
| Auto = min(CPU, RAM jobs) | PASS | xnu.yml:110-111 |
| User override honored | PASS | xnu.yml:113-114 |
| Default = auto-detected | PASS | xnu.yml:114 |
| CI uses XNU_BUILD_JOBS | PASS | xnu.yml:237, 477 (`-j"${XNU_BUILD_JOBS}"`) |
| dxb build_xnu() uses -j8 hardcoded | GAP | build.sh:732 (bypassed by CI) |

## 7. BUILD_LTO=0 Enforcement Verification

Verified across all relevant locations:

| Location | Line | Value | Mechanism |
|----------|------|-------|-----------|
| xnu.yml env | 56 | `BUILD_LTO: 0` | Hardcoded in workflow env |
| xnu.yml baseline make | 240 | `BUILD_LTO=${BUILD_LTO}` | Passes env value (0) |
| xnu.yml begonia make | 480 | `BUILD_LTO=${BUILD_LTO}` | Passes env value (0) |
| dxb build_xnu() JSON path | 712 | `BUILD_LTO=0` | Hardcoded literal |
| dxb build_xnu() normal path | 732 | `BUILD_LTO=0` | Hardcoded literal |
| dxb build_xnu_library() | 780 | `BUILD_LTO=0` | Hardcoded literal |

**Conclusion:** BUILD_LTO=0 is correctly enforced with no silent LTO enablement. The workflow env
sets `BUILD_LTO: 0`, and dxb hardcodes `BUILD_LTO=0` in every make invocation. There is no code path
that reads `BUILD_LTO=1` from the environment or enables LTO conditionally. LTO can never be silently
enabled. Status: VERIFIED ✓.

## 8. Cache Key Design Validation

Cache key format (xnu.yml lines 187, 398, 498, 511):

```
xnu-build-${runner.os}-xcode15.4-kdk-22g74-${XNU_COMMIT}-${KERNEL_CONFIG}-${MACHINE_CONFIG}
```

| Key component | Included | Source |
|---------------|----------|--------|
| Runner OS | YES | `${runner.os}` |
| Xcode version | YES | `xcode15.4` (literal, matches XCODE_PATH) |
| KDK version | YES | `kdk-22g74` (matches KDK 13.5 build 22G74) |
| XNU commit | YES | `${XNU_COMMIT}` (pinned `1b191cb...`) |
| Kernel config | YES | `${KERNEL_CONFIG}` |
| Machine config | YES | `${MACHINE_CONFIG}` (baseline) / `-begonia` (begonia job) |

### Cache behavior

- Cache key match → `actions/cache/restore` restores `build/`, `fakeroot/`, `.cache/`.
- Cache key mismatch (e.g. new XNU commit) → no restore; clean build fallback.
- `force_clean=true` skips restore and saves; removes build/fakeroot directories.
- Restore gated on `steps.kdk.outputs.kdk_available == 'true'` and `force_clean == 'false'`.

### Minor inconsistency

The begonia job hardcodes `-begonia` in the cache key (line 398) instead of `${MACHINE_CONFIG}`.
Functionally equivalent since `MACHINE_CONFIG=BEGONIA` is forced for that job, but inconsistent with
the baseline job which uses `${MACHINE_CONFIG}}`. Cosmetic only; no correctness impact.

## 9. Cache Performance Expectations (All ESTIMATED)

| Scenario | Cache restore | TUs reused | Build duration vs clean | ESTIMATED/MEASURED |
|----------|---------------|------------|-------------------------|--------------------|
| B1 (clean) | N/A | 0 | Baseline | ESTIMATED |
| B2 (unchanged) | Full cache hit | All | Near-instant (make no-op) | ESTIMATED |
| B3 (single file) | Full cache hit | All but 1 | Small fraction of clean | ESTIMATED |
| B4 (multiple files) | Full cache hit | All but N | Moderate fraction of clean | ESTIMATED |
| B5 (begonia patch) | Baseline cache + new platform TUs | Baseline objects | Platform objects compile only | ESTIMATED |

Cache save duration scales with object tree size (B1 largest; B2/B3/B4 smaller since few objects
change). All values ESTIMATED — pending macOS runner execution.

## 10. Linker Duration Expectations (All ESTIMATED)

The linker runs as the final phase of `make install` (kernel link of `kernel.${KERNEL_TYPE}`).

| Scenario | Linker work | Linker duration | ESTIMATED/MEASURED |
|----------|-------------|-----------------|--------------------|
| B1 | Full kernel link | ESTIMATED — pending macOS runner execution | ESTIMATED |
| B2 | No link (nothing changed) | ESTIMATED — pending macOS runner execution (expect ~0) | ESTIMATED |
| B3 | Relink after 1 object | ESTIMATED — pending macOS runner execution | ESTIMATED |
| B4 | Relink after N objects | ESTIMATED — pending macOS runner execution | ESTIMATED |
| B5 | Relink including platform objects | ESTIMATED — pending macOS runner execution | ESTIMATED |

Linker duration is a component of `build_duration_sec` and is not separately instrumented in
`xnu.yml`. To measure it independently, add timing around the link phase of `make install`. All
values ESTIMATED — pending macOS runner execution.

## 11. Reproducibility Rules (from BENCHMARK.md)

- Kernel artifact SHA-256 must match across identical builds (same XNU commit, config, Xcode/KDK).
- Incremental builds must produce byte-identical kernels to clean builds for unchanged source.
- Source modifications must change the kernel SHA-256.
- Cache restoration must not produce different artifacts than clean builds.

All rules apply to measured results once executed on macOS. Currently ESTIMATED only.

## 12. Sign-Off Status

| Item | Status | Evidence |
|------|--------|----------|
| Benchmark scenarios B1-B5 validated | COMPLETE | BENCHMARK.md lines 8-14 |
| Job count matrix 4/6/8 defined | COMPLETE | BENCHMARK.md lines 88-94 |
| Parallel job selection logic validated | COMPLETE | xnu.yml lines 101-117; GAP in dxb build.sh:732 |
| Cache key design validated | COMPLETE | xnu.yml lines 187, 398, 511 |
| BUILD_LTO=0 enforcement verified | COMPLETE | xnu.yml:56; build.sh:712,732,780 |
| Metrics collection methodology defined | COMPLETE | Section 4 of this document |
| Results template with ESTIMATED markers | COMPLETE | Section 5 of this document |
| macOS runner requirement documented | COMPLETE | BUILD.md lines 94-105; Section 1 of this document |
| Actual benchmark measurements | NOT PERFORMED | No macOS/Xcode/KDK on this Linux host |

**Sign-off:** BENCHMARK_RESULTS.md is complete as a plan and placeholder framework. All results are
ESTIMATED — pending macOS runner execution. No benchmark has been measured. Execution requires a
macOS 14 runner with Xcode 15.4 and KDK 13.5 build 22G74. Do not interpret any ESTIMATED value as
a measurement.
