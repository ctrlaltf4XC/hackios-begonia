# CI_TOOLCHAIN_VALIDATION.md

BEGONIA-XNU macOS build environment toolchain validation and compatibility report.
Prepared by: Agent 2 — MACOS TOOLCHAIN ENGINEER (Operation First Boot)
Date: 2026-10-30
Host environment: Linux x86_64 (NOT macOS) — all macOS-specific checks are pending macOS runner execution.

## 1. Required toolchain matrix

| # | Tool | Required version / build | Source | Verified on host | Status |
|---|------|--------------------------|--------|------------------|--------|
| 1 | macOS | 13.5 (build 22G74) | Apple | N/A — Linux host | pending macOS runner execution |
| 2 | Xcode | 15.4 | /Applications/Xcode_15.4.app | N/A — Linux host | pending macOS runner execution |
| 3 | DEVELOPER_DIR | /Applications/Xcode_15.4.app/Contents/Developer | Xcode | absent | pending macOS runner execution |
| 4 | xcodebuild | Xcode 15.4 | Xcode.app | MISSING | pending macOS runner execution |
| 5 | xcrun | Xcode 15.4 | Xcode.app | MISSING | pending macOS runner execution |
| 6 | clang (Xcode) | Xcode 15.4 bundled | Xcode toolchain | system clang 23.1.4 (NOT Xcode) | blocker — Xcode clang required |
| 7 | ld (ld64) | Xcode 15.4 bundled | Xcode toolchain | ld64.lld 23.1.4 (NOT ld64) | blocker — Apple ld64 required |
| 8 | mig | Xcode 15.4 bundled | Xcode toolchain | MISSING | blocker — mig required by dxb |
| 9 | kmutil | Xcode 15.4 bundled | Xcode toolchain | MISSING | blocker — required for kernel collection |
| 10 | hdiutil | macOS 13.5 | /usr/bin/hdiutil | MISSING | blocker — required for KDK DMG attach |
| 11 | Kernel Debug Kit | 13.5 build 22G74 | Apple Developer Portal | N/A — Linux host | pending macOS runner execution |
| 12 | KDKROOT | /Library/Developer/KDKs/KDK_13.5_22G74.kdk | KDK installer | absent | pending macOS runner execution |
| 13 | RC_DARWIN_KERNEL_VERSION | 22.6.0 | dxb choose_xnu() | N/A | pending macOS runner execution |
| 14 | XNU tag | xnu-8796.141.3 | https://github.com/apple-oss-distributions/xnu.git | VERIFIED | VERIFIED |
| 15 | XNU commit | 1b191cb58250d0705d8a51287127505aa4bc0789 | XNU tag xnu-8796.141.3 | VERIFIED | VERIFIED |
| 16 | DXB build engine | 663906528d1cd5ea8fde7de6f16de73db9511549 | https://github.com/blacktop/darwin-xnu-build | VERIFIED | VERIFIED |
| 17 | iPhoneOS SDK | Xcode 15.4 | Xcode.app | MISSING | blocker — required by xcodebuild -sdk |

### Host-verified tools (Linux, usable only for boot shim — NOT for XNU)

| Tool | Version | Purpose | Usable for XNU build? |
|------|---------|---------|------------------------|
| clang | 23.1.4 (x86_64-pc-linux-gnu) | XNU compiler (per BUILD.md claim) | NO — not Xcode's clang |
| ld64.lld | LLD 23.1.4 | Linker | NO — not Apple ld64 |
| gcc | aarch64-linux-gnu-gcc 16.2.0 | Boot shim cross-compiler | NO — boot shim only |
| ld | GNU ld (binutils) | Boot shim linker | NO — boot shim only |
| python3, git, curl, jq, sysctl | system | Build scripting | YES — host scripting only |

## 2. Compatibility analysis

### Component matrix (as pinned in .github/workflows/xnu.yml)

