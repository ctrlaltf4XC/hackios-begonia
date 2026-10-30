# INCREMENTAL_BUILD_ANALYSIS.md — DXB Existing-Kernel Guard Workaround

**Repository:** BEGONIA-XNU  
**XNU source:** `/home/yu/BEGONIA-XNU/xnu/xnu-8796` @ `1b191cb58250d0705d8a51287127505aa4bc0789`  
**Build engine:** `/home/yu/BEGONIA-XNU/tools/darwin-xnu-build` @ `663906528d1cd5ea8fde7de6f16de73db9511549`  
**Agent:** Agent 4 — INCREMENTAL BUILD ENGINEER  
**Status:** Workaround implemented in `.github/workflows/xnu.yml`; not testable on Linux (no macOS/Xcode/KDK)

---

## 1. DXB Guard Analysis

### 1.1 Exact code

File: `tools/darwin-xnu-build/build.sh`, function `build_xnu()` (lines 697-738):

```bash
build_xnu() {
    if [ ! -f "${BUILD_DIR}/xnu.obj/kernel.${KERNEL_TYPE}" ]; then
        # ... compilation path (JSON DB or make install) ...
    else
        info "📦 XNU kernel.${KERNEL_TYPE} already built"
    fi
}
```

The guard at **line 698** is a single file-existence test:

```bash
if [ ! -f "${BUILD_DIR}/xnu.obj/kernel.${KERNEL_TYPE}" ]; then
```

`KERNEL_TYPE` is derived from `KERNEL_CONFIG` and `MACHINE_CONFIG` (e.g. `kernel.DEVELOPMENT.vmapple` or `kernel.DEVELOPMENT.begonia`).

### 1.2 Why it is wrong for incremental builds

The guard is a **binary existence check** with no dependency awareness:

| Check performed? | Guard result |
|---|---|
| Source `.c`/`.cpp`/`.s` mtime vs object mtime | **No** |
| Header mtime vs object mtime | **No** |
| `.d` dependency file staleness | **No** |
| Config / machine / arch change | **No** |
| `KDKROOT` / SDK version change | **No** |
| `RC_DARWIN_KERNEL_VERSION` change | **No** |

If `kernel.DEVELOPMENT.begonia` exists, `build_xnu()` short-circuits and prints "already built". Make is never invoked, so make's native timestamp-based dependency tracking never runs. Any source change — one line in one `.c` file, one header edit, one config flag — is silently ignored. This is **not incremental**; it is a "build once, never rebuild" guard.

The same pattern repeats in `build_xnu_library()` (line 750) and is re-checked in `build_kc()` (line 803).

---

## 2. Workaround Validation

### 2.1 Direct make call

File: `.github/workflows/xnu.yml`, step "Build XNU kernel (incremental-aware, no guard)" (lines 209-249, `xnu-baseline` job) and lines 456-489 (`xnu-begonia` job).

The workflow replaces `build_xnu()` with a direct `make install` invocation:

