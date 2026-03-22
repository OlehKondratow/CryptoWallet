/**
 * @file fw_integrity.h
 * @brief Runtime CRC32 over the programmed application region in internal Flash.
 *
 * Used for lab/CI: compare device-reported CRC with `scripts/fw_integrity_check.py`
 * on build/cryptowallet.bin. This is **not** a substitute for a verified bootloader
 * (see stm32_secure_boot); it detects accidental corruption or wrong image flashed.
 */
#ifndef FW_INTEGRITY_H
#define FW_INTEGRITY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Call once after clocks/HAL are up (e.g. after HW_Init). */
void fw_integrity_init(void);

uint32_t fw_integrity_get_crc32(void);
uint32_t fw_integrity_get_image_length(void);
void fw_integrity_get_bounds(uint32_t *flash_start, uint32_t *flash_end);

/**
 * One line for logs / future CWUP AT+FWINFO: "FWINFO crc32=0x...,len=...,start=0x...,end=0x..."
 * Returns number of characters written (excluding '\0'), or 0 on error.
 */
int fw_integrity_snprint(char *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* FW_INTEGRITY_H */
