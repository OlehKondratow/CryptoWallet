#ifndef __SHA256_MINIMAL_H__
#define __SHA256_MINIMAL_H__

#include <stdint.h>
#include <stddef.h>

void sha256_minimal(const uint8_t *data, size_t len, uint8_t digest[32]);

#endif