```bash
cd "${SRCROOT}"
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

With `MAKEFLAGS=-k` (when `keep_going=true`) set as an env var, and `DEVELOPER_DIR`/`PATH` pointing at Xcode 15.4.

### 2.2 Parameter comparison with dxb's build_xnu()

| Parameter | dxb `build_xnu()` (build.sh:732) | Workaround (xnu.yml:237-245) | Match? |
|---|---|---|---|
| Target | `make install` | `make install` | Yes |
| Parallelism | `-j8` (hardcoded) | `-j"${XNU_BUILD_JOBS}"` (auto-detected) | Improved |
| VERBOSE | `VERBOSE=YES` | `VERBOSE=YES` | Yes |
| SDKROOT | `SDKROOT=macosx` | `SDKROOT=macosx` | Yes |
| TARGET_CONFIGS | `"$KERNEL_CONFIG $ARCH_CONFIG $MACHINE_CONFIG"` | Same | Yes |
| CONCISE / LOGCOLORS | `CONCISE=0 LOGCOLORS=y` | Same | Yes |
| BUILD_WERROR / BUILD_LTO | `BUILD_WERROR=0 BUILD_LTO=0` | Same (from env) | Yes |
| SRCROOT / OBJROOT / SYMROOT / DSTROOT | Same variables | Same | Yes |
| FAKEROOT_DIR / KDKROOT | Same variables | Same | Yes |
| TIGHTBEAMC | `${TIGHTBEAMC}` | **omitted** | Minor gap |
| CTF tool overrides | `ctf_tool_overrides` (CTFCONVERT/CTFMERGE/CTFDUMP) | **not passed** | Minor gap |
| RC_DARWIN_KERNEL_VERSION | `${RC_DARWIN_KERNEL_VERSION}` | `22.6.0` (hardcoded, matches dxb choose_xnu for 13.5) | Yes |
| MEMORY_SIZE / PHYS_CPU / LOGICAL_CPU | From overrides | Empty (`MEMORY_SIZE= PHYS_CPU_OVERRIDE= LOGICAL_CPU_OVERRIDE=`) | dxb sets overrides via `ensure_hw_env_overrides`; workaround leaves empty → make falls back to `sysctl` |
| KERNEL_BUILDS_IN_PARALLEL | `${KERNEL_PARALLELISM_OVERRIDE:-1}` | `1` | Yes (dxb also forces 1 via `KERNEL_PARALLELISM_OVERRIDE=1` at line 664) |

**Verdict:** The workaround passes the same essential parameters as dxb's `build_xnu()`. The omitted `TIGHTBEAMC` and CTF tool overrides are minor gaps; CTF is disabled by default when tools are missing (MakeInc.def:870-877), and `TIGHTBEAMC` is only needed for tightbeam tracing. The workaround correctly bypasses the guard and lets make drive the build.

---

## 3. Incremental Rebuild Correctness Analysis

### 3.1 Make's native timestamp-based dependency tracking

When `make install` is called directly, make's core algorithm governs rebuild decisions:

1. **Target exists?** If the kernel binary (`$(OBJROOT)/$(KERNEL_FILE_NAME)` / `$(DSTROOT)/...`) exists and is newer than all prerequisites, make considers it up to date.
2. **Prerequisite newer than target?** If any prerequisite (source, header, object, `.d` file) has an mtime newer than the target, the target is out of date and its recipe runs.
3. **Recursive application:** This applies at every level of the recursive make hierarchy — top-level `Makefile` → `MakeInc.top` → per-config sub-make → `MakeInc.kernel` → component `Makefile` → object rules.

### 3.2 Object reuse mechanism

Object compilation rules in `makedefs/MakeInc.rule` (lines 395-427):

```make
C_RULE_1A=$(_v)${KCC} -c ${filter-out ${$@_CFLAGS_RM}, ${CFLAGS} ${CWARNFLAGS}} \
    -MD -MF $(@:o=d) -MP ${$@_CFLAGS_ADD} ${$@_CWARNFLAGS_ADD} ${INCFLAGS} ${$@_INCFLAGS}
```

Key flags:
- `-MD` — generate dependency file alongside `.o`
- `-MF $(@:o=d)` — write deps to `.o`→`.d` path
- `-MP` — add phony targets for each header (prevents "No rule to make target" if a header is deleted)

Make compares `source.c` mtime against `object.o` mtime:
- `object.o` newer than `source.c` and all headers → **reuse object, skip compile**
- `source.c` newer than `object.o` → **recompile that source file**
- Header newer than `object.o` → **recompile all sources that include that header** (via `.d` deps)

This is correct, standard make behavior. No timestamp manipulation is required; it works natively.

### 3.3 Header dependency tracking (XNU .d dependency files)

The `-MD -MF -MP` flags generate `.d` files (one per `.o`) that list every header included by the source. Example `.d` content:

```
osfmk/arm64/sleh.o: osfmk/arm64/sleh.c \
  osfmk/arm64/pmap.h \
  osfmk/osfmk/kernel.h \
  ...
