# BEGONIA INTEGRATION REVIEW — Agent 6

Operation First Boot — XNU 8796 on Xiaomi Redmi Note 8 Pro (begonia / MT6785).

**Scope:** Platform/begonia/ integration, second CI job (xnu-begonia), patch review.
**XNU source:** /home/yu/BEGONIA-XNU/xnu/xnu-8796 @ 1b191cb58250d0705d8a51287127505aa4bc0789 (tag xnu-8796.141.3)
**Build engine:** /home/yu/BEGONIA-XNU/tools/darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549
**ios-sweet patches:** /home/yu/BEGONIA-XNU/ios-sweet/xnu/patches/
**POX hardware reference:** /home/yu/BEGONIA-XNU/pox-begonia-kernel @ 2fb79cd9cba6, arch/arm64/boot/dts/mediatek/mt6785.dts

---

## Sign-off status (summary)

| Area | Status |
|------|--------|
| begonia.h | BLOCKED — timer PPI misassigned (see §Timer audit) |
| BegoniaPlatformExpert.cpp | OK as template; missing KernelConfigTables.cpp registration |
| BegoniaCPU.cpp | OK as template; depends on 0016/0018/0065 tree changes |
| begonia_master_config.md | OK (template) |
| begonia_makedefs.md | OK (template) |
| README.md | OK |
| Patches 0004/0006/0016 | Apply cleanly to xnu-8796.141.3 (Sweet-specific; need Begonia rewrite) |
| Patches 0017/0018/0065 | Do NOT apply cleanly (context/order); need Begonia rewrite |
| Patch order 0004→0006→0016→0018→0065→0017 | CORRECT |
| CI job xnu-begonia | MACHINE_CONFIG=BEGONIA correct; missing patch application + board_config include-path risk |
| **OVERALL** | **BLOCKED — timer PPI must be corrected before any boot attempt** |

---

## 1. Platform file review

### 1.1 platform/begonia/begonia.h (board config)

**Purpose:** Board configuration header defining the BEGONIA machine config macro, GICv3 register definitions, timer INTID, and SMP limits. Included via pexpert/pexpert/arm64/board_config.h when ARM64_BOARD_CONFIG_BEGONIA is defined.

**Correctness:**
- `#define BEGONIA 1`, `#define HAS_GIC_V3 1`, `#define ARM_ARCH_TIMER`, `#define __ARM_16K_PG__ 1` — correct for generic ARMv8.2, 16K pages.
- `#define NO_MONITOR 1`, `#define NO_ECORE 1` — correct (no Apple SoC, no Apple efficiency-core model).
- `#define PLATFORM_PANIC_LOG_DISABLED` — acceptable for bring-up.
- GICv3 register offset definitions (GICD_CTLR, GICR_TYPER/WAKER/IGROUPR0/ISENABLER0/IPRIORITYR0, ICC_*) match Arm IHI 0069G. `GICR_TYPER 0x08` is correct for the PE-specific redistro frame base.
- `BEGONIA_SMP_MAX_CPUS 6`, `BEGONIA_MAX_CPUS 8`, `BEGONIA_MAX_CPU_CLUSTERS 2` — consistent with 6x A55 + 2x A76 topology; A55s brought up first, A76s unaudited.

**Issues (CRITICAL — timer):**

`BEGONIA_VTIMER_INTID 13` (line 63) is **wrong**. Per POX DTS `mt6785.dts:779-782` the `arm,armv8-timer` node lists interrupts in the standard binding order:

```
interrupts = <GIC_PPI 13>,   // 1st = secure physical (CNTP)
             <GIC_PPI 14>,   // 2nd = non-secure physical
             <GIC_PPI 11>,   // 3rd = virtual   (CNTV)  <-- XNU's timer
             <GIC_PPI 10>;   // 4th = hypervisor (CNTHP)
```

The `arm,armv8-timer` DT binding orders the four specifiers as: secure physical, non-secure physical, virtual, hypervisor. Therefore:
- PPI 13 = INTID 29 = secure physical timer (TrustZone-owned on this GIC)
- PPI 14 = INTID 30 = non-secure physical timer
- **PPI 11 = INTID 27 = virtual timer (CNTV) ← the one XNU's `rtclock_intr` services**
- PPI 10 = INTID 26 = hypervisor timer

