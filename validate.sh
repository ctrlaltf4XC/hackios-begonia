#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Validate the BEGONIA-XNU boot shim build and host tests.
#
# Runs:
#   1. boot shim build (build.sh) - verifies cross-compilation, no absolute
#      relocations, correct Image size, sha256 in build.json
#   2. host FDT parser + ADT builder test (tests/test_fdt) - verifies the
#      FDT inspector correctly parses a dtc-compiled begonia-like DTB
set -euo pipefail
root=$(cd "$(dirname "$0")" && root=$PWD; cd "$root"; pwd)
echo "== BEGONIA-XNU validate =="

fail=0

echo "[1/2] Building boot shim..."
(cd "$root/bootshim" && ./build.sh) || { echo "FAIL: boot shim build"; fail=1; }

echo "[2/2] Running host FDT parser + ADT test..."
gcc -DHANDOFF_HOST_TEST -Wall -Wextra -I"$root/bootshim" -I"$root/tests" \
    "$root/tests/test_fdt.c" "$root/bootshim/fdt.c" "$root/bootshim/adt.c" \
    "$root/bootshim/handoff.c" -o "$root/tests/test_fdt" || { echo "FAIL: test compile"; fail=1; }
if [ $fail -eq 0 ]; then
    "$root/tests/test_fdt" "$root/tests/begonia-test.dtb" || { echo "FAIL: test run"; fail=1; }
fi

echo ""
if [ $fail -eq 0 ]; then
    echo "VALIDATE: ALL CHECKS PASSED"
    exit 0
else
    echo "VALIDATE: FAILURES DETECTED"
    exit 1
fi
