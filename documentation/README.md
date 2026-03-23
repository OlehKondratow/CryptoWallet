# CryptoWallet — documentation

Single technical canon (English). The firmware is treated as **one system**: trust boundaries, surfaces, crypto, verification, and operations are described at the same depth.

| # | Document | Scope |
|---|----------|--------|
| 1 | [01-trust-model-and-architecture.md](01-trust-model-and-architecture.md) | Boot chain, trust boundaries, threat framing, how components relate |
| 2 | [02-firmware-structure.md](02-firmware-structure.md) | Tasks, IPC, memory, module map → source files |
| 3 | [03-cryptography-and-signing.md](03-cryptography-and-signing.md) | Keys, trezor-crypto path, signing pipeline, operational rules |
| 4 | [04-http-and-webusb.md](04-http-and-webusb.md) | Ethernet HTTP API, WebUSB role, what is *not* authenticated |
| 5 | [05-uart-cwup-protocol.md](05-uart-cwup-protocol.md) | CWUP-0.1: phases, commands, RNG modes, implementation status |
| 6 | [06-integrity-rng-verification.md](06-integrity-rng-verification.md) | `fw_integrity`, UART/TRNG capture, host tests, dieharder, CI semantics |
| 7 | [07-build-ci-infrastructure.md](07-build-ci-infrastructure.md) | Build flags, sibling repos / `CRYPTO_DEPS_ROOT`, Gitea runner, containers |

**Generated (do not edit by hand):**

- `generated/reference-code.md` — from `@file` / `@brief` in sources (`make docs-code-md`)
- `generated/testing-plan-signing-rng.md` — from `scripts/test_plan_signing_rng.py`

**Tooling:** [MAINTENANCE.md](MAINTENANCE.md) — MkDocs, Doxygen, optional HTML site (`site/`).

**Repository entrypoint:** [../README.md](../README.md)
