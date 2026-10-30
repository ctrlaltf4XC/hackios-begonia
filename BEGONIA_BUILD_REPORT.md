# BEGONIA-XNU Build Report — XNU 8796.141.3 on Begonia (MT6785)

**Report date:** 2026-10-30T15:40:00Z
**Repository:** https://github.com/ctrlaltf4XC/hackios-begonia
**Branch:** ci/first-boot-validation
**Workflow:** .github/workflows/xnu.yml (workflow ID: 395447326)
**Agent:** Agent 4 — Begonia Platform Build Engineer

---

## Build Result: NOT EXECUTED

The `xnu-begonia` job was **never executed**. The `xnu-baseline` dependency job did not succeed, so `xnu-begonia` was skipped in every workflow run.

| Run ID | URL | Job | Status | Conclusion | Duration |
|--------|-----|-----|--------|------------|----------|
| 41076237577 | https://github.com/ctrlaltf4XC/hackios-begonia/actions/runs/41076237577 | xnu-baseline (VMAPPLE) | completed | **failure** | ~1 min |
| 41077222355 | https://github.com/ctrlaltf4XC/hackios-begonia/actions/runs/41077222355 | xnu-baseline (VMAPPLE) | completed | **cancelled** | ~15 min |
| 41079807007 | https://github.com/ctrlaltf4XC/hackios-begonia/actions/runs/41079807007 | xnu-baseline (VMAPPLE) | completed | **cancelled** | ~15 min |
| 41076237577 | https://github.com/ctrlaltf4XC/hackios-begonia/actions/runs/41076237577 | xnu-begonia | completed | skipped | — |
| 41077222355 | https://github.com/ctrlaltf4XC/hackios-begonia/actions/runs/41077222355 | xnu-begonia | completed | skipped | — |
| 41079807007 | https://github.com/ctrlaltf4XC/hackios-begonia/actions/runs/41079807007 | xnu-begonia | completed | skipped | — |

**Reason:** The `xnu-begonia` job has `needs: xnu-baseline` with `if: needs.xnu-baseline.result == 'success'`. The baseline job never reached a successful conclusion:
- **Run 1:** Failed at the build step due to an `SRCROOT` path bug (fixed — see below).
- **Runs 2–3:** Cancelled after ~15 minutes due to macOS 14 runner reclamation during the active brownout window (Oct 30 14:00 – Oct 31 00:00 UTC). Logs for cancelled runs are unavailable (BlobNotFound).

---

## Fix Applied: SRCROOT Path Bug

**Root cause:** In `.github/workflows/xnu.yml`, both the `xnu-baseline` and `xnu-begonia` build steps set `working-directory: BEGONIA-XNU/tools/dxb` but defined `SRCROOT="BEGONIA-XNU/tools/dxb/xnu"` — a path relative to the repository root, not the working directory. The `cd "${SRCROOT}"` command therefore resolved to `BEGONIA-XNU/tools/dxb/BEGONIA-XNU/tools/dxb/xnu`, which does not exist, causing:

```
cd: BEGONIA-XNU/tools/dxb/xnu: No such file or directory
```

**Fix:** Changed `SRCROOT="BEGONIA-XNU/tools/dxb/xnu"` to `SRCROOT="${WORK_DIR}/xnu"` (absolute path via the existing `WORK_DIR` env var) in both jobs.

**Files changed:** `.github/workflows/xnu.yml` (2 occurrences, 1 line each)
**Commits:**
- `main`: d21c3a4 — pushed to origin/main
- `ci/first-boot-validation`: f78a865 — pushed to origin/ci/first-boot-validation

---

## Runner and Toolchain Information

From Run 1 logs (the only run that produced output before failure):

| Field | Value |
|-------|-------|
| Runner image | macos-14-arm64 |
| Runner image version | 20260831.0302.1 |
| OS | macOS 14.8.9 (23J631) |
| Azure region | westus |
| Xcode | 15.4 (Build version 15F31d) |
| Xcode path | /Applications/Xcode_15.4.app/Contents/Developer |
| clang | /Applications/Xcode_15.4.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang |
| ld | /Applications/Xcode_15.4.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ld |
| mig | /Applications/Xcode_15.4.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/mig |
| kmutil | /usr/bin/kmutil |
| hdiutil | /usr/bin/hdiutil |
| CPU cores | 3 |
| RAM | 7 GB |
| Build jobs | 4 |

