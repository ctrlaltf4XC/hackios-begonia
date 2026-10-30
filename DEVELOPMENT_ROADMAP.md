# DEVELOPMENT_ROADMAP.md

BEGONIA-XNU development roadmap — path to first XNU boot on MT6785/Begonia.

## Phase 0 — Repository Audit (DONE)
Three repositories cloned and audited:
- pox-begonia-kernel @ 2fb79cd9cba6 (Linux 4.14.357, MT6785 DTS)
- ios-sweet @ 45263be (Qualcomm sweet XNU port reference)
- xnu_bootshim_benesse_a04br3 @ f82c3b8 (MT8167 bootshim reference)

## Phase 1 — Hardware Extraction (DONE)
POX_HARDWARE_ANALYSIS.md delivered with exact source references:
- DRAM: 0x40000000 / 0x3e605000 (VERIFY vs 6GiB)
- UART0: 0x11002000, mt6577-uart, IRQ 115, 921600 baud
- GICv3: dist 0x0c000000/0x40000, redist 0x0c040000/0x200000
- armv8-timer: PPI 13/14/11/10, CNTFRQ from bootloader
- PSCI: smc, psci-1.0
- CPU: 6x A55 + 2x big (DTS says A75, target says A76)

## Phase 2 — Boot Architecture (DONE)
ARCHITECTURE.md, BOOT_FLOW.md, BOOT_VALIDATION.md delivered. Boot shim ABI
frozen (boot_args 736 bytes rev/ver 2, ADT layout, entry convention).

## Phase 3 — Minimum XNU Platform (IN PROGRESS)
platform/begonia/ stubs ready. Requires XNU 8796 source to implement:
- CPU state (MIDR/MPIDR, VBAR, DAIF)
- Physical memory (16K pages, bootmem from boot_args memSize)
- MMU (stage-1, TCR T0SZ=28, MAIR Device/WB/NC)
- GICv3 (distributor + redistributor, ICC sysreg interface)
- Architectural timer (CNTV_CTL, PPI 13)
- PSCI CPU_ON for secondary cores

## Phase 4 — Build and Boot (PARTIAL)
- build.sh, validate.sh, package.sh operational
- Host FDT/ADT test: 20/20 pass
- XNU 8796 source not yet obtained
- XNU compilation not yet attempted

## Phase 5 — First Boot Verification (NOT STARTED)
Success criteria:
- Bootshim execution confirmed (UART: XNU_BEGONIA_SHIM_V1)
- XNU image loaded (UART: XNU_HANDOFF_JUMP)
- XNU entry reached (UART: XNU_BEGONIA_KERNEL_ENTRY)
- XNU initialization confirmed (UART: XNU console init)
- Repeatable test evidence (BOOT_LOGS/)

## Known discrepancies (to resolve)
- POX DTS: cpu6/cpu7 = cortex-a75; target: Cortex-A76 → probe MIDR_EL1
- POX DTS: ~1GiB DRAM; device: 6GiB → parse live FDT /memory
- XNU source: must be obtained from Apple open-source (xnu-8796.x.x)
