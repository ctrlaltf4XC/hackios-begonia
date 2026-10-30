# BEGONIA KERNEL VALIDATION REPORT

**Agent:** Agent 3 — BEGONIA KERNEL VALIDATOR (Operation Watchtower)  
**Repository:** /home/yu/BEGONIA-XNU  
**Report date:** 2026-10-30T16:30:12Z  
**Source of truth:** GitHub Actions run 41088218009 job logs (no artifacts fabricated).

---

## 1. GitHub Actions Run

| Field | Value |
|-------|-------|
| Run ID | 41088218009 |
| Run number | 5 |
| Run URL | https://github.com/ctrlaltf4XC/hackios-begonia/actions/runs/41088218009 |
| Workflow | XNU 8796 Build Performance Optimized (macOS CI) |
| Head branch | ci/first-boot-validation |
| Head SHA | fbf0057775b77379461f2c0aa175f66ec38487fc |
| Event | (push) |
| Status | completed |
| Conclusion | failure |

## 2. Runner Environment

| Field | Value |
|-------|-------|
| Runner image | macos-14-arm64 (image version 20260831.0302.1) |
| OS | macOS 14.8.9 (23J631) |
| Runner version | 2.338.2 |
| Azure region | westus |
| Xcode version | Xcode 15.4 (`/Applications/Xcode_15.4.app/Contents/Developer`) |
| SDK | MacOSX14.5.sdk |
| KDK version | Kernel Debug Kit 13.5 build 22G74 (`/Library/Developer/KDKs/KDK_13.5_22G74.kdk`) |
| XNU tag | xnu-8796.141.3 |
| XNU commit | 1b191cb58250d0705d8a51287127505aa4bc0789 |
| MACHINE_CONFIG | VMAPPLE (baseline) |
| ARCH_CONFIG | ARM64 |
| KERNEL_CONFIG | DEVELOPMENT |
| XNU_BUILD_JOBS | 4 |
| BUILD_LTO | 0 |
| BUILD_WERROR | 0 |

## 3. Build Duration

| Phase | Started | Completed | Duration |
|-------|---------|-----------|----------|
| Full run (all jobs) | 2026-10-30T16:15:30Z | 2026-10-30T16:22:26Z | ~6 min 56 s (incl. queue) |
| Baseline job (VMAPPLE) | 2026-10-30T16:20:46Z | 2026-10-30T16:22:26Z | ~1 min 40 s |
| Baseline build step (step 14) | 2026-10-30T16:22:02Z | 2026-10-30T16:22:21Z | ~19 s |

## 4. Jobs

| Job | Name | Conclusion | Dependency |
|-----|------|------------|------------|
| 124174917698 | XNU 8796 Optimized Build (VMAPPLE) | **FAILED** | — |
| 124178572031 | XNU 8796 with Begonia Platform Integration | **SKIPPED** | needs: xnu-baseline (unsatisfied) |

The Begonia job was **SKIPPED / NOT EXECUTED** because its dependency `xnu-baseline` failed. The begonia build never ran.

## 5. First Meaningful Error (baseline VMAPPLE build)

The baseline build failed during the `build_setup` phase, before any XNU source was compiled. This is a build-setup / shell-environment failure, NOT a Begonia source issue.

```
2026-10-30T16:22:19.5248660Z /bin/sh: line 0: [: -le: unary operator expected
2026-10-30T16:22:19.5249130Z /bin/sh: line 0: [: -gt: unary operator expected
2026-10-30T16:22:19.5268340Z expr: syntax error
...
make[5]: *** [_symbol_aliasing.h] Error 1
make[5]: Target `do_build_setup' not remade because of errors.
...
make[1]: *** [build_setup_bootstrap_DEVELOPMENT^ARM64^VMAPPLE] Error 2
make[1]: Target `install' not remade because of errors.
make: *** [install] Error 2
##[error]Process completed with exit code 2.
```

Root cause (first meaningful): `/bin/sh: line 0: [: -le: unary operator expected` and `expr: syntax error` during `_symbol_aliasing.h` generation in `build_setup`, cascading to `make: *** [install] Error 2`.

## 6. Artifacts

