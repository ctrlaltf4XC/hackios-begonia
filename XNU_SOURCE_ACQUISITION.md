# XNU_SOURCE_ACQUISITION.md

Record of the exact XNU 8796 source used by the BEGONIA-XNU project. Source acquired from
Apple's official public mirror; no Apple Developer login required.

## Source identity

| Field | Value |
|-------|-------|
| Project | XNU (Darwin kernel) |
| Release line | XNU 8796 (iOS 16 / Darwin 22 generation) |
| Exact tag | `xnu-8796.141.3` |
| Commit SHA | `1b191cb58250d0705d8a51287127505aa4bc0789` |
| Tag object | `f625db25f47631424dc62ac1c3414b6e530f7e8a` |
| Tree SHA | `5c0fc728332a322c2fb8fde61dfc6152a34c9ad6` |
| Commit date | 2023-08-14 05:06:07 +0000 |
| Source tarball SHA-256 | `a19fedd902d2237cae2f4db93942dc868c6288715ba303930f2f719f5215f352` |
| Clone URL | https://github.com/apple-oss-distributions/xnu.git |
| Local path | `/home/yu/BEGONIA-XNU/xnu/xnu-8796` |
| Access | Public GitHub mirror — no Apple Developer login required |

## Why xnu-8796.141.3 (not 8796.101.5)

The reference project ios-sweet (dipa-studio/ios-sweet @ 45263be) builds XNU via
blacktop/darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549. Its CI
workflow (`.github/workflows/xnu.yml`) defaults to `MACOS_VERSION=13.5`, which
resolves through the macOS 13.5 release.json to xnu tag `xnu-8796.141.3`
(RC_DARWIN_KERNEL_VERSION=22.6.0, KDK 13.5 build 22G74).

xnu-8796.101.5 (commit aca3beaa3dfbd42498b42c5e5ce20a938e6554e5, 2023-06-13) is an
earlier 8796 sub-build and was NOT used by the reference pipeline. Both are the
"XNU 8796 release line"; no newer XNU release (e.g. 8792, 12377) is substituted,
per project rule.

## Acquisition method

```sh
git clone --depth 1 --branch xnu-8796.141.3 \
  https://github.com/apple-oss-distributions/xnu.git \
  /home/yu/BEGONIA-XNU/xnu/xnu-8796
# verified: git rev-parse HEAD = 1b191cb58250d0705d8a51287127505aa4bc0789
#           git describe --tags --exact-match = xnu-8796.141.3
```

## Build engine (reference)

| Field | Value |
|-------|-------|
| Repo | https://github.com/blacktop/darwin-xnu-build |
| Pinned SHA | `663906528d1cd5ea8fde7de6f16de73db9511549` |
| Local path | `/home/yu/BEGONIA-XNU/tools/darwin-xnu-build` |
| Default config | MACOS_VERSION=13.5, KERNEL_CONFIG=DEVELOPMENT, MACHINE_CONFIG=SWEET |
| macOS runner | macos-14 (Xcode 15.x — iig matches xnu-8796 IORPC ABI) |

## Build environment blocker (documented, not fabricated)

The reference build pipeline (`darwin-xnu-build/build.sh`) requires:
- macOS host (macos-14 / Xcode 15.x) — `xcodebuild`, `xcrun`, `mig`, `kmutil`, `hdiutil`
- Xcode Command Line Tools + `DEVELOPER_DIR`
- Kernel Debug Kit (KDK) from the Apple Developer Portal (`/Library/Developer/KDKs/`)
- Homebrew deps: jq, gum, xcodes, cmake, ninja, ipsw

This host is **Linux (x86_64)**. Available: clang 23.1.4, ld64.lld. Missing: iPhoneOS SDK,
mig (Mach interface generator), macOS SDK/KDK headers, xcodebuild. Therefore a **baseline XNU
build cannot be performed on this Linux host**. It must be performed on a macOS runner with Xcode
15.x and the KDK installed (e.g. GitHub Actions macos-14, or a local Mac).

This blocker is recorded here rather than fabricating a local build. Success criterion
"Baseline XNU build succeeds" is pending a macOS build host.

## Success criteria status

1. [x] Exact XNU 8796 source acquired (xnu-8796.141.3 @ 1b191cb)
2. [x] Source revision and checksum recorded (this document)
3. [ ] Baseline XNU build succeeds — BLOCKED (no macOS/Xcode/KDK host)
4. [ ] Begonia platform skeletons compile — pending XNU build host
5. [ ] Adapted patches apply cleanly — pending macOS build + patch adaptation
6. [ ] Modified XNU kernel builds — pending macOS build
7. [ ] Kernel image passes validation — pending hardware boot
