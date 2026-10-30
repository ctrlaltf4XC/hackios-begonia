# XNU Build Analysis — BEGONIA-XNU (Agent 3)

**Repository:** /home/yu/BEGONIA-XNU
**XNU source:** /home/yu/BEGONIA-XNU/xnu/xnu-8796 @ `1b191cb58250d0705d8a51287127505aa4bc0789` (tag `xnu-8796.141.3`)
**Build engine:** /home/yu/BEGONIA-XNU/tools/darwin-xnu-build @ `663906528d1cd5ea8fde7de6f16de73db9511549` (build.sh, 881 lines)
**Host:** Linux — **no macOS / Xcode runner available**

---

## 1. DXB Build Engine Architecture (`tools/darwin-xnu-build/build.sh`)

`build.sh` (881 lines) is a single bash entry point (`main "$@"`) that orchestrates the full XNU build dependency chain on macOS. Functions and purposes:

| Function | Lines | Purpose |
|---|---|---|
| `setup_xcode_toolchain` | 40-95 | Sets DEVELOPER_DIR (xcode-select), rebuilds PATH to prioritize Xcode toolchain over Homebrew, exports CC/CXX/LD/AR/RANLIB/STRIP/LIBTOOL via `xcrun -find`. Fails if Xcode CLT missing. |
| `choose_xnu` | 206-402 | Interactive macOS version picker (gum). Sets MACOS_VERSION, KDKROOT, KDK_NAME, RC_DARWIN_KERNEL_VERSION per release (12.5 → 26.5). Downloads/installs KDK via hdiutil/installer if missing. |
| `get_xnu` | 413-423 | Clones apple-oss-distributions/xnu at tag from release.json; restores cached compile_commands.json. |
| `patches` | 426-482 | Applies sed rewrites to xnu/makedefs (LDFLAGS_KERNEL_SDK, INCFLAGS_SDK, MIG/MIGCOM paths → FAKEROOT_DIR) and applies release-specific .sh/.patch files from `patches/`. Skipped when CODEQL=1. |
| `build_bootstrap_cmds` | 484-509 | Builds `mig`/`migcom` via xcodebuild (mig.xcodeproj, ARCHS=arm64 x86_64) into DSTROOT/fakeroot. |
| `build_dtrace` | 511-526 | Builds ctfconvert/ctfdump/ctfmerge via xcodebuild into DSTROOT/fakeroot. |
| `build_availabilityversions` | 528-542 | `make install` of AvailabilityVersions → `availability.pl` into DSTROOT/fakeroot. |
| `xnu_headers` | 544-556 | `make installhdrs` for xnu (SDKROOT=macosx, ARCH_CONFIGS=X86_64 ARM64) → headers into DSTROOT/fakeroot. Touch sentinel `.xnu_headers_installed`. |
| `libsystem_headers` | 558-574 | xcodebuild installhdrs for Libsystem → `System.framework` into DSTROOT/fakeroot. |
| `libsyscall_headers` | 576-587 | xcodebuild installhdrs for xnu/libsyscall → `usr/include/os/proc.h` into DSTROOT/fakeroot. |
| `build_libplatform` | 589-602 | `ditto` of libplatform include/private → `DSTROOT/usr/local/include`. |
| `build_libdispatch` | 605-624 | xcodebuild `libfirehose_kernel` target → `liblibfirehose_kernel.a`, renamed to `libfirehose_kernel.a` in `DSTROOT/usr/local/lib/kernel/`. |
| `ensure_hw_env_overrides` | 626-665 | Probes host memory/CPU (sysctl / python os.sysconf / os.cpu_count) to set MEMORY_SIZE/PHYS_CPU/LOGICAL_CPU/KERNEL_PARALLELISM overrides (defaults on Linux: 1GB / 1 cpu / parallelism 1). |
| `ctf_tool_overrides` | 671-684 | Validates ctfconvert/ctfmerge/ctfdump exist in fakeroot/usr/local/bin; builds CTF_TOOL_OVERRIDES array. Errors out if missing. |
| `kernel_version_env` | 689-695 | Stamps KERNEL_BUILD_DATE (UTC commit date) and KERNEL_BUILD_OBJROOT (xnu tag) for reproducible `uname -v`. |
| `build_xnu` | 697-738 | **Core build.** If `build/xnu.obj/kernel.${KERNEL_TYPE}` missing, runs `make install -j8 ...` (see §2). Skips if already built. JSONDB variant writes compile_commands.json. |
| `build_xnu_library` | 740-788 | Builds `libkernel.${KERNEL_TYPE}.a` (RC_ProjectName=xnu_libraries) when --lib passed. |
| `build_kc` | 798-828 | `kmutil create` kernel collection (requires ipsw + kmutil, macOS only). Skipped when BUILDLIB=1. |
| `main` | 830-881 | Arg parse (-h/-c/-k/--lib), then fixed sequence: install_deps → setup_xcode_toolchain → choose_xnu → get_xnu → patches → venv → build_bootstrap_cmds → build_dtrace → build_availabilityversions → xnu_headers → libsystem_headers → libsyscall_headers → build_libplatform → build_libdispatch → build_xnu (or build_xnu_library) → [build_kc]. |