---

## KDK (Kernel Debug Kit)

| Field | Value |
|-------|-------|
| KDK name | Kernel Debug Kit 13.5 build 22G74 |
| KDK path | /Library/Developer/KDKs/KDK_13.5_22G74.kdk |
| Source | dortania KdkSupportPkg manifest |
| RC_DARWIN_KERNEL_VERSION | 22.6.0 |
| Status | Successfully installed (verified in Run 1) |

The KDK download and installation completed successfully in Run 1 (~90 seconds for ~300MB DMG).

---

## XNU Source

| Field | Value |
|-------|-------|
| Repository | https://github.com/apple-oss-distributions/xnu.git |
| Tag | xnu-8796.141.3 |
| Commit | 1b191cb58250d0705d8a51287127505aa4bc0789 |
| Commit verified | Yes (Run 1, line 334) |

---

## Build Configuration

| Variable | Value |
|----------|-------|
| MACHINE_CONFIG | VMAPPLE (baseline), BEGONIA (begonia job, hardcoded at job level) |
| KERNEL_CONFIG | DEVELOPMENT |
| ARCH_CONFIG | ARM64 |
| XNU_BUILD_JOBS | 4 |
| build_kc | false |
| keep_going | false |
| force_clean | true |
| BUILD_LTO | 0 |
| BUILD_WERROR | 0 |
| SDKROOT | macosx |

---

## Patch Order Verification

Intended patch sequence (per mission specification):

| Order | Patch | Status |
|-------|-------|--------|
| 1 | 0004 | Not applied (not in workflow) |
| 2 | 0006 | Not applied (not in workflow) |
| 3 | 0016 | Not applied (not in workflow) |
| 4 | 0018 | Not applied (not in workflow) |
| 5 | 0065 | Not applied (not in workflow) |

**Note:** Patch 0017 is NOT applied — it requires a Begonia-specific rewrite.

**Important:** The workflow does not apply numbered patches (0004, 0006, etc.). Instead, the `xnu-begonia` job integrates the `platform/begonia/` sources directly into the XNU source tree:
- `platform/begonia/BegoniaPlatformExpert.cpp` → `iokit/Kernel/arm/BegoniaPlatformExpert.cpp`
- `platform/begonia/BegoniaCPU.cpp` → `iokit/Kernel/arm/BegoniaCPU.cpp`
- `platform/begonia/begonia.h` → `pexpert/pexpert/arm64/begonia.h`
- Entries added to `iokit/conf/files.arm64` (optional iokitcpp)
- Board config defines added to `pexpert/pexpert/arm64/board_config.h`

Because the baseline job never succeeded, the begonia integration step never ran.

---

## Timer Requirements Verification

| Requirement | Value | Source | Verified |
|-------------|-------|--------|----------|
| Virtual timer PPI | 11 | POX DTS mt6785.dts:779-782 | Yes (POX_HARDWARE_ANALYSIS.md) |
| GIC INTID (virtual timer) | 27 | PPI 11 → INTID 27 | Yes |
| Secure physical timer INTID | 29 (PPI 13) | NOT used as virtual timer | Yes — correctly excluded |

The `begonia.h` board config was previously fixed: `BEGONIA_VTIMER_INTID` corrected from 13→27 (virtual timer CNTV = INTID 27). PPI 13 is the secure physical timer (TrustZone-owned) and must not be used as the XNU virtual timer.

**Status:** Not verified in build — begonia job never executed.

---

## Compiler/Linker Output Summary

**No compiler or linker output was produced.** The XNU build never reached compilation:

- **Run 1:** Failed at `cd "${SRCROOT}"` — path bug prevented entering the XNU source directory. No `make` output.
- **Runs 2–3:** Cancelled after ~15 minutes (runner reclamation during brownout). Logs unavailable.

