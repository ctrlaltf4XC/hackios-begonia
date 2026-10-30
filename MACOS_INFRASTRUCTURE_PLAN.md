# MACOS_INFRASTRUCTURE_PLAN.md — BEGONIA-XNU Operation Launchpad

**Agent:** Agent 3 — Cloud macOS Infrastructure Engineer
**Repository:** `/home/yu/BEGONIA-XNU`
**Target:** Compatible remote macOS build environment for XNU 8796.141.3 compilation
**Document status:** Deliverable — sign-off at end of document

---

## 1. Immutable requirement matrix

| # | Requirement | Value | Source of truth |
|---|-------------|-------|-----------------|
| 1 | Xcode | 15.4 | `xnu.yml:39` XCODE_PATH; `CI_TOOLCHAIN_VALIDATION.md:13` |
| 2 | macOS host | Sonoma 14.x | `BUILD.md:87` MACOS_VERSION 13.5 target on macos-14 |
| 3 | KDK | Kernel Debug Kit 13.5 build 22G74 | `CI_TOOLCHAIN_VALIDATION.md:22` KDKROOT `/Library/Developer/KDKs/KDK_13.5_22G74.kdk` |
| 4 | XNU source | xnu-8796.141.3 @ 1b191cb58250d0705d8a51287127505aa4bc0789 | `xnu.yml:36-37`, `PROJECT_STATUS.md:14` |
| 5 | Build engine | blacktop/darwin-xnu-build @ 663906528d1cd5ea8fde7de6f16de73db9511549 | `xnu.yml:34`, `BUILD.md:76` |
| 6 | macOS-specific tools | xcodebuild, xcrun, mig, clang (Xcode), ld, kmutil, hdiutil | `CI_TOOLCHAIN_VALIDATION.md:3-11` |

**Hard constraints (do not violate):**
- Do NOT blindly replace `macos-14` with `macos-15` or `macos-latest`.
- Xcode 15.4 requires a supported Sonoma 14.x environment.
- Do not invent Xcode/KDK compatibility combinations.
- October 30-31 scheduled brownout must be respected.
- macOS 14 runner retirement is November 2, 2026.

---

## 2. Current operational context (verified, message time 2026-10-30T14:31:49Z)

| Item | Status | Evidence |
|------|--------|----------|
| macOS 14 runner retirement | **November 2, 2026** (2 days away) | GitHub Changelog, 2026-10-01: "The macOS 14 runner image will be retired on November 2, 2026" |
| macOS 14 brownout window #8 | **ACTIVE NOW** — Oct 30 14:00 UTC to Oct 31 00:00 UTC | GitHub Changelog brownout schedule |
| Jobs using macos-14 during brownout | Temporarily fail | GitHub Changelog: "jobs using macOS 14 will temporarily fail during the following scheduled brownout periods" |
| After Nov 2, 2026 | macos-14 label removed entirely; jobs fail "image not found" | GitHub Changelog / actions/runner-images#13518 |
| Affected labels | `macos-14`, `macos-14-large`, `macos-14-xlarge` | GitHub Changelog |

Implication: macos-14 is usable ONLY outside brownout windows and ONLY before November 2, 2026. A post-retirement alternative (Option B or C) is required for sustained builds.

---

## 3. Verified fact base (sources consulted)