Default config (lines 98-111):
- `KERNEL_CONFIG=RELEASE`, `ARCH_CONFIG=ARM64`, `MACHINE_CONFIG=VMAPPLE`
- `BUILDKC=0`, `BUILDLIB=0`, `CODEQL=0`, `JSONDB=0`
- `KERNEL_TYPE=release.vmapple` (lines 121-122)

Path defaults (lines 112-118):
- `WORK_DIR="$PWD"`, `BUILD_DIR="${WORK_DIR}/build"`, `FAKEROOT_DIR="${WORK_DIR}/fakeroot"`, `DSTROOT="${FAKEROOT_DIR"}`

---

## 2. Exact Make Command for Baseline Build

Source: `build.sh` lines 720-732 (the `else` branch of `build_xnu`, the non-JSONDB baseline path). `cd "${WORK_DIR}/xnu"` then:

```bash
make install -j8 VERBOSE=YES "${CTF_TOOL_OVERRIDES[@]}" \
  SDKROOT=macosx \
  TARGET_CONFIGS="$KERNEL_CONFIG $ARCH_CONFIG $MACHINE_CONFIG" \
  CONCISE=0 LOGCOLORS=y \
  BUILD_WERROR=0 BUILD_LTO=0 \
  SRCROOT="${SRCROOT}" \
  OBJROOT="${OBJROOT}" SYMROOT="${SYMROOT}" DSTROOT="${DSTROOT}" \
  FAKEROOT_DIR="${FAKEROOT_DIR}" \
  KDKROOT="${KDKROOT}" TIGHTBEAMC=${TIGHTBEAMC} \
  RC_DARWIN_KERNEL_VERSION=${RC_DARWIN_KERNEL_VERSION} \
  MEMORY_SIZE="${MEMORY_SIZE_OVERRIDE}" \
  SYSCTL_HW_PHYSICALCPU="${PHYS_CPU_OVERRIDE}" \
  SYSCTL_HW_LOGICALCPU="${LOGICAL_CPU_OVERRIDE}" \
  KERNEL_BUILDS_IN_PARALLEL="${KERNEL_PARALLELISM_OVERRIDE:-1}"
```

