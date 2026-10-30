# KNOWN_ISSUES.md

Known issues and discrepancies in the BEGONIA-XNU port.

## Hardware discrepancies

### CPU type mismatch
POX DTS (arch/arm64/boot/dts/mediatek/mt6785.dts:110-128) marks cpu6/cpu7 as
`arm,cortex-a75`. The target hardware (MT6785 / Helio G90T) has 2x
Cortex-A76 + 6x Cortex-A55. The DTS compatible string may be inaccurate.
Impact: XNU platform code must probe MIDR_EL1 at boot to determine the actual
CPU type, not rely on the DTS compatible string.
Status: open — verify by reading MIDR_EL1 on real hardware.

### DRAM size
POX DTS describes memory at 0x40000000 / 0x3e605000 (~998 MiB). The Redmi
Note 8 Pro has 6 GiB RAM. Either a second high memory bank exists (not in
this DTS) or the POX DTS only describes the Linux-visible low bank.
Impact: XNU mem_size must come from the live FDT /memory node(s), not the
POX DTS value.
Status: open — verify live FDT on real hardware.

## Implementation issues

### Boot shim entry exception level
The shim supports both EL1 and EL2 entry (start.S checks CurrentEL). The
MediaTek LK bootloader's actual entry EL for payloads is unconfirmed.
Impact: if LK enters at EL2, the shim transitions to EL1h; if EL1, direct.
Status: open — confirm MediaTek LK entry convention.

### Watchdog
The toprgu watchdog (0x10007000, SPI 72) is running from the bootloader. The
shim must not stall, or the device will reset.
Impact: keep boot shim code paths short; do not stall in loops.
Status: open — measure boot time on real hardware.

### XNU source not yet obtained
Apple XNU 8796 source (xnu-8796.x.x) must be obtained from Apple's open
source release and patched with the ios-sweet-compatible patch set.
Impact: platform/begonia/ and XNU compilation cannot proceed without it.
Status: blocked — obtain XNU source.

## Not-yet-tested
- Boot shim on real hardware (V2/V3)
- XNU compilation (V4)
- First XNU boot (V5)
