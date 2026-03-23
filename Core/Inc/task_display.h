/**
  ******************************************************************************
  * @file    task_display.h
  * @brief   OLED task API — SSD1306 UI types and @c Task_Display_Log .
  ******************************************************************************
  * @details
  *          **Role:** Defines how @c task_net , @c task_sign , and others push UI data:
  *          @c Transaction_Data_t on @c g_display_queue ; @c Task_Display_Log() writes
  *          UART + scrollable log line (see @c documentation/02-firmware-structure.md ).
  *
  *          **USER confirm** is not in this module — @c task_user.c sets event bits
  *          consumed by @c task_sign.c .
  ******************************************************************************
  */

#ifndef __TASK_DISPLAY_H
#define __TASK_DISPLAY_H

#include "main.h"
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Primary OLED view modes (full @c task_display.c UI).
 * @details Drives which four-line layout is rendered; may differ from @c display_state_t naming in @c wallet_shared.h .
 */
typedef enum {
    UI_STATE_WALLET,    /**< Coin, balance, pending amount / recipient summary. */
    UI_STATE_SECURITY,  /**< Locked/unlocked and signature status. */
    UI_STATE_NETWORK,   /**< IP, MAC, USB/HID hints. */
    UI_STATE_LOG        /**< Scrolling diagnostic log. */
} UI_State_t;

/**
 * @brief Pending transaction snapshot pushed to the display queue (often from @c task_net.c ).
 * @details @c is_pending triggers confirm UX; fields are copied into internal @c UI_Display_Data_t .
 */
typedef struct {
    char coin_name[8];     /**< Short ticker label (e.g. "BTC"). */
    double amount;         /**< Parsed numeric amount for UI (may differ from string in @c wallet_tx_t ). */
    char recipient[35];    /**< Truncated or short address for display. */
    uint8_t is_pending;    /**< Non-zero: show “confirm” style screen until @c UI_ClearPending() . */
} Transaction_Data_t;

/**
 * @brief Merged UI state used inside @c task_display.c (protected by @c g_ui_mutex ).
 * @details Combines queue-driven pending tx with network strings and last log fragment.
 */
typedef struct {
    UI_State_t current_state; /**< Active screen mode. */
    char ip_addr[16];         /**< Copy of IP string for NETWORK view. */
    char mac_addr[18];        /**< Copy of MAC string for NETWORK view. */
    char last_log[32];        /**< Short tail of last log for inline display. */
    uint8_t is_safe_locked;   /**< 1 = wallet treated as locked in UI. */
    Transaction_Data_t pending_tx; /**< Last pending transaction from queue. */
} UI_Display_Data_t;

/**
 * @brief   Legacy Cube/RTOS entry name (not used if only @c Task_Display_Create is called).
 * @param   argument  Unused FreeRTOS parameter.
 */
void StartDisplayTask(void *argument);

/**
 * @brief   Create the display FreeRTOS task (@c display_task or minimal variant per build).
 * @details Called from @c main.c . Requires I2C/OLED init unless @c SKIP_OLED .
 */
void Task_Display_Create(void);

/**
 * @brief   Push a full UI snapshot to the display task via @c g_display_queue .
 * @details Copies @a new_data ; blocks up to internal timeout if queue full.
 * @param   new_data  Pointer to snapshot; must remain valid for duration of copy.
 * @return  @c pdTRUE if queued, @c pdFALSE on timeout or null handle.
 */
BaseType_t UI_UpdateData(const UI_Display_Data_t *new_data);

/**
 * @brief   Log a line to UART and update the scrollable log buffer on the OLED.
 * @details Thread-safe with respect to display mutex usage inside implementation. Truncates long strings.
 * @param   msg  Null-terminated message (ASCII recommended).
 */
void Task_Display_Log(const char *msg);

/**
 * @brief   Clear @c pending_tx.is_pending after sign/reject so UI leaves confirm state.
 * @details Called from @c task_sign.c after successful signature path.
 */
void UI_ClearPending(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_DISPLAY_H */