XNU's default `pe_fiq.c:148` enables `(1 << 27)` = INTID 27 = PPI 11 (Apple's virtual timer). Begonia must use the same INTID 27 / PPI 11, routed as **Group 1 NS** (GICv3 SecurityExtn=1, Group 0 = TrustZone per `mt6785.dts:404` `compatible = "arm,gic-v3"`).

`begonia.h` currently assigns the virtual timer to PPI 13 (INTID 29), which is the **secure physical timer** owned by TrustZone — XNU (EL1 NS) can never receive it. This would make the kernel clock dead at boot.

Comments at lines 21, 57-61 repeat the false claim "virtual timer = PPI 13" and "virtual timer CNTV is PPI 13". These must be corrected.

**Required fix:** Change `BEGONIA_VTIMER_INTID` to reflect INTID 27 (PPI 11). Note: XNU's `sleh_irq` compares `ICC_IAR1_EL1` INTID against this value, so it must be the INTID (27), not the raw PPI number (11), to match how `SWEET_VTIMER_INTID = 19` (INTID) is used in `sleh.c`. The comment math (INTID = 16 + PPI) is correct, but the base PPI is wrong: 16 + 11 = 27, not 16 + 13 = 29.

Also: `BEGONIA_VTIMER_INTID` is declared but not referenced anywhere in the platform tree — the Begonia `sleh.c`/`pe_fiq.c` equivalent of 0017 must be written to actually use it.

### 1.2 platform/begonia/BegoniaPlatformExpert.cpp

**Purpose:** Minimal `IODTPlatformExpert` subclass matching the ADT root compatible `xiaomi,begonia`, publishing CPU/topology nubs without inventing an Apple NVRAM `/options` node.

**Correctness:**
- `start()` chains to `IODTPlatformExpert::start`, registers service. Good.
- `deleteList()` returns `"()"`; `excludeList()` returns `('chosen', 'defaults', 'cpus', 'options')`. Correct for no-NVRAM board.
- `processTopLevel()` finds `cpus` node, creates nubs from its children, then creates nubs from root excluding NVRAM/options nodes. Good.
- `getTargetName`/`getProductName` return "begonia" / "Xiaomi Redmi Note 8 Pro". Good.
- All `#if defined(BEGONIA)` gated. Good.

**Issues:**
- This file alone is not enough: the board must be registered in `iokit/KernelConfigTables.cpp` with an `IONameMatch = 'xiaomi,begonia'` entry (exactly as patch 0016 does for `xiaomi,sweet`). Without that entry, `BegoniaPlatformExpert` never matches the ADT root and no platform expert runs. The CI integration step copies the .cpp but does **not** edit KernelConfigTables.cpp — a gap.
- `IOProbeScore` is not set in this file (it lives in KernelConfigTables.cpp in 0016). Fine as long as the table entry is added.

### 1.3 platform/begonia/BegoniaCPU.cpp

**Purpose:** `IOCPU` subclass for A55 SMP bring-up via PSCI CPU_ON (SMC), SGI1 cross-CPU signalling, and secondary entry through `begonia_secondary_entry`.

**Correctness:**
- `begonia_psci_cpu_on()` issues SMCCC 1.0 `CPU_ON` (0xC4000003) with x1=target_affinity, x2=entry, x3=context; returns signed x0. Correct.
- `start()` validates `logical-cpu-id` and `reg` against `ml_topology_info()`, registers `begonia_cpus[]`, spawns `startAll` kernel thread on boot CPU. Good.
- `startAll()` waits for all CPUs registered, panics on timeout, calls `processor_start()` per CPU, waits for `initialized` + `begonia_cpu_signal_is_enabled()`, then `ml_cpu_init_completed()` and `cpusRunning()`. Good pattern (mirrors 0065).
- `initCPU(boot)` panics on non-boot resume (unimplemented), broadcasts `CPU_BOOTED`, self-SGI via `signalCPU`, sets running state, publishes `initialized=1`. Good — avoids deadlocking secondaries at splsched.
- `signalCPU()` builds `ICC_SGI1R_EL1` from TARGET MPIDR affinity (Aff1:Aff0), never sender's MPIDR. Correct.
- `startCPU()` rejects non-NULL args / phys_id 0 / already-initialized, computes `begonia_secondary_entry` PA, calls `begonia_psci_cpu_on`, panics on rc != 0. Good.

