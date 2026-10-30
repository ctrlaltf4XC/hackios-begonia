# PATCH_COMPATIBILITY.md

Compatibility of the six ios-sweet XNU patches (0004, 0006, 0016, 0017, 0018, 0065)
against the reference-pinned source xnu-8796.141.3 (commit 1b191cb).

Patches were tested with `git apply --check` from
`/home/yu/BEGONIA-XNU/ios-sweet/xnu/patches/`.

## Compatibility matrix (against xnu-8796.141.3)

| Patch | Purpose | Applies? | Files modified | Notes |
|-------|---------|----------|----------------|-------|
| 0004 | Machine config: add BEGONIA/SWEET board config, cache-line defines, sleh_irq GICv3 path, start.s EL1 check, pe_fiq GIC setup, board_config.h | YES | makedefs/MakeInc.def, osfmk/arm64/proc_reg.h, osfmk/arm64/sleh.c, osfmk/arm64/start.s, pexpert/arm/pe_fiq.c, pexpert/pexpert/arm64/Makefile, pexpert/pexpert/arm64/board_config.h (+new SWEET.h) | Board-config skeleton already in platform/begonia/begonia.h |
| 0006 | New MASTER.arm64.sweet master config (open-source nos_arm_asm/nos_arm_pmap) | YES | config/MASTER.arm64.sweet (new) | Template for MASTER.arm64.begonia |
| 0016 | Device-tree platform expert (SweetPlatformExpert) + IONameMatch xiaomi,sweet in KernelConfigTables.cpp | YES | iokit/KernelConfigTables.cpp, iokit/conf/files.arm64, iokit/Kernel/arm/SweetPlatformExpert.cpp (new) | Template for BegoniaPlatformExpert.cpp |
| 0017 | Virtual-timer IRQ: read virtual-timer-intid from ADT, route timer PPI via sleh_irq/pe_fiq GICv3 Group 1 path | NO | pexpert/arm/pe_fiq.c, osfmk/arm64/sleh.c, pexpert/pexpert/arm/protos.h | Context mismatch: source pe_fiq.c GIC block differs (uses GICR_WAKER wake loop, `(1 << 27)` PPI enable, no GICR_IPRIORITYR0 loop). Patch was developed against an intermediate ios-sweet commit, not an official tag. Must be rewritten for MT6785 (virtual-timer-intid=13). |
| 0018 | Boot CPU driver (SweetCPU, IOCPU interface, SGI1 self-test) | NO | iokit/Kernel/arm/SweetCPU.cpp (new), iokit/KernelConfigTables.cpp, iokit/conf/files.arm64 | Fails because KernelConfigTables.cpp lacks the `xiaomi,sweet` line added by 0016. Order dependency: 0018 requires 0016 first. Template for BegoniaCPU.cpp |
| 0065 | SMP via PSCI: sweet_psci_cpu_on SMC, secondary entry (sweet_secondary_entry in start.s), cpu.c/machine_routines.c/start.s changes | PARTIAL | iokit/Kernel/arm/SweetCPU.cpp, osfmk/arm64/cpu.c, osfmk/arm64/machine_routines.c, osfmk/arm64/start.s, pexpert/pexpert/arm64/SWEET.h | Fails because SweetCPU.cpp doesn't exist (needs 0018 first). cpu.c/machine_routines.c/start.s hunks apply. Order dependency: 0065 requires 0018. |

## Patch dependency order

Correct application order (each patch's prerequisites must be applied first):

```
0004  -> board config + GIC/timer foundation
0006  -> master config (standalone new file)
0016  -> device-tree platform expert (adds xiaomi,sweet IONameMatch)
0018  -> boot CPU driver (requires 0016's KernelConfigTables.cpp line)
0065  -> SMP PSCI (requires 0018's SweetCPU.cpp)
0017  -> virtual-timer IRQ (independent, but needs adaptation)
```

## Snapdragon-specific assumptions to strip for Begonia

ios-sweet targets Qualcomm SM7150 (sweet). These assumptions must NOT be copied to Begonia:

- `SWEET_VTIMER_INTID = 19` (Qualcomm numbers arch timer PPIs 1/2/3/0; MT6785 uses PPI 13 per POX DTS arm,armv8-timer). Begonia value = 13.
- GIC Group 0 owned by TrustZone, all interrupts as Group 1 NS (smc-based). MT6785 GICv3 also has SecurityExtn=1, but the exact Group assignment must be verified from the live FDT / arm,gic-v3 node, not assumed.
- `ml_readprop(child, "die-cluster-id", 0)` and `cluster-core-id` for topology (Qualcomm-specific DT properties). MT6785 uses standard arm cpu-map / MPIDR affinity.
- `startCPU` sets x20/x21 via `LowResetVectorBase` (Qualcomm secondary entry convention). MT6785 secondary entry via PSCI CPU_ON + `begonia_secondary_entry` must be defined per MT6785 reset vector behavior.
- `SWEET_SMP_MAX_CPUS = 6` (A55 only, A76 unaudited). For Begonia, start with 6 A55s; A76s require MIDR_EL1 probing and separate audit.
- XNU build on macOS with Xcode + KDK; Begonia target is bare-metal boot via boot shim (no macOS KDK needed at runtime).

## Begonia adaptation plan

Rather than blindly applying the Qualcomm patches, write Begonia-specific equivalents:

1. **platform/begonia/begonia.h** — done (GICv3, 16K pages, PPI 13, BEGONIA_SMP_MAX_CPUS=6)
2. **platform/begonia/BegoniaPlatformExpert.cpp** — done (matches xiaomi,begonia)
3. **platform/begonia/BegoniaCPU.cpp** — done (PSCI CPU_ON, SGI1, 6x A55 + 2x A76)
4. **config/MASTER.arm64.begonia** — documented (begonia_master_config.md)
5. **makedefs/MakeInc.def** — add BEGONIA machine config (begonia_makedefs.md)
6. **pexpert/arm/pe_fiq.c** — GICv3 Group 1 timer path (adapt 0017, use PPI 13)
7. **osfmk/arm64/sleh.c** — timer + IPI dispatch (adapt 0017)
8. **osfmk/arm64/start.s** — begonia_secondary_entry stub (adapt 0065)
9. **osfmk/arm64/cpu.c + machine_routines.c** — PSCI secondary registration (adapt 0065)

## Verification commands (run on macOS build host)

```sh
cd /home/yu/BEGONIA-XNU/xnu/xnu-8796
# Test each patch without applying:
git apply --check xnu/patches/0004-sweet-machine-config.patch
git apply --check xnu/patches/0006-sweet-master-config.patch
git apply --check xnu/patches/0016-sweet-device-tree-platform-expert.patch
git apply --check xnu/patches/0017-sweet-device-tree-virtual-timer-irq.patch
git apply --check xnu/patches/0018-sweet-boot-cpu-driver.patch
git apply --check xnu/patches/0065-sweet-smp-psci.patch
```

## Status

- Patches 0004/0006/0016 apply cleanly to xnu-8796.141.3
- Patches 0017/0018/0065 do NOT apply cleanly (context mismatches + ordering dependencies)
- No patch has been applied to the XNU source tree (tree is pristine at 1b191cb)
