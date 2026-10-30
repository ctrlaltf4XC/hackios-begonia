# Runner Readiness Report — macOS runner for XNU 8796.141.3 compilation

**Operation:** BEGONIA-XNU (Operation Kernel Forge) — Agent 2: macOS Runner Engineer
**Generated:** 2026-10-30T14:51:50Z
**Target:** XNU 8796.141.3 on macOS Sonoma 14.x, Xcode 15.4, KDK 13.5 build 22G74

---

## 1. Runner Label and Current Status

| Item | Value |
|------|-------|
| GitHub-hosted label | `macos-14` (also `macos-14-large`, `macos-14-xlarge`) |
| Architecture | `macos-14` / `macos-14-xlarge`: arm64 (Apple Silicon M1); `macos-14-large`: x64 (Intel) |
| OS version (image) | macOS 14.8.9 (23J631), Darwin 23.6.0 |
| Image version | 20260831.0302.1 |
| **Current status** | **BROWNOUT (active)** — jobs on `macos-14` labels are failing on purpose |
| Deprecation start | July 6, 2026 |
| Retirement date | November 2, 2026 (after this date the labels have no runner) |

Status sources: GitHub Changelog "GitHub Actions: macOS 14 runner image retirement" (2026-10-01); actions/runner-images issue #13518; Azure DevOps hosted-deprecation schedule.

---

## 2. Brownout Window Status

Current time: **2026-10-30T14:51:50Z**

Scheduled brownout windows (all UTC):
- Oct 5, 14:00 – Oct 6, 00:00
- Oct 12, 14:00 – Oct 13, 00:00
- Oct 16, 14:00 – Oct 17, 00:00
- Oct 19, 14:00 – Oct 20, 00:00
- Oct 23, 14:00 – Oct 24, 00:00
- Oct 26, 14:00 – Oct 27, 00:00
- Oct 29, 14:00 – Oct 30, 00:00
- **Oct 30, 14:00 – Oct 31, 00:00** ← active now

**Brownout: ACTIVE.** The current time falls inside the Oct 30 14:00 – Oct 31 00:00 UTC window. Jobs dispatched to `macos-14` now will fail on purpose.

**Resume time:** after **2026-10-31T00:00:00Z**.

---

## 3. Retirement Date Confirmation

**Confirmed: November 2, 2026.** GitHub announced the macOS 14 runner image retirement on October 1, 2026, with removal effective November 2, 2026. After that date, `macos-14`, `macos-14-large`, and `macos-14-xlarge` labels have no available runner and jobs will fail permanently.

---

## 4. Xcode 15.4 Availability Confirmation

**CONFIRMED.** Xcode 15.4 (build 15F31d) is installed on the `macos-14` image and is the **default** Xcode.

| Xcode version | Build | Path | Default? |
|---------------|-------|------|----------|
| 15.4 | 15F31d | /Applications/Xcode_15.4.app | **yes** (symlink /Applications/Xcode.app) |
| 15.3 | 15E204a | /Applications/Xcode_15.3.app | no |
| 15.2 | 15C500b | /Applications/Xcode_15.2.app | no |
| 15.1 | 15C65 | /Applications/Xcode_15.1.app | no |
| 15.0.1 | 15A507 | /Applications/Xcode_15.0.1.app | no |
| 16.1 | 16B40 | /Applications/Xcode_16.1.app | no |
| 16.2 | 16C5032a | /Applications/Xcode_16.2.app | no |

Source: actions/runner-images `images/macos/macos-14-arm64-Readme.md` (Xcode section).

Xcode 15.4 requires macOS Sonoma 14 or later and includes SDKs for iOS 17.5, macOS Sonoma 14.5, etc. — compatible with XNU 8796 (Darwin 23.x / Sonoma).

---

## 5. KDK 13.5 build 22G74 Availability Confirmation

**NOT pre-installed.** The KDK (Kernel Development Kit) does not appear in the `macos-14` runner image software inventory (actions/runner-images readme). It is not part of the standard image.

