# CI_AUDIT.md — GitHub Actions CI Infrastructure Audit

BEGONIA-XNU Operation First Boot — CI Architect (Agent 1)
Repository: /home/yu/BEGONIA-XNU
Branch: ci/first-boot-validation
XNU: xnu-8796.141.3 @ 1b191cb58250d0705d8a51287127505aa4bc0789
Build engine: blacktop/darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549

---

## Executive Summary

The CI workflow `.github/workflows/xnu.yml` is syntactically valid and structurally sound with two properly ordered jobs:
- `xnu-baseline`: builds unmodified XNU 8796.141.3 (no patches)
- `xnu-begonia`: builds XNU with Begonia platform integration (depends on baseline)

Two critical fixes were applied during this audit:
1. **Timer PPI corrected**: `BEGONIA_VTIMER_INTID` changed from 13 (secure physical timer) to 27 (virtual timer INTID) per POX DTS `mt6785.dts:779-782`.
2. **Cache key consistency**: Both baseline and begonia jobs now use `${{ env.MACHINE_CONFIG }}` instead of hardcoded `-begonia`.

The XNU build **cannot execute on this Linux x86_64 host** — it requires macOS 14 + Xcode 15.4 + KDK 13.5 build 22G74. All macOS-dependent validations are marked "pending macOS runner execution."

---

## Workflow Structure Overview

```
.github/workflows/xnu.yml
├── on: workflow_dispatch
│   ├── kernel_config (default: DEVELOPMENT)
│   ├── machine_config (default: VMAPPLE)
│   ├── xnu_build_jobs (default: auto-detect)
│   ├── build_kc (default: false)
│   ├── keep_going (default: true)
│   └── force_clean (default: false)
├── env (top-level):
│   ├── DXB_REPO, DXB_SHA (pinned build engine)
│   ├── XNU_TAG, XNU_COMMIT (pinned XNU source)
│   ├── XCODE_PATH (Xcode 15.4 explicit path)
│   ├── MACOS_VERSION: 13.5
│   ├── KDK_NAME, KDKROOT (KDK 22G74)
│   ├── BUILD_LTO: 0, BUILD_WERROR: 0
│   └── KERNEL_CONFIG, MACHINE_CONFIG, ARCH_CONFIG: ARM64
├── Job: xnu-baseline
│   ├── runs-on: macos-14
│   ├── needs: none
│   ├── Steps: checkout → Xcode select → tool verify → KDK inspect → DXB fetch → XNU clone+verify → brew deps → KDK install → cache restore → force clean → build → checksums → cache save → upload artifact
│   └── Outputs: build_jobs, clean_duration_sec, cache_restore_duration_sec, cache_save_duration_sec
└── Job: xnu-begonia
    ├── runs-on: macos-14
    ├── needs: xnu-baseline
    ├── if: needs.xnu-baseline.result == 'success'
    ├── env: MACHINE_CONFIG: BEGONIA
    ├── Steps: checkout → Xcode select → tool verify → KDK inspect → DXB fetch → XNU clone+verify → brew deps → KDK install → cache restore → force clean → detect parallelism → integrate Begonia platform → build → checksums → cache save → upload artifact
    └── Outputs: build_jobs, build_duration_sec, kernel_checksum
```

---

## Findings

### F-001: XNU Build Cannot Run on Linux Host (BLOCKER)

**Severity:** CRITICAL

The current execution environment is Linux x86_64. The DXB build engine requires:
- `xcodebuild`, `xcrun`, `mig`, `clang` (Apple), `ld` (Apple), `kmutil`, `hdiutil`
- macOS SDK/KDK headers
- Kernel Debug Kit 13.5 build 22G74

None of these are present on the Linux host. Verified by Agent 2 (CI_TOOLCHAIN_VALIDATION.md): all macOS-specific tools are absent; Xcode 15.4 path and KDKROOT do not exist.

**Recommendation:** Execute CI on GitHub Actions `macos-14` runner (workflow already configured) or a local Mac with macOS 13.5 + Xcode 15.4 + KDK 22G74.

---

### F-002: Timer PPI Corrected (FIXED)

**Severity:** CRITICAL (was), RESOLVED

`platform/begonia/begonia.h:63` previously set `BEGONIA_VTIMER_INTID = 13`, which is the INTID for PPI 13 (secure physical timer, TrustZone-owned per POX DTS `mt6785.dts:779`).