| Fact | Value | Source |
|------|-------|--------|
| macos-14 image OS | macOS 14.8.7 (23J520), Darwin 23.6.0 | runner-images `images/macos/macos-14-Readme.md` |
| macos-14 image version | 20260629.0276.1 | runner-images `images/macos/macos-14-Readme.md` |
| Xcode on macos-14 | 15.4 (default, build 15F31d) at `/Applications/Xcode_15.4.app`; also 15.0.1–15.3, 16.1, 16.2 | runner-images `images/macos/macos-14-Readme.md` [Xcode table] |
| macOS SDKs w/ Xcode 15.4 | macosx 14.0, 14.2, 14.4, 14.5 | runner-images `images/macos/macos-14-Readme.md` [Installed SDKs] |
| Xcode on macos-15 | 16.0–16.4 (default 16.4); **NO Xcode 15.x** | runner-images `images/macos/macos-15-Readme.md` [Xcode table] |
| KDK 13.5 build 22G74 | Present in dortania KdkSupportPkg manifest | manifest.json entry: name "Kernel Debug Kit 13.5 build 22G74", build 22G74, version 13.5, URL `.../download/22G74/Kernel_Debug_Kit_13.5_build_22G74.dmg` |
| GitHub-hosted macOS arm64 (M1) | 3 CPU, 7 GB RAM, 14 GB SSD | GitHub-hosted runners reference |
| GitHub-hosted macOS Intel | 4 CPU, 14 GB RAM, 14 GB SSD | GitHub-hosted runners reference |
| macOS Large (Intel) | 12 CPU, 30 GB RAM, 14 GB SSD (`macos-14-large`) | Larger runners reference |
| macOS XLarge (arm64 M2) | 5 CPU + 8 GPU, 14 GB RAM, 14 GB SSD (`macos-14-xlarge`) | Larger runners reference |
| macos-14 = arm64; macos-14-large = Intel | Architecture mapping | GitHub-hosted runners reference; runner-images README symlinks |

Note on Xcode 15.4 on macos-15: macos-15's oldest Xcode is 16.0. Xcode 15.4 is NOT preinstalled. Installing it manually requires downloading the Xcode 15.4 XIP from the Apple Developer Portal (Apple Developer credentials) — this contradicts the project's no-credentials workflow rule and is therefore NOT a supported migration path without independent compatibility verification (CI_AUDIT.md F-007 / CI_TOOLCHAIN_VALIDATION.md Section 2).

---

## 4. Option A — GitHub-hosted macOS 14 runner (`runs-on: macos-14`)

Already configured in `.github/workflows/xnu.yml` (both `xnu-baseline` and `xnu-begonia` jobs).

### 4.1 Availability & retirement status
- `macos-14` is a GitHub-hosted runner image, currently available but deprecated.
- Retirement: **November 2, 2026** (verified, GitHub Changelog 2026-10-01).
- After removal, `runs-on: macos-14` resolves to no image → jobs fail until retargeted.
- `macos-14` does NOT map to a newer OS; it is removed, not renamed.

### 4.2 Brownout schedule (verified)
Eight brownout windows, all 2026, all 14:00–00:00 UTC:

| # | Window (UTC) | Status relative to 2026-10-30T14:31Z |
|---|--------------|--------------------------------------|
| 1 | Oct 5 14:00 – Oct 6 00:00 | Passed |
| 2 | Oct 12 14:00 – Oct 13 00:00 | Passed |
| 3 | Oct 16 14:00 – Oct 17 00:00 | Passed |
| 4 | Oct 19 14:00 – Oct 20 00:00 | Passed |
| 5 | Oct 23 14:00 – Oct 24 00:00 | Passed |
| 6 | Oct 26 14:00 – Oct 27 00:00 | Passed |
| 7 | Oct 29 14:00 – Oct 30 00:00 | Passed |
| 8 | **Oct 30 14:00 – Oct 31 00:00** | **ACTIVE NOW** |

Jobs scheduled on macos-14 during window #8 will temporarily fail. Next safe execution window: after Oct 31 00:00 UTC and before Nov 2, 2026 retirement.

### 4.3 Xcode 15.4 availability on macos-14
- **VERIFIED YES.** Xcode 15.4 (build 15F31d) is the DEFAULT Xcode on macos-14 at `/Applications/Xcode_15.4.app`.
- Workflow selects it explicitly: `sudo xcode-select -s /Applications/Xcode_15.4.app/Contents/Developer` (xnu.yml:76-80).
- Symlinks: `/Applications/Xcode_15.4.0.app`, `/Applications/Xcode.app`.
- Compatible SDKs: macOS 14.0/14.2/14.4/14.5 (Xcode 15.4 column). SDKROOT=macosx resolves to the latest macOS 14.x SDK on this image.
- Requirement satisfied: Xcode 15.4 on Sonoma 14.x.

