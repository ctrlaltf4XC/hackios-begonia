# POX_HARDWARE_ANALYSIS.md

MT6785 / Begonia hardware map extracted from pox-begonia-kernel @ 2fb79cd9cba6.
Source: Linux 4.14.357 device tree + Kconfig. ALL addresses below are taken
directly from the checked-out DTS; none are invented.

## Source files (exact paths)

- SoC DTS: `arch/arm64/boot/dts/mediatek/mt6785.dts`
- Board DTS: `arch/arm64/boot/dts/mediatek/begonia.dts`
- Board include: `arch/arm64/boot/dts/mediatek/begonia-mt6785.dtsi`
- Common include: `arch/arm64/boot/dts/mediatek/xiaomi-mt6785-common.dtsi`
- Defconfig: `arch/arm64/configs/begonia_user_defconfig`
- Makefile version: `Makefile` (VERSION=4 PATCHLEVEL=14 SUBLEVEL=357)

## 1. Processor / CPU topology

`mt6785.dts:46-128` — `cpus` node, 8 CPUs in two clusters, enable-method "psci":

| CPU | reg (MPIDR aff0) | compatible | cluster | frequency |
|-----|------------------|------------|---------|-----------|
| cpu0 | 0x000 | arm,cortex-a55 | CL0 | 1701000000 |
| cpu1 | 0x01 | arm,cortex-a55 | CL0 | 1701000000 |
| cpu2 | 0x02 | arm,cortex-a55 | CL0 | 1701000000 |
| cpu3 | 0x03 | arm,cortex-a55 | CL0 | 1701000000 |
| cpu4 | 0x04 | arm,cortex-a55 | CL0 | 1701000000 |
| cpu5 | 0x05 | arm,cortex-a55 | CL0 | 1701000000 |
| cpu6 | 0x06 | arm,cortex-a75 | CL1 | 2171000000 |
| cpu7 | 0x07 | arm,cortex-a75 | CL1 | 2171000000 |

`cpu-map` (mt6785.dts:131-165): cluster0 = cpu0..cpu5, cluster1 = cpu6..cpu7.

**DISCREPANCY:** Target hardware spec states 2x Cortex-A76 + 6x Cortex-A55
(MT6785/Helio G90T). POX DTS marks cpu6/cpu7 as `arm,cortex-a75`. The DTS
compatible string may be inaccurate; the actual silicon is A76. XNU platform
code must probe MIDR_EL1 at boot and not rely on the DTS compatible string.

CPU idle states use `arm,psci` entry method (mt6785.dts:167-229). PSCI node:
`mt6785.dts:263-266` — compatible "arm,psci-1.0", method "smc".

PMU: `arm,armv8-pmuv3`, PPI 7 (mt6785.dts:268-272).

## 2. Physical memory

`mt6785.dts:281-284` — `memory` node:
```
reg = <0 0x40000000 0 0x3e605000>;
```
- DRAM base: **0x40000000**
- DRAM size: **0x3e605000** (~998 MiB)

**VERIFICATION REQUIRED:** The Redmi Note 8 Pro ships with 6 GiB RAM. A single
~1 GiB bank at 0x40000000 cannot be the full DRAM. Either (a) a second high
memory bank exists (e.g. at 0x100000000 as on Qualcomm sweet) not described in
this DTS, or (b) the POX DTS only carves the Linux-visible low bank. The
bootshim must parse the *live* FDT `/memory` node(s) at runtime and not trust
this value. XNU `mem_size` in boot_args must come from the live FDT.

## 3. Reserved memory

`mt6785.dts:301-388` — `/reserved-memory`, `#address-cells=2`, `#size-cells=2`:

