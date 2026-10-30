# XNU_BOOT_REQUIREMENTS.md

Minimum XNU platform support required to reach the first meaningful kernel
initialization milestone on MT6785 / Begonia. Based on XNU 8796 (iOS 16
generation) and the ios-sweet port reference (dipa-studio/ios-sweet @ 45263be).

## Source references

- ios-sweet shim: `shim/shim.c`, `shim/start.S`, `shim/mmu.c`, `shim/loader.c`
- ios-sweet XNU patches: `xnu/patches/0001-..0081-*.patch`
- bootshim ABI: `xnu_bootshim_benesse_a04br3/handoff.h`, `handoff.c`, `adt.c`

## boot_args ABI (XNU pexpert/arm64/boot.h, revision/version 2)

Struct layout (from ios-sweet `shim/loader.c:36-50` and bootshim
`handoff.h:13-23`), total 736 bytes:

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| 0 | Revision / version | uint16 x2 | = 2, 2 |
| 4 | virtBase | uint64 | kernel virtual base |
| 12 | physBase | uint64 | kernel physical base |
| 20 | memSize | uint64 | requested memory size |
| 28 | topOfKernelData | uint64 | end of shim-managed memory |
| 36 | Video (v_baseAddr, v_display, v_rowBytes, v_width, v_height, v_depth) | 6 x uint64 | framebuffer |
| 84 | machineType | uint32 | board id |
| 96 | deviceTreeP | uint64 | ADT physical address (VA when MMU on) |
| 104 | deviceTreeLength | uint32 | ADT size |
| 108 | CommandLine | char[608] | NUL-terminated boot args |
| 716 | bootFlags | uint64 | |
| 724 | memSizeActual | uint64 | actual memory size |
| total | | 736 | static-asserted |

Static asserts (ios-sweet `loader.c:47-50`):
- offsetof(Video) == 40
- offsetof(CommandLine) == 108
- sizeof == 736

Boot args are passed at a **physical** address in x0 on entry; XNU re-interprets
deviceTreeP as a VA once the MMU is on (ios-sweet `loader.c:132`).

## ADT (Apple Device Tree) requirements

The ADT is an Apple-flavored flattened device tree, separate from the Linux FDT.
Minimum required nodes (from bootshim `adt.c`):

- Root: name "device-tree", device_type "device-tree", compatible, model,
  #address-cells=2, #size-cells=2
- `/chosen`: boot-manifest-hash, dram-base, dram-size, debug-enabled=1,
  memory-map, (optional) RAMDisk, cpus count, asmb, unique-chip-id,
  certificate-production-status=0 (development), random-seed
- `/cpus`: one entry per CPU, compatible "arm,cortex-a55\0ARM,v8", reg, state
  ("running"/"stopped"), timebase-frequency, cpu-id, cluster-id
- `/arm-io`: device_type "soc", compatible "mediatek,mt6785", ranges,
  `#address-cells=2`, `#size-cells=2`
  - `uart0`: device_type "serial", compatible "mediatek,mt6577-uart", reg
  - `interrupt-controller`: compatible "arm,gic-v3" (or gic-400), reg,
    virtual-timer-intid
- `/defaults`: no-effaceable-storage=1 (no Apple effaceable storage hardware)

XNU's `pmap_compute_max_asids` requires `/defaults`; an absent ASID override
selects XNU's MAX_ASIDS — do not invent a board-specific count.

## Minimum platform subsystems XNU must provide (platform/begonia/)

Priority order. Only what is needed to reach kernel initialization:

1. **ARM64 CPU state** — read MIDR_EL1/MPIDR_EL1, confirm ARMv8, set up
   exception vectors (VBAR_EL1), mask DAIF.
2. **Physical memory management** — mem_map / physmem region from boot_args
   memSize, 16K page granularity, bootmem-style early alloc before pmap.
3. **MMU configuration** — stage-1 translation, 16K granule, identity or
   direct-map, MAIR attributes (Device / WB / NC), TTBR0_EL1, TCR_EL1
   (T0SZ=28 for 36-bit VA), SCTLR_EL1.M/C/I.
4. **Interrupt controller** — GICv3 distributor + redistributor init, PPI/SPI
   enables, ICC_PMR/ICC_BSR, route virtual timer PPI to EL1.
5. **Architectural timer** — CNTFRQ_EL0 from bootloader, CNTV_CTL/CNTP_CTL
   setup, virtual timer interrupt (PPI 13 per POX DTS) handled at EL1.
6. **CPU initialization** — boot CPU: pmap bootstrap, console init, kernel
   continue; secondaries held in PSCI CPU_OFF state until started.
7. **SMP requirements** — PSCI CPU_ON via SMC to bring up secondary A55/A76
   cores, each reporting MPIDR.

## What XNU does NOT need for first boot (out of scope)

- Storage (UFS/eMMC/SD), filesystem, block layer
- Network, Wi-Fi, Bluetooth
- Display / GPU / framebuffer driver (console can be UART polled or a
  stub; ios-sweet uses framebuffer but UART is simpler for bring-up)
- USB, PCI, DMA, IOMMU
- Power management / cpuidle beyond PSCI CPU_OFF
- Watchdog feeding (leave to bootloader; rule: preserve Android boot path)
- Security: AMFI, sandbox, code-signing, AppleKeyStore (stub/permissive)
- Userspace: launchd, SpringBoard, HFS+ root

## Entry convention (handoff contract)

bootshim jumps to XNU Mach-O entry:
- x0 = physical address of boot_args (736-byte struct)
- MMU off, data + instruction caches off
- DAIF = 0xf (all exceptions masked)
- EL1 (or EL2->EL1 as in bootshim reference)
- Stack set by shim; XNU `start_first_cpu` sets its own stack

The entry point is the Mach-O `entry_va` relocated to physical:
`entry_pa = phys_base + (entry_va - virt_base)`.

## XNU patches needed (from ios-sweet, adapted for MT6785)

ios-sweet `xnu/patches/` categories relevant to a minimum MT6785 port:

- 0001-0009: toolchain compatibility, machine config, non-PAC arm64, stubs
- 0004: sweet machine-config → begonia machine-config
- 0006: master config, open-source asm pmap
- 0007: stub apple-internal headers
- 0008: fill closed lib gaps
- 0009: drop Apple-SoC-only exports
- 0010-0018: framebuffer console (replace with UART for MT6785), physmap,
  PRNG without corecrypto, device-tree platform expert, timer PPI,
  boot CPU driver
- 0016: device-tree platform expert (required — XNU must parse ADT)
- 0017: device-tree virtual-timer IRQ (required — map PPI 13)
- 0018: boot CPU driver
- 0065: SMP via PSCI (required — MT6785 uses PSCI smc like Qualcomm)

Not needed for first boot: UFS/HFS+ (0021-0034), AMFI/sandbox (0025-0036,
0050), display (0046), input (0058-0063), storage (0028-0034).

## Open questions / verification items

1. XNU 8796 source tree must be obtained (Apple open-source xnu-8796.x.x).
   ios-sweet builds on macOS via blacktop/darwin-xnu-build; Linux build
   needs clang + cctools or equivalent.
2. The virtual timer PPI for MT6785 is 13 (POX DTS); XNU expects to program
   CNTV_CTL and receive the interrupt at EL1 via GIC.
3. Whether XNU enters at EL1 or requires EL2->EL1 transition (bootshim
   reference handles both).
4. Memory size: use live FDT `/memory` total, not POX's ~1 GiB DTS value.