With baseline defaults substituted:
- `SRCROOT=${WORK_DIR}/xnu` (line 726)
- `OBJROOT=${BUILD_DIR}/xnu.obj` → `build/xnu.obj` (line 727)
- `SYMROOT=${BUILD_DIR}/xnu.sym` → `build/xnu.sym` (line 728)
- `DSTROOT=${FAKEROOT_DIR}` → `fakeroot` (line 116)
- `FAKEROOT_DIR=${WORK_DIR}/fakeroot` (line 115)
- `TARGET_CONFIGS="RELEASE ARM64 VMAPPLE"`
- `SDKROOT=macosx` (resolved by MakeInc.cmd line 117-129 via `xcrun -sdk macosx -show-sdk-path`, PLATFORM=MacOSX)
- `KDKROOT` = per MACOS_VERSION (e.g. `/Library/Developer/KDKs/KDK_26.0_25A353.kdk/`)
- `RC_DARWIN_KERNEL_VERSION` = per MACOS_VERSION (e.g. `25.0.0`)
- `TIGHTBEAMC=tightbeamc-not-supported` (line 212)
- `BUILD_WERROR=0`, `BUILD_LTO=0` (overrides MakeInc.def defaults of 1)
- `CTF_TOOL_OVERRIDES` = `CTFCONVERT=${FAKEROOT_DIR}/usr/local/bin/ctfconvert CTFMERGE=...ctfmerge CTFDUMP=...ctfdump` (line 679-683)

Preceding setup calls within `build_xnu` (lines 730-731): `ctf_tool_overrides` then `kernel_version_env`.

Guard: line 698 skips the build if `${BUILD_DIR}/xnu.obj/kernel.${KERNEL_TYPE}` already exists.

---

## 3. Build Dependency Chain (what must be built before xnu)

Execution order from `main()` (lines 860-872):

1. `build_bootstrap_cmds` → produces `mig`, `migcom` in `${FAKEROOT_DIR}` (consumed by MakeInc.cmd MIG/MIGCOM overrides and xnu MIG codegen).
2. `build_dtrace` → produces `ctfconvert`, `ctfdump`, `ctfmerge` in `${FAKEROOT_DIR}/usr/local/bin/` (consumed via CTF_TOOL_OVERRIDES; without them `DO_CTFMERGE=0`, no __CTF section).
3. `build_availabilityversions` → produces `availability.pl` in `${FAKEROOT_DIR}`.
4. `xnu_headers` → installs xnu kernel headers into `${DSTROOT}` (sentinel `.xnu_headers_installed`).
5. `libsystem_headers` → installs into `${FAKEROOT_DIR}/System/Library/Frameworks/System.framework`.
6. `libsyscall_headers` → installs `${FAKEROOT_DIR}/usr/include/os/proc.h`.
7. `build_libplatform` → `ditto` into `${DSTROOT}/usr/local/include`.
8. `build_libdispatch` → builds/installs `${FAKEROOT_DIR}/usr/local/lib/kernel/libfirehose_kernel.a`.
9. `build_xnu` → links against `-lfirehose_kernel` (LD_KERNEL_ARCHIVES, MakeInc.def line 860) and produces `build/xnu.obj/kernel.release.vmapple`.

`patches()` runs before all of the above and rewrites MakeInc.def/MakeInc.cmd to point SDK-relative paths at `${FAKEROOT_DIR}`.

---

## 4. Generated Headers and Install Paths

| Step | Generated artifact | Install path (relative to DSTROOT/fakeroot) |
|---|---|---|
| `xnu_headers` | xnu kernel headers (`make installhdrs`) | `${DSTROOT}/` (frameworks + usr/include per MakeInc.def § "Header file destinations", e.g. `System/Library/Frameworks/Kernel.framework/Versions/A/Headers/...`) |
| `libsystem_headers` | Libsystem public headers | `${DSTROOT}/System/Library/Frameworks/System.framework/Versions/B/Headers/...` |
| `libsyscall_headers` | libsyscall headers | `${DSTROOT}/usr/include/os/proc.h` (sentinel checked at line 577) |
| `build_libplatform` | libplatform headers | `${DSTROOT}/usr/local/include/` (via `ditto include` + `ditto private`) |
| `build_libdispatch` | `libfirehose_kernel.a` | `${DSTROOT}/usr/local/lib/kernel/libfirehose_kernel.a` (renamed from `liblibfirehose_kernel.a`, line 622) |
| `build_dtrace` | ctfconvert/ctfdump/ctfmerge | `${DSTROOT}/usr/local/bin/` (sentinel checked at line 674) |
| `build_bootstrap_cmds` | mig / migcom | `${DSTROOT}/` (searched via `find ${FAKEROOT_DIR} -name mig`, line 485) |
| `build_availabilityversions` | availability.pl | `${DSTROOT}/` (sentinel checked at line 529) |

