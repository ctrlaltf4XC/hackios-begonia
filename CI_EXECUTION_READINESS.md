# CI Execution Readiness — BEGONIA-XNU XNU 8796 Build

**Workflow:** `.github/workflows/xnu.yml` (529 lines)
**Agent:** Agent 4 — CI Workflow Engineer
**Status:** READY for execution (via `workflow_dispatch`) — see gap notes below

---

## 1. Workflow Structure Summary

| Item | Detail |
|------|--------|
| Workflow name | XNU 8796 Build Performance Optimized (macOS CI) |
| Trigger | `workflow_dispatch` only (manual) |
| Jobs | `xnu-baseline`, `xnu-begonia` |
| Dependency | `xnu-begonia` `needs: xnu-baseline` |
| Runner | `macos-14` (both jobs) |
| Timeout | 120 min (both jobs) |
| Pinned engine | darwin-xnu-build @ `6639065` |
| Pinned XNU | tag `xnu-8796.141.3`, commit `1b191cb` |
| Xcode | `/Applications/Xcode_15.4.app/Contents/Developer` |
| KDK | Kernel Debug Kit 13.5 build 22G74 |

### Job: `xnu-baseline`
**Purpose:** XNU 8796 optimized build for `${{ inputs.machine_config }}` (default `VMAPPLE`).

Steps (in order):
1. Checkout BEGONIA-XNU repository (`actions/checkout@v4`, path `BEGONIA-XNU`, fetch-depth 1)
2. Select Xcode 15.4 (`sudo xcode-select -s`, sets `DEVELOPER_DIR`)
3. Verify required build tools (xcodebuild, xcrun mig/clang/ld/kmutil, hdiutil)
4. Inspect Kernel Debug Kits (`/Library/Developer/KDKs`)
5. Detect CPU and RAM limits for parallelism (id `detect-parallelism`, outputs `build_jobs`)
6. Fetch pinned darwin-xnu-build engine (git fetch/checkout `DXB_SHA`)
7. Clone and verify exact XNU source commit (fails on commit mismatch)
8. Install Homebrew dependencies (jq, gum, bash, cmake, ninja; shim `xcodes`)
9. Install KDK (id `kdk`, outputs `kdk_available`, `kdk_skipped`; exits 1 on failure)
10. Restore cached build outputs (`actions/cache/restore@v4`, pinned key)
11. Record cache restore duration
12. Force clean (if `force_clean == 'true'`)
13. Build XNU kernel (`make install`, id `xnu-build`, outputs `build_duration_sec`)
14. Compute artifact checksums (id `checksums`, outputs `kernel_checksum`)
15. Save cached build outputs (`actions/cache/save@v4`, pinned key)
16. Record cache save duration
17. Upload XNU build artifacts (`actions/upload-artifact@v4`, `if: always()`)

Outputs: `build_jobs`, `clean_duration_sec`, `cache_restore_duration_sec`, `cache_save_duration_sec`

### Job: `xnu-begonia`
**Purpose:** XNU 8796 with Begonia platform integration (`MACHINE_CONFIG: BEGONIA`).

Dependency: `needs: xnu-baseline`, runs only `if: needs.xnu-baseline.result == 'success'`.

Steps: same as baseline plus:
- Integrate Begonia platform into XNU source tree (copies `BegoniaPlatformExpert.cpp`, `BegoniaCPU.cpp`, `begonia.h`; appends to `files.arm64` and `board_config.h`)

Outputs: `build_jobs`, `build_duration_sec`, `kernel_checksum`

---

## 2. workflow_dispatch Input Table

| Name | Description | Required | Default | Type (implicit) | Used? |
|------|-------------|----------|---------|-----------------|-------|
| `kernel_config` | XNU kernel config (RELEASE / DEVELOPMENT / DEBUG) | true | `DEVELOPMENT` | string | yes (KERNEL_CONFIG env, cache key, TARGET_CONFIGS) |
| `machine_config` | XNU MACHINE_CONFIG (VMAPPLE / BEGONIA) | true | `VMAPPLE` | string | yes (baseline TARGET_CONFIGS; overridden to BEGONIA in begonia job env) |
| `xnu_build_jobs` | Parallel compilation jobs (auto if empty) | false | `''` | string | yes (XNU_BUILD_JOBS env, passed to make -j) |
| `build_kc` | Build Kernel Collection (kmutil create) | true | `false` | string | **no** — defined but never referenced in workflow |
| `keep_going` | Run make -k to collect all compile errors | true | `true` | string | yes (MAKEFLAGS `-k` vs `''`) |
| `force_clean` | Force clean build (discard cache) | true | `false` | string | yes (cache restore/save condition, force clean step) |

**Finding:** `build_kc` is a required input with default `false` but is never consumed anywhere in the workflow. It does not break execution, but it is dead configuration. Recommendation: either remove it or wire it to a `kmutil create` step.

---

## 3. Branch Selection — `ci/first-boot-validation`