**Issues / dependencies (not bugs in this file, but required tree changes):**
- `extern "C" void begonia_secondary_entry(void)` — must be defined in `osfmk/arm64/start.s` (0065 adds `sweet_secondary_entry`; Begonia needs its own `begonia_secondary_entry`).
- `extern "C" boolean_t begonia_cpu_signal_is_enabled(unsigned int cpu)` — must be defined in `osfmk/arm64/cpu.c` (0065 adds `sweet_cpu_signal_is_enabled`).
- `cpu->phys_id > 0x500 || (cpu->phys_id & 0xff) != 0` — assumes Aff0 < 16 and Aff1 in bits 8-15; MT6785 MPIDR Aff0 is 0-7, Aff1 is 0-1, so fine for 8 CPUs.
- `BEGONIA_MAX_CPUS` bound check present. Good.

### 1.4 platform/begonia/begonia_master_config.md

**Purpose:** Documents `config/MASTER.arm64.begonia`.

**Correctness:** Modeled on `MASTER.arm64.sweet`. Uses `nos_arm_asm nos_arm_pmap` (open-source arm64 asm + pmap; no Apple closed lib), drops kpc (Apple PMU), enables `CONFIG_KERNEL_INTEGRITY`, `CONFIG_KERNEL_TBI`, 16K pages, plain `arm64` (no PAC/arm64e). Build command uses `CURRENT_MACHINE_CONFIG=BEGONIA`. All consistent with begonia.h.

**Issues:** This is a markdown template, not an applied file. `config/MASTER.arm64.begonia` must be created in the XNU tree (0006 equivalent). Nothing to block here.

### 1.5 platform/begonia/begonia_makedefs.md

**Purpose:** Documents `makedefs/MakeInc.def` changes + `proc_reg.h` cache-line definitions.

**Correctness:** Adds `BEGONIA` to `SUPPORTED_ARM64_MACHINE_CONFIGS`, `MACHINE_FLAGS_ARM64_BEGONIA = -DARM64_BOARD_CONFIG_BEGONIA -march=armv8.2-a+crypto`, forces plain `arm64` arch string for BEGONIA (no PAC), adds `MMU_I_CLINE`/`MMU_CLINE = 6` for BEGONIA in `proc_reg.h`. All correct and consistent with begonia.h.

**Issues:** Template only; must be applied to the XNU tree (0004 equivalent).

### 1.6 platform/begonia/README.md

**Purpose:** Documents required platform files, hardware sources (POX DTS), entry contract, and the patch list adapted from ios-sweet.

**Correctness:** Required file table is accurate (machine.c/pmap.c/pmapping.c/gic.c/timer.c/cpu.c/psci.c/console.c). States hardware addresses come from ADT, not hardcoded — correct per project rule 4. Entry contract (x0=boot_args PA, MMU/caches off, DAIF=0xf, EL1) matches the bootshim contract. Patch list references the six patches under review.

**Issues:** README references patch 0007/0008/0009 which are not in the six-patch review set — those are out of scope for this review but noted as future work. Not a blocker.

---

## 2. Timer audit (CRITICAL)

### 2.1 POX DTS source of truth

File: `pox-begonia-kernel/arch/arm64/boot/dts/mediatek/mt6785.dts`, tag 2fb79cd9cba6.

`mt6785.dts:776-784` — `timer` node:
```
timer: timer {
    compatible = "arm,armv8-timer";
    interrupt-parent = <&gic>;
    interrupts = <GIC_PPI 13 IRQ_TYPE_LEVEL_LOW>,   // mt6785.dts:779
                 <GIC_PPI 14 IRQ_TYPE_LEVEL_LOW>,   // mt6785.dts:780
                 <GIC_PPI 11 IRQ_TYPE_LEVEL_LOW>,   // mt6785.dts:781
                 <GIC_PPI 10 IRQ_TYPE_LEVEL_LOW>;   // mt6785.dts:782
    clock-frequency = <13000000>;                    // mt6785.dts:783
};
```

GIC node: `mt6785.dts:403-415`, `compatible = "arm,gic-v3"`, reg `0x0c000000` (dist, 0x40000), `0x0c040000` (redistro, 0x200000), `0x0c53a650` (INT_POL). GICv3 with security extensions (SecurityExtn=1 per begonia.h analysis).

