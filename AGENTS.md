# BEGONIA-XNU — Agent Working Rules

## Mission
Achieve the first verified hardware boot of Apple XNU 8796 on Xiaomi Redmi Note 8 Pro (Begonia / MT6785). Kernel boot ONLY. No iOS UI, SpringBoard, apps, Wi-Fi, Bluetooth, camera, audio, or complete userspace.

## Repository layout
```
BEGONIA-XNU/
├── bootshim/        # Agent 4: MediaTek-specific XNU boot shim
├── xnu/             # Agent 3/6: XNU 8796 source + patches
├── platform/begonia/# Agent 5: minimum XNU platform support
├── drivers/         # shim-only drivers (UART, timer, GIC helpers)
├── tools/           # build/packaging/validation helpers
├── scripts/         # automation
├── configs/         # build configs
├── docs/            # design notes
├── BOOT_LOGS/       # UART capture evidence
├── tests/           # host validation tests
├── AGENTS.md        # this file
├── PROJECT_STATUS.md# current milestone state
├── TODO.md          # pending work
├── ARCHITECTURE.md  # subsystem interfaces
├── POX_HARDWARE_ANALYSIS.md   # Agent 2 deliverable
├── XNU_BOOT_REQUIREMENTS.md   # Agent 3 deliverable
├── DEVELOPMENT_ROADMAP.md
├── BUILD.md         # Agent 6 deliverable
├── build.sh         # Agent 6 deliverable
├── validate.sh      # Agent 6 deliverable
├── package.sh       # Agent 6 deliverable
├── BOOT_FLOW.md     # Agent 4 deliverable
├── BOOT_VALIDATION.md # Agent 4 deliverable
├── DEBUGGING.md     # Agent 7 deliverable
├── TEST_RESULTS.md  # Agent 7 deliverable
└── KNOWN_ISSUES.md  # Agent 7 deliverable
```

## Critical rules (from project charter)
1. POX is a Linux hardware reference, NOT an XNU-compatible kernel.
2. Do not port entire Linux drivers into XNU.
3. Do not assume Snapdragon (ios-sweet) implementations work on MT6785.
4. Do not invent hardware addresses. Every value must trace to POX DTS/source.
5. Never modify critical boot partitions during initial development.
6. Prefer RAM-based / `fastboot boot` temporary testing.
7. Maintain separate branches per agent.
8. No two agents independently modify the same shared interface.
9. Every implementation must include validation evidence.
10. Never fabricate hardware test results.
11. Preserve the original Android boot path.
12. Scope: XNU first kernel boot only.

## Source references (read-only, do not modify)
- POX kernel: `/home/yu/BEGONIA-XNU/pox-begonia-kernel` @ 2fb79cd9cba6 (tag v2.0.0)
- ios-sweet: `/home/yu/BEGONIA-XNU/ios-sweet` @ 45263be
- xnu_bootshim_benesse_a04br3: `/home/yu/BEGONIA-XNU/xnu_bootshim_benesse_a04br3` @ f82c3b8

## Toolchain
- aarch64-linux-gnu-gcc 16.2.0 (host)
- clang (XNU build)
- dtc available

## Agent handoff convention
Update `PROJECT_STATUS.md` after each milestone. Record exact commits, build results, and UART evidence. Distinguish observations from guesses.
