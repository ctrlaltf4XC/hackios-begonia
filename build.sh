#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Top-level build orchestration for BEGONIA-XNU.
#
# Usage:
#   ./build.sh [bootshim|xnu|platform|all|clean]
#
# Default target: build the boot shim (the only component that can be built
# without the XNU source tree).
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
target=${1:-bootshim}

echo "== BEGONIA-XNU build: $target =="

case "$target" in
  bootshim)
    echo "[1/1] Building boot shim (MT6785)..."
    (cd "$root/bootshim" && ./build.sh)
    echo "[bootshim] done -> bootshim/out/Image"
    ;;
  xnu)
    echo "[xnu] XNU 8796 source not yet present in xnu/."
    echo "[xnu] Obtain Apple xnu-8796 source and apply ios-sweet-compatible patches."
    echo "[xnu] See XNU_BOOT_REQUIREMENTS.md for the required patch set."
    exit 1
    ;;
  platform)
    echo "[platform] platform/begonia/ stubs await XNU 8796 source."
    echo "[platform] See XNU_BOOT_REQUIREMENTS.md for the minimum platform set."
    exit 1
    ;;
  all)
    "$0" bootshim
    "$0" xnu || true
    "$0" platform || true
    ;;
  clean)
    rm -rf "$root/bootshim/out"
    echo "[clean] done"
    ;;
  *)
    echo "Unknown target: $target" >&2
    echo "Usage: $0 [bootshim|xnu|platform|all|clean]" >&2
    exit 2
    ;;
esac