Header destination macros from MakeInc.def (lines 1000-1029):
- `KINCDIR` = `Kernel.framework/Versions/A/Headers`, `KPINCDIR` = `.../PrivateHeaders`
- `LCLDIR` = `usr/local/include`, `INCDIR` = `usr/include`

`INCFLAGS_SDK` (patched by build.sh line 437) → `-I$(FAKEROOT_DIR)/usr/local/include/kernel` (original MakeInc.def line 887: `-I$(SDKROOT)/usr/local/include/kernel`).

---

## 5. Kernel Configuration for ARM64 + VMAPPLE Baseline

From `makedefs/MakeInc.def`:
- `SUPPORTED_ARM64_MACHINE_CONFIGS = BCM2837 T6000 T8101 T8103 VMAPPLE` (line 32) — VMAPPLE is valid.
- `MACHINE_FLAGS_ARM64_VMAPPLE = -DARM64_BOARD_CONFIG_VMAPPLE -march=armv8.5a` (line 62).
- `CFLAGS_ARM64` (line 338): `-Darm64 -DARM64 -D__ARM64__ -DLP64 -DPAGE_SIZE_FIXED -DVM_KERNEL_LINK_ADDRESS=$(ARM64_LINK_ADDR) $(LARGE_MEMORY_DEFINE) -momit-leaf-frame-pointer -fno-strict-aliasing -D__API__=v4 -mkernel`.
- `ARM64_PLKSEG_ADDR=0xfffffff004004000`, `ARM64_LINK_ADDR=0xfffffff007004000` (lines 327-328) for non-MacOSX platform; MacOSX overrides to `0xfffffe00...` (lines 331-335) — but SDKROOT=macosx ⇒ PLATFORM=MacOSX ⇒ `LARGE_MEMORY_DEFINE=-DARM_LARGE_MEMORY=1` and link addresses `0xfffffe0004004000`/`0xfffffe0007004000`.
- `ARCH_FLAGS_ARM64` = `-arch arm64e` unless `ARCH_STRING_FOR_CURRENT_MACHINE_CONFIG` set (lines 242-263); EMBEDDED_DEVICE_MAP empty by default ⇒ falls to `arm64e`. Note: VMAPPLE build would use arch string `arm64e` unless overridden.
- `DEFINES` (line 104): `-DAPPLE -DKERNEL -DKERNEL_PRIVATE -DXNU_KERNEL_PRIVATE -DPRIVATE -D__MACHO__=1 -Dvolatile=__volatile -DXNU_KERN_EVENT_DATA_IS_VLA ...`
- `DEPLOYMENT_TARGET_FLAGS` = `-mmacosx-version-min=$(SDKVERSION) -DXNU_TARGET_OS_OSX` (line 70).
- `LTO_ENABLED_RELEASE=1` but `BUILD_LTO=0` passed ⇒ LTO disabled (line 933-937).
- `USE_WERROR` default 1 but `BUILD_WERROR=0` passed ⇒ `-Werror` disabled (lines 126-133).