| Reservation | compatible | size | alignment | alloc-range | no-map |
|-------------|------------|------|-----------|-------------|--------|
| reserve-memory-ssmr | mediatek,reserve-memory-ssmr | 0x1000 | — | 0xc0000000 (4GB) | yes |
| zmc-default | mediatek,zone_movable_cma | 0x2d000000 | 0x1000000 | 0xc0000000 | — |
| reserve-memory-sspm_share | mediatek,reserve-memory-sspm_share | 0x110000 or 0x510000 | 0x10000 | 0x40000000..0x60000000 | yes |
| reserve-memory-scp_share | mediatek,reserve-memory-scp_share | 0x300000 | 0x1000000 | 0x40000000..0x50000000 | yes |
| ion-carveout-heap | mediatek,ion-carveout-heap | 0xc000 | 0x1000 | 0x40000000..0x80000000 | yes |
| consys-reserve-memory | mediatek,consys-reserve-memory | 0x400000 | 0x1000000 | 0x40000000..0x80000000 | yes |
| wifi_mem | shared-dma-pool | 0x300000 | 0x1000000 | 0x40000000..0x80000000 | yes |
| reserve-memory-adsp_share | mediatek,reserve-memory-adsp_share | 0x1000000 | 0x1000 | 0x40000000..0x40000000 | yes |
| soter-shared-mem | microtrust,shared_mem | 0x200000 or 0xc00000 | 0x200000 | 0x40000000..0x50000000 | yes |

`xiaomi-mt6785-common.dtsi:22-34` — `/chosen` framebuffer reservation hints:
- atag,videolfb-fb_base_l = 0x7e605000
- atag,videolfb-vramSize = 0x017e8000

These framebuffer addresses (0x7e605000) lie below DRAM base 0x40000000,
suggesting the POX DTS describes a fragmented/aliased view. The bootshim must
use the live FDT reserved-memory node to exclude carve-outs from the XNU
memory window.

## 4. Interrupt controller (GIC)

`mt6785.dts:403-415` — `gic` node:
```
compatible = "arm,gic-v3";
reg = <0 0x0c000000 0 0x40000>,   // distributor
      <0 0x0c040000 0 0x200000>,  // redistributor
      <0 0x0c53a650 0 0x50>;      // INT_POL
#interrupt-cells = <3>;
#redistributor-regions = <1>;
interrupts = <GIC_PPI 9 IRQ_TYPE_LEVEL_HIGH>;
```

- **GICv3** with security extensions
- GICD base: **0x0c000000**, size 0x40000
- GICR base: **0x0c040000**, size 0x200000
- INT_POL: 0x0c53a650
- Uses GIC system-register interface (ICC_* sysregs), not memory-mapped CPU
  interface (consistent with ios-sweet which uses ICC_SRE/ICC_IAR1_EL1 etc.)

`sysirq` (mt6785.dts:417-423): mediatek,mt6577-sysirq, parent = &gic, reg at
INT_POL 0x0c53a650.

Notable SPI allocations: UART0 IRQ 115, UART1 IRQ 116, sys_timer SPI 224,
apdma 134-137, toprgu (watchdog) SPI 72, eint/pio SPI 204.

## 5. Architectural timer

`mt6785.dts:776-784` — `timer` node:
```
compatible = "arm,armv8-timer";
interrupts = <GIC_PPI 13 IRQ_TYPE_LEVEL_LOW>,   // virtual timer
             <GIC_PPI 14 IRQ_TYPE_LEVEL_LOW>,
             <GIC_PPI 11 IRQ_TYPE_LEVEL_LOW>,
             <GIC_PPI 10 IRQ_TYPE_LEVEL_LOW>;
clock-frequency = <13000000>;
```

- Virtual timer PPI = **13** (physical timer PPI would be different; on XNU
  the virtual timer is typically INTID 27 on Apple, but here the DT describes
  PPI 13-10).
- clock-frequency property = 13000000 (this is the DT description; the actual
  CNTFRQ_EL0 is programmed by the bootloader and must be read at runtime).