**Gap found.** The workflow declares only `workflow_dispatch`. There is no `push` or `pull_request` trigger, and no `branches:` filter for `ci/first-boot-validation`.

The workflow is executable manually via the Actions tab / `gh workflow run`, but it will **not** automatically run on pushes or PRs to branch `ci/first-boot-validation`.

Recommendation: add a push/pull_request trigger targeting `ci/first-boot-validation` (with `workflow_dispatch` retained for manual runs) if automatic CI on that branch is required.

---

## 4. Job Dependency Chain Verification

| Dependency | Verification |
|------------|--------------|
| `xnu-baseline` → no `needs` | ✓ Entry job, runs first |
| `xnu-begonia` `needs: xnu-baseline` | ✓ Declared at line 298 |
| `xnu-begonia` condition `needs.xnu-baseline.result == 'success'` | ✓ Correct — skipped (not failed) when baseline fails; failure does not silently propagate as success |

Chain: `xnu-baseline` (success) → `xnu-begonia` (runs). Correct two-job serial architecture.

---

## 5. Runner and Toolchain Verification

| Item | Value | Status |
|------|-------|--------|
| Runner label | `macos-14` (both jobs) | ✓ GitHub-hosted macOS 14 runner |
| Xcode selection | `sudo xcode-select -s /Applications/Xcode_15.4.app/Contents/Developer` | ✓ Line 78 |
| `DEVELOPER_DIR` env | Set to `XCODE_PATH` via `$GITHUB_ENV` | ✓ Line 79 |
| `XCODE_PATH` env | `/Applications/Xcode_15.4.app/Contents/Developer` | ✓ Line 39 |
| Build tools verified | xcodebuild, xcrun mig/clang/ld/kmutil, hdiutil | ✓ Lines 84-90 |
| PATH | Xcode toolchain + system dirs | ✓ Lines 215, 464 |

---

## 6. KDK Installation Verification

| Item | Value | Status |
|------|-------|--------|
| `KDK_NAME` | `Kernel Debug Kit 13.5 build 22G74` | ✓ Line 43 |
| `KDKROOT` | `/Library/Developer/KDKs/KDK_13.5_22G74.kdk` | ✓ Line 44 |
| Skip-if-installed | `[ -d "${KDKROOT}" ]` → exit 0 | ✓ Lines 149-154 |
| Download source | dortania KdkSupportPkg `manifest.json` filtered by name | ✓ Lines 156-158 |
| Installer | `hdiutil attach` + `sudo installer -pkg ...KernelDebugKit.pkg -target /` | ✓ Lines 166-168 |
| Failure handling | `::error::` + `exit 1` if URL missing or KDKROOT absent after install | ✓ Lines 159-163, 174-178 |
| Output `kdk_available` | Set `true`/`false`; gates downstream steps | ✓ Lines 151-177 |

All KDK steps are idempotent and fail loudly. Both jobs replicate the KDK step identically.

---

## 7. Cache Configuration Analysis

| Item | Detail | Status |
|------|--------|--------|
| Restore action | `actions/cache/restore@v4` | ✓ |
| Save action | `actions/cache/save@v4` | ✓ |
| Cache key | `xnu-build-${{ runner.os }}-xcode15.4-kdk-22g74-${{ env.XNU_COMMIT }}-${{ env.KERNEL_CONFIG }}-${{ env.MACHINE_CONFIG }}` | ✓ Pinned (OS, Xcode, KDK, XNU commit, config, machine) |
| Restore-keys | Same as key (exact-match fallback) | ✓ |
| Paths | `build`, `fakeroot`, `.cache` under `tools/dxb` | ✓ |
| Condition | `kdk_available == 'true' && force_clean == 'false'` | ✓ Cache skipped on clean builds / KDK failure |

Both jobs use identical cache keys scoped by their `MACHINE_CONFIG` (baseline uses input default `VMAPPLE`; begonia overrides to `BEGONIA`), so caches do not collide between jobs.

**Minor output-reference issue:** `xnu-baseline` declares outputs `clean_duration_sec` (refs `steps.clean-build.outputs.duration_sec`), `cache_restore_duration_sec`, and `cache_save_duration_sec` (refs `steps.cache-restore-timing`/`cache-save-timing`). None of those steps set a `duration_sec` output, so these three outputs will be empty strings. Does not block execution; cosmetic only. Recommendation: remove the dangling output references or add duration outputs to the timing steps.

---

## 8. Build Commands Verification

Both jobs run:

```
make install -j"${XNU_BUILD_JOBS}" VERBOSE=YES \
  SDKROOT=macosx \
  TARGET_CONFIGS="${KERNEL_CONFIG} ${ARCH_CONFIG} ${MACHINE_CONFIG}" \
  CONCISE=0 LOGCOLORS=y BUILD_WERROR=${BUILD_WERROR} BUILD_LTO=${BUILD_LTO} \
  SRCROOT="${SRCROOT}" OBJROOT="${OBJROOT}" SYMROOT="${SYMROOT}" DSTROOT="${DSTROOT}" \
  FAKEROOT_DIR="${FAKEROOT_DIR}" KDKROOT="${KDKROOT}" \
  RC_DARWIN_KERNEL_VERSION=22.6.0 \
  MEMORY_SIZE= PHYS_CPU_OVERRIDE= LOGICAL_CPU_OVERRIDE= \
  KERNEL_BUILDS_IN_PARALLEL=1
```