From `config/MASTER.arm64` (99 lines, reference master config):
- `machine "arm64"`, `OSFMK_MACHINE="arm64"`.
- Options: `COUNT_SYSCALLS`, `TRASH_VFP_ON_SAVE`, `ALTERNATE_DEBUGGER`, `CONFIG_VNODES=1024`, `CONFIG_FREEZE_SUSPENDED_MIN=4`, `CONFIG_MACH_APPROXIMATE_TIME`, `CONFIG_KERNEL_INTEGRITY`, `CONFIG_RESLIDE_SHARED_CACHE`, `CONFIG_KERNEL_TBI`.
- Comments document the config recipe hierarchy (KERNEL_BASE/RELEASE, BSD_RELEASE, VM_RELEASE, etc.) but this file is a *reference* — the actual kernel config is selected via `TARGET_CONFIGS` and the config files in `config/` (not fully inspected here).

From `pexpert/pexpert/arm64/Makefile`:
- Exports MakeInc cmd/def/rule/dir; installs `VMAPPLE.h` + `board_config.h` etc. into Kernel.framework PrivateHeaders (`INSTALL_KF_MD_LCL_LIST`).
- `EXPORT_MD_LIST` = private kernel files + `platform.h`; `EXPORT_MD_DIR=pexpert/arm64`.

From `iokit/conf/files.arm64` (7 lines):
- IONVRAMController.cpp (optional iokitcpp), IOPMrootDomain.cpp (optional iokitcpp), AppleARMSMP.cpp (optional iokitcpp).

---

## 6. Make Parameter Reference Table

| Parameter | Baseline value | Source / notes |
|---|---|---|
| `SDKROOT` | `macosx` | build.sh line 732; resolved to full path by MakeInc.cmd:124 (`xcrun -sdk macosx -show-sdk-path`) |
| `TARGET_CONFIGS` | `RELEASE ARM64 VMAPPLE` | `"$KERNEL_CONFIG $ARCH_CONFIG $MACHINE_CONFIG"`; defaults lines 98-100 |
| `OBJROOT` | `build/xnu.obj` | `${BUILD_DIR}/xnu.obj`, line 727 |
| `SYMROOT` | `build/xnu.sym` | `${BUILD_DIR}/xnu.sym`, line 728 |
| `DSTROOT` | `fakeroot` | `${FAKEROOT_DIR}`, line 116 = line 115 |
| `FAKEROOT_DIR` | `${WORK_DIR}/fakeroot` | line 115; install target root |
| `KDKROOT` | per MACOS_VERSION | e.g. `/Library/Developer/KDKs/KDK_26.0_25A353.kdk/`; choose_xnu lines 214-377; required (line 721-723) |
| `RC_DARWIN_KERNEL_VERSION` | per MACOS_VERSION | e.g. `25.0.0`; choose_xnu |
| `BUILD_WERROR` | `0` | line 732 (overrides MakeInc.def default 1) |
| `BUILD_LTO` | `0` | line 732 (overrides MakeInc.def `LTO_ENABLED_RELEASE=1`) |
| `CONCISE` | `0` | line 732 |
| `LOGCOLORS` | `y` | line 732 |
| `VERBOSE` | `YES` | line 732 |
| `SRCROOT` | `${WORK_DIR}/xnu` | line 726 |
| `TIGHTBEAMC` | `tightbeamc-not-supported` | line 212 |
| `MEMORY_SIZE` | `MEMORY_SIZE_OVERRIDE` | ensure_hw_env_overrides (Linux fallback 1073741824) |
| `SYSCTL_HW_PHYSICALCPU` | `PHYS_CPU_OVERRIDE` | ensure_hw_env_overrides (Linux fallback 1) |
| `SYSCTL_HW_LOGICALCPU` | `LOGICAL_CPU_OVERRIDE` | ensure_hw_env_overrides |
| `KERNEL_BUILDS_IN_PARALLEL` | `KERNEL_PARALLELISM_OVERRIDE:-1` | ensure_hw_env_overrides sets to 1 |
| `CTFCONVERT/CTFMERGE/CTFDUMP` | `${FAKEROOT_DIR}/usr/local/bin/<tool>` | ctf_tool_overrides, lines 679-683 |
| `make target` | `install` | line 732 |
| `make -j` | `8` | line 732 |
| Kernel output | `build/xnu.obj/kernel.release.vmapple` | `KERNEL_TYPE=release.vmapple`; guard line 698 |

