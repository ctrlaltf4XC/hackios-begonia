# BEGONIA-XNU — Project Status

Current phase: PHASE 3 — Minimum XNU Platform (platform/begonia/ skeleton complete; XNU 8796 source acquired — M5b blocked on macOS build host)

## Repositories audited
| Repo | Path | Commit | Branch |
|------|------|--------|--------|
| pox-begonia-kernel | pox-begonia-kernel/ | 2fb79cd9cba6 | main |
| ios-sweet | ios-sweet/ | 45263be | main |
| xnu_bootshim_benesse_a04br3 | xnu_bootshim_benesse_a04br3/ | f82c3b8 | main |
| apple-oss-distributions/xnu | xnu/xnu-8796/ | 1b191cb58250d0705d8a51287127505aa4bc0789 | xnu-8796.141.3 |
| blacktop/darwin-xnu-build | tools/darwin-xnu-build/ | 663906528d1cd5ea8fde7de6f16de73db9511549 | main |

## Milestones
- [x] M0: Repo audit complete
- [x] M1: POX_HARDWARE_ANALYSIS.md delivered
- [x] M2: XNU_BOOT_REQUIREMENTS.md delivered
- [x] M3: ARCHITECTURE.md interfaces frozen
- [x] M4: bootshim/ implemented + BOOT_FLOW.md + BOOT_VALIDATION.md
  - bootshim builds cleanly (37477 bytes, AArch64 ELF, ARM\x64 header)
  - Host FDT parser + ADT builder: 20/20 checks passed
  - build.sh, validate.sh, package.sh operational
- [x] M5a: platform/begonia/ skeleton delivered
  - begonia.h, BegoniaPlatformExpert.cpp, BegoniaCPU.cpp
  - begonia_master_config.md, begonia_makedefs.md
- [x] M5b-pre: XNU 8796 source acquired (xnu-8796.141.3 @ 1b191cb) + patch compatibility matrix (PATCH_COMPATIBILITY.md) + XNU_SOURCE_ACQUISITION.md
- [ ] M5b: Apply skeletons + adapted patches to XNU 8796 source — BLOCKED (macOS build host required)
- [ ] M6: Build baseline XNU on macOS — BLOCKED (no macOS/Xcode/KDK on this Linux host)
- [ ] M7: First XNU boot verified via UART evidence — BLOCKED (hardware access required)

## Blockers
1. **No macOS/Xcode/KDK build host** — Baseline XNU build cannot run on this Linux x86_64
   host (needs Xcode 15.x, mig, KDK from Apple Developer Portal). Documented in BUILD.md.
2. **No real hardware access** — First boot verification via UART requires physical
   Redmi Note 8 Pro. Cannot be done in this environment.

## Last update
2026-10-30 — XNU 8796.141.3 source acquired and pinned (M5b-pre complete). Patch compatibility
matrix done: 0004/0006/0016 apply cleanly; 0017/0018/0065 need adaptation (order deps + context
mismatches). Baseline build blocked on macOS host.