### 2.2 PPI assignment (arm,armv8-timer binding order)

The `arm,armv8-timer` device-tree binding orders the four interrupt specifiers as:
1. Secure physical timer (CNTP_EL1 / CNTP_CTL_EL0)
2. Non-secure physical timer
3. Virtual timer (CNTV_CTL_EL0)
4. Hypervisor timer (CNTHP_CTL_EL2)

Therefore, for MT6785:

| Timer | PPI | INTID (16+PPI) | Owner | XNU uses? |
|-------|-----|----------------|-------|-----------|
| Secure physical | 13 | 29 | TrustZone (Group 0) | No |
| Non-secure physical | 14 | 30 | EL1 NS | No (XNU uses virtual) |
| **Virtual** | **11** | **27** | EL1 NS (Group 1) | **Yes — CNTV** |
| Hypervisor | 10 | 26 | EL2 | No |

### 2.3 XNU expectation

- XNU default `pexpert/arm/pe_fiq.c:148` enables `(1 << 27)` = INTID 27 = PPI 11 (the virtual timer) — but as Group 0 (Apple GIC has no security split usable by XNU).
- XNU `sleh_irq` services `rtclock_intr` for the virtual/decrementer timer (CNTV), keyed on the virtual-timer INTID.
- On Apple, virtual timer INTID = 27 (PPI 11). On Qualcomm sweet, virtual timer INTID = 19 (PPI 3) via `SWEET_VTIMER_INTID = 19`.

### 2.4 Verification status

| Item | Value | Source | Status |
|------|-------|--------|--------|
| Secure physical PPI | 13 (INTID 29) | mt6785.dts:779 | VERIFIED (1st specifier) |
| Non-secure physical PPI | 14 (INTID 30) | mt6785.dts:780 | VERIFIED (2nd specifier) |
| **Virtual PPI** | **11 (INTID 27)** | **mt6785.dts:781** | **VERIFIED (3rd specifier)** |
| Hypervisor PPI | 10 (INTID 26) | mt6785.dts:782 | VERIFIED (4th specifier) |
| GIC version | GICv3, SecurityExtn=1 | mt6785.dts:404 + begonia.h | VERIFIED |
| Timer delivery group | Group 1 NS (IRQ via sleh_irq) | GICv3 security model | DERIVED (Group 0 = TrustZone) |
| XNU virtual-timer INTID | 27 (PPI 11) | pe_fiq.c:148 default | MATCHES MT6785 |

### 2.5 Defect in begonia.h

`begonia.h:63` `#define BEGONIA_VTIMER_INTID 13` assigns the virtual timer to PPI 13 / INTID 29 — the **secure physical timer**, which TrustZone owns and XNU cannot take as an IRQ. Comments at lines 21 and 57-61 repeat this error ("virtual timer = PPI 13").

**This is an integration blocker.** The kernel clock (`rtclock_intr` / CNTV) will never fire if PPI 13 is programmed instead of PPI 11 / INTID 27.

**Required fix:** `BEGONIA_VTIMER_INTID` must be 27 (INTID, matching ICC_IAR1_EL1 output and the `SWEET_VTIMER_INTID` convention), corresponding to PPI 11 per `mt6785.dts:781`. Update the comments accordingly. Never substitute PPI 13 for the virtual timer.

---

## 3. Patch review (ios-sweet xnu/patches/)

All patches tested against `xnu-8796.141.3` (commit 1b191cb) per PATCH_COMPATIBILITY.md. None are applied to the tree (pristine).

### 3.1 0004-sweet-machine-config.patch — Applies cleanly (YES)

**Purpose:** Adds SWEET board config to MakeInc.def, `proc_reg.h` cache-line defines, `sleh.c` GICv3 Group-1 timer/IPI path, `start.s` EL1 SCTLR check exemption, `pe_fiq.c` GICv3 Group-1 init, board_config.h + new `SWEET.h`, pexpert Makefile.

**Files:** makedefs/MakeInc.def, osfmk/arm64/proc_reg.h, osfmk/arm64/sleh.c, osfmk/arm64/start.s, pexpert/arm/pe_fiq.c, pexpert/pexpert/arm64/Makefile, pexpert/pexpert/arm64/board_config.h, pexpert/pexpert/arm64/SWEET.h (new).

