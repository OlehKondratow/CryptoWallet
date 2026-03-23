\page ci_pipeline_en "CI: build, TRNG on UART, pipeline"
\related INFRASTRUCTURE
\related TESTING_GUIDE_RNG_SIGNING
\related security_and_testing_en

# CI pipeline (Gitea Actions)

<brief>Workflow: `.gitea/workflows/simple-ci.yml`. By default the firmware is built with **`USE_RNG_DUMP=1`**: UART carries a **binary stream from the hardware TRNG** (STM32 RNG → dump for dieharder). Plain-text boot markers `[WALLET]` are **not** waited for — the “Analyse UART Log” job skips string matching in this mode.</brief>

## Summary

| Mode | `CI_BUILD_USE_RNG_DUMP` | UART | `uart_boot_markers.txt` | `hardware-test` RNG capture |
|------|-------------------------|------|-------------------------|-----------------------------|
| **Default (push/PR)** | `1` | Binary TRNG | Skipped | Enabled |
| **Manual run** | input `ci_build_use_rng_dump`: **1** or **0** | 1 → TRNG / 0 → text | Only if **0** | **1** → yes; **0** → auto-skip (no timeout) |

- **Text logs for debugging:** **Run workflow** → set `ci_build_use_rng_dump` to **0** (markers run; RNG capture expects no binary stream).
- **Disable capture:** `CI_SKIP_RNG_UART_CAPTURE=1` (runner env).
- **Build is 0 but board has RNG firmware:** `CI_RNG_UART_CAPTURE_FORCE=1`.

There is no separate CWUP-only workflow: UART HIL runs in **Simple CI** (manual **Run workflow** with `ci_build_use_rng_dump=0`, or the “Analyse UART Log” job when the build uses text mode).

## Why text UART markers are not waited in RNG mode

This is not a hardware limit — it follows **current firmware + test design**:

1. **Dieharder expects a raw TRNG byte stream.** Any log text on the same wire **contaminates** the sample. With `USE_RNG_DUMP`, `rng_dump.c` continuously TXs **binary** blocks; other tasks may still log to the same UART → **interleaving** that stats tools are not meant to parse as “pure RNG”.

2. **The marker script** looks for **readable substrings**. In a stream dominated by random bytes, substring search is **unreliable** (false hits / no clean “lines”).

3. **An AT-style command/data split is feasible** as a **future protocol** (modes, escapes, host state machine). It is **not implemented** today — CI chooses **either** text markers **or** RNG capture for a given firmware build.

RNG/signing testing: [TESTING_GUIDE_RNG_SIGNING.md](TESTING_GUIDE_RNG_SIGNING.md).  
Runner setup: [INFRASTRUCTURE.md](INFRASTRUCTURE.md).

**Русский:** [CI_PIPELINE_ru.md](CI_PIPELINE_ru.md)

**UART protocol (MVP):** [UART_PROTOCOL_MVP_en.md](UART_PROTOCOL_MVP_en.md)

**Security & testing overview:** [SECURITY_AND_TESTING_EN.md](SECURITY_AND_TESTING_EN.md) (full Russian: [SECURITY_AND_TESTING_RU.md](SECURITY_AND_TESTING_RU.md))