### 4.4 KDK 13.5 build 22G74 installability
- **VERIFIED YES (automatic).** Present in dortania `KdkSupportPkg` manifest with direct URL.
- Workflow `Install KDK` step (xnu.yml:146-179) downloads from `https://raw.githubusercontent.com/dortania/KdkSupportPkg/gh-pages/manifest.json`, selects entry by name `Kernel Debug Kit 13.5 build 22G74`, mounts DMG via `hdiutil`, runs `installer -pkg .../KernelDebugKit.pkg -target /`.
- Manual fallback documented in `BUILD.md:175-181` (download DMG → `hdiutil attach` → `sudo installer` → `hdiutil detach` → verify `/Library/Developer/KDKs/KDK_13.5_22G74.kdk`).
- Requirement satisfied: KDK 22G74 installable on macos-14 (host macOS version need not match KDK target version; KDK provides headers for Darwin 22 / XNU 8796).

### 4.5 Disk space, CPU, RAM resources

| Label | Arch | CPU | RAM | Storage | Xcode 15.4 fit |
|-------|------|-----|-----|---------|----------------|
| macos-14 | arm64 (M1) | 3 | 7 GB | 14 GB SSD | Tight |
| macos-14-large | Intel | 12 | 30 GB | 14 GB SSD | Tight |
| macos-14-xlarge | arm64 (M2) | 5 + 8 GPU | 14 GB | 14 GB SSD | Tight |

**Disk constraint (risk):** Runner SSD is 14 GB total, shared with the OS image and ALL preinstalled Xcode versions (15.0.1–15.4, 16.1, 16.2). Xcode 15.4 installed size is on the order of 10–13 GB. XNU source + `build/xnu.obj` + `build/xnu.sym` + `fakeroot/` + KDK DMG (~1.2 GB) + downloaded DMG can approach or exceed remaining free space. Mitigation: run `df -h` at job start, avoid installing additional tooling, rely on `force_clean`/cache hygiene, and prefer the largest label available (xlarge/large) for headroom. This is the principal operational risk of Option A.

**CPU/RAM for parallelism:** Workflow auto-detects jobs = min(sysctl hw.ncpu, RAM_GB/2, min 1).
- arm64 M1 (3 CPU, 7 GB): ~3 jobs (RAM-limited at 3).
- Intel (4 CPU, 14 GB): ~4 jobs (RAM-limited at 7, CPU-capped at 4).
- xlarge (5 CPU, 14 GB): ~5 jobs.
- large (12 CPU, 30 GB): ~12 jobs (RAM-limited at 15, CPU-capped at 12).

### 4.6 Build duration
- Job timeout: 120 minutes (xnu.yml:63).
- Build duration recorded via `date +%s` around `make install`, output as `build_duration_sec` (xnu.yml:248-249).
- Estimated clean baseline build: within 120-min timeout on 4–12 job labels; incremental/cache-hit builds near-instant.
- BENCHMARK.md scenarios B1–B5 apply.

### 4.7 Runner availability
- GitHub-managed, ephemeral VM (fresh per job). No persistent state between jobs except via `actions/cache` and artifacts.
- Capacity may be reduced during deprecation → longer queue times (GitHub Changelog).
- No availability during brownout windows; unavailable entirely after Nov 2, 2026.

### 4.8 Security
- Ephemeral VM discarded after job; no residual state.
- Workflow exposes no secrets (`CI_AUDIT.md F-006`: no `secrets.` context, no credentials printed).
- KDK download uses public dortania manifest only (no Apple Developer credentials).
- XNU source from public mirror `apple-oss-distributions/xnu` (no Apple Developer login).
- Standard GitHub-hosted runner supply chain; SBOM available for images.

### 4.9 Cost
- Free and unlimited for public repositories (GitHub-hosted runners reference). BEGONIA-XNU is public → $0/minute.
- Artifact retention 7 days; GitHub Actions storage/cache usage within free quota for public repos.