| Parameter | Value | Status |
|-----------|-------|--------|
| Target | `make install` | ✓ |
| Parallelism | `-j"${XNU_BUILD_JOBS}"` (auto-detected or user override) | ✓ |
| SDKROOT | `macosx` | ✓ |
| TARGET_CONFIGS | `${KERNEL_CONFIG} ARM64 ${MACHINE_CONFIG}` | ✓ |
| `RC_DARWIN_KERNEL_VERSION` | `22.6.0` (matches macOS 13.5 / XNU 8796.141.3) | ✓ |
| `BUILD_WERROR` / `BUILD_LTO` | `0` / `0` | ✓ |
| `KEEP_GOING` | `-k` when `keep_going != 'false'` (default `true`) | ✓ |

`make` is invoked directly (bypassing dxb's `build_xnu()` guard) so timestamp-based incremental rebuilds work correctly. Verified correct.

---

## 9. Artifact Upload Verification

| Job | Action | Name | Paths | Retention | Condition |
|-----|--------|------|-------|-----------|-----------|
| `xnu-baseline` | `actions/upload-artifact@v4` | `xnu-build-${{ inputs.machine_config }}` | `build/`, `fakeroot/`, `xnu/xnu-*.sha256` | 7 days | `always()` |
| `xnu-begonia` | `actions/upload-artifact@v4` | `xnu-build-begonia` | `build/`, `fakeroot/` | 7 days | `always()` |

✓ Correct action version (`v4`), unique names per job, `retention-days: 7`, `if: always()` ensures artifacts upload even when the build fails (supports failure diagnosis).

Minor: baseline path line `BEGONIA-XNU/tools/dxb/xnu/xnu-*.sha256 2>/dev/null || true` includes shell redirection text inside an artifact path pattern — harmless but non-idiomatic. `upload-artifact` treats it as a literal glob; no functional break.

---

## 10. Failure Propagation Verification

No error suppression found.

| Mechanism | Evidence |
|-----------|----------|
| KDK URL missing | `::error::` + `exit 1` (line 160-163) |
| KDK install failed | `::error::` + `exit 1` (line 175-178) |
| XNU commit mismatch | `::error::` + `exit 1` (lines 134-137, 348-351) |
| Missing kernel binary | `::warning::` (non-fatal; checksum step degrades gracefully) |
| Job dependency | `xnu-begonia` skipped when baseline not `success` |
| Artifact upload | `if: always()` — runs on failure, does not mask exit code |

No `|| true`, `|| exit 0`, or `set +e` on critical build steps. Failures propagate correctly.

---

## 11. Two-Job Architecture Verification

✓ Correct.

- `xnu-baseline`: builds reference XNU (`VMAPPLE` by default), produces baseline artifacts/cache.
- `xnu-begonia`: `needs: xnu-baseline`, only runs on baseline success, applies Begonia platform patches, rebuilds with `MACHINE_CONFIG: BEGONIA`.

Serial dependency ensures Begonia integration is validated against a known-good baseline. No diamond dependencies, no circular needs, no redundant job duplication beyond the intentional baseline→begonia progression.

---

## Issues Found and Recommendations

| # | Severity | Issue | Recommendation |
|---|----------|-------|----------------|
| 1 | Gap (not blocking manual run) | No `push`/`pull_request` trigger for `ci/first-boot-validation`; workflow is `workflow_dispatch`-only. | Add `push`/`pull_request: branches: [ci/first-boot-validation]` if automatic CI on that branch is required. |
| 2 | Minor | `build_kc` input declared (required, default `false`) but never used. | Remove the input or wire it to a `kmutil create` step. |
| 3 | Minor | `xnu-baseline` outputs `clean_duration_sec`, `cache_restore_duration_sec`, `cache_save_duration_sec` reference steps that set no `duration_sec` output. | Remove dangling output refs or add duration outputs to `clean-build`/timing steps. |
| 4 | Minor | Baseline artifact path includes `2>/dev/null || true` shell text inside a glob pattern. | Remove shell redirection from artifact `path`. |

No issues block manual execution via `workflow_dispatch`.

---

## Sign-Off Status

**READY for execution** (via `workflow_dispatch` on `macos-14`).

The workflow is syntactically valid, all toolchain/KDK/cache/artifact steps are correctly configured, failure propagation is intact, and the two-job dependency chain is correct. The only non-blocking gap is the absence of an automatic branch trigger for `ci/first-boot-validation`.

Recommendation before first production run: manually trigger via the GitHub Actions tab and confirm `xnu-baseline` completes; then `xnu-begonia` runs automatically on success.
