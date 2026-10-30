# DEBUGGING.md

Debugging approach for the BEGONIA-XNU boot bring-up.

## UART console

UART0 at 0x11002000 (mt6577-uart), 921600 baud, 8N1. The boot shim uses
polled UART output (no interrupts) at every stage:
- shim_main: entry register dump (CurrentEL, MIDR, MPIDR, SCTLR, CNTFRQ)
- shim_el1: FDT parse results (memory, reserved, UART, GIC addresses)
- handoff_boot: boot_args parameters, ADT address, entry point

## Exception reporting

The shim installs exception vectors at VBAR_EL1/VBAR_EL2. On any synchronous
or asynchronous exception, `exception_report` prints the vector number,
CurrentEL, ESR/ELR/FAR for the current exception level, then halts in `wfe`.

Exception vectors are spaced 128 bytes apart starting at the `vectors` symbol.

## MMU fault diagnosis

The shim runs with the MMU off. XNU brings up its own MMU. If XNU faults:
1. Check the boot shim's final register dump (SCTLR_EL1 before jump)
2. Verify boot_args deviceTreeP is a valid VA relative to the kernel
3. Verify ADT and boot_args are within the kernel's mem_size window
4. Check that the MMU is actually off at handoff (SCTLR_EL1.M == 0)

## Boot stage markers

The shim emits these markers in order on UART:

| Marker | Meaning |
|--------|---------|
| XNU_BEGONIA_SHIM_V1 | Shim entered at EL1/EL2 |
| XNU_BEGONIA_SHIM_DONE | Entry register dump complete |
| XNU_BEGONIA_EL1_V2 | Running at EL1, FDT parsed |
| XNU_BOOT_ARGS <cmdline> | Boot args command line |
| XNU_HANDOFF_JUMP | Jumping to XNU entry |
| XNU_ENTRY_RETURNED | XNU returned (unexpected) |

A successful boot reaches XNU_HANDOFF_JUMP and then XNU's own output.

## Capturing logs

Use a USB-TTL adapter on the device UART0 TX/RX pins at 921600 8N1.
Save captures to BOOT_LOGS/YYYYMMDD-HHMMSS-attemptN.txt with an accompanying
.meta file recording the git commit and ELF sha256.

## Never

- Never fabricate UART output
- Never guess register values — read them from the live FDT
- Never modify critical boot partitions during initial development
