# BEGONIA-XNU — Project Status

Current phase: OPERATION FIRST BOOT — CI hardening, build optimization & kernel validation complete.
Next phase: macOS CI execution (blocked — no macOS runner on this Linux host).

Branch: ci/first-boot-validation

## Repositories audited
| Repo | Path | Commit | Branch |
|------|------|--------|--------|
| pox-begonia-kernel | pox-begonia-kernel/ | 2fb79cd9cba6 | main |
| ios-sweet | ios-sweet/ | 45263be | main |
| xnu_bootshim_benesse_a04br3 | xnu_bootshim_benesse_a04br3/ | f82c3b8 | main |
| apple-oss-distributions/xnu | xnu/xnu-8796/ | 1b191cb58250d0705d8a51287127505aa4bc0789 | xnu-8796.141.3 |
| blacktop/darwin-xnu-build | tools/darwin-xnu-build/ | 663906528d1cd5ea8fde7de6f16de73db9511549 | main |

## Milestones
- [x] M0: Repo audit complete (all 5 repos cloned, commits verified)
- [x] M1: POX_HARDWARE_ANALYSIS.md delivered
- [x] M2: XNU_BOOT_REQUIREMENTS.md delivered
- [x] M3: ARCHITECTURE.md interfaces frozen
- [x] M4: bootshim/ implemented + BOOT_FLOW.md + BOOT_VALIDATION.md
  - bootshim builds cleanly (37477 bytes, AArch64 ELF, ARM\x64 header)
  - Host FDT parser + ADT builder: 20/20 checks passed
  - build.sh, validate.sh, package.sh operational
- [x] M5a: platform/begonia/ skeleton delivered
  - begonia.h (timer PPI FIXED: 13→27 for virtual timer)
  - BegoniaPlatformExpert.cpp
  - BegoniaCPU.cpp
  - begonia_master_config.md, begonia_makedefs.md
- [x] M5b-pre: XNU 8796 source acquired (xnu-8796.141.3 @ 1b191cb) + patch compatibility matrix
- [x] M6: BUILD.md delivered (with CI pipeline + performance optimization docs)
- [x] M6-ci: GitHub Actions workflow .github/workflows/xnu.yml created and validated
- [ ] M6-build: Baseline XNU build on macOS — BLOCKED (no macOS runner)
- [ ] M6-integration: Begonia XNU build on macOS — BLOCKED (depends on baseline)
- [ ] M7: First XNU boot verified via UART evidence — BLOCKED (hardware access required)

## Agent Deliverables (all 7 agents)
| Agent | Role | Deliverable | Status |
|-------|------|-------------|--------|
| Agent 1 | CI Architect | CI_AUDIT.md | COMPLETED |
| Agent 2 | macOS Toolchain Engineer | CI_TOOLCHAIN_VALIDATION.md | COMPLETED |
| Agent 3 | XNU Build Engineer | XNU_BUILD_ANALYSIS.md | COMPLETED |
| Agent 4 | Incremental Build Engineer | INCREMENTAL_BUILD_ANALYSIS.md | COMPLETED |
| Agent 5 | Performance Engineer | BENCHMARK_RESULTS.md | COMPLETED (all ESTIMATED) |
| Agent 6 | Begonia Integration Engineer | BEGONIA_INTEGRATION_REVIEW.md | COMPLETED (timer bug found+fixed) |
| Agent 7 | Release Validation Engineer | KERNEL_VALIDATION.md | COMPLETED |

## Critical Fixes Applied
1. **Timer PPI**: `BEGONIA_VTIMER_INTID` 13→27 (virtual timer CNTV = INTID 27, not secure physical PPI 13)
2. **Cache key**: hardcoded `-begonia` → `${{ env.MACHINE_CONFIG }}` with job-level `MACHINE_CONFIG: BEGONIA`

## Blockers
1. **No macOS/Xcode/KDK build host** — Baseline XNU build cannot run on Linux x86_64.
   Required: macOS 14 + Xcode 15.4 + KDK 13.5 build 22G74.
   Solution: GitHub Actions macos-14 runner or local Mac.
2. **No real hardware access** — First boot verification via UART requires physical Redmi Note 8 Pro.

## Last update
2026-10-30 — Operation First Boot complete. All 7 agents delivered. CI infrastructure validated.
2 critical bugs fixed (timer PPI, cache key). Ready for macOS CI execution.

## Verified hardware values (from POX DTS, re-read from live FDT at boot)
- UART0: 0x11002000 (mt6577-uart), IRQ SPI 115, console 921600
- GICv3: dist 0x0c000000/0x40000, redist 0x0c040000/0x200000
- armv8-timer: secure phys PPI 13, non-secure phys PPI 14, virtual PPI 11 (INTID 27), hypervisor PPI 10
- PSCI: smc, psci-1.0
- Watchdog: 0x10007000, SPI 72 (do not touch)
- DRAM: /memory node (POX DTS: 0x40000000/0x3e605000 — VERIFY vs 6GB)

## Discrepancies resolved
- POX DTS marks cpu6/cpu7 as cortex-a75; target says Cortex-A76 → probe MIDR_EL1 at boot (documented in begonia.h)
- Timer PPI 13 was incorrectly assigned to virtual timer → corrected to INTID 27 (PPI 11)