The following build stages completed successfully in Run 1 before the failure:
1. Repository checkout (ci/first-boot-validation @ c411cfb)
2. Xcode 15.4 selection
3. Build tool verification (xcodebuild, xcrun, mig, clang, ld, kmutil, hdiutil)
4. KDK download and installation (Kernel Debug Kit 13.5 build 22G74)
5. XNU source clone and commit verification (1b191cb58250d0705d8a51287127505aa4bc0789)
6. Homebrew dependency installation (jq, gum, bash, cmake, ninja)
7. Force clean (build/fakeroot directories removed)

---

## Kernel Artifact

**No kernel artifact was produced.** The build never completed, so:
- No `kernel.development.begonia` binary
- No SHA256 checksum
- No artifact uploaded

Expected artifact path (had the build succeeded):
`BEGONIA-XNU/tools/dxb/build/xnu.obj/kernel.development.begonia`

---

## Mach-O Validation

**Not performed.** No kernel artifact exists to validate.

Expected validation (had the build succeeded):
- Architecture: arm64
- CPU subtype: ARM64_ALL (or specific CPU subtype)
- Entry point: XNU kernel entry
- File format: Mach-O 64-bit executable (arm64)

---

## Platform Integration Verification

**Not performed.** The `xnu-begonia` job was skipped because `xnu-baseline` did not succeed.

The following platform integrations were intended (defined in `.github/workflows/xnu.yml` lines 432-456):
- **Platform Expert:** `BegoniaPlatformExpert.cpp` → `iokit/Kernel/arm/`
- **CPU Init:** `BegoniaCPU.cpp` → `iokit/Kernel/arm/` (PSCI CPU_ON SMP bring-up)
- **GIC:** Board config defines in `pexpert/pexpert/arm64/board_config.h`
- **Timer:** Virtual timer INTID 27 (PPI 11), NOT secure physical timer INTID 29 (PPI 13)

---

## Build Duration

| Run | Start | End | Duration | Result |
|-----|-------|-----|----------|--------|
| 1 | 14:57:29Z | 14:58:49Z | ~80s | Failed (SRCROOT bug) |
| 2 | 15:03:52Z | 15:18:53Z | ~15 min | Cancelled (brownout) |
| 3 | 15:20:33Z | 15:35:34Z | ~15 min | Cancelled (brownout) |

**Successful build duration:** N/A — no successful build.

---

## Infrastructure Blocker: macOS 14 Runner Brownout

The macOS 14 runner brownout window (Oct 30 14:00 – Oct 31 00:00 UTC) was active during all three workflow runs. This caused:

1. Extended queue times (~12 minutes for runs 2–3 vs ~5 seconds for run 1)
2. Runner reclamation after ~15 minutes of job execution (runs 2–3 cancelled)
3. Unavailable logs for cancelled jobs (BlobNotFound)

Per `PROJECT_STATUS.md`: "Wait until after Oct 31 00:00 UTC, then dispatch before Nov 2 2026 (when macOS 14 retires)."

---

## Sign-Off Status

| Item | Status |
|------|--------|
| SRCROOT path bug fix | COMPLETED (pushed to main and ci/first-boot-validation) |
| xnu-baseline build | NOT EXECUTED (cancelled by runner reclamation during brownout) |
| xnu-begonia build | NOT EXECUTED (skipped — baseline dependency not met) |
| Kernel artifact | NOT PRODUCED |
| SHA256 checksum | NOT GENERATED |
| Mach-O validation | NOT PERFORMED |
| Platform integration verification | NOT PERFORMED |

**Overall sign-off:** BLOCKED — infrastructure issue (macOS 14 runner brownout). The SRCROOT bug has been fixed and the workflow is ready to re-dispatch after the brownout window closes (after Oct 31 00:00 UTC, before Nov 2 2026 when macOS 14 runners retire).

**Recommended next action:** Re-dispatch `xnu.yml` workflow_dispatch on `ci/first-boot-validation` with:
- kernel_config=DEVELOPMENT
- machine_config=VMAPPLE
- xnu_build_jobs=4
- build_kc=false
- keep_going=false
- force_clean=true

After Oct 31 00:00 UTC when macOS 14 runners are stable.