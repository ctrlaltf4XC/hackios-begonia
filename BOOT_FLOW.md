# BOOT_FLOW.md

Begonia (MT6785) boot flow: MediaTek bootloader -> Begonia boot shim -> XNU 8796.

Documented interfaces are frozen (see ARCHITECTURE.md). All hardware addresses
are read from the live FDT at boot; nothing is invented.

## Boot stages

```
Stage 0: MediaTek Bootloader (LK/ABL)
Stage 1: Begonia Boot Shimm (bootshim/)
Stage 2: XNU 8796 Kernel (xnu/ + platform/begonia/)
```

### Stage 0 — MediaTek bootloader
- Loads the boot shim Image (ARM64 Linux Image format, "ARM\x64" magic).
- Enters the shim at EL1 (or EL2, both supported) with:
  - MMU off, data + instruction caches off
  - DAIF = 0xf (all exceptions masked)
  - x0 = pointer to the live device tree blob (DTB)
  - SP set by the bootloader
- The bootloader has already initialized UART0 (0x11002000) at 921600 baud;
  the shim preserves the inherited baud rate.
- PSCI (arm,psci-1.0 via SMC) is available for secondary CPU bring-up.
- The watchdog (toprgu, 0x10007000) is running; the shim must not stall.

### Stage 1 — Begonia boot shim (`bootshim/`)
Entry: `_start` in `start.S`, after the 64-byte Linux Image header.
`start.S`:
1. Masks DAIF, saves bootloader registers (x0-x3, sp) into x19-x23.
2. Sets up a private stack at `_stack_top`.
3. Installs exception vectors (VBAR_EL1 or VBAR_EL2 depending on CurrentEL).
4. Zeroes .bss.
5. Jumps to `shim_main(x0=dtb, x1-x3, x4=old_sp)`.

`shim_main` (`shim.c`):
1. Reports CurrentEL, MIDR_EL1, MPIDR_EL1, SCTLR, CNTFRQ over UART.
2. If at EL2: verifies SCTLR_EL2 has MMU/I-cache/D-cache off, then transitions
   to EL1h via `enter_el1` (`start.S`) which installs EL1 vectors/stack and
   erets to `shim_el1`.
3. If at EL1: calls `shim_el1(dtb)` directly.

`shim_el1` (`shim.c`):
1. Parses the live FDT via `fdt_inspect()` (`fdt.c`): DRAM regions,
   reserved-memory, /chosen linux,initrd{start,end}, UART base, GIC
   distributor/redistributor bases.
2. Reports memory map, UART, GIC addresses over UART.
3. Calls `handoff_boot(&observed_fdt, dtb)` (`handoff.c`).

`handoff_boot` (`handoff.c`):
1. Locates the XNU payload in the initrd region
   (fdt->initrd_start .. fdt->initrd_end).
2. Validates the payload:
   - Magic "BEGXNU01"
   - Segment bounds within payload mem_size
   - No overlap with FDT, reserved memory, or shim image
   - Entry point covered by exactly one segment
   - `aligned(kernel_end) == kernel_end_pa`
   - Command line NUL-terminated
   - CPU state: EL1, SCTLR_EL1.M/C/I off, CNTFRQ fits in 32 bits
   - If any check fails, returns a status string and stops.
3. Builds the Apple ADT via `xnu_make_adt()` (`adt.c`):
   - Root: compatible "xiaomi,begonia\0mediatek,MT6785", model
   - /chosen: dram-base, dram-size, debug-enabled, memory-map (Kernel-Begonia)
   - /cpus: one entry per requested CPU (running/stopped), timebase-frequency
   - /arm-io: uart0 (reg from FDT), interrupt-controller arm,gic-v3
     (regs from FDT), virtual-timer-intid=13
   - /defaults: no-effaceable-storage=1
4. Builds the XNU `boot_args` (736 bytes, revision/version 2):
   - virt_base, phys_base, mem_size, top_of_kernel_data
   - deviceTreeP = ADT (VA relative to kernel), deviceTreeLength
   - machineType = 0x6785
   - CommandLine copied from payload
   - Video fields zeroed (no framebuffer driver in minimum boot)
5. Copies each payload segment to its physical address, zeroes BSS.
6. Writes ADT at `adt_pa`, boot_args at `args_pa`, zeroes the page gap.
7. Emits "XNU_HANDOFF_JUMP", does `dsb sy; ic iallu; dsb sy; isb`.
8. Jumps to `entry_pa` with x0 = `args_pa` (physical), MMU off, caches off.

### Stage 2 — XNU kernel entry (`xnu/` + `platform/begonia/`)
- XNU `start_first_cpu` receives x0 = boot_args physical address.
- MMU and caches are off at entry; XNU brings up its own pmap/MMU.
- XNU parses the ADT via its device-tree platform expert to discover:
  - CPU topology (/cpus), DRAM (/chosen dram-base/dram-size)
  - UART (/arm-io/uart0) for early console
  - GIC (/arm-io/interrupt-controller) for interrupts
  - Virtual timer PPI (virtual-timer-intid=13)
- XNU initializes: pmap, physical memory, GIC, architectural timer, console.
- First success milestone: XNU reaches `kernel_bootstrap` and emits its first
  UART diagnostics.

## Address contract (all from live FDT)

| Item | Source | POX DTS reference value |
|------|--------|-------------------------|
| UART0 base | /arm-io/uart0 reg (fdt) | 0x11002000 (mt6785.dts:2093) |
| GIC distributor | /arm-io/interrupt-controller reg | 0x0c000000 (mt6785.dts:411) |
| GIC redistributor | /arm-io/interrupt-controller reg | 0x0c040000 (mt6785.dts:412) |
| DRAM base/size | /memory reg | 0x40000000 / 0x3e605000 |
| Virtual timer PPI | arm,armv8-timer interrupts | PPI 13 (mt6785.dts:779) |
| PSCI | /psci method=smc | arm,psci-1.0 (mt6785.dts:264) |

## Exit conditions

The shim never returns to the bootloader on success. On failure it emits a
status string over UART and enters `wfe` halt. Failure strings:
- `FDT_INVALID` — DTB parse failed
- `XNU_PAYLOAD_NOT_PRESENT` — no valid initrd payload
- `XNU_PAYLOAD_PLAN_REJECTED` — handoff_validate failed
- `XNU_HANDOFF_CPU_STATE_REJECTED` — wrong EL / MMU on / bad CNTFRQ
- `XNU_ADT_BUILD_FAILED` — ADT serialization failed
- `XNU_PAYLOAD_PLAN_REJECTED` / overlap errors
- `XNU_ENTRY_RETURNED` — XNU returned to shim (unexpected)

## Validation evidence required (BOOT_VALIDATION.md)

- UART capture showing `XNU_BEGONIA_SHIM_V1`, register dump, `XNU_BEGONIA_SHIM_DONE`
- UART capture showing `XNU_BEGONIA_EL1_V2`, FDT memory/UART/GIC values
- UART capture showing `XNU_BOOT_ARGS <cmdline>`, `XNU_HANDOFF_JUMP`
- UART capture showing XNU's own `XNU_BEGONIA_KERNEL_ENTRY` marker
- build.json with sha256 of the Image (reproducible)