---

## 7. Kernel Output Path and Format

- **Output path:** `build/xnu.obj/kernel.release.vmapple` (checked at line 698, produced by the `install` target).
- **Format:** Mach-O, arm64.
  - Evidence: `ARCH_FLAGS_ARM64=-arch arm64e`; `CFLAGS_ARM64` includes `-D__ARM64__ -DLP64 -mkernel`; `DEFINES` includes `-D__MACHO__=1`; `LDFLAGS_KERNEL_GENARM64` uses `-Wl,-pie -Wl,-static`, `__TEXT_EXEC`/`__HIBDATA` segment renames, `-Wl,-kernel`-style flags (`LDFLAGS_KERNEL_ONLY_SUBARCH_arm64e` line 831-833); `KERNEL_FILE_NAME=kernel.release.vmapple` (MakeInc.def line 1139, machine config != NONE).
- Note: `ARCH_STRING_FOR_CURRENT_MACHINE_CONFIG` defaults to `arm64e` (MakeInc.def line 262) unless `XNU_LIB_ARCH_STRING` is set (only relevant to `build_xnu_library`). A plain `arm64` kernel requires overriding this.

---

## 8. Build Log Structure

`build.sh` does not write to a log file; all output is stdout/stderr with color-coded prefixes:
- `running` → `⇒` (magenta)
- `info` → `[info]` (blue)
- `error` → `[error]` (red)
- `warning` → `[warning]` (yellow)

Key log milestones in order:
1. `setup_xcode_toolchain` → "Using Xcode at: ...", "Using Xcode clang: ..."
2. `choose_xnu` → "Building XNU for macOS ${MACOS_VERSION}"
3. `get_xnu` → "⬇️ Cloning xnu" / cached compile_commands.json restore
4. `patches` → "🩹 Patching xnu files" + per-patch "Applying patch"/"Running script"
5. `build_bootstrap_cmds` → "📦 Building bootstrap_cmds"
6. `build_dtrace` → "📦 Building dtrace"
7. `build_availabilityversions` → "📦 Building AvailabilityVersions"
8. `xnu_headers` → "Installing xnu headers"
9. `libsystem_headers` → "Installing Libsystem headers"
10. `libsyscall_headers` → "Installing libsyscall headers"
11. `build_libplatform` → "📦 Building libplatform"
12. `build_libdispatch` → "📦 Building libdispatch"
13. `build_xnu` → "📦 Building XNU kernel TARGET_CONFIGS="RELEASE ARM64 VMAPPLE"" then verbose make output, "Kernel version banner: ..."
14. "🎉 XNU Build Done!" (line 873)

To preserve, redirect stdout/stderr to a log: `./build.sh 2>&1 | tee build.log` (requires macOS).

---

## 9. Potential Build Failure Points (code inspection only)

