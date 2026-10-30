# CI_TROUBLESHOOTING.md — BEGONIA-XNU CI Troubleshooting Guide

Operation First Boot — CI/CD troubleshooting reference.

Repository: /home/yu/BEGONIA-XNU
Workflow: .github/workflows/xnu.yml

---

## Common Failures and Resolution

### T-001: XNU Commit Mismatch

**Symptom:**
```
::error::XNU commit mismatch! Expected 1b191cb58250d0705d8a51287127505aa4bc0789, got <actual>
```

**Cause:** The cloned `xnu-8796.141.3` tag resolved to a different commit than the pinned `1b191cb58250d0705d8a51287127505aa4bc0789`.

**Resolution:**
1. Verify the tag still exists: `git ls-remote https://github.com/apple-oss-distributions/xnu.git refs/tags/xnu-8796.141.3`
2. If the tag moved (force-pushed), update `XNU_COMMIT` in `.github/workflows/xnu.yml` env
3. Never silently substitute a different XNU release

---

### T-002: KDK Not Found / Automatic Download Failed

**Symptom:**
```
::error::Could not find KDK URL for 'Kernel Debug Kit 13.5 build 22G74' in the dortania KdkSupportPkg manifest.
```
or
```
::error::KDK installation failed — /Library/Developer/KDKs/KDK_13.5_22G74.kdk not found after installer ran.
```

**Cause:** The KDK is not pre-installed on the runner and automatic download from the dortania manifest failed or the installer did not place it at the expected path.

**Resolution (manual):**
1. Download Kernel Debug Kit 13.5 (build 22G74) DMG from Apple Developer Portal or dortania KdkSupportPkg mirror
2. Mount: `hdiutil attach /path/to/KernelDebugKit.dmg`
3. Install: `sudo installer -pkg '/Volumes/Kernel Debug Kit/KernelDebugKit.pkg' -target /`
4. Unmount: `hdiutil detach '/Volumes/Kernel Debug Kit'`
5. Verify: `ls -la /Library/Developer/KDKs/KDK_13.5_22G74.kdk`
6. Re-run the workflow

---

### T-003: Xcode 15.4 Not Found

**Symptom:**
```
xcode-select: error: '/Applications/Xcode_15.4.app/Contents/Developer' does not exist
```

**Cause:** The macOS runner does not have Xcode 15.4 installed at the expected path.

**Resolution:**
1. List available Xcode versions: `ls /Applications/Xcode*.app`
2. If Xcode 15.4 is not available, the workflow cannot proceed — do NOT substitute a different Xcode version
3. Use Runner Strategy B (self-hosted macOS with Xcode 15.4) or wait for GitHub to provide macos-14 with Xcode 15.4

---

### T-004: Build Tool Missing (mig, xcrun, kmutil, hdiutil)

**Symptom:**
```
xcrun: error: unable to find utility 'mig', not a developer tool or in PATH
```

**Cause:** Xcode Command Line Tools are not fully installed or `DEVELOPER_DIR` is not set.

**Resolution:**
1. Verify Xcode selection: `xcode-select -p` should return `/Applications/Xcode_15.4.app/Contents/Developer`
2. Install additional components: `sudo xcode-select --install`
3. Verify `DEVELOPER_DIR` is exported in the workflow step

---

### T-005: Cache Restore Fails / Empty Cache

**Symptom:**
Cache miss on first run; subsequent runs restore but objects recompile due to mtime skew.

**Cause:**
- First run: no cache exists (expected)
- Subsequent runs: `git checkout` sets source mtimes to "now", which are newer than cached object mtimes, causing make to recompile everything

**Resolution:**
- This is a performance issue, not correctness. Kernel SHA-256 should still match clean builds.
- For consistent incremental behavior, use Runner Strategy B (persistent self-hosted runner) where mtimes are preserved across runs.

---

### T-006: Build Fails with Compiler Errors

**Symptom:**
`make install` exits with non-zero status due to C/C++ compilation errors.

**Resolution:**
1. Check uploaded build-logs artifact for compiler diagnostics
2. Verify `BUILD_LTO=0` is set (hardcoded in workflow env)
3. Verify `BUILD_WERROR=0` (warnings-as-errors disabled)
4. Check that `TARGET_CONFIGS` matches expected format: `"${KERNEL_CONFIG} ${ARCH_CONFIG} ${MACHINE_CONFIG}"`
5. Do NOT suppress errors to make the build green

---

### T-007: Kernel Binary Not Found After Build

**Symptom:**
```
::warning::Kernel binary not found at build/xnu.obj/kernel.development.vmapple
```

**Cause:** The build did not produce the expected kernel output at the expected path.

**Resolution:**
1. Check the full build output directory: `find build/ -name "kernel*" -type f`
2. Verify `KERNEL_CONFIG` and `MACHINE_CONFIG` values match the expected output filename
3. Check dxb build.sh `build_xnu()` function for the actual output path
4. Compare against XNU_BUILD_ANALYSIS.md for documented output paths

---

### T-008: macOS 14 Runner Unavailable / Retired

**Symptom:**
Workflow fails to schedule on `macos-14` or GitHub announces runner retirement.

**Resolution:**
1. Check GitHub Actions runner availability: https://github.com/actions/virtual-environments
2. If macos-14 is retired, evaluate Runner Strategy C (newer macOS runner) with independent toolchain verification
3. Do NOT silently substitute Xcode 16 or a different XNU toolchain
4. Fail with actionable instructions rather than selecting an incompatible environment

---

### T-009: Patch Application Fails (Begonia Job)

**Symptom:**
`git apply` fails for patches 0017, 0018, or 0065.

**Cause:** These patches were developed against intermediate ios-sweet commits, not official XNU tags. They have context mismatches and ordering dependencies.

**Resolution:**
1. Apply patches in order: 0004 → 0006 → 0016 → 0018 → 0065 → 0017
2. Patch 0017 requires Begonia-specific rewrite (virtual timer IRQ)
3. Patches 0018 and 0065 have order dependencies (0018 needs 0016's KernelConfigTables.cpp line; 0065 needs 0018's SweetCPU.cpp)
4. Do NOT blindly copy Qualcomm-specific code — adapt for MT6785
5. See PATCH_COMPATIBILITY.md for detailed analysis

---

### T-010: Artifact Upload Fails

**Symptom:**
`actions/upload-artifact@v4` fails or paths don't match.

**Resolution:**
1. Verify paths exist before upload: `ls -la BEGONIA-XNU/tools/dxb/build/`
2. Check artifact name uniqueness within the workflow run
3. Verify retention-days is set (7 days)
4. Check GitHub Actions artifact storage limits for the repository

---

## Workflow Debug Commands

To debug a failed run on macOS:

```sh
# Verify Xcode selection
xcode-select -p
xcodebuild -version

# Verify tool availability
xcrun --find mig
xcrun --find clang
xcrun --find ld
xcrun --find kmutil
which hdiutil

# Verify KDK
ls -la /Library/Developer/KDKs/KDK_13.5_22G74.kdk

# Verify XNU source
cd BEGONIA-XNU/tools/dxb/xnu
git rev-parse HEAD
git describe --tags --exact-match

# Check build output
ls -la build/xnu.obj/kernel.*.*
sha256sum build/xnu.obj/kernel.*.*
```

---

## Escalation

If none of the above resolutions apply:
1. Check CI_AUDIT.md for workflow architecture review
2. Check CI_TOOLCHAIN_VALIDATION.md for toolchain compatibility
3. Check XNU_BUILD_ANALYSIS.md for build parameter reference
4. Open an issue with the workflow run ID, full build logs, and error output