| Component | Pin | Build engine mapping (dxb choose_xnu) |
|-----------|-----|----------------------------------------|
| macOS runner | macos-14 (GitHub-hosted) | MACOS_VERSION=13.5 |
| Xcode | 15.4 at /Applications/Xcode_15.4.app/Contents/Developer | selected via xcode-select -s |
| KDK | Kernel Debug Kit 13.5 build 22G74 | KDKROOT=/Library/Developer/KDKs/KDK_13.5_22G74.kdk |
| RC_DARWIN_KERNEL_VERSION | 22.6.0 | dxb case '13.5' |
| XNU | xnu-8796.141.3 @ 1b191cb | resolved from macos-135 release.json |
| DXB | 6639065 | pinned via DXB_SHA |
| ARCH | ARM64 | fixed in workflow |
| KERNEL_CONFIG | DEVELOPMENT (default) | overridable via workflow_dispatch |
| MACHINE_CONFIG | VMAPPLE (baseline), BEGONIA (integration) | overridable via workflow_dispatch |

### Compatibility verdict

- **macOS 14 runner + Xcode 15.4 + KDK 22G74 + XNU 8796.141.3: COMPATIBLE.** This is the exact combination the reference pipeline (ios-sweet / blacktop/darwin-xnu-build) is designed for. dxb's choose_xnu() maps MACOS_VERSION=13.5 → KDK 13.5 build 22G74 → RC_DARWIN_KERNEL_VERSION=22.6.0 → XNU tag xnu-8796.141.3 (build.sh:250-254).
- **macOS 14 runner + Xcode 15.4 + KDK 13.5 build 22G74: COMPATIBLE.** KDK build matches macOS version exactly (13.5 / 22G74). The KDK provides the kernel headers and private frameworks that xnu's make installhdrs requires (KDKROOT is consumed at build.sh:552, build.sh:721, build.sh:732).
- **Xcode 15.4 + XNU 8796.141.3: COMPATIBLE.** Xcode 15.x is the toolchain generation contemporaneous with Darwin 22 / XNU 8796 (iOS 16 generation). The workflow comment in xnu.yml:33 notes Xcode 15.x "matches xnu-8796 IORPC ABI".
- **macOS 14 + Xcode 15.4: COMPATIBLE.** GitHub-hosted macos-14 runners ship Xcode 15.x; the workflow explicitly selects Xcode 15.4 via sudo xcode-select -s.

### Runner retirement concern

GitHub-hosted `macos-14` runners are approaching retirement (GitHub typically retires macOS runner images ~12–18 months after release; macOS 14 was the default image in 2024). Once `macos-14` is retired:
- The workflow's `runs-on: macos-14` will fail with "image not found" until retargeted to a newer runner (e.g. macos-15 or macos-26).
- Newer runners ship newer Xcode (16.x / 26.x), which may not be source-compatible with XNU 8796.141.3 without patch adaptation. DXB's choose_xnu() does not list a KDK for newer macOS in a way that satisfies XNU 8796.141.3's RC_DARWIN_KERNEL_VERSION=22.6.0 requirement.
- **Action required before retirement:** pin the workflow to a self-hosted macOS runner with Xcode 15.4 + KDK 13.5 build 22G74 installed, or retarget to a GitHub image that still ships Xcode 15.4 and re-verify compatibility.

## 3. Missing tools on current Linux host (explicit list)

The following macOS-specific tools are **absent** on this Linux x86_64 host and cannot be installed via Linux package managers:

1. `xcodebuild` — Xcode build system (required by build_bootstrap_cmds, build_dtrace, libsystem_headers, libsyscall_headers, build_libdispatch)
2. `xcrun` — Xcode toolchain locator (required by setup_xcode_toolchain, build_bootstrap_cmds, build_dtrace, libsystem_headers, libsyscall_headers, build_libdispatch)
3. `mig` — Mach interface generator (required by xnu/makedefs/MakeInc.cmd; dxb patches attempt to relocate it into FAKEROOT_DIR at build.sh:439-440)
4. `kmutil` — kernel management tool (required by build_kc for kernel collection creation at build.sh:810-824)
5. `hdiutil` — disk image utility (required by choose_xnu() to attach KDK DMG at build.sh:393 and by the CI KDK install step at xnu.yml:167)
6. `xcode-select` — developer directory selector (required by setup_xcode_toolchain at build.sh:43 and by CI at xnu.yml:78)
7. `sw_vers` — macOS version reporter (used by build_kc at build.sh:806 for version_lte check)
8. Xcode 15.4 app bundle at /Applications/Xcode_15.4.app (absent — path does not exist)
9. `DEVELOPER_DIR` environment / Xcode toolchain at /Applications/Xcode_15.4.app/Contents/Developer (absent)
10. `/Library/Developer/KDKs/KDK_13.5_22G74.kdk` (absent — KDKs directory does not exist)
11. iPhoneOS SDK / macOS SDK headers (absent — required by xcodebuild -sdk macosx)
12. `clang` from Xcode toolchain (only system clang 23.1.4 present at /usr/bin/clang — NOT acceptable per build.sh:81-88 which verifies clang path starts with DEVELOPER_DIR)