### 4.10 Persistent build-cache support
- **PARTIAL (non-persistent host, persistent cache service).** `actions/cache/restore` + `actions/cache/save` keyed on `runner.os`-xcode15.4-kdk-22g74-XNU_COMMIT-KERNEL_CONFIG-MACHINE_CONFIG (xnu.yml:187, 271). Cache persists across runs on GitHub's cache service (not tied to runner VM).
- `build/xnu.obj`, `build/xnu.sym`, `fakeroot/`, `.cache/` cached (xnu.yml:189-193, 273-277).
- Cache key mismatch (e.g. new XNU commit) → clean build fallback (BUILD.md:258).
- Because each job has an independent filesystem (CI_AUDIT.md F-004), redundant XNU clones, DXB fetches, and KDK installs occur per job — cache mitigates build outputs but not setup time.

### 4.11 Option A evaluation

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Xcode 15.4 availability | YES | Default on macos-14 (15F31d) |
| KDK 22G74 installation | YES | Auto-download from dortania manifest; manual fallback |
| Disk space | RISK | 14 GB SSD shared; Xcode 15.4 + XNU + KDK may be tight |
| CPU/RAM | OK–GOOD | 3–12 jobs depending on label |
| Build duration | OK | 120-min timeout; duration tracked |
| Runner availability | LIMITED | Brownout #8 active now; retired Nov 2, 2026 |
| Security | GOOD | Ephemeral, no secrets, public KDK mirror |
| Cost | FREE | Public repo, no per-minute charge |
| Persistent build-cache | PARTIAL | GitHub cache service; host itself is ephemeral |

**Verdict:** Viable short-term path. Use only outside brownout windows and before Nov 2, 2026. Do not treat as a long-term solution.

---

## 5. Option B — Authorized cloud Mac hosting on genuine Apple hardware

Dedicated physical Macs (Mac mini / Mac Studio) hosted by a provider; full root access; persistent across builds.

### 5.1 Providers identified

| Provider | Offering | Hardware | Notes |
|----------|----------|----------|-------|
| MacStadium | Mac Cloud Compute (bare metal), Orka virtualization | Mac mini M2/M4, Mac Studio M1 Max/M2 Ultra; data centers, AWS (Orka on AWS), on-prem | Genuine Apple hardware, dedicated IP, pre-installed macOS (Sonoma available on request), root access, $109–$369+/mo (portal pricing sample) |
| AWS EC2 Mac instances | Dedicated Host, bare metal | mac1.metal (Intel i7, 32 GB), mac2.metal (M1, 16 GB), mac2-m1ultra.metal (128 GB), mac2-m2.metal (24 GB), mac2-m2pro.metal (32 GB), mac-m4.metal (24 GB), mac-m4pro.metal (48 GB) | Base macOS AMI without Xcode; install Xcode from Apple Developer website; can snapshot AMI with Xcode preinstalled; min 24-hour Dedicated Host allocation; On-Demand only |

### 5.2 Xcode 15.4 installability
- **YES.** Host runs macOS 14.x (Sonoma); Xcode 15.4 XIP downloaded from Apple Developer Portal and installed to `/Applications/Xcode_15.4.app`, then `xcode-select -s`. Requires Apple Developer account for the download (one-time manual operation outside the CI workflow; no credentials stored in workflow).
- macOS AMI compatibility is hardware-bound (Apple hardware supports the macOS version it shipped with or later). M1/M2/M4 minis can run Sonoma 14.x.

### 5.3 KDK 22G74 installability
- **YES.** Manual installation per `BUILD.md:175-181`: download Kernel Debug Kit 13.5 build 22G74 DMG (Apple Developer Portal or dortania mirror), `hdiutil attach`, `sudo installer -pkg .../KernelDebugKit.pkg -target /`, `hdiutil detach`, verify `/Library/Developer/KDKs/KDK_13.5_22G74.kdk`. Installs on macOS 14.x host; KDK targets Darwin 22 (XNU 8796).

