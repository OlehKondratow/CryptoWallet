/**
  ******************************************************************************
  * @file    time_service.c
  * @brief   SNTP client — wall-clock epoch and UTC strings for logs/UI.
  ******************************************************************************
  * @details
  *          **Support module** in architecture: not on the signing hot path; provides
  *          time after Ethernet is up (@c time_service_start() ). Ported from lwip_zero,
  *          uses @c pool.ntp.org , logs via @c Task_Display_Log() (UART + OLED line).
  *          Leap-year and epoch→UTC formatting for human-readable timestamps.
  ******************************************************************************
  */
#include "time_service.h"
#include "hw_init.h"
#include "main.h"
#include "task_display.h"
#include "app_log.h"
#include "lwip/apps/sntp.h"
#include "lwip/tcpip.h"
#include <stdio.h>

static volatile uint8_t s_sntp_started = 0U;
static volatile uint8_t s_time_synced = 0U;
static volatile uint32_t s_epoch_base = 0U;
static volatile uint32_t s_tick_base_ms = 0U;

/**
 * @brief LwIP tcpip_callback: start SNTP client.
 * @param arg Unused.
 */
static void time_service_sntp_start_cb(void *arg)
{
    (void)arg;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    APP_LOG_INFO("[TIME] SNTP started (pool.ntp.org)");
}

/**
 * @brief Check if year is leap year.
 * @param year Year (e.g. 2026).
 * @return 1 if leap, 0 otherwise.
 */
static uint8_t time_service_is_leap(uint32_t year)
{
    if ((year % 400U) == 0U) return 1U;
    if ((year % 100U) == 0U) return 0U;
    return ((year % 4U) == 0U) ? 1U : 0U;
}

static void time_service_epoch_to_utc(uint32_t epoch_sec,
                                      uint32_t *year, uint32_t *month, uint32_t *day,
                                      uint32_t *hour, uint32_t *minute, uint32_t *second)
{
    static const uint8_t month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t days = epoch_sec / 86400U;
    uint32_t rem = epoch_sec % 86400U;
    uint32_t y = 1970U;
    uint32_t m = 0U;
    uint32_t mdays;

    *hour = rem / 3600U;
    rem %= 3600U;
    *minute = rem / 60U;
    *second = rem % 60U;

    while (1) {
        uint32_t year_days = time_service_is_leap(y) ? 366U : 365U;
        if (days < year_days) break;
        days -= year_days;
        y++;
    }

    while (m < 12U) {
        mdays = month_days[m];
        if (m == 1U && time_service_is_leap(y)) mdays = 29U;
        if (days < mdays) break;
        days -= mdays;
        m++;
    }

    *year = y;
    *month = m + 1U;
    *day = days + 1U;
}

/**
 * @brief Initialize time service state.
 */
void time_service_init(void)
{
    s_sntp_started = 0U;
    s_time_synced = 0U;
    s_epoch_base = 0U;
    s_tick_base_ms = HAL_GetTick();
}

/**
 * @brief Start SNTP client via tcpip_callback.
 */
void time_service_start(void)
{
    err_t err;

    if (s_sntp_started) return;

    s_sntp_started = 1U;
    err = tcpip_callback(time_service_sntp_start_cb, NULL);
    if (err != ERR_OK) {
        s_sntp_started = 0U;
        APP_LOG_WARN("[TIME] SNTP start callback failed");
    }
}

/**
 * @brief Set epoch from SNTP sync callback.
 * @param epoch_sec Unix timestamp.
 */
void time_service_set_epoch(uint32_t epoch_sec)
{
    char line[64];

    s_epoch_base = epoch_sec;
    s_tick_base_ms = HAL_GetTick();
    s_time_synced = 1U;

    (void)snprintf(line, sizeof(line), "[INFO] [TIME] SNTP sync epoch=%lu", (unsigned long)epoch_sec);
    Task_Display_Log(line);
}

uint8_t time_service_is_synced(void)
{
    return s_time_synced;
}

/**
 * @brief Get current epoch (seconds since 1970-01-01).
 * @return Unix timestamp.
 */
uint32_t time_service_now_epoch(void)
{
    uint32_t base_epoch = s_epoch_base;
    uint32_t base_tick = s_tick_base_ms;
    uint32_t elapsed_ms = HAL_GetTick() - base_tick;
    return base_epoch + (elapsed_ms / 1000U);
}

/**
 * @brief Format current time as "YYYY-MM-DD HH:MM:SS UTC".
 * @param out     Output buffer.
 * @param out_len Buffer size.
 */
void time_service_now_string(char *out, size_t out_len)
{
    uint32_t year, month, day, hour, minute, second;

    if (out == NULL || out_len == 0U) return;

    if (!s_time_synced) {
        (void)snprintf(out, out_len, "UNSYNCED");
        return;
    }

    uint32_t epoch_now = time_service_now_epoch();
    time_service_epoch_to_utc(epoch_now, &year, &month, &day, &hour, &minute, &second);
    (void)snprintf(out, out_len, "%04lu-%02lu-%02lu %02lu:%02lu:%02lu UTC",
                  (unsigned long)year, (unsigned long)month, (unsigned long)day,
                  (unsigned long)hour, (unsigned long)minute, (unsigned long)second);
}
