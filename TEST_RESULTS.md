# TEST_RESULTS.md

Test results for the BEGONIA-XNU project.

## V0 — Boot shim build

| Test | Result | Evidence |
|------|--------|----------|
| boot shim cross-compiles (aarch64-linux-gnu-gcc 16.2.0) | PASS | `bootshim/build.sh`, no warnings, -Werror |
| No absolute C relocations (R_AARCH64_ABS) | PASS | `build.sh` readelf check |
| Image has ARM\x64 Linux boot header | PASS | offset 56 = "ARM\x64" |
| Entry point at 0x800 (after 64-byte header) | PASS | readelf -h Entry=0x800 |
| Image size 37477 bytes | PASS | build.json |
| sha256 recorded in build.json | PASS | build.json |

## V1 — Host FDT parser + ADT builder

| Test | Result | Evidence |
|------|--------|----------|
| Parse dtc-compiled begonia DTB | PASS | tests/test_fdt, 20/20 checks |
| Memory bank: 0x40000000 / 0x3e605000 | PASS | test_fdt output |
| UART0: 0x11002000 / 0x1000 | PASS | test_fdt output |
| GIC dist: 0x0c000000 / 0x40000 | PASS | test_fdt output |
| GIC redist: 0x0c040000 / 0x200000 | PASS | test_fdt output |
| Initrd: 0x43000000 / 0x43800000 | PASS | test_fdt output |
| fdt_chosen_u64 helper | PASS | test_fdt output |
| ADT builds, fits in 1 page | PASS | test_fdt output |

## V2 — Emulation (QEMU/MT6785 model)

Not started — no MT6785 emulation target available.

## V3 — Real hardware (fastboot boot, temporary)

Not started — awaiting hardware access.

## V4 — XNU compilation

Blocked — XNU 8796 source not yet obtained.

## V5 — First XNU boot

Not started — requires V4.