**Correction:** Changed to `BEGONIA_VTIMER_INTID = 27` (INTID for PPI 11, the virtual timer CNTV that XNU's `pe_fiq.c:148` already enables via `(1 << 27)`).

**Verification:** POX DTS `mt6785.dts:779-782` lists interrupts in order [secure physical PPI 13, non-secure physical PPI 14, virtual PPI 11, hypervisor PPI 10]. PPI 11 = INTID 16 + 11 = 27.

---

### F-003: Cache Key Consistency (FIXED)

**Severity:** HIGH (was), RESOLVED

The `xnu-begonia` job's cache-save step used a hardcoded `-begonia` suffix while cache-restore used `${{ env.MACHINE_CONFIG }}`. This meant restore and save could use different keys.

**Correction:** Added `MACHINE_CONFIG: BEGONIA` to the `xnu-begonia` job-level env, and changed both cache-restore and cache-save to use `${{ env.MACHINE_CONFIG }}`. Now both steps consistently resolve to "begonia" for the begonia job.

---

### F-004: Redundant Steps Across Jobs

**Severity:** MEDIUM

Both jobs duplicate the following steps:
- Checkout BEGONIA-XNU repository
- Select Xcode 15.4
- Verify required build tools
- Inspect Kernel Debug Kits
- Fetch pinned darwin-xnu-build engine
- Clone and verify exact XNU source commit
- Install Homebrew dependencies
- Install KDK

This redundancy is inherent to GitHub Actions' independent job filesystems — each job needs its own workspace. No actionable fix without shared persistent runners.

**Recommendation:** Accept redundancy for correctness; consider self-hosted persistent macOS runner (Runner Strategy B) to eliminate redundant clones and KDK installs.

---

### F-005: Cache Namespace Independence

**Severity:** LOW (verified correct)

Cache keys use the format:
```
xnu-build-${runner.os}-xcode15.4-kdk-22g74-${XNU_COMMIT}-${KERNEL_CONFIG}-${MACHINE_CONFIG}
```

Both jobs use the same key format but with different `MACHINE_CONFIG` values (VMAPPLE vs BEGONIA), creating independent cache namespaces. No cross-contamination risk.

---

### F-006: Secret/Security Review

**Severity:** LOW (verified correct)

The workflow:
- Never references `secrets.` context
- Never prints credentials
- KDK download uses public dortania KdkSupportPkg manifest (no credentials in URL)
- No Apple Developer credentials required for XNU source (public mirror)
- `actions/checkout@v4` uses default token (read-only repository access)

**Verdict:** No security issues identified.

---

### F-007: Runner Compatibility

**Severity:** MEDIUM

- `macos-14` runner is currently available on GitHub Actions
- Xcode 15.4 must be explicitly selected via `xcode-select -s`
- macOS 14 runner has an approaching retirement date — verify availability before executing
- If `macos-14` is retired, Runner Strategy C (newer macOS runner) requires independent toolchain/KDK compatibility verification

**Recommendation:** Check GitHub runner availability before workflow execution. If unavailable, fail with actionable instructions (per project rule 5).

---

### F-008: Reproducibility Assessment

**Severity:** LOW (verified correct)

- XNU commit verified before build (fails on mismatch)
- Build engine commit pinned
- Xcode version explicitly selected (15.4)
- KDK version pinned (22G74)
- `BUILD_LTO=0` hardcoded
- Cache keys include all version dimensions
- No silent substitution of any component

---

### F-009: Job Ordering

**Severity:** LOW (verified correct)

- `xnu-begonia` has `needs: xnu-baseline`
- `xnu-begonia` has `if: needs.xnu-baseline.result == 'success'`
- Baseline must succeed before integration builds

**Verdict:** Correct.

---

### F-010: Artifact Sharing

**Severity:** MEDIUM

GitHub Actions jobs have independent filesystems. The workflow uses `actions/upload-artifact@v4` for:
- `xnu-baseline-build` (baseline job)
- `xnu-build-begonia` (begonia job)

No `download-artifact` steps exist — artifacts are uploaded for external retrieval, not cross-job consumption. This is correct for the two-stage architecture where each job independently clones and builds.

---

## Required Fixes

| Fix | Location | Status |
|-----|----------|--------|
| Timer PPI 13 → 27 | `platform/begonia/begonia.h:63` | APPLIED |
| Cache key `-begonia` → `${MACHINE_CONFIG}` | `.github/workflows/xnu.yml` | APPLIED |
| Add `MACHINE_CONFIG: BEGONIA` to begonia job env | `.github/workflows/xnu.yml` | APPLIED |
| Baseline XNU build on macOS | Pending | BLOCKED (no macOS runner) |
| Begonia XNU build on macOS | Pending | BLOCKED (depends on baseline) |

---

## Runner Strategy

Per project requirements:

**A. Current compatible hosted runner (macOS-14):** Primary strategy. Workflow already configured. Verify availability before execution.

**B. Persistent self-hosted macOS Sonoma 14 runner with Xcode 15.4 + KDK 22G74:** Recommended for iterative development. Eliminates redundant clones/KDK installs across jobs.

**C. Experimental newer macOS runner:** Only after independent toolchain and KDK compatibility verification. Do NOT silently substitute Xcode 16 or newer XNU toolchain.

---

## Sign-Off

| Item | Status |
|------|--------|
| Workflow YAML validates | PASS |
| Job ordering correct | PASS |
| Cache namespaces independent | PASS |
| No credentials exposed | PASS |
| Timer PPI corrected | FIXED |
| Cache key consistency | FIXED |
| XNU build executable | BLOCKED (macOS runner required) |
| Baseline build verified | BLOCKED (macOS runner required) |
| Begonia build verified | BLOCKED (macOS runner required) |

**Overall: INFRASTRUCTURE READY — awaiting macOS runner execution.**