**Snapdragon assumptions:** `SWEET` flag, `SWEET_VTIMER_INTID = 19` (Qualcomm PPI 3), `SWEET_IPI_SGI = 1`, Qualcomm PPI numbering 1/2/3/0, `MAX_CPU_CLUSTERS 1`.

**Begonia equivalent needed:** Yes — full rewrite as `BEGONIA` with `BEGONIA_VTIMER_INTID = 27` (PPI 11), MT6785 GICv3 bases, `MAX_CPU_CLUSTERS 2`, `BEGONIA.h` board config. The structural changes (Group-1 sleh_irq path, pe_fiq.c GICv3 init, board_config.h section) are directly applicable with SWEET→BEGONIA substitution plus the timer PPI correction.

### 3.2 0006-sweet-master-config-open-source-asm-pmap.patch — Applies cleanly (YES)

**Purpose:** New `config/MASTER.arm64.sweet` — open-source nos_arm_asm/nos_arm_pmap, drops kpc and Apple closed lib.

**Files:** config/MASTER.arm64.sweet (new).

**Snapdragon assumptions:** None build-critical (config-only).

**Begonia equivalent needed:** Yes — create `config/MASTER.arm64.begonia` per begonia_master_config.md. Standalone file, no ordering constraint.

### 3.3 0016-sweet-device-tree-platform-expert.patch — Applies cleanly (YES)

**Purpose:** `SweetPlatformExpert` (IODTPlatformExpert matching `xiaomi,sweet`) + KernelConfigTables.cpp IONameMatch entry + files.arm64 registration.

**Files:** iokit/KernelConfigTables.cpp, iotkit/conf/files.arm64, iotkit/Kernel/arm/SweetPlatformExpert.cpp (new).

**Snapdragon assumptions:** `IONameMatch = 'xiaomi,sweet'`, `SweetPlatformExpert` class name.

**Begonia equivalent needed:** Yes — `BegoniaPlatformExpert` matching `xiaomi,begonia` (already written in platform/begonia/), registered in KernelConfigTables.cpp and files.arm64. **Critical:** this patch's KernelConfigTables.cpp entry is a prerequisite for 0018.

### 3.4 0017-sweet-device-tree-virtual-timer-irq.patch — Does NOT apply cleanly (NO)

**Purpose:** Read `virtual-timer-intid` from ADT via `SecureDTGetProperty`, route timer PPI through `sleh_irq`/pe_fiq GICv3 Group-1 path, `sweet_vtimer_intid` RO-late variable, protos.h extern.

**Files:** pexpert/arm/pe_fiq.c, osfmk/arm64/sleh.c, pexpert/pexpert/arm/protos.h.

**Snapdragon assumptions:** `SWEET` flag, `sweet_vtimer_intid` symbol, `SWEET_IPI_SGI`.