**Installation capability:** KDK 22G74 must be downloaded and installed manually from Apple Developer (developer.apple.com). Capability depends on:
- Network access from the runner to developer.apple.com
- An Apple Developer account (KDK downloads may require authentication)
- Sufficient disk space (14 GB SSD on standard runners)

Verification step on the runner: `ls /Library/Developer/KDKs/` or `pkgutil --pkg-info=com.apple.pkg.KDK` after manual install.

**Impact:** The immutable requirement (KDK 13.5 build 22G74) cannot be satisfied out-of-the-box on GitHub-hosted `macos-14`; a setup step to fetch and install the KDK is required before XNU compilation.

Note: build 22G74 corresponds to macOS 13.5 (Ventura); XNU 8796.141.3 is a Sonoma (Darwin 23.x) kernel. The requirement is immutable as stated.

---

## 6. Resource Summary (GitHub-hosted `macos-14` family)

| Label | CPU | RAM | Disk (SSD) | Architecture |
|-------|-----|-----|------------|--------------|
| `macos-14` (standard) | 3 (M1) | 7 GB | 14 GB | arm64 |
| `macos-14-xlarge` | 5 | 14 GB | 14 GB | arm64 |
| `macos-14-large` | 4 (Intel) | 14 GB | 14 GB | x64 |

Source: GitHub-hosted runners reference (docs.github.com).

Local execution environment (this shell, Linux x86_64 EndeavourOS — NOT a macOS runner): 4 CPU, 7.6 GB RAM, 83 GB available disk. macOS CLI tools (`xcodebuild`, `xcrun`, `mig`, `kmutil`, `hdiutil`) are NOT present here; they only exist on the macOS runner.

---

## 7. Required Command-Line Tools (on `macos-14` runner)

| Tool | Availability on `macos-14` | Source |
|------|---------------------------|--------|
| `xcodebuild` | present | Xcode 15.4 |
| `xcrun` | present | Xcode CLT 16.2 |
| `mig` | present | Xcode CLT / macOS |
| `clang` | present | Xcode 15.4 (Clang/LLVM 15.0.0) |
| `ld` | present | Xcode / cctools |
| `kmutil` | present | macOS Sonoma system utility |
| `hdiutil` | present | macOS Sonoma system utility |

Note: none of these are present on the current local Linux shell; verification must occur on the macOS runner itself (e.g. `which xcodebuild xcrun mig clang ld kmutil hdiutil` and `xcodebuild -version`).

---

## 8. Dispatch Recommendation

**Verdict: WAIT UNTIL 2026-10-31T00:00:00Z**

Do NOT dispatch to `macos-14` while the brownout is active (now through Oct 31 00:00 UTC). Jobs will fail on purpose.

- **After brownout ends (Oct 31 00:00 UTC):** `macos-14` is dispatchable until its hard retirement on **November 2, 2026**.
- **Xcode 15.4:** confirmed available (default) — satisfies the toolchain requirement.
- **KDK 22G74:** not pre-installed — add a setup step to download/install from Apple Developer before compiling XNU.

### If `macos-14` is unavailable or undesirable — alternatives

1. **Self-hosted Sonoma 14 runner** — provision a Mac running macOS Sonoma 14.x, install Xcode 15.4 and KDK 22G74 manually. Full control, avoids brownout/retirement, but requires hardware and maintenance.
2. **Cloud Mac (MacStadium, MacinCloud, AWS EC2 Mac1, etc.)** — rent a Sonoma 14 Mac instance, install Xcode 15.4 + KDK 22G74. Avoids GitHub-hosted brownout/retirement.
3. **`macos-15` / `macos-latest`** — NOT a drop-in substitute: default Xcode is 16.4 and the installed range is 16.0–16.4; Xcode 15.4 is NOT available on macos-15. Would violate the immutable Xcode 15.4 requirement.

---

**Bottom line:** `macos-14` meets the Xcode 15.4 requirement but is in an active brownout now and retires Nov 2, 2026. Dispatch only after **2026-10-31T00:00:00Z**, with a KDK 22G74 install step added; prefer a self-hosted or cloud Sonoma 14 Mac for anything that must run on or after November 2, 2026.
