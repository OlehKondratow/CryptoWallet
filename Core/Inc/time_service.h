/**
  ******************************************************************************
  * @file    time_service.h
  * @brief   SNTP API — init, start after link, epoch + formatted UTC.
  ******************************************************************************
  * @details
  *          Call @c time_service_init() before @c tcpip_init ; @c time_service_start()
  *          after Ethernet link up. Feeds wallet logs / UI time strings; see
  *          @c time_service.c and @c documentation/02-firmware-structure.md (support modules).
  ******************************************************************************
  */

#ifndef CRYPTOWALLET_TIME_SERVICE_H
#define CRYPTOWALLET_TIME_SERVICE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize time service (reset state). Call before tcpip_init.
 * @return None.
 */
void time_service_init(void);

/**
 * @brief Start SNTP client (pool.ntp.org). Call after Ethernet link up.
 * @return None.
 */
void time_service_start(void);

/**
 * @brief Set epoch from SNTP callback.
 * @param epoch_sec Unix timestamp (seconds since 1970-01-01 00:00 UTC).
 * @return None.
 */
void time_service_set_epoch(uint32_t epoch_sec);

/**
 * @brief Check if time has been synced via SNTP.
 * @return 1 if synced, 0 otherwise.
 */
uint8_t time_service_is_synced(void);

/**
 * @brief Get current epoch (seconds). Uses HAL_GetTick() delta if synced.
 * @return Unix timestamp in seconds.
 */
uint32_t time_service_now_epoch(void);

/**
 * @brief Format current time as "YYYY-MM-DD HH:MM:SS UTC" or "UNSYNCED".
 * @param out     Output buffer.
 * @param out_len Buffer size (including null terminator).
 * @return None.
 */
void time_service_now_string(char *out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTOWALLET_TIME_SERVICE_H */
