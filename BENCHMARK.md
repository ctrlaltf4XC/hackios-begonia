# BENCHMARK.md — XNU 8796 Build Performance Benchmark

Benchmark plan and results for the optimized XNU 8796.141.3 build pipeline.
All benchmarks run on GitHub-hosted macOS 14 with Xcode 15.4.

## Benchmark Scenarios

| Scenario | Description | Expected Behavior |
|----------|-------------|-------------------|
| B1 | Clean baseline build | Full compilation from scratch, no cache |
| B2 | Second unchanged build | Incremental, no source changes, cache hit |
| B3 | Single-file source modification | One `.c` file touched, minimal recompile |
| B4 | Multiple-file source modification | Several `.c`/`.h` files touched, moderate recompile |
| B5 | Begonia platform patch rebuild | platform/begonia/ sources added, XNU rebuilt |

## Metrics Collected

| Metric | Source |
|--------|--------|
| Clean build duration | `steps.xnu-build.outputs.build_duration_sec` |
| Incremental build duration | `steps.xnu-build.outputs.build_duration_sec` |
| Cache restore duration | `steps.cache-restore.outputs.duration_sec` |
| Cache save duration | `steps.cache-save.outputs.duration_sec` |
| Compilation job count | `steps.detect-parallelism.outputs.build_jobs` |
| Build artifact checksums | `steps.checksums.outputs.kernel_checksum` |
| CPU utilization | Runner metrics (GitHub) |
| Peak RAM consumption | Runner metrics (GitHub) |

## Results Template

### Scenario B1 — Clean baseline build

| Metric | Value |
|--------|-------|
| Build duration (sec) | TBD |
| Compilation jobs | TBD |
| CPU utilization (%) | TBD |
| Peak RAM (GB) | TBD |
| Cache restore (sec) | N/A (clean) |
| Cache save (sec) | TBD |
| Kernel SHA-256 | TBD |
| Xcode version | 15.4 |
| KDK version | 22G74 |
| XNU commit | 1b191cb58250d0705d8a51287127505aa4bc0789 |

### Scenario B2 — Second unchanged build (incremental, cache hit)

| Metric | Value |
|--------|-------|
| Build duration (sec) | TBD |
| Compilation jobs | TBD |
| CPU utilization (%) | TBD |
| Peak RAM (GB) | TBD |
| Cache restore (sec) | TBD |
| Cache save (sec) | TBD |
| Kernel SHA-256 | TBD (should match B1) |

### Scenario B3 — Single-file source modification

| Metric | Value |
|--------|-------|
| File modified | TBD |
| Build duration (sec) | TBD |
| Compilation jobs | TBD |
| Objects recompiled | TBD |
| Kernel SHA-256 | TBD (should differ from B1) |

### Scenario B4 — Multiple-file source modification

| Metric | Value |
|--------|-------|
| Files modified | TBD |
| Build duration (sec) | TBD |
| Compilation jobs | TBD |
| Objects recompiled | TBD |
| Kernel SHA-256 | TBD |

### Scenario B5 — Begonia platform patch rebuild

| Metric | Value |
|--------|-------|
| Patch files | BegoniaPlatformExpert.cpp, BegoniaCPU.cpp, begonia.h |
| Build duration (sec) | TBD |
| Compilation jobs | TBD |
| Objects recompiled | TBD |
| Kernel SHA-256 | TBD |

## Job Count Benchmark

| Jobs | Clean Duration (sec) | Incremental Duration (sec) | Speedup vs 4x |
|------|---------------------|---------------------------|---------------|
| 4 | TBD | TBD | 1.0x |
| 6 | TBD | TBD | TBD |
| 8 | TBD | TBD | TBD |

## Optimization Objectives — Implementation Status

| Objective | Implementation | Status |
|-----------|---------------|--------|
| 1. Configurable XNU_BUILD_JOBS | `XNU_BUILD_JOBS` env var, auto-detected if empty | Done |
| 2. Benchmark 4/6/8 jobs | `xnu_build_jobs` workflow_dispatch input | Done |
| 3. Preserve build/xnu.obj and build/xnu.sym | Cached via actions/cache, never cleaned | Done |
| 4. Preserve fakeroot and dependency outputs | Cached via actions/cache | Done |
| 5. Correct incremental rebuild | `make` called directly (bypasses dxb guard) | Done |
| 6. Fix existing-kernel output guard | Guard bypassed; make handles timestamp deps | Done |
| 7. Never delete intermediate objects | No `rm -rf build/` unless force_clean=true | Done |
| 8. GitHub Actions caching | actions/cache/restore + actions/cache/save | Done |
| 9. Pinned cache keys | Includes runner OS, Xcode, KDK, XNU commit, config | Done |
| 10. Avoid corrupt cache restoration | Cache key mismatch = no restore; clean build fallback | Done |
| 11. Cache dependency repos and headers | fakeroot/ and .cache/ included in cache | Done |
| 12. Skip Kernel Collection unless requested | `build_kc` input defaults to false | Done |
| 13. Skip CodeQL and analysis | CodeQL not invoked in workflow | Done |
| 14. Avoid repeated KDK installation | KDKROOT check skips reinstall | Done |
| 15. Per-stage build timing | `date +%s` around build step | Done |
| 16. CPU/RAM detection | `sysctl hw.ncpu`, `sysctl hw.memsize` | Done |
| 17. BUILD_LTO=0 | Hardcoded in env, no benchmark exception | Done |
| 18. Preserve XNU revision and patch order | XNU tag/commit pinned, no patches in baseline | Done |

## Reproducibility Rules

- Kernel artifact SHA-256 must match across identical builds (same XNU commit, same config, same Xcode/KDK)
- Incremental builds must produce byte-identical kernels to clean builds for unchanged source
- Source modifications must change the kernel SHA-256
- Cache restoration must not produce different artifacts than clean builds

## Running Benchmarks Locally

```sh
# Clean baseline build
./build.sh --clean xnu

# Second unchanged build (incremental)
./build.sh xnu

# Single-file modification
touch osfmk/arm64/sleh.c
./build.sh xnu

# Multiple-file modification
touch osfmk/arm64/sleh.c pexpert/arm/pe_fiq.c
./build.sh xnu

# Begonia platform rebuild
./build.sh all  # MACHINE_CONFIG=BEGONIA
```

## Notes

- All benchmarks must be run on the same runner type (macOS 14) for comparability
- Cache warm-up affects B2; run B2 twice if measuring steady-state incremental performance
- Do not sacrifice correctness: verify kernel SHA-256 consistency across scenarios