### 5.4 Resources
- Depends on selected instance/plan. AWS mac-m4.metal: 10 CPU, 24 GB RAM; mac-m4pro.metal: 14 CPU, 48 GB RAM; mac2-m1ultra.metal: 20 CPU, 128 GB RAM. MacStadium M4.L: 12 CPU, 48 GB RAM, 1 TB SSD. Storage much less constrained than GitHub-hosted 14 GB SSD.

### 5.5 Build duration
- Proportional to instance CPU/RAM; M4/M2 Ultra instances faster than GitHub-hosted M1. No imposed 120-minute job timeout (self-controlled).

### 5.6 Runner availability
- Dedicated, on-demand. AWS: min 24-hour allocation per Dedicated Host. MacStadium: monthly/portal provisioning. No brownout/retirement windows. 100% controllable uptime.

### 5.7 Security
- Dedicated genuine Apple hardware, root access, dedicated IP (MacStadium), private network/firewall options, full control of OS and toolchain. No shared ephemeral VM risk.

### 5.8 Cost
- Paid: hourly (AWS Dedicated Host) or monthly (MacStadium portal). Not free. Charged regardless of idle time.

### 5.9 Persistent build-cache support
- **YES (full).** Persistent filesystem: Xcode 15.4, KDK 22G74, XNU source, `build/xnu.obj`, `fakeroot/`, `.cache/`, and GitHub Actions cache all persist across builds. Eliminates redundant clones/KDK installs across baseline/begonia jobs (CI_AUDIT.md F-004). Incremental `make` timestamp deps work correctly.

### 5.10 Option B evaluation

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Xcode 15.4 availability | YES | Manual install on macOS 14.x host |
| KDK 22G74 installation | YES | Manual install per BUILD.md |
| Disk space | GOOD | TB-scale storage options |
| CPU/RAM | GOOD–EXCELLENT | Up to 20 CPU / 128 GB RAM |
| Build duration | GOOD | No fixed timeout; fast hardware |
| Runner availability | EXCELLENT | Dedicated, no brownouts/retirement |
| Security | EXCELLENT | Dedicated hardware, root, private net |
| Cost | PAID | Hourly/monthly |
| Persistent build-cache | YES | Full local persistence |

**Verdict:** Strong long-term/iterative-development option. Cost is the trade-off. Requires Apple Developer account for one-time Xcode/KDK downloads.

---

## 6. Option C — Persistent self-hosted macOS Sonoma 14 runner

A self-owned Mac (Mac mini/Mac Studio) registered as a GitHub self-hosted runner at repository level, running macOS 14.x.

### 6.1 Requirements for self-hosted setup

| Component | Requirement |
|-----------|-------------|
| Hardware | Mac capable of macOS 14.x (Sonoma) — Mac mini / Mac Studio |
| OS | macOS Sonoma 14.x (installer from Apple; supports downgrade from newer if hardware permits) |
| Xcode | 15.4 at `/Applications/Xcode_15.4.app`; `sudo xcode-select -s /Applications/Xcode_15.4.app/Contents/Developer`; `DEVELOPER_DIR` set |
| KDK | 13.5 build 22G74 at `/Library/Developer/KDKs/KDK_13.5_22G74.kdk` (manual install per BUILD.md) |
| Homebrew deps | jq, gum, bash, cmake, ninja (+ `xcodes` shim as in xnu.yml:142-144) |
| Runner app | GitHub Actions self-hosted runner registered at BEGONIA-XNU repo level |
| Disk/CPU/RAM | Sufficient for Xcode + KDK + XNU build (suggest ≥ 100 GB free SSD, 4+ CPU, 16+ GB RAM headroom) |
| Network | Outbound access to github.com and actions cache/upload endpoints |

