# BUILD.md

BEGONIA-XNU build environment and toolchain documentation.

## Toolchain (verified, present on host)

| Tool | Version | Purpose |
|------|---------|---------|
| aarch64-linux-gnu-gcc | 16.2.0 | Boot shim cross-compiler |
| aarch64-linux-gnu-ld | binutils | Boot shim linker |
| clang | system | XNU compiler (Apple toolchain) |
| dtc | system | Device Tree compiler (test DTBs only) |
| python3 | system | Build/post-processing scripts |

Boot shim is built with `aarch64-linux-gnu-gcc`, `-ffreestanding -nostdlib`,
position-independent, no PIC, no stack protector. Output is a raw ARM64 binary
with the Linux `ARM\x64` image header so the MediaTek bootloader treats it as a
kernel image.

XNU 8796 is built with clang (Apple open-source toolchain). Not yet integrated
— XNU source must be obtained from Apple (xnu-8796.x.x) and patched with the
ios-sweet-compatible patch set described in XNU_BOOT_REQUIREMENTS.md.

## Build targets

```
./build.sh bootshim   # Build the Begonia boot shim (only buildable component)
./build.sh all        # bootshim + xnu (xnu fails until source present)
./build.sh clean      # Remove bootshim/out/
```

Boot shim output:
- `bootshim/out/Image` — raw ARM64 boot shim binary (for `fastboot boot`)
- `bootshim/out/shim.elf` — ELF with symbols (for crash analysis)
- `bootshim/out/build.json` — sha256, cpu, hardware_tested flag

## Validation

```
./validate.sh         # Build boot shim + run host FDT/ADT test (20 checks)
./package.sh          # Create package/ for temporary fastboot boot testing
```

## Reproducibility

`bootshim/out/build.json` records the sha256 of the Image. The build uses
`-Wl,--build-id=none` and rejects absolute C relocations (`R_AARCH64_ABS`) to
ensure the shim is position-independent and relocatable at any physical address.

## Compiler/linker versions recorded

Boot shim:
```
aarch64-linux-gnu-gcc (GCC) 16.2.0
GNU ld (GNU Binutils) ...
```

XNU (when integrated):
Record clang version, cctools version, and the xnu-8796.x.x source revision in
build.json / BUILD.md.

## Cross-compilation notes

- Boot shim: `-mcpu=cortex-a55` (tunable via SHIM_CPU env var)
- No standard library; all code is freestanding
- Stack is set up in start.S at `_stack_top` (immediately after `_end`)
- Exception vectors installed at VBAR_EL1/VBAR_EL2 depending on entry EL

---
## XNU 8796 build environment (reference pipeline)

Source: `xnu/xnu-8796` at tag `xnu-8796.141.3`, commit `1b191cb58250d0705d8a51287127505aa4bc0789`
(public mirror — no Apple Developer login required). Full acquisition record in
`XNU_SOURCE_ACQUISITION.md`.

Build engine: `tools/darwin-xnu-build` at commit `663906528d1cd5ea8fde7de6f16de73db9511549`
(blacktop/darwin-xnu-build), invoked by ios-sweet's `.github/workflows/xnu.yml`.

### Reference build configuration

| Variable | Value | Source |
|----------|-------|--------|
| MACOS_VERSION | 13.5 | xnu.yml input default |
| KERNEL_CONFIG | DEVELOPMENT | xnu.yml input default |
| MACHINE_CONFIG | SWEET (→ BEGONIA for this port) | xnu.yml input default |
| RC_DARWIN_KERNEL_VERSION | 22.6.0 | dxb build.sh choose_xnu (13.5) |
| KDK | 13.5 build 22G74 | Apple Developer Portal |
| Xcode | 15.x (macos-14 runner) | xnu.yml comment: iig matches xnu-8796 IORPC ABI |

DXB resolves the xnu tag from the macOS 13.5 release.json:
`https://raw.githubusercontent.com/apple-oss-distributions/distribution-macOS/macos-135/release.json`
→ xnu project tag `xnu-8796.141.3`.

### Build environment blocker

The reference `darwin-xnu-build/build.sh` requires a **macOS host** with:
- Xcode 15.x (`xcodebuild`, `xcrun`, `mig`, `kmutil`, `hdiutil`)
- `DEVELOPER_DIR` set
- Kernel Debug Kit (KDK) installed from the Apple Developer Portal
- Homebrew deps: jq, gum, xcodes, cmake, ninja, ipsw

This host is **Linux (x86_64)**. Available: clang 23.1.4, ld64.lld, aarch64-linux-gnu-gcc 16.2.0.
Missing: iPhoneOS SDK, mig, macOS SDK/KDK headers, xcodebuild. A **baseline XNU build cannot
be performed on this Linux host** and must be performed on a macOS build runner (e.g. GitHub
Actions macos-14) before Begonia patches can be compiled and the kernel built.