| Artifact name | Present? | Size | SHA256 digest |
|---------------|----------|------|---------------|
| xnu-build-VMAPPLE | YES (baseline only) | 172,547 bytes | sha256:a9e327a8838f6acc4c79152ea7e435b5725960484a06e6e9f9dd14b6923033b7 |
| xnu-build-begonia | **NO** | — | — |

Expected begonia artifact `build/xnu.obj/kernel.development.begonia` (Mach-O arm64) was **NOT produced** because the begonia job was skipped. No begonia artifact exists to download or validate.

## 7. Build Result

| Target | Result |
|--------|--------|
| xnu-baseline (VMAPPLE) | **FAILED** |
| xnu-begonia | **NOT EXECUTED** (SKIPPED; dependency `needs: xnu-baseline` unsatisfied) |
| Overall run | **FAILED** |

Per charter: a passing baseline does not establish Begonia build success. The baseline did not pass, so Begonia build success is **PENDING / BLOCKED**.

## 8. Patch Order Verification (0004→0006→0016→0018→0065)

Source: `PATCH_COMPATIBILITY.md` (against xnu-8796.141.3, commit 1b191cb).

| Patch | Purpose | Applies cleanly? | Notes |
|-------|---------|------------------|-------|
| 0004 | Machine config: BEGONIA/SWEET board config, GIC/timer foundation | YES | makedefs/MakeInc.def, proc_reg.h, sleh.c, start.s, pe_fiq.c, board_config.h |
| 0006 | MASTER.arm64.sweet master config | YES | New file; template for MASTER.arm64.begonia |
| 0016 | Device-tree platform expert (SweetPlatformExpert) | YES | Adds `xiaomi,sweet` IONameMatch; template for BegoniaPlatformExpert.cpp |
| 0018 | Boot CPU driver (SweetCPU, IOCPU, SGI1 self-test) | **NO** | Requires 0016's KernelConfigTables.cpp line first |
| 0065 | SMP via PSCI (sweet_psci_cpu_on, secondary entry) | **PARTIAL** | Requires 0018's SweetCPU.cpp first |
| 0017 | Virtual-timer IRQ | **NO** | Context mismatch; needs rewrite for MT6785 |

Correct dependency order per doc:
```
0004 -> 0006 -> 0016 -> 0018 -> 0065 -> 0017
```

Verification status: **PENDING**. No patch has been applied to the XNU source tree (tree is pristine at 1b191cb). Patches 0018 and 0065 cannot apply until their prerequisites (0016, then 0018) are satisfied. Begonia-specific equivalents are planned (see PATCH_COMPATIBILITY.md "Begonia adaptation plan") but not yet applied.

## 9. Compiler/Linker Output Summary

Compiler: Apple clang (Xcode 15.4 toolchain), target macOS (host cross to ARM64 via darwin-xnu-build engine).

- Build reached only `build_setup` (bootstrap setup) for DEVELOPMENT^ARM64^VMAPPLE.
- No kernel compilation (no `kernel.development.begonia` or `kernel.development` object produced).
- Warnings only (deprecated `NXArchInfo` in kextsymboltool; unused `lpos` in mkmakefile.c) — no errors reached the compiler proper because setup failed first.
- Linker: not reached.
- Exit code: 2.

## 10. Kernel Artifact Path and SHA256

| Field | Value |
|-------|-------|
| Expected path | build/xnu.obj/kernel.development.begonia |
| Expected artifact name | xnu-build-begonia |
| Actual path | **N/A (not built)** |
| SHA256 | **N/A (not available)** |

Artifact not produced; do not fabricate.

## 11. Mach-O Validation

All fields **PENDING** (no Mach-O artifact exists):

| Field | Value |
|-------|-------|
| Architecture | PENDING (expected: arm64) |
| CPU subtype | PENDING |
| Load commands | PENDING |
| Segment layout | PENDING |
| Entry point | PENDING |
| Tooling to use (when available) | `file`, `otool -l`, `otool -vh`, `nm`, `codesign -dv` |

## 12. Platform Integration Verification

All fields **PENDING** (begonia build never ran):

