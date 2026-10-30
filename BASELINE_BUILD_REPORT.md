# BEGONIA-XNU — Baseline XNU 8796.141.3 Build Report

**Agent:** Agent 3 — Baseline XNU Build Engineer  
**Mission:** Execute the pristine XNU 8796.141.3 baseline build via GitHub Actions (`workflow_dispatch`)  
**Report date:** 2026-10-30  
**Status:** NOT EXECUTED — BLOCKED (no GitHub Actions run created)

---

## 1. Scope & Configuration (baseline, pristine)

| Parameter | Value |
|---|---|
| Repository | https://github.com/ctrlaltf4XC/hackios-begonia |
| Branch | `ci/first-boot-validation` |
| Workflow file | `.github/workflows/xnu.yml` |
| Target job | `xnu-baseline` |
| XNU source | `xnu-8796.141.3` @ commit `1b191cb58250d0705d8a51287127505aa4bc0789` (pinned, exact) |
| Patches | NONE (pristine — no Begonia patches, no platform modifications) |
| Xcode | 15.4 (`/Applications/Xcode_15.4.app/Contents/Developer`) |
| KDK | Kernel Debug Kit 13.5 build 22G74 |
| BUILD_LTO | 0 |
| BUILD_WERROR | 0 |
| XNU_BUILD_JOBS | 4 |
| MACHINE_CONFIG | VMAPPLE |
| KERNEL_CONFIG | DEVELOPMENT |
| build_kc | false |
| keep_going | false |
| force_clean | true |
| RC_DARWIN_KERNEL_VERSION | 22.6.0 |
| ARCH_CONFIG | ARM64 |

Baseline integrity: **preserved** — no Begonia patches were mixed into this build.

---

## 2. Pre-flight verification (Agent 1 / Agent 2 readiness)

The shared board shows Agent 1 (`ses_e72b46a21...`, Repository & GitHub Operator) and Agent 2 (`ses_e72b44d88...`, macOS Runner Engineer) in `busy` state with **no readiness messages posted** (`messages: []`). Independently verified prerequisites:

- `gh` authenticated as `ctrlaltf4XC` with `repo` + `workflow` scopes (`gh auth status`).
- `git` remote `origin` = `https://github.com/ctrlaltf4XC/hackios-begonia.git`, local branch `ci/first-boot-validation` up to date with `origin/ci/first-boot-validation`, working tree clean.

Agent 1/Agent 2 confirmation was not received on the board; regardless, a technical blocker (below) prevents dispatch independent of runner availability.

---

## 3. Workflow dispatch attempts

The `workflow_dispatch`-enabled XNU baseline workflow (`.github/workflows/xnu.yml`) exists **only on branch `ci/first-boot-validation`**. It is **not registered on the repository's default branch `main`**.

GitHub Actions `workflow_dispatch` can only be triggered for workflows registered on the default branch. The default branch (`main`) contains exactly one workflow:

| ID | File (default branch `main`) | Purpose | `workflow_dispatch`? |
|---|---|---|---|
| 395447326 | `.github/workflows/build.yml` | Boot.img packaging (Ubuntu `loader`/`bootimg` jobs) | Yes, but unrelated to XNU |

Dispatch attempts against `xnu.yml` all failed with HTTP 404 (workflow not found on the default branch):

| # | Command | Result |
|---|---|---|
| 1 | `gh workflow run xnu.yml --ref ci/first-boot-validation -f ...` | `HTTP 404: workflow xnu.yml not found on the default branch` |
| 2 | `gh api POST .../actions/workflows/.github/workflows/xnu.yml/dispatches -f ref=ci/first-boot-validation` | `{"message":"Not Found","status":"404"}` |
| 3 | `gh api POST .../actions/workflows/workflows/xnu.yml/dispatches -f ref=ci/first-boot-validation` | `{"message":"Not Found","status":"404"}` |

Control test confirming cross-branch `ref` dispatch itself works: dispatching registered workflow ID 395447326 (`build.yml`) with `-f ref=ci/first-boot-validation` was structurally accepted by GitHub (progressed past workflow resolution to `inputs` validation). The limitation is specifically that `xnu.yml` is not registered on `main`.

`build.yml` (ID 395447326, cited in the task as the workflow ID) is an **unrelated boot image packaging workflow** and cannot produce the XNU kernel baseline. Dispatching it would not satisfy the mission.

---

## 4. Build result

| Field | Value |
|---|---|
| GitHub Actions run URL | N/A — no run created |
| GitHub Actions run ID | N/A — no run created |
| Build result | **NOT EXECUTED** |
| Reason | The `workflow_dispatch`-enabled baseline workflow `.github/workflows/xnu.yml` is not registered on GitHub's default branch (`main`), so it cannot be triggered via the `workflow_dispatch` API (HTTP 404). The only default-branch workflow (`build.yml`, ID 395447326) is an unrelated boot.img packaging workflow. No macos-14 runner was provisioned and no XNU build ran. |
| Runner image used | N/A (planned: `macos-14`) |
| Xcode version used | N/A (planned: Xcode 15.4 — not verified) |
| KDK version used | N/A (planned: Kernel Debug Kit 13.5 build 22G74 — not used) |
| Build duration | N/A |
| Compiler/linker output | N/A — no compilation occurred |

---

## 5. Kernel artifact & Mach-O validation

| Field | Value |
|---|---|
| Kernel artifact path | N/A — no artifact produced |
| SHA256 | N/A |
| Architecture (Mach-O) | N/A |
| CPU subtype | N/A |
| Entry point | N/A |

No kernel binary was built, so no Mach-O validation (architecture, CPU subtype, entry point) could be performed, and no SHA256 checksum was generated.

---

## 6. Sign-off status

**Sign-off: BLOCKED — NOT SIGNED OFF**

Per the mission charter: *"Do not claim success without an actual successful GitHub Actions run."* No GitHub Actions run was created and no baseline build executed, so success cannot be claimed.

**Blocker:** `.github/workflows/xnu.yml` must be present on the repository's default branch (`main`) to be triggerable via `workflow_dispatch`, OR an alternative dispatch path must be established (e.g., register `xnu.yml` on `main`, or use a self-hosted runner workflow definition). Until then, the baseline XNU 8796.141.3 build cannot be executed via GitHub Actions.

**Recommended next steps:**
1. Register `.github/workflows/xnu.yml` on the default branch `main` (or make it dispatchable from `ci/first-boot-validation` via a supported path) so `workflow_dispatch` can target it with `ref=ci/first-boot-validation`.
2. Re-confirm Agent 1 (remote readiness) and Agent 2 (runner availability) on the board.
3. Re-trigger `workflow_dispatch` with `machine_config=VMAPPLE`, `kernel_config=DEVELOPMENT`, `xnu_build_jobs=4`, `build_kc=false`, `keep_going=false`, `force_clean=true`.
4. Monitor the `xnu-baseline` job; on success, download the `xnu-build-vmapple` artifact, verify Mach-O format, and generate SHA256; on failure, capture complete error logs for Agents 4/6.

---

*No Begonia patches were introduced into this baseline. No success is claimed without an actual successful GitHub Actions run.*