### 6.2 Build-cache persistence
- **YES (full).** Persistent local filesystem retains Xcode, KDK, XNU source, `build/xnu.obj`, `build/xnu.sym`, `fakeroot/`, `.cache/` between runs. `actions/cache` restores/save to local disk. Baseline and begonia jobs can run sequentially on the same machine without re-cloning XNU or re-installing KDK (resolves CI_AUDIT.md F-004 redundancy). Incremental `make` rebuilds use timestamp dependencies.

### 6.3 Security
- Self-hosted runners on public repositories carry a risk: untrusted workflow code runs on your machine. GitHub recommends self-hosted runners for private/internal repositories or with hardened runner isolation for public repos. No credentials are stored in the BEGONIA-XNU workflow (CI_AUDIT.md F-006), reducing exposure.

### 6.4 Cost
- Hardware purchase + electricity/internet. No per-minute GitHub charges for self-hosted runners on public repos.

### 6.5 Runner availability
- 100% controlled by owner. No GitHub brownouts, no retirement, no queue delays.

### 6.6 Option C evaluation

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Xcode 15.4 availability | YES | Install once, persists |
| KDK 22G74 installation | YES | Install once, persists |
| Disk space | GOOD | Owner-controlled, scalable |
| CPU/RAM | GOOD | Owner-selected hardware |
| Build duration | GOOD | No fixed timeout |
| Runner availability | EXCELLENT | Owned hardware, no external schedules |
| Security | GOOD (with caveats) | Self-hosted public-repo risk; no secrets in workflow |
| Cost | HARDWARE + UTILITIES | Upfront hardware; no per-minute CI fees |
| Persistent build-cache | YES | Full local persistence |

**Verdict:** Recommended post-Nov 2, 2026 replacement for Option A and best fit for iterative development. Requires hardware procurement.

---

## 7. Comparative evaluation matrix

| Criterion | A: GitHub macos-14 | B: Cloud Mac (MacStadium/AWS) | C: Self-hosted Sonoma 14 |
|-----------|-------------------|-------------------------------|--------------------------|
| Xcode 15.4 availability | YES (default) | YES (manual install) | YES (install once) |
| KDK 22G74 installation | YES (auto-dortania) | YES (manual) | YES (install once) |
| Disk space | RISK (14 GB SSD) | GOOD (TB) | GOOD (owner-controlled) |
| CPU/RAM | OK–GOOD (3–12 jobs) | GOOD–EXCELLENT | GOOD (owner-selected) |
| Build duration | OK (120-min cap) | GOOD | GOOD |
| Runner availability | LIMITED (brownout + Nov 2 retirement) | EXCELLENT (dedicated) | EXCELLENT (owned) |
| Security | GOOD (ephemeral, no secrets) | EXCELLENT (dedicated) | GOOD (self-hosted caveat) |
| Cost | FREE | PAID | Hardware + utilities |
| Persistent build-cache | PARTIAL (cache service only) | YES | YES |
| Recommended use | Short-term, pre-Nov 2, outside brownouts | Dedicated/secure long-term | Iterative development, post-retirement |

---

## 8. Recommended strategy

1. **Immediate (now – Nov 1, 2026):** Execute the XNU baseline build on GitHub-hosted `macos-14` ONLY outside brownout windows. Current window #8 (Oct 30 14:00 – Oct 31 00:00 UTC) is active — do not schedule macos-14 jobs now. Next safe window: after Oct 31 00:00 UTC, before Nov 2, 2026 retirement. Monitor disk (`df -h`) given the 14 GB SSD constraint.
2. **Pre-retirement (before Nov 2, 2026):** Stand up Option C (persistent self-hosted macOS Sonoma 14 runner with Xcode 15.4 + KDK 22G74) OR Option B (dedicated cloud Mac) so the pipeline is not blocked when `macos-14` is removed.
3. **Do NOT** retarget `xnu.yml` to `macos-15`/`macos-latest`/`macos-26` without independently verifying Xcode 15.4 + KDK 22G74 + XNU 8796.141.3 compatibility — macos-15 does not ship Xcode 15.x (oldest is 16.0), and substituting toolchains violates the pinned-combination rule.
4. Update `.github/workflows/xnu.yml` `runs-on` to the chosen persistent runner once provisioned; keep `macos-14` as a documented short-term fallback.