1. **No macOS / Xcode runner (BLOCKER).** `setup_xcode_toolchain` requires `xcode-select -p` and a valid `DEVELOPER_DIR`; all compiler tools resolved via `xcrun -find`. Not available on Linux.
2. **`choose_xnu` is interactive.** `MACOS_VERSION` defaults to `""` (line 101), triggering `gum choose` (line 209). Non-interactive/CI runs must export `MACOS_VERSION` or `gum` is absent → fail.
3. **KDK installation is macOS-only.** KDK download via `curl` → `hdiutil attach` → `installer -pkg ... -target /` → `hdiutil detach` (lines 389-399). `KDKROOT` must exist or `build_xnu` errors (line 721-723).
4. **`xcodebuild` required.** `build_bootstrap_cmds`, `build_dtrace`, `libsystem_headers`, `libsyscall_headers`, `build_libdispatch` all invoke `xcodebuild` (macOS only).
5. **`sed -i ''` (BSD syntax).** Five `sed -i ''` calls in `patches()` (lines 429, 431, 433, 437, 439, 440). Written for BSD sed; behavior on GNU sed differs by version (empty suffix handling). Portability risk.
6. **`ctf_tool_overrides` hard-fails.** Line 674-677: if `ctfconvert/ctfmerge/ctfdump` are missing from fakeroot (because `build_dtrace` didn't run on macOS), the script `exit 1`.
7. **Host environment probes are macOS-biased.** `ensure_hw_env_overrides` uses `/usr/sbin/sysctl -n hw.memsize|hw.physicalcpu|hw.logicalcpu` — absent on Linux; Python fallbacks (`os.sysconf`, `os.cpu_count`) exist but return container/host values, not Apple hardware.
8. **`make installhdrs` / `make install` require the full XNU make system** (`makedefs/MakeInc.*`, `config/`, `Makefile` top-level), which itself invokes `mig`, `unifdef`, `dsymutil` via `xcrun`.
9. **`build_kc` requires `kmutil` + `ipsw`.** Lines 806-824 use `sw_vers`, `kmutil create`, `ipsw kernel kmutil inspect` — macOS-only; also `BUILDKC=0` by default so skipped in baseline.
10. **`BUILD_WERROR=0` and `BUILD_LTO=0` are explicitly passed**, masking potential `-Werror` / LTO failures that would surface with Apple's default settings.
11. **`LDFLAGS_KERNEL_SDK` patch (line 433)** rewrites to `-L$(FAKEROOT_DIR)/usr/local/lib/kernel -lfirehose_kernel`, replacing the file's original `-L$(SDKROOT)/usr/local/lib/kernel -L$(SDKROOT)/usr/local/lib/kernel/platform` (MakeInc.def line 645). If `libfirehose_kernel.a` is missing from the fakeroot, the link fails.
12. **`INCFLAGS_SDK` patch (line 437)** rewrites to `-I$(FAKEROOT_DIR)/usr/local/include/kernel`; if libdispatch headers aren't installed there, `os/firehose_buffer_private.h` is unresolved.

---

## 10. Baseline Build Status: BLOCKED

**Status: BLOCKED — no macOS runner.**

This environment is Linux. `build.sh` unconditionally requires:
- Xcode Command Line Tools (`xcode-select`, `xcrun`, `clang`, `ld`, ...)
- `xcodebuild` (bootstrap_cmds, dtrace, libsystem, libsyscall, libdispatch)
- `hdiutil` / `installer` (KDK installation)
- `gum` (interactive version picker, unless `MACOS_VERSION` exported)
- `kmutil` / `ipsw` / `sw_vers` (kernel collection, only if `--kc`)

No compilation has been attempted and none can be. Nothing in this document claims a successful build. All parameter values above are derived from static code inspection of `build.sh`, `MakeInc.def`, `MASTER.arm64`, `pexpert/pexpert/arm64/Makefile`, and `iokit/conf/files.arm64`.

To unblock: run `build.sh` on macOS with Xcode installed, export `MACOS_VERSION` (e.g. `26.0`) and `KDKROOT` (or let it download), then `./build.sh`. Expected baseline artifact: `build/xnu.obj/kernel.release.vmapple` (Mach-O arm64).

---

## Sign-off

| Item | Status |
|---|---|
| build.sh inspected (all 881 lines) | Complete |
| Exact make command extracted (lines 720-732) | Complete |
| Build dependency chain mapped | Complete |
| Generated headers & install paths documented | Complete |
| Kernel output path + format verified | Complete |
| Make parameter reference table | Complete |
| Potential failure points diagnosed | Complete |
| Actual build executed | Not possible — Linux host, BLOCKED |
| Build succeeded | Not claimed; cannot verify on this platform |

**Agent 3 — XNU Build Engineer** — Analysis complete. Deliverable: `/home/yu/BEGONIA-XNU/XNU_BUILD_ANALYSIS.md`.
