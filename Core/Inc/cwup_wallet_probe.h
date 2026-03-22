/**
 * @file cwup_wallet_probe.h
 * @brief CWUP / lab: non-secret snapshot of wallet seed availability (no authentication).
 */
#ifndef CWUP_WALLET_PROBE_H
#define CWUP_WALLET_PROBE_H

#include <stdint.h>

/**
 * @return 1 if get_wallet_seed() would succeed, 0 otherwise (never exposes seed bytes).
 */
int cwup_wallet_probe_seed_available(void);

#endif
