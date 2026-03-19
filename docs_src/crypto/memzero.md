\page memzero "memzero: secure buffer zeroing"
\related memzero

# `memzero.c` + `memzero.h`

<brief>The `memzero` module securely clears sensitive buffers (private keys, digests, seeds) via volatile writes, preventing compiler optimization and eliminating traces of dangerous data from memory.</brief>

## Overview

<brief>The `memzero` module securely clears sensitive buffers (private keys, digests, seeds) via volatile writes, preventing compiler optimization and eliminating traces of dangerous data from memory.</brief>

## Abstract (Logic Synthesis)

`memzero` solves one critical security problem: modern compilers may optimize away a "useless" `memset(buf, 0, len)` if the compiler notices that `buf` is no longer used afterward. When working with private keys, this is dangerous — sensitive data remains in memory and can be extracted during attacks. Solution: use a `volatile` pointer that the compiler cannot optimize away. Business role: guarantee that on every exit path (success or error), private data is immediately overwritten with zeros before the function returns.

## Logic Flow

Function `memzero(void *pnt, size_t len)`:

1. Casts pointer to `volatile unsigned char *`
2. Loop: byte-wise zero writes via the volatile pointer
3. Return

No state machine, no conditional logic — just reliable overwriting.

## Interrupts/Registers

No ISRs or registers: memory-write operation at C level.

## Timings and Branch Conditions

| Parameter | Behavior |
|---|---|
| NULL pointer | no-op (len-- immediately exits loop) |
| len = 0 | no-op (no iterations) |
| Any buffer size | one byte per iteration, via volatile write |

## Dependencies

No dependencies: only `<stddef.h>` for `size_t`.

Used from:
- `task_sign.c` — clears seed/digest/signature after errors or successful signing
- `task_security.c` — same in legacy FSM
- `crypto_wallet.c` — clears HDNode and private key
- `wallet_seed.c` — clears seed buffer

## Relations

- `task_sign.md` (primary consumer)
- `crypto_wallet.md` (security in signing)
- `wallet_seed.md` (seed handling)
