# BEGONIA-XNU — TODO

## Phase 0 — Repository Audit (DONE)
- [x] Clone pox-begonia-kernel @ 2fb79cd9cba6
- [x] Clone ios-sweet @ 45263be
- [x] Clone xnu_bootshim_benesse_a04br3 @ f82c3b8
- [x] Record branches, commits, toolchain versions

## Phase 1 — Hardware Extraction
- [x] Extract DRAM boundaries (0x40000000, size 0x3e605000 — VERIFY vs 6GB)
- [x] Extract reserved memory map
- [x] Extract GIC registers (0x0c000000 dist, 0x0c040000 redist)
- [x] Extract timer (armv8-timer PPI 13/14/11/10, sys_timer 0x10017000)
- [x] Extract UART (UART0 0x11002000, console 921600)
- [x] Extract CPU topology (6x A55 + 2x big, DTS says A75 vs target A76)
- [x] Extract PSCI method (smc, psci-1.0)
- [x] Deliver POX_HARDWARE_ANALYSIS.md

## Phase 2 — Boot Architecture (DONE)
- [x] Deliver XNU_BOOT_REQUIREMENTS.md
- [x] Deliver ARCHITECTURE.md (bootshim -> XNU interfaces)
- [x] Approve first implementation milestone
- [x] bootshim/ implemented (start.S, shim.c, fdt.c, adt.c, handoff.c, build.sh, shim.ld)
- [x] bootshim builds cleanly (37477 bytes, AArch64, ARM\x64 header)
- [x] Deliver BOOT_FLOW.md
- [x] Deliver BOOT_VALIDATION.md

## Phase 3 — Minimum XNU Platform (DONE — skeleton)
- [x] platform/begonia/begonia.h — board config (GICv3, 16K pages, PPI 13, 8 CPUs)
- [x] platform/begonia/BegoniaPlatformExpert.cpp — device-tree platform expert
- [x] platform/begonia/BegoniaCPU.cpp — PSCI CPU_ON SMP bring-up
- [x] platform/begonia/begonia_master_config.md — XNU MASTER.arm64.begonia
- [x] platform/begonia/begonia_makedefs.md — MakeInc.def machine config
- [x] XNU_SOURCE_ACQUISITION.md — XNU 8796.141.3 @ 1b191cb acquired + pinned
- [x] PATCH_COMPATIBILITY.md — 0004/0006/0016 apply cleanly; 0017/0018/0065 need adaptation
- [x] BUILD.md — XNU build environment + macOS blocker documented
- [ ] Apply skeletons + adapted patches to XNU 8796 source (BLOCKED — macOS host required)
- [ ] pmap.c / early physical memory from boot_args memSize
- [ ] GIC driver init (distributor + redistributor)
- [ ] Timer driver (CNTV_CTL, PPI 13)
- [ ] Console driver (polled UART0)

## Phase 4 — Build and Boot
- [x] Top-level build.sh, validate.sh, package.sh
- [x] Host validation test for FDT parser + ADT builder (20/20 pass)
- [x] BUILD.md (with XNU build env + blocker)
- [ ] Baseline XNU build (BLOCKED — no macOS/Xcode/KDK on Linux host)
- [ ] Obtain XNU 8796 source — DONE (xnu-8796.141.3 @ 1b191cb)
- [ ] Compile XNU + bootshim (macOS host required)
- [ ] Validate images, memory ranges, boot args
- [ ] Test via fastboot boot (temporary)
- [ ] Capture serial output

## Phase 5 — First Boot Verification (BLOCKED — hardware access required)
- [ ] Bootshim execution confirmed (UART marker)
- [ ] XNU image loaded
- [ ] XNU entry reached
- [ ] XNU initialization confirmed (diagnostics)
- [ ] Repeatable test evidence