`sys_timer` (mt6785.dts:768-774): mediatek,sys_timer, reg 0x10017000, SPI 224,
clock TOP_CLK13M. This is a MediaTek system timer, NOT the architectural timer
XNU uses. XNU uses the ARMv8 architectural timer (CNTP_CTL/CNTV_CTL).

## 6. UART (console)

`mt6785.dts:2091-2100` — `apuart0`:
```
compatible = "mediatek,mt6577-uart";
reg = <0 0x11002000 0 0x1000>;
interrupts = <GIC_SPI 115 IRQ_TYPE_LEVEL_LOW>;
clocks = <&clk26m>, <&infracfg_ao INFRACFG_AO_UART0_CG>;
clock-names = "baud", "bus";
```

`mt6785.dts:2102-2110` — `apuart1`: 0x11003000, SPI 116.

`mt6785.dts:38-42` — bootargs console:
```
console=tty0 console=ttyS0,921600n1
```
- **UART0 base: 0x11002000**, compatible mt6577-uart, baud 921600, IRQ 115.
- Clock source: clk26m (26 MHz fixed clock, mt6785.dts:432-436).

The bootshim should poll UART0 at 0x11002000 for diagnostics. The bootloader
(LK) is expected to have already initialized UART0 at 921600; the shim must
preserve the inherited baud rate (as ios-sweet/bootshim reference does).

## 7. Watchdog

`mt6785.dts:576-580` — `toprgu`:
```
compatible = "mediatek,toprgu";
reg = <0 0x10007000 0 0x1000>;
interrupts = <GIC_SPI 72 IRQ_TYPE_LEVEL_LOW>;
```
- Watchdog base: 0x10007000, SPI 72.
- Rule 11: preserve Android boot path; do not touch watchdog during initial
  development. The inherited watchdog may reset the device if the shim stalls.

## 8. Exception level / boot state

The MediaTek bootloader (LK) enters payloads at **EL1** with MMU and caches
off, x0 = DTB pointer (this is the Qualcomm ABL convention documented in
ios-sweet `shim/start.S`; MediaTek LK convention must be confirmed — see
bootshim agent). PSCI is available via SMC for CPU ON/OFF.

## 9. Cache / MMU assumptions (Linux)

- `begonia_user_defconfig:9` — CONFIG_ARM64_PAGE_SHIFT=12 (4K pages, Linux)
- `begonia_user_defconfig:479` — CONFIG_ARM64_VA_BITS=39
- `begonia_user_defconfig:36` — CONFIG_PGTABLE_LEVELS=3

XNU uses **16K pages** (XNU_PAGE = 0x4000) and a 36-bit VA (T0SZ=28) as in
ios-sweet `shim/mmu.c`. The shim and XNU must use 16K granules regardless of
Linux's 4K config.

## 10. Device tree structure

- Root: model="BEGONIA", compatible="mediatek,MT6785"
- interrupt-parent = &sysirq (mt6785.dts:32)
- `#address-cells = <2>`, `#size-cells = <2>` at root
- `/chosen` with bootargs and kaslr-seed (mt6785.dts:37-44)
- `/cpus` with cpu-map and idle-states (mt6785.dts:46-230)
- `/memory` at 0x40000000 (mt6785.dts:281-284)
- `/reserved-memory` (mt6785.dts:301-388)
- `/chosen` framebuffer hints in xiaomi-mt6785-common.dtsi:22-34

## 11. Notes / open questions for bootshim agent

1. DRAM size (~1 GiB) vs actual 6 GiB — parse live FDT at runtime.
2. Bootloader entry EL (EL1 vs EL2) for MediaTek LK — must be confirmed; the
   bootshim reference supports both EL1 and EL2 entry.
3. UART0 is already initialized by LK at 921600 — preserve baud, do not
   reprogram clocks.
4. PSCI via SMC is available for secondary CPU bring-up.
5. GICv3 redistributor must be walked to find this PE's GICR base (as in
   ios-sweet `find_gicr()`).
