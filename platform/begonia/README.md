# platform/begonia/ — Minimum XNU Platform Support

This directory holds the XNU 8796 platform code for MT6785 / Begonia. It is
intentionally minimal: only what XNU needs to reach the first kernel
initialization milestone. No Linux driver ports.

Required files (to be implemented once XNU 8796 source is available):

| File | Purpose | Hardware source (POX DTS) |
|------|---------|---------------------------|
| machine.c / platform.c | Platform registration, machine type 0x6785 | compatible="mediatek,MT6785" |
| pmap.c | Stage-1 MMU, 16K pages, identity/direct map | TCR T0SZ=28, MAIR Device/WB/NC |
| pmapping.c | Early physical memory from boot_args memSize | /memory node (live FDT) |
| gic.c | GICv3 distributor + redistributor init | dist 0x0c000000, redist 0x0c040000 |
| timer.c | Architectural timer, CNTV_CTL, PPI 13 | arm,armv8-timer PPI 13 |
| cpu.c | Boot CPU init, MIDR/MPIDR, secondary bring-up | 6x A55 + 2x A76 (probe MIDR_EL1) |
| psci.c | PSCI CPU_ON via SMC for secondaries | psci-1.0 method=smc |
| console.c | Polled UART0 early console | 0x11002000, mt6577-uart, 921600 |

Hardware addresses are NEVER hardcoded: they are read from the ADT built by
the boot shim (/arm-io/uart0, /arm-io/interrupt-controller).

Entry contract (from boot shim):
- x0 = physical address of boot_args (736 bytes, rev/ver 2)
- MMU off, data + instruction caches off
- DAIF = 0xf
- EL1

XNU patches needed (adapted from ios-sweet xnu/patches/):
- Machine config (0004), master config + open-source pmap (0006)
- Stub apple-internal headers (0007), fill closed lib gaps (0008)
- Drop Apple-SoC-only exports (0009)
- UART console instead of framebuffer (replace 0010-0015)
- Device-tree platform expert (0016) — REQUIRED
- Device-tree virtual-timer IRQ mapping PPI 13 (0017) — REQUIRED
- Boot CPU driver (0018)
- SMP via PSCI (0065) — REQUIRED

Not needed for first boot: UFS/HFS+, AMFI/sandbox, display, input, storage.