**Why it fails:** Developed against an intermediate ios-sweet commit, not xnu-8796.141.3. Context mismatch in `pe_fiq.c` GIC block (the reference tree's `pe_init_fiq()` GIC block differs — uses GICR_WAKER wake loop, `(1 << 27)` PPI enable, no GICR_IPRIORITYR0 loop).

**Begonia equivalent needed:** **Yes — independent Begonia-specific rewrite required (Responsibility 12).** Must:
- Define `begonia_vtimer_intid` (INTID 27) in `pe_fiq.c`, read from ADT `virtual-timer-intid` property on boot CPU.
- Add `pe_fiq.c` Group-1 GICv3 init: set `GICR_IGROUPR0` for the timer PPI as Group 1, program `GICR_IPRIORITYR0`, enable `(1U << begonia_vtimer_intid) | (1U << SGI1)` in `GICR_ISENABLER0`, enable `GICD_CTLR` Group 1 (`ENABLEGRP1 | ENABLEGRP1A | ARE_NS`), set `ICC_IGRPEN1_EL1 = 1`.
- Update `sleh.c` to dispatch `intid == begonia_vtimer_intid` to `rtclock_intr`, and `intid < 16` (SGI1) to `cpu_signal_handler()`.
- Declare `extern uint32_t begonia_vtimer_intid` in `pexpert/pexpert/arm/protos.h`.
- Do NOT copy Qualcomm PPI numbering (1/2/3/0) or `SWEET_IPI_SGI = 1` assumptions; use MT6785 PPI 11 / INTID 27.

### 3.5 0018-sweet-boot-cpu-driver.patch — Does NOT apply cleanly (NO)

**Purpose:** `SweetCPU` IOCPU driver — single-CPU bring-up, `ml_processor_register`, `processor_start`, SGI1 self-test, `startCPU` returns KERN_NOT_SUPPORTED.

**Files:** iotkit/Kernel/arm/SweetCPU.cpp (new), iotkit/KernelConfigTables.cpp, iotkit/conf/files.arm64.

**Why it fails:** `KernelConfigTables.cpp` lacks the `xiaomi,sweet` IONameMatch line added by 0016 — the patch context expects that line to exist. Order dependency: 0018 requires 0016 first.

**Begonia equivalent needed:** Yes — `BegoniaCPU` (already written in platform/begonia/), registered in KernelConfigTables.cpp with `IONameMatch` matching the ADT CPU nodes, and files.arm64 registration. Must come after 0016.

### 3.6 0065-sweet-smp-psci.patch — PARTIAL

**Purpose:** SMP via PSCI: `sweet_psci_cpu_on` SMC, `startAll` secondary bring-up loop, `begonia_cpu_signal_is_enabled` observer in cpu.c, `die-cluster-id`/`cluster-core-id` topology in machine_routines.c, `sweet_secondary_entry` in start.s, `SWEET_SMP_MAX_CPUS` in SWEET.h.

**Files:** iotkit/Kernel/arm/SweetCPU.cpp, osfmk/arm64/cpu.c, osfmk/arm64/machine_routines.c, osfmk/arm64/start.s, pexpert/pexpert/arm64/SWEET.h.

**Why partial:** `SweetCPU.cpp` doesn't exist yet (needs 0018). cpu.c/machine_routines.c/start.s hunks apply. Order dependency: 0065 requires 0018.

**Snapdragon assumptions:** `SWEET_SMP_MAX_CPUS = 6`, `die-cluster-id`/`cluster-core-id` DT properties (Qualcomm-specific — MT6785 uses standard arm cpu-map / MPIDR affinity), `sweet_secondary_entry` EL1/EL2 entry logic, x20/x21 via `LowResetVectorBase`.

**Begonia equivalent needed:** Yes — adapt to `BEGONIA`:
- `begonia_psci_cpu_on` (already in BegoniaCPU.cpp) + `begonia_secondary_entry` in start.s.
- `begonia_cpu_signal_is_enabled` in cpu.c (BegoniaCPU.cpp already references it).
- machine_routines.c: use MPIDR affinity (not Qualcomm `die-cluster-id`) for cluster/core ID; clamp `cpus=N` to `BEGONIA_SMP_MAX_CPUS`.
- start.s: `begonia_secondary_entry` (MT6785 PSCI CPU_ON entry; confirm EL1 vs EL2 per bootshim — POX_HARDWARE_ANALYSIS.md §8 notes MediaTek LK EL is unconfirmed).
- Do NOT copy Qualcomm `die-cluster-id`/`cluster-core-id` or the `LowResetVectorBase` x20/x21 convention.

---

## 4. Patch ordering verification

Correct application order (prerequisites first), per PATCH_COMPATIBILITY.md §Patch dependency order and this review:

```
0004  -> board config + GICv3/timer/sleh foundation (BEGONIA rewrite)
0006  -> config/MASTER.arm64.begonia (standalone)
0016  -> device-tree platform expert + xiaomi,begonia IONameMatch (prereq for 0018)
0018  -> Begonia CPU driver (requires 0016's KernelConfigTables.cpp line)
0065  -> SMP via PSCI (requires 0018's SweetCPU/BegoniaCPU.cpp)
0017  -> virtual-timer IRQ (independent, but depends on 0004's sleh/pe_fiq foundation; must be rewritten for MT6785 PPI 11/INTID 27)
```

The user-specified order `0004 → 0006 → 0016 → 0018 → 0065` is **correct**; 0017 is independent in application but must come after 0004 (it touches the same sleh.c/pe_fiq.c regions) and must use PPI 11/INTID 27, not Qualcomm PPI 19 / MT6785-misassigned PPI 13.

**Verification:** Patch 0017 must NOT be applied before 0004 (both modify `osfmk/arm64/sleh.c` and `pexpert/arm/pe_fiq.c`). Patch 0018 must NOT be applied before 0016 (KernelConfigTables.cpp context). Patch 0065 must NOT be applied before 0018 (SweetCPU.cpp/BegoniaCPU.cpp context).

---

## 5. CI job xnu-begonia review (.github/workflows/xnu.yml lines 296-527)

**Purpose:** Second CI job, builds XNU 8796 with the Begonia platform integrated.

**Correctness:**
- `MACHINE_CONFIG: BEGONIA` set explicitly at line 465 — **correct** (Responsibility 14 satisfied).
- Kernel artifact path `kernel.${KERNEL_CONFIG}.begonia` (line 496) matches the BEGONIA config suffix — correct.
- `needs: xnu-baseline` with `if: needs.xnu-baseline.result == 'success'` — correct gating.
- Pins DXB_SHA 6639065..., XNU_TAG xnu-8796.141.3, XNU_COMMIT 1b191cb..., Xcode 15.4, KDK 13.5 — consistent with baseline job. Good.
- Integration step (lines 430-454) copies `BegoniaPlatformExpert.cpp`, `BegoniaCPU.cpp`, `begonia.h` into the XNU tree and appends entries to `files.arm64` and `board_config.h`.

**Issues / gaps:**
1. **No XNU patches applied.** The job copies only the three platform files. It does **not** apply 0004/0006/0016/0017/0018/0065 (or Begonia equivalents). Without `board_config.h`'s `ARM64_BOARD_CONFIG_BEGONIA` section, `sleh.c` Group-1 timer path, `pe_fiq.c` GICv3 Group-1 init, `start.s` secondary entry, `cpu.c`/`machine_routines.c` SMP changes, the `BEGONIA` define never takes effect and the build either fails or produces a non-functional kernel. The job must apply the Begonia patch set (or equivalent tree edits) before building.
2. **KernelConfigTables.cpp not edited.** The integration step registers the two .cpp files in `files.arm64` but does not add the `xiaomi,begonia` IONameMatch entry to `KernelConfigTables.cpp` (0016's change). Without it, `BegoniaPlatformExpert` never matches the ADT and `BegoniaCPU` never matches CPU nodes.
3. **`board_config.h` include path.** The step appends `#include <pexpert/arm64/begonia.h>` while copying `begonia.h` to `pexpert/pexpert/arm64/`. This matches the existing pattern (`VMAPPLE.h`/`BCM2837.h` live in `pexpert/pexpert/arm64/` and are included as `<pexpert/arm64/...>`), so it should resolve — but this is fragile and should be verified against the dxb include paths.
4. **`ARM64_BOARD_CONFIG_BEGONIA` is never defined.** The `board_config.h` append is wrapped in `#ifdef ARM64_BOARD_CONFIG_BEGONIA`, but nothing in the job passes `-DARM64_BOARD_CONFIG_BEGONIA`. The `MACHINE_FLAGS_ARM64_BEGONIA` define (from makedefs, 0004 equivalent) must be in effect for the include to be active. Since patches aren't applied, this macro is undefined and the begonia.h include is skipped — dead code.
5. **No build-success validation beyond binary presence.** The job only checks that the kernel binary exists; no boot/timer/UART validation is possible in CI (no device), but a link/pass check is implicit in `make install`.

---

## 6. Memory / MMU / GIC / SMP integration notes (Responsibilities 4, 5, 7, 8, 9)

These are not implemented in the reviewed platform files (only documented in README.md) and must be addressed before boot:

- **Memory (pmap.c/pmapping.c):** DRAM from live FDT `/memory` node (`mt6785.dts:281-284`, 0x40000000 / 0x3e605000 in POX, but target ships 6 GiB — bootshim must parse live FDT, POX value is not trustworthy per POX_HARDWARE_ANALYSIS.md §2). XNU 16K pages, T0SZ=28, MAIR Device/WB/NC. Not yet implemented.
- **MMU:** Stage-1 MMU, identity/direct map early, `nos_arm_pmap`. `__ARM_16K_PG__` set in begonia.h. Cache-line `MMU_CLINE = 6` in begonia_makedefs.md. Not yet implemented in tree.
- **GIC integration:** GICv3 dist 0x0c000000 / redistro 0x0c040000 (`mt6785.dts:411-412`), ICC_* system-register interface (not MMIO CPU interface). `pe_fiq.c` already has `find_gicr_pe_base()` walking redistributors. Group-1 timer path needed (0017 equivalent). Not yet implemented in tree.
- **SMP integration:** PSCI `arm,psci-1.0` method=smc (`mt6785.dts:263-266`), 6x A55 (cluster0) + 2x A76 (cluster1, DTS says a75 — probe MIDR_EL1). `BegoniaCPU.cpp` implements the IOCPU side; needs `start.s` secondary entry and `cpu.c` signal observer (0065 equivalent). Not yet implemented in tree.

---

## 7. Integration blockers

1. **TIMER PPI MISASSIGNMENT (CRITICAL).** `begonia.h:63` `BEGONIA_VTIMER_INTID = 13` targets the secure physical timer (PPI 13 / INTID 29, TrustZone-owned). The virtual timer is PPI 11 / INTID 27 per `mt6785.dts:781`. XNU's `rtclock_intr` will never fire. Fix before any boot attempt. Do not substitute PPI 13 for the virtual timer.
2. **No patches applied to XNU tree.** Tree is pristine at 1b191cb; none of 0004/0006/0016/0017/0018/0065 (or Begonia equivalents) are applied. `BEGONIA` board config, sleh_irq Group-1 path, pe_fiq.c GICv3 Group-1 init, platform expert, CPU driver, and SMP PSCI are all absent.
3. **0017 requires an independent Begonia rewrite.** Cannot be blindly applied (context mismatch) and must not copy Qualcomm PPI numbering (19) — must use MT6785 PPI 11 / INTID 27.
4. **CI job does not apply patches nor register KernelConfigTables.cpp entry.** The xnu-begonia job copies files but the BEGONIA macro and IONameMatch registration are inactive, so the build would not exercise the Begonia path.
5. **Missing tree files:** `config/MASTER.arm64.begonia`, `osfmk/arm64/start.s` `begonia_secondary_entry`, `osfmk/arm64/cpu.c` `begonia_cpu_signal_is_enabled`, `osfmk/arm64/machine_routines.c` MPIDR-affinity topology, GIC/timer drivers (`gic.c`, `timer.c`), MMU (`pmap.c`, `pmapping.c`), console (`console.c`). These are out of scope for this review but are required for boot.

---

## 8. Sign-off

| Review item | Verdict |
|-------------|---------|
| begonia.h | REJECT — timer PPI must be corrected to INTID 27 (PPI 11) per mt6785.dts:781 |
| BegoniaPlatformExpert.cpp | ACCEPT AS TEMPLATE — requires KernelConfigTables.cpp registration |
| BegoniaCPU.cpp | ACCEPT AS TEMPLATE — requires start.s + cpu.c counterparts |
| begonia_master_config.md | ACCEPT (template) |
| begonia_makedefs.md | ACCEPT (template) |
| README.md | ACCEPT |
| Patch 0004 | ACCEPT (clean) — Begonia rewrite required |
| Patch 0006 | ACCEPT (clean) — MASTER.arm64.begonia required |
| Patch 0016 | ACCEPT (clean) — BegoniaPlatformExpert registration required |
| Patch 0017 | REJECT AS-IS — independent Begonia rewrite required (PPI 11/INTID 27) |
| Patch 0018 | REJECT AS-IS — requires 0016 first; BegoniaCPU equivalent required |
| Patch 0065 | REJECT AS-IS — requires 0018 first; Begonia SMP rewrite required |
| Patch order 0004→0006→0016→0018→0065→0017 | APPROVED |
| CI job xnu-begonia | REJECT AS-IS — MACHINE_CONFIG=BEGONIA correct, but patches not applied, KernelConfigTables.cpp not registered |

**Overall sign-off: BLOCKED.** The single blocking defect is the timer PPI misassignment in `begonia.h` (PPI 13 → must be PPI 11 / INTID 27). No hardware timer support may be claimed as verified until this is corrected and validated against `mt6785.dts:779-782`. All other items are template/implementation gaps, not blockers on the review itself.

Agent 6 — BEGONIA INTEGRATION ENGINEER
