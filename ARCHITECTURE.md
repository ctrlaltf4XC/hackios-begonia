# BEGONIA-XNU — Architecture

## Boot flow (Phase 2 design)

```
MediaTek Bootloader (LK/ABL)
       |  enters at EL1, MMU off, x0 = DTB pointer
       v
Begonia Bootshim (bootshim/)
       |  - validate CPU state (EL1, MMU/cache off)
       |  - parse live FDT for DRAM/reserved/initrd
       |  - load XNU Mach-O payload (from initrd/ramdisk)
       |  - build Apple boot_args (physical) + ADT
       |  - cache maintenance (dcache flush, icache invalidate)
       v
ARM64 Environment (MMU off, caches off)
       |
       v
XNU Mach-O Entry (xnu/)
       |  x0 = boot_args physical address
       v
XNU Kernel Initialization (platform/begonia/)
```

## Subsystem boundaries
- **bootshim/**: bare-metal, position-independent, `aarch64-linux-gnu-gcc`, Linux Image header format. Owns MMU-off handoff.
- **xnu/**: XNU 8796 source + platform patches. Owns kernel init once entered.
- **platform/begonia/**: minimum platform files (startup, pmap, GIC, timer, CPU). No Linux driver ports.
- **drivers/**: shim-only helpers (polled UART, spin delays).
- **tools/**: payload packing, ADT generation, build scripts.

## Shared interfaces (frozen after Architect approval)
- boot_args layout: XNU revision/version 2, 736 bytes, device_tree at offset 96, command_line at offset 108 (matches ios-sweet `shim/loader.c` and bootshim `handoff.h`).
- ADT: Apple flattened device tree, `/chosen` + `/cpus` + `/arm-io` + `/defaults`.
- Entry convention: jump to physical entry address, x0 = boot_args PA, MMU off, caches off, DAIF masked.

## Address assumptions (ALL must be verified against live FDT, never hardcoded)
- DRAM base/size: from FDT `/memory` node (POX DTS: 0x40000000 / 0x3e605000).
- UART base: from FDT `serial` node (POX DTS: 0x11002000).
- GIC base: from FDT `interrupt-controller` node (POX DTS: 0x0c000000).
- Reserved regions: from FDT `/reserved-memory`.