---

## 9. Sign-off status

| Item | Status | Evidence |
|------|--------|----------|
| Requirement matrix defined (Xcode 15.4, Sonoma 14.x, KDK 22G74, XNU 8796.141.3) | COMPLETE | Section 1 |
| macOS 14 retirement verified (Nov 2, 2026) | VERIFIED | GitHub Changelog 2026-10-01; actions/runner-images#13518 |
| Brownout schedule verified (incl. Oct 30-31 active window) | VERIFIED | GitHub Changelog brownout list |
| macos-14 + Xcode 15.4 availability verified | VERIFIED | runner-images macos-14-Readme.md (15.4 default, 15F31d) |
| macos-15 does NOT ship Xcode 15.x verified | VERIFIED | runner-images macos-15-Readme.md (oldest 16.0) |
| KDK 13.5 build 22G74 in dortania manifest verified | VERIFIED | manifest.json entry, build 22G74, direct DMG URL |
| GitHub-hosted runner resources (CPU/RAM/disk) verified | VERIFIED | GitHub-hosted runners reference; Larger runners reference |
| Cloud Mac providers identified (MacStadium, AWS EC2 Mac) | VERIFIED | macstadium.com; aws.amazon.com/ec2/mac |
| Option A evaluation (GitHub macos-14) | COMPLETE | Section 4 |
| Option B evaluation (cloud Mac hosting) | COMPLETE | Section 5 |
| Option C evaluation (self-hosted Sonoma 14) | COMPLETE | Section 6 |
| Comparative matrix + recommended strategy | COMPLETE | Sections 7–8 |

**Overall sign-off:** INFRASTRUCTURE PLAN DELIVERED.

**Recommended path:** Use GitHub-hosted `macos-14` for the immediate baseline/begonia XNU builds (outside the active Oct 30–31 brownout, before Nov 2, 2026 retirement), then migrate to Option C (persistent self-hosted macOS Sonoma 14 runner with Xcode 15.4 + KDK 22G74) as the supported long-term build host. Option B is the dedicated-hardware fallback.

**Blocker note:** No macOS runner is currently executable because brownout window #8 is active (Oct 30 14:00 – Oct 31 00:00 UTC) and `macos-14` is retired Nov 2, 2026. A persistent Sonoma 14 host (Option C/B) is required for builds after Nov 1, 2026.

---

## 10. References

- GitHub Changelog: "GitHub Actions: macOS 14 runner image retirement" (2026-10-01) — https://github.blog/changelog/2026-10-01-github-actions-macos-14-runner-image-retirement/
- actions/runner-images#13518 — macOS 14 Sonoma deprecation (Jul 6 2026 – Nov 2 2026)
- runner-images `images/macos/macos-14-Readme.md` — Xcode 15.4 default, macOS 14.8.7, SDKs
- runner-images `images/macos/macos-15-Readme.md` — Xcode 16.0–16.4, no Xcode 15.x
- GitHub-hosted runners reference — CPU/RAM/disk per label
- Larger runners reference — macos-14-large/xlarge specs
- dortania KdkSupportPkg manifest — KDK 13.5 build 22G74 entry + DMG URL
- `BEGONIA-XNU/.github/workflows/xnu.yml` — pinned toolchain, runs-on macos-14
- `BEGONIA-XNU/BUILD.md` — reference pipeline, KDK manual install
- `BEGONIA-XNU/CI_TOOLCHAIN_VALIDATION.md` — toolchain matrix, compatibility verdict
- `BEGONIA-XNU/CI_AUDIT.md` — runner strategy F-007, redundancy F-004
- MacStadium pricing/bare metal — https://macstadium.com/pricing, https://macstadium.com/bare-metal-mac
- Amazon EC2 Mac instances — https://aws.amazon.com/ec2/instance-types/mac/, docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-mac-instances.html