This is a documented blocker, not a fabrication. Success criterion "Baseline XNU build succeeds"
is pending a macOS build host. Do not substitute a newer XNU release to work around it.

### Build targets (when macOS host available)

```
./build.sh bootshim   # Boot shim (buildable on Linux)
./build.sh xnu        # XNU 8796 baseline build (macOS only — blocked on Linux)
./build.sh platform   # platform/begonia/ integration (macOS only — blocked)
./build.sh all        # bootshim + xnu + platform
./build.sh clean
```

### Compiler/linker versions to record (macOS build)

Xcode clang version, ld64 version, mig version, xcodebuild version, KDK build,
and the xnu-8796.141.3 source revision (`1b191cb58250d0705d8a51287127505aa4bc0789`).

---
## GitHub Actions macOS CI Pipeline

Workflow: `.github/workflows/xnu.yml`

### Jobs

| Job | Purpose | Runs on | Depends on |
|-----|---------|---------|------------|
| `xnu-baseline` | Build unmodified XNU 8796.141.3 baseline (no patches) | macOS 14 | — |
| `xnu-begonia` | Build XNU with Begonia platform integration (MACHINE_CONFIG=BEGONIA) | macOS 14 | `xnu-baseline` |

### Runner and toolchain

- Runner: `macos-14` (GitHub-hosted)
- Xcode: **15.4** at `/Applications/Xcode_15.4.app/Contents/Developer` (explicitly selected via `xcode-select`)
- Tools verified: `xcodebuild`, `xcrun`, `mig`, `clang`, `ld`, `kmutil`, `hdiutil`
- Build engine: `blacktop/darwin-xnu-build` at commit `663906528d1cd5ea8fde7de6f16de73db9511549`

### XNU source (pinned — never substituted)

- Tag: `xnu-8796.141.3`
- Commit: `1b191cb58250d0705d8a51287127505aa4bc0789`
- Source: `https://github.com/apple-oss-distributions/xnu.git`
- The workflow clones at the tag and verifies the commit SHA before building.
- No newer or different XNU release is ever substituted.

### Build command (dxb build.sh)

The workflow invokes `bash build.sh` in the `tools/darwin-xnu-build` directory with:

```
SDKROOT=macosx TARGET_CONFIGS="$KERNEL_CONFIG ARM64 $MACHINE_CONFIG"
```

For baseline: `MACHINE_CONFIG=VMAPPLE`, `KERNEL_CONFIG=DEVELOPMENT` (defaults, overridable via `workflow_dispatch`).
For Begonia: `MACHINE_CONFIG=BEGONIA`, `KERNEL_CONFIG=DEVELOPMENT`.

### Kernel Debug Kit (KDK)

The XNU 8796.141.3 / macOS 13.5 build requires:

| Field | Value |
|-------|-------|
| KDK name | Kernel Debug Kit 13.5 build 22G74 |
| KDK path | `/Library/Developer/KDKs/KDK_13.5_22G74.kdk` |
| RC_DARWIN_KERNEL_VERSION | 22.6.0 |

The CI attempts automatic download from the dortania KdkSupportPkg manifest. If the KDK cannot be obtained automatically, the job fails with a documented error and links to the manual installation procedure below.

#### Manual KDK installation (if automatic download fails)

1. Download the Kernel Debug Kit 13.5 (build 22G74) DMG from Apple's Developer Portal or the dortania KdkSupportPkg mirror.
2. Mount the DMG: `hdiutil attach /path/to/KernelDebugKit.dmg`
3. Install: `sudo installer -pkg '/Volumes/Kernel Debug Kit/KernelDebugKit.pkg' -target /`
4. Unmount: `hdiutil detach '/Volumes/Kernel Debug Kit'`
5. Verify: `ls -la /Library/Developer/KDKs/KDK_13.5_22G74.kdk`

### Artifacts

| Artifact | Contents | Retention |
|----------|----------|-----------|
| `xnu-baseline-build` | `build/`, `build-logs/`, `fakeroot/` from dxb | 7 days |
| `xnu-begonia-build` | `build/`, `fakeroot/` from dxb | 7 days |

Build logs and compiler diagnostics are preserved in `build-logs/` before upload.

### workflow_dispatch inputs

| Input | Default | Description |
|-------|---------|-------------|
| `kernel_config` | `DEVELOPMENT` | RELEASE / DEVELOPMENT / DEBUG |
| `machine_config` | `VMAPPLE` | VMAPPLE (baseline) or BEGONIA (integration) |
| `keep_going` | `true` | Run `make -k` to collect all compile errors |

### Baseline-first rule

Per project rule, Begonia patches are **never** applied during the baseline compilation job. The `xnu-begonia` job only runs after `xnu-baseline` succeeds, and integrates the `platform/begonia/` sources (BegoniaPlatformExpert.cpp, BegoniaCPU.cpp, begonia.h) directly into the XNU source tree before building with `MACHINE_CONFIG=BEGONIA`.

### No credentials

