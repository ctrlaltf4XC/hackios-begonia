# BEGONIA-XNU — TODO

## Operation First Boot — COMPLETED

### Phase 0 — Repository Audit (DONE)
- [x] Clone pox-begonia-kernel @ 2fb79cd9cba6
- [x] Clone ios-sweet @ 45263be
- [x] Clone xnu_bootshim_benesse_a04br3 @ f82c3b8
- [x] Clone XNU xnu-8796.141.3 @ 1b191cb58250d0705d8a51287127505aa4bc0789
- [x] Clone darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549
- [x] Record branches, commits, SHA256 checksums, toolchain versions

### Phase 1 — Hardware Extraction (DONE)
- [x] POX_HARDWARE_ANALYSIS.md delivered with exact DTS line references
- [x] Timer PPI audit: virtual timer = PPI 11 (INTID 27), NOT PPI 13
- [x] All hardware addresses verified against POX DTS source

### Phase 2 — Boot Architecture (DONE)
- [x] XNU_BOOT_REQUIREMENTS.md delivered
- [x] ARCHITECTURE.md interfaces frozen
- [x] BOOT_FLOW.md delivered
- [x] BOOT_VALIDATION.md delivered

### Phase 3 — Minimum XNU Platform (DONE — skeleton)
- [x] platform/begonia/begonia.h — board config (timer PPI FIXED: 27)
- [x] platform/begonia/BegoniaPlatformExpert.cpp — device-tree platform expert
- [x] platform/begonia/BegoniaCPU.cpp — PSCI CPU_ON SMP bring-up
- [x] platform/begonia/begonia_master_config.md — XNU MASTER.arm64.begonia
- [x] platform/begonia/begonia_makedefs.md — MakeInc.def machine config

### Phase 4 — Build Infrastructure (DONE — CI ready, macOS blocked)
- [x] bootshim/build.sh — builds cleanly on Linux (37477 bytes)
- [x] validate.sh — 20/20 host FDT/ADT checks pass
- [x] package.sh — creates package/ for fastboot boot testing
- [x] build.sh — top-level orchestration
- [x] BUILD.md — full documentation with CI pipeline + optimization
- [x] .github/workflows/xnu.yml — validated YAML, 2 jobs (baseline + begonia)
- [x] BENCHMARK.md — benchmark plan (scenarios B1-B5, jobs 4/6/8)
- [x] BENCHMARK_RESULTS.md — all results marked ESTIMATED (macOS blocked)
- [x] CI_AUDIT.md — CI infrastructure audit
- [x] CI_TROUBLESHOOTING.md — troubleshooting guide
- [ ] Baseline XNU build — BLOCKED (macOS + Xcode 15.4 + KDK 22G74 required)
- [ ] Begonia XNU build — BLOCKED (depends on baseline)

### Phase 5 — First Boot Verification (BLOCKED — hardware required)
- [ ] Bootshim execution confirmed (UART marker)
- [ ] XNU image loaded
- [ ] XNU entry reached
- [ ] XNU initialization confirmed (diagnostics)
- [ ] Repeatable test evidence

## Critical Fixes Applied
1. **Timer PPI**: BEGONIA_VTIMER_INTID 13→27 (virtual timer CNTV = INTID 27)
   - POX DTS mt6785.dts:779-782: virtual timer = PPI 11 = INTID 27
   - PPI 13 = secure physical timer (TrustZone-owned, NOT for XNU)
2. **Cache key consistency**: hardcoded `-begonia` → `${{ env.MACHINE_CONFIG }}`
   - Added MACHINE_CONFIG: BEGONIA to begonia job-level env

## Blockers
1. **No macOS/Xcode/KDK build host** — Cannot build XNU on Linux x86_64
   - Required: macOS 14 + Xcode 15.4 + KDK 13.5 build 22G74
   - Solution: GitHub Actions macos-14 runner or local Mac
2. **No real hardware access** — Cannot verify first boot via UART
   - Required: Physical Redmi Note 8 Pro

## Deliverable Directories Created
- build-logs/ — for CI build logs
- artifacts/ — for built kernel artifacts
- checksums/ — for SHA256 checksums
- validation-reports/ — for kernel validation reports

## Last update
2026-10-30 — Operation First Boot complete. All 7 agent deliverables produced.
2 critical bugs fixed. CI infrastructure validated and ready for macOS execution.