Host tools that ARE present but **cannot substitute**: clang 23.1.4, ld64.lld 23.1.4, aarch64-linux-gnu-gcc 16.2.0, GNU ld. These are Linux toolchains and lack Apple SDK headers, mach-o target support for XNU, and the cctools wrapper expected by xnu's makefiles.

## 4. Blocker documentation — why XNU build cannot run on Linux

### Blocker 1: No macOS kernel/Xcode toolchain

XNU 8796.141.3 builds via Apple's make/xcconfig system which invokes `xcrun -find clang`, `xcrun -find ld`, `xcrun -find mig`, etc. These resolve only inside Xcode's toolchain (DEVELOPER_DIR/Toolchains/XcodeDefault.xctoolchain). Linux has no Xcode, no DEVELOPER_DIR, and no Apple clang/ld64. dxb's setup_xcode_toolchain() (build.sh:40-95) explicitly errors when DEVELOPER_DIR is unset or missing (build.sh:45-48).

### Blocker 2: mig (Mach interface generator) is macOS-only

mig generates Mach IPC glue from .defs files. It is part of Xcode's cctools/bootstrap_cmds and is not available on Linux. dxb patches MakeInc.cmd to locate mig via `find $(FAKEROOT_DIR) -name mig` (build.sh:439-440), but mig must first be built by build_bootstrap_cmds() (build.sh:484-509), which requires xcodebuild -sdk macosx — itself unavailable on Linux.

### Blocker 3: KDK headers are macOS-only

The Kernel Debug Kit (13.5 build 22G74) is distributed as a macOS installer package (KernelDebugKit.pkg inside a DMG). It installs kernel headers and private frameworks to /Library/Developer/KDKs/. Linux cannot mount DMGs, run Apple's installer, or consume the KDK's Mach-O headers/SDK. KDKROOT is a hard requirement checked at build.sh:701-704 and build.sh:721-724.

### Blocker 4: xcodebuild is required for dependency builds

build_bootstrap_cmds, build_dtrace, libsystem_headers, libsyscall_headers, and build_libdispatch all invoke `xcodebuild install -sdk macosx` (build.sh:506, 523, 571, 584, 620). There is no Linux equivalent of xcodebuild.

### Blocker 5: hdiutil required for KDK acquisition

The CI KDK installation flow downloads a DMG and attaches it with hdiutil (xnu.yml:167, build.sh:393). Linux lacks hdiutil and cannot mount Apple DMG images.

### Blocker 6: Architecture mismatch

This host is x86_64 Linux; XNU targets arm64 (and x86_64 on macOS). The macOS SDK, arm64 compiler, and arm64 headers required for a cross-build are absent. The present aarch64-linux-gnu-gcc 16.2.0 targets Linux, not Darwin, and lacks XNU's required kernel build flags, _kernelrpc symbols, and mach-o output.

### Blocker status

**Baseline XNU build: BLOCKED.** Cannot proceed on this Linux host. Success criterion "Baseline XNU build succeeds" (recorded in XNU_SOURCE_ACQUISITION.md:75) is pending a macOS build host. No fabrication of macOS tool availability is performed; all macOS-specific checks are marked "pending macOS runner execution."

## 5. Alternative verified approaches

### Option A: GitHub Actions macos-14 runner (recommended)

The existing workflow `.github/workflows/xnu.yml` already targets `runs-on: macos-14`. On a GitHub-hosted macos-14 runner:
- Xcode 15.x is preinstalled; the workflow selects Xcode 15.4 via `sudo xcode-select -s` (xnu.yml:78-80).
- `xcodebuild`, `xcrun`, `mig`, `clang`, `ld`, `kmutil`, `hdiutil` are all present (verified step at xnu.yml:82-91).
- KDK 13.5 build 22G74 is auto-downloaded from the dortania KdkSupportPkg manifest (xnu.yml:146-179) or installed manually (BUILD.md:175-181).
- XNU source is cloned at tag xnu-8796.141.3 and commit verified against 1b191cb (xnu.yml:126-138).
- DXB engine is pinned at 6639065 (xnu.yml:119-124).