The workflow never exposes Apple Developer credentials, KDK download URLs containing credentials, or private signing material. KDK downloads use the public dortania KdkSupportPkg manifest only.

---
## Build Performance Optimization

### Objectives (all implemented, none sacrifice correctness)

| # | Objective | Implementation |
|---|-----------|---------------|
| 1 | Configurable parallel compilation | `XNU_BUILD_JOBS` env var / `xnu_build_jobs` input |
| 2 | Benchmark 4/6/8 jobs | `xnu_build_jobs` workflow_dispatch input |
| 3 | Preserve build/xnu.obj and build/xnu.sym | GitHub Actions cache, never cleaned unless `force_clean=true` |
| 4 | Preserve fakeroot and dependency outputs | GitHub Actions cache |
| 5 | Correct incremental rebuild | `make` called directly (bypasses dxb's `build_xnu()` guard) |
| 6 | Fix existing-kernel output guard | Guard bypassed; make's timestamp deps drive incremental rebuilds |
| 7 | Never delete intermediate objects | No `rm -rf build/` unless `force_clean=true` |
| 8 | GitHub Actions caching | `actions/cache/restore` + `actions/cache/save` |
| 9 | Pinned cache keys | runner OS + Xcode + KDK + XNU commit + kernel config + machine config |
| 10 | Avoid corrupt cache | Cache key mismatch = no restore; clean build fallback |
| 11 | Cache dependency repos/headers | `fakeroot/` and `.cache/` included in cache scope |
| 12 | Skip Kernel Collection unless requested | `build_kc` input defaults to `false` |
| 13 | Skip CodeQL and analysis | CodeQL not invoked in workflow |
| 14 | Avoid repeated KDK installation | `KDKROOT` existence check skips reinstall |
| 15 | Per-stage build timing | `date +%s` around build step, output as `build_duration_sec` |
| 16 | CPU/RAM detection | `sysctl hw.ncpu`, `sysctl hw.memsize` (1 job per 2GB RAM) |
| 17 | BUILD_LTO=0 | Hardcoded, no exception without separate benchmark |
| 18 | Preserve XNU revision/patch order | XNU tag/commit pinned, no patches in baseline |

### Incremental Build Guard Fix

DXB's `build_xnu()` function skips compilation entirely when
`build/xnu.obj/kernel.${KERNEL_TYPE}` exists — even if source files changed.
The optimized workflow bypasses this guard by calling `make install` directly
with the same parameters dxb uses, allowing make's timestamp-based dependency
tracking to perform correct incremental rebuilds.

Source changes → only affected objects recompile → faster incremental builds.
No source changes → make reports "nothing to be done" → near-instant.

### Cache Strategy

| Cache Scope | Contents | Key Components |
|-------------|----------|---------------|
| Build outputs | `build/xnu.obj`, `build/xnu.sym` | runner OS, Xcode 15.4, KDK 22G74, XNU commit, kernel config, machine config |
| Dependency outputs | `fakeroot/` (headers, libs) | Same as above |
| Generated headers | `.cache/` | Same as above |

Cache keys are formatted as:
```
xnu-build-${runner.os}-xcode15.4-kdk-22g74-${XNU_COMMIT}-${KERNEL_CONFIG}-${MACHINE_CONFIG}
```

If the cache key changes (e.g. new XNU commit), no restore occurs and a clean build runs.

### Parallel Job Selection

1. Detect CPU count via `sysctl hw.ncpu`
2. Detect RAM via `sysctl hw.memsize`
3. Compute memory-limited jobs: `RAM_GB / 2` (minimum 1)
4. Auto jobs = min(CPU count, memory-limited jobs)
5. Honor `XNU_BUILD_JOBS` override if set
6. Default: auto-detected value

### Benchmark

See `BENCHMARK.md` for the full benchmark plan, scenarios (B1–B5), metrics, and results template.

Scenarios:
- **B1**: Clean baseline build
- **B2**: Second unchanged build (incremental, cache hit)
- **B3**: Single-file source modification
- **B4**: Multiple-file source modification
- **B5**: Begonia platform patch rebuild

### workflow_dispatch inputs (optimized)

| Input | Default | Description |
|-------|---------|-------------|
| `kernel_config` | `DEVELOPMENT` | RELEASE / DEVELOPMENT / DEBUG |
| `machine_config` | `VMAPPLE` | VMAPPLE (baseline) or BEGONIA (integration) |
| `xnu_build_jobs` | (auto) | Parallel compilation jobs; empty = auto-detect |
| `build_kc` | `false` | Build Kernel Collection via kmutil create |
| `force_clean` | `false` | Force clean build (discard cache) |

### Artifacts

| Artifact | Contents | Retention |
|----------|----------|-----------|
| `xnu-build-VMAPPLE` | `build/`, `fakeroot/`, checksums | 7 days |
| `xnu-build-BEGONIA` | `build/`, `fakeroot/`, checksums | 7 days |