```

Make auto-includes these `.d` files (via `-include` patterns in `MakeInc.kernel` for generated files like `version.d`, `genassym.d`, and via component-level inclusion), so header changes propagate correctly:
- Edit `osfmk/arm64/sleh.h` → `.d` lists `sleh.h` as prereq of `sleh.o` → `sleh.o` out of date → recompile `sleh.c` → relink kernel.

This is **correct incremental header tracking**. No source timestamps are faked.

### 3.4 Source modification detection

Make's native `prerequisite-newer-than-target` comparison handles source modification detection:
- Single `.c` file edited → only that file's `.o` is recompiled → kernel relinked
- No edits → make reports "nothing to be done" (or installs already-current kernel)
- This is exactly the behavior described in `BUILD.md` (objective #5) and `BENCHMARK.md` scenarios B2/B3/B4.

**Correctness verdict:** The direct `make install` call provides correct incremental rebuild behavior. The guard removal is the right fix.

---

## 4. Generated-Header Invalidation

### 4.1 installhdrs step

`make install` at the top level expands to (Makefile top, `MakeInc.top:628`):

```
install: installhdrs install_textfiles install_config install_kernels install_aliases
```

`installhdrs` → `installhdrs_mi` + `installhdrs_md` (MakeInc.top:454). These recurse into all components (`INSTINC_SUBDIRS` = security, bsd, iokit, osfmk, pexpert, libkern, libsa, config, san + EXTERNAL_HEADERS) and run `do_installhdrs_mi` / `do_installhdrs_md` (MakeInc.rule:320-343), which copy headers to `DSTROOT` and `OBJROOT/EXPORT_HDRS/` (MakeInc.rule:348-384 export rules).

Header install rules use `ALWAYS` prerequisites and `.UNIFDEF_FLAGS` sentinel files (MakeInc.rule:63-89), so header processing is driven by flag changes, not mtime alone.

### 4.2 Config headers

Config-generated headers (e.g. `meta_features.h` in `osfmk/conf/Makefile.template:19`) are produced by the `config` phase (`config_all`, MakeInc.top:533-541). These headers are dependencies in `.d` files, so config header changes trigger recompilation of affected objects.

### 4.3 Kernel timestamp invalidation

`MakeInc.top:392-401` (`setup` target) touches `.mach_kernel.timestamp` and `.symbolset.timestamp`. `MakeInc.kernel:317-329` uses these timestamps to decide whether to (re)install the kernel into `DSTROOT`:

```make
$(DSTROOT)/$(INSTALL_KERNEL_DIR)/$(KERNEL_FILE_NAME): $(TARGET)/$(KERNEL_FILE_NAME) ALWAYS
    # if .mach_kernel.timestamp is newer than installed kernel, reinstall
    if [ $(OBJROOT)/.mach_kernel.timestamp -nt $@ -o ... ]; then
        $(INSTALL) $< $@
    else
        $(LIPO) -create $@ $< -output $@
    fi
