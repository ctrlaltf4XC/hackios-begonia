# BOOT_VALIDATION.md

Boot validation evidence requirements for the first XNU boot on MT6785 /
Begonia. Rule 9: every implementation must include validation evidence. Rule
10: never fabricate hardware test results.

## Validation levels

| Level | What must be shown | Method |
|-------|-------------------|--------|
| V0 | Boot shim builds cleanly | `./build.sh` + `./validate.sh` |
| V1 | Host FDT parser + ADT builder validated | `tests/test_fdt` against dtc-compiled begonia DTB |
| V2 | Boot shim runs under emulation (QEMU virt/MT6785 model) | UART capture of shim register dump |
| V3 | Boot shim runs on real hardware via `fastboot boot` (temporary, RAM only) | UART capture of full handoff |
| V4 | XNU entry reached | UART: `XNU_BEGONIA_SHIM_DONE` -> `XNU_HANDOFF_JUMP` -> XNU marker |
| V5 | XNU initialization confirmed | UART: XNU `kernel_bootstrap` / console init output |

## Required UART evidence (per boot attempt)

Each boot attempt must capture the full UART log at 921600 8N1 on UART0
(0x11002000). The log must contain, in order:

1. `XNU_BEGONIA_SHIM_V1` — shim entered, register dump
2. `XNU_BEGONIA_EL1_V2` — EL1 state, FDT memory/UART/GIC values
3. `XNU_BOOT_ARGS <cmdline>` — boot args command line
4. `XNU_HANDOFF_JUMP` — jumping to XNU entry
5. (XNU) `XNU_BEGONIA_KERNEL_ENTRY` — XNU reached its entry point
6. (XNU) first `kprintf`/`printf` from XNU console init

A boot is only "verified" at level V4+ when items 1-5 appear in a single
captured UART log.

## Artifacts to preserve

- `bootshim/out/build.json` — sha256 of the Image (reproducible build)
- `bootshim/out/symbols.txt` — symbol addresses for crash analysis
- `BOOT_LOGS/<date>-<attempt>.txt` — raw UART capture
- `BOOT_LOGS/<date>-<attempt>.meta` — build git commit, ELF sha256, DTB source

## BOOT_LOGS directory

Empty until hardware testing begins. Logs are named:
`YYYYMMDD-HHMMSS-attemptN.txt`

## Current status

V0: PASS — boot shim builds cleanly (37349 bytes, AArch64, ARM\x64 header)
V1: PASS — host FDT parser + ADT builder: 20/20 checks passed against a
    dtc-compiled begonia-like DTB
V2: NOT STARTED — no QEMU/emulation target for MT6785 yet
V3: NOT STARTED — awaiting real hardware access
V4: NOT STARTED — XNU 8796 source not yet integrated
V5: NOT STARTED

## Failure strings to watch for (shim -> UART)

- `FDT_INVALID` — DTB parse failed
- `XNU_PAYLOAD_NOT_PRESENT` — no valid BEGXNU01 payload in initrd
- `XNU_PAYLOAD_PLAN_REJECTED` — handoff validation failed (overlap/bounds)
- `XNU_HANDOFF_CPU_STATE_REJECTED` — wrong EL / MMU on / bad CNTFRQ
- `XNU_ADT_BUILD_FAILED` — ADT serialization failed
- `XNU_ENTRY_RETURNED` — XNU returned to shim (unexpected)
