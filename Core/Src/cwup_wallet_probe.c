/**
 * @file cwup_wallet_probe.c
 * @brief Probe get_wallet_seed() with a temp buffer — lab/CI only; clears buffer immediately.
 */
#include "cwup_wallet_probe.h"
#include "memzero.h"
#include <stddef.h>
#include <stdint.h>

extern int get_wallet_seed(uint8_t *seed_out, size_t max_len);

int cwup_wallet_probe_seed_available(void)
{
    uint8_t tmp[64];

    memzero(tmp, sizeof(tmp));
    int r = get_wallet_seed(tmp, sizeof(tmp));
    memzero(tmp, sizeof(tmp));
    return (r == 0) ? 1 : 0;
}