```

This ensures install-time correctness even when object timestamps are preserved.

---

## 5. Cache Restore / Incremental Interaction Analysis

### 5.1 Cache key and scope

File: `.github/workflows/xnu.yml` lines 181-197 (`cache-restore`) and 266-277 (`cache-save`).

**Cache key:**
```
xnu-build-${runner.os}-xcode15.4-kdk-22g74-${XNU_COMMIT}-${KERNEL_CONFIG}-${MACHINE_CONFIG}
```

**Cached paths:** `build/`, `fakeroot/`, `.cache/` (under `BEGONIA-XNU/tools/dxb/`).

**Restore condition:** `steps.kdk.outputs.kdk_available == 'true' && inputs.force_clean == 'false'`.

**Save condition:** same, plus not force_clean.

### 5.2 Timestamp preservation on cache restore

`actions/cache/restore@v4` downloads a tar archive and extracts it. GNU tar preserves mtime by default. Restored object files (`.o`), dependency files (`.d`), and the kernel binary retain their **original build mtimes**.

Source files come from `actions/checkout@v4` (`git clone` + `git checkout`), which sets file mtime to **checkout time** (current time), not commit time.

### 5.3 The mtime mismatch problem (critical finding)

After cache restore on a fresh GitHub-hosted runner:

| File type | mtime | Source |
|---|---|---|
| Source `.c`/`.h` | now (checkout time) | `actions/checkout` |
| Cached `.o` / `.d` | original build time | tar extraction preserves mtime |
| Cached kernel binary | original build time | tar extraction preserves mtime |

Make compares source mtime vs object mtime: **source (now) > object (old)** → make considers every object out of date → **full recompilation of all source files**, even though source content is byte-identical to the cached build.

This means the workaround (`make install` directly) is correct for incremental builds **on a persistent checkout**, but **does not achieve true incremental behavior on the first build after cache restore** on a fresh runner. The cache still saves wall-clock time (avoids re-downloading/re-extracting is faster than full compile? No — actually if make recompiles everything, the cache only saves the time of extracting the tarball vs. not having objects at all; but compilation still happens).

Wait — more precisely: if make recompiles everything because source mtime > object mtime, the cache provides **zero compilation time savings** on a fresh runner. The cache only avoids re-fetching dependencies (fakeroot headers, KDK, etc.) if those are also cached.

This is a known limitation of `actions/cache` + make timestamp tracking on fresh checkouts. It does not invalidate the workaround; it is a separate cache-restore interaction issue.

**Does this cause incorrect builds?** No. Recompiling identical source produces identical objects (assuming deterministic compiler). The kernel SHA-256 should match the clean build. It is a **performance** issue, not a **correctness** issue.

### 5.4 Incremental on persistent checkout (non-CI)

On a developer's machine where the checkout persists between builds:
- Source files keep their mtime from the last edit
- Object files keep their mtime from the last build
- If nothing changed: object mtime > source mtime → make skips recompile → near-incremental
- If one file changed: only that file recompiles → correct incremental

This is the intended behavior and works correctly with the workaround.

---

## 6. Cache Corruption Detection and Handling

### 6.1 Cache key mismatch = clean build fallback

The cache key includes `XNU_COMMIT`, `KERNEL_CONFIG`, `MACHINE_CONFIG`, `runner.os`, `xcode15.4`, `kdk-22g74`. If any component changes:

| Change | Effect |
|---|---|
| New XNU commit | Key changes → no restore → clean build |
| Different kernel config | Key changes → no restore → clean build |
| Different machine config | Key changes → no restore → clean build |
| Xcode/KDK version change | Key changes → no restore → clean build |

`actions/cache/restore` with a non-matching key silently does nothing (no error, no restore). The build then proceeds from scratch. This is **safe**: no stale objects from a different commit/config can pollute the build.

### 6.2 force_clean input

`inputs.force_clean == 'true'` triggers `rm -rf "${BUILD_DIR}" "${FAKEROOT_DIR}"` (xnu.yml:204-207, 409-411), discarding all cached outputs. The cache-save step is also skipped when `force_clean=true`.

### 6.3 Corrupted cache archive

If a cache archive is corrupted (download fails, extraction fails), `actions/cache/restore` reports a warning but does not fail the job by default. The build proceeds from scratch. This is a **clean-build fallback** — safe but slower.

### 6.4 Stale objects from partial cache

If a previous build was interrupted (e.g. timeout at 120 min), some objects may be missing or partially written. Make's dependency tracking handles this: missing objects are rebuilt; partially written objects have old mtimes and may be relinked. The `.d` files ensure header consistency. No corruption propagation to the kernel binary because linking only happens after all objects are present.

---

## 7. Point-by-Point Responsibility Assessment

| # | Responsibility | Finding |
|---|---|---|
| 1 | Correct make targets (make install with all parameters) | **Validated.** `make install` with dxb's parameters; minor gaps (TIGHTBEAMC, CTF overrides) are non-blocking. |
| 2 | Timestamp-based dependency tracking (make's native behavior) | **Confirmed.** Make compares source/header mtimes vs object/target mtimes natively. |
| 3 | Object reuse mechanism (make compares source vs object timestamps) | **Confirmed.** `-MD -MF -MP` rules in MakeInc.rule; object reused when source older than object. |
| 4 | Header dependency tracking (XNU .d dependency files) | **Confirmed.** `.d` files generated per object; header changes propagate via make. |
| 5 | Generated-header invalidation (installhdrs, config headers) | **Confirmed.** `installhdrs` copies headers; `setup` touches timestamps; config headers are `.d` deps. |
| 6 | Source modification detection (make's native timestamp comparison) | **Confirmed.** Single-file edit → single-object recompile. |
| 7 | Cache restoration behavior (GitHub Actions cache restore) | **Confirmed.** `actions/cache/restore@v4` with pinned key; tar preserves mtime. |
| 8 | Cache corruption handling (cache key mismatch = clean build fallback) | **Confirmed.** Mismatched key → no restore → clean build. `force_clean` discards cache. |
| 9 | Restored objects trigger unnecessary recompilation on fresh runners | **Confirmed as potential issue.** Fresh checkout source mtime (now) > cached object mtime (old) → full recompile on first build after restore. Performance issue, not correctness issue. |
| 10 | Do NOT manipulate source timestamps | **Compliant.** No `touch` of source files anywhere in the workaround or dxb. |

---

## 8. Recommendations for Production Use

### 8.1 Keep the workaround (do not revert to dxb guard)

The direct `make install` call is the correct approach. dxb's `build_xnu()` guard must remain bypassed. Do not re-introduce the file-existence guard.

### 8.2 Address the cache-restore mtime mismatch (optional optimization)

To achieve true incremental builds on fresh runners after cache restore, consider one of:

1. **Touch objects after restore** to be newer than sources:
   ```bash
   find "${BUILD_DIR}/xnu.obj" -name '*.o' -newer "${SRCROOT}" -exec touch {} +
   ```
   Actually this is backwards — we want objects to be *newer* than sources so make skips them. Touch sources to be older than objects, or touch objects to be newer than sources. But touching objects to be newer than sources when source content is identical is safe (make skips compile, objects are valid).
   
   Better: touch source files to match commit date, or use `git` mtime preservation.

2. **Use `make -o file` (old-file) option** to treat all sources as older than targets, forcing object reuse. Risky if a source actually changed.

3. **Accept the limitation:** On fresh runners, first build after cache restore recompiles everything. Subsequent builds (without cache restore) are incremental. Given 120-min timeout and cache save after each build, this is acceptable.

**Recommendation:** Do not add timestamp manipulation (violates responsibility #10 and risks incorrect builds). Accept the cache-restore recompile as a known trade-off. Document it in BENCHMARK.md.

### 8.3 Add cache key for source tree state

Consider adding a hash of the source tree (or at least the XNU commit + begonia patch files) to the cache key. The current key includes `XNU_COMMIT` but not the begonia platform files copied into the source tree (xnu.yml:439-453). If begonia patches change but XNU_COMMIT stays the same, the cache would incorrectly restore old objects.

**Recommendation:** Include a hash of `platform/begonia/` files in the cache key for the `xnu-begonia` job.

### 8.4 Preserve CTF tool overrides

dxb's `ctf_tool_overrides()` passes `CTFCONVERT`/`CTFMERGE`/`CTFDUMP` paths. The workaround omits these. If CTF generation is desired, add them. Currently CTF is silently disabled when tools are missing (MakeInc.def:870-877), which is acceptable for baseline builds but means the kernel ships without `__CTF` type data.

### 8.5 Benchmark scenarios need cache-restore awareness

BENCHMARK.md scenario B2 ("Second unchanged build, cache hit") should clarify that on a fresh runner with cache restore, the first B2 run may recompile everything due to mtime mismatch. Run B2 twice to measure steady-state incremental performance (already noted in BENCHMARK.md:151).

---

## 9. Status

**The workaround is implemented in `.github/workflows/xnu.yml`** (lines 209-249 and 456-489), bypassing dxb's `build_xnu()` guard by calling `make install` directly with matching parameters.

**Cannot be tested on Linux:** The build requires macOS with Xcode 15.4, KDK 13.5 build 22G74, and Apple's clang/mig/kmutil toolchain. This host is Linux x86_64 with only `aarch64-linux-gnu-gcc 16.2.0` and system clang. No macOS SDK, no KDK, no `xcodebuild`/`mig`/`kmutil`. A baseline XNU build cannot be performed here (documented blocker in BUILD.md:94-108).

**Validation pending:** All incremental-build claims (correctness, SHA-256 stability, cache-restore behavior) must be validated on a macOS GitHub-hosted runner (macos-14) per BENCHMARK.md scenarios B1-B5.

---

## 10. Sign-off

| Item | Status |
|---|---|
| DXB guard analysis | Complete — guard at `build.sh:698` confirmed wrong for incremental builds |
| Workaround validation | Complete — `make install` parameters match dxb; verified against build.sh:732 and xnu.yml:237-245 |
| Incremental rebuild correctness | Confirmed — make's native timestamp tracking + XNU `.d` dep files provide correct incremental behavior |
| Generated-header invalidation | Confirmed — `installhdrs` + `.mach_kernel.timestamp` mechanism (MakeInc.kernel:317-329) |
| Cache restore/incremental interaction | Analyzed — mtime mismatch on fresh runners causes full recompile after cache restore; performance only, not correctness |
| Cache corruption handling | Confirmed — key mismatch = clean build fallback; `force_clean` discards cache |
| Source timestamp manipulation | None — compliant with responsibility #10 |
| Testability on Linux | Blocked — no macOS/Xcode/KDK; must validate on GitHub macos-14 runner |

**Sign-off: ANALYSIS COMPLETE. Workaround is correct for incremental builds on persistent checkouts. Cache-restore mtime mismatch is a known performance limitation on fresh runners. Production deployment on macOS CI is recommended pending benchmark validation (B1-B5).**

Agent 4 — INCREMENTAL BUILD ENGINEER  
Operation First Boot — BEGONIA-XNU