| Interface | File | Status |
|-----------|------|--------|
| Platform expert (matches `xiaomi,begonia`) | platform/begonia/BegoniaPlatformExpert.cpp | PENDING (file exists; reviewed) |
| CPU initialization (IOCPU, PSCI CPU_ON, SGI1) | platform/begonia/BegoniaCPU.cpp | PENDING (file exists; reviewed) |
| GIC integration (GICv3, Group 1, sleh_irq/pe_fiq) | pexpert/arm/pe_fiq.c, osfmk/arm64/sleh.c | PENDING (not applied) |
| Timer (architected timer, CNTV virtual timer) | osfmk/arm64/sleh.c, pexpert/arm/pe_fiq.c | PENDING |
| SMP (PSCI CPU_ON, secondary entry) | osfmk/arm64/cpu.c, machine_routines.c, start.s | PENDING |
| Linker completion | — | PENDING |

Note: board config `platform/begonia/begonia.h` exists and defines `BEGONIA_VTIMER_INTID 27`, `HAS_GIC_V3`, `BEGONIA_SMP_MAX_CPUS 6`, `__ARM_16K_PG__`. These are source-level definitions only; not validated by a build.

## 13. Timer Configuration Verification (PPI 11 → INTID 27)

Per `platform/begonia/begonia.h` (lines 56-73):

- POX DTS `arm,armv8-timer` interrupts: PPI 13 (secure physical), PPI 14 (non-secure physical), **PPI 11 (virtual)**, PPI 10 (hypervisor).
- PPIs map to INTID 16-31, so PPI 11 = **INTID 27** (virtual timer).
- `BEGONIA_VTIMER_INTID = 27` (the INTID, not the raw PPI 11).
- `pe_fiq.c` enables `(1 << 27)` for the virtual timer, matching PPI 11.
- XNU uses PPI 11 / INTID 27 for `rtclock_intr` / CNTV_CTL.
- NOT secure physical INTID 29 (that is PPI 13, TrustZone-owned).

Verification status: **PENDING** (documented correctly in source; not exercised by a build because no kernel was produced).

## 14. SMP Dependencies Verification

| Dependency | Status |
|------------|--------|
| PSCI CPU_ON SMC (0xC4000003) | PENDING |
| Secondary entry (`begonia_secondary_entry` in start.s) | PENDING |
| cpu.c / machine_routines.c secondary registration | PENDING (0065 hunks apply but depend on 0018) |
| BEGONIA_SMP_MAX_CPUS = 6 (A55s) | PENDING |
| SGI1 via ICC_SGI1R_EL1 | PENDING |

All PENDING (no build executed).

## 15. Linker Completion Verification

Status: **PENDING / NOT REACHED**. The baseline build failed in `build_setup` (setup phase) before linking; the begonia build never ran. No link map, no `__START_XNU__`/entry resolution performed.

## 16. Sign-off Status

**Status: BLOCKED — PENDING**

Reason: Run 41088218009 completed with conclusion `failure`. The VMAPPLE baseline build failed during `build_setup` (`[: -le: unary operator expected` / `expr: syntax error` → `_symbol_aliasing.h` Error 1 → `make: *** [install] Error 2`). Because `needs: xnu-baseline` was unsatisfied, the `xnu-begonia` job was SKIPPED and never executed. No `xnu-build-begonia` artifact exists, no Mach-O artifact was produced, and no validation of kernel architecture, segment layout, entry point, platform integration, GIC, timer, SMP, or linker completion can be performed.

Required before sign-off:
1. Fix/understand the baseline `build_setup` shell arithmetic failure (unary operator / expr syntax) so xnu-baseline succeeds.
2. Re-run the workflow so xnu-begonia executes.
3. Download `xnu-build-begonia` artifact, compute SHA256, validate Mach-O (arm64, load commands, segments, entry point).
4. Apply and verify patch order 0004→0006→0016→0018→0065 (with Begonia adaptations).
5. Verify timer PPI 11 → INTID 27 and SMP PSCI bring-up via UART boot log.

**Sign-off: NOT SIGNED (BLOCKED on baseline build success).**

---

*No hardware test results, UART captures, or artifact contents were fabricated. All "PENDING" fields reflect that the begonia build did not execute.*