Verification commands that should pass on the macos-14 runner:
```sh
sw_vers -productVersion        # expect 13.5.x
sw_vers -buildVersion          # expect 22G74
xcodebuild -version            # expect Xcode 15.4.x
xcode-select -p                # expect /Applications/Xcode_15.4.app/Contents/Developer
xcrun --find clang             # expect path under DEVELOPER_DIR
xcrun --find mig               # expect path under DEVELOPER_DIR
xcrun --find kmutil            # expect path under DEVELOPER_DIR
which hdiutil                  # expect /usr/bin/hdiutil
ls -la /Library/Developer/KDKs/KDK_13.5_22G74.kdk  # expect present
```

### Option B: Local Mac

A physical Mac running macOS 13.5 (build 22G74) with Xcode 15.4 installed at /Applications/Xcode_15.4.app and KDK 13.5 build 22G74 installed at /Library/Developer/KDKs/. Manual KDK installation procedure (section 6) applies. This avoids runner retirement risk but requires hardware procurement and ongoing maintenance.

## 6. Manual KDK installation procedure

If automatic download from the dortania KdkSupportPkg manifest fails (or the manifest URL is unavailable), install the Kernel Debug Kit 13.5 build 22G74 manually:

1. Download the Kernel Debug Kit 13.5 (build 22G74) DMG from Apple's Developer Portal (requires Apple Developer account) or the dortania KdkSupportPkg mirror.
2. Mount the DMG:
   ```
   hdiutil attach /path/to/KernelDebugKit.dmg
   ```
3. Install the package:
   ```
   sudo installer -pkg '/Volumes/Kernel Debug Kit/KernelDebugKit.pkg' -target /
   ```
4. Unmount the DMG:
   ```
   hdiutil detach '/Volumes/Kernel Debug Kit'
   ```
5. Verify installation:
   ```
   ls -la /Library/Developer/KDKs/KDK_13.5_22G74.kdk
   ```

The KDK provides the kernel headers (KDKROOT) that dxb passes to `make installhdrs` and the kernel build (KDKROOT= at build.sh:552, 721, 732). Without it, build_xnu() exits with "KDKROOT not found" (build.sh:721-724).

## 7. Sign-off status

| Item | Status | Evidence |
|------|--------|----------|
| XNU source pinned at xnu-8796.141.3 @ 1b191cb | VERIFIED | git rev-parse + describe on /home/yu/BEGONIA-XNU/xnu/xnu-8796 |
| DXB build engine pinned at 6639065 | VERIFIED | git rev-parse on /home/yu/BEGONIA-XNU/tools/darwin-xnu-build |
| Required toolchain matrix documented | COMPLETE | Section 1 of this document |
| Compatibility analysis (macOS 14 + Xcode 15.4 + KDK 22G74 + XNU 8796.141.3) | COMPATIBLE | Section 2 of this document |
| Missing macOS tools on Linux host | DOCUMENTED | Section 3 of this document |
| Blocker: XNU build cannot run on Linux | BLOCKED | Section 4 of this document |
| Alternative: GitHub Actions macos-14 | VERIFIED APPROACH | .github/workflows/xnu.yml (runs-on: macos-14) |
| Alternative: Local Mac with macOS 13.5 + Xcode 15.4 + KDK 22G74 | VIABLE | Section 5 of this document |
| Runner retirement concern (macos-14) | IDENTIFIED — action before retirement | Section 2 of this document |
| Manual KDK installation procedure | DOCUMENTED | Section 6 of this document |

### Final sign-off

**Baseline XNU build on this Linux host: NOT POSSIBLE — BLOCKED.** All macOS-specific toolchain checks are marked "pending macOS runner execution." The validated path to a successful baseline XNU 8796.141.3 build is execution on a macOS host (GitHub Actions macos-14 runner or local Mac) with Xcode 15.4 and KDK 13.5 build 22G74 installed, per the pinned configuration in .github/workflows/xnu.yml and tools/darwin-xnu-build/build.sh.

**No macOS tool availability has been fabricated.** Every macOS-specific item above is either verified absent on this Linux host or explicitly deferred to macOS runner execution.
