/**
  ******************************************************************************
  * @file    task_display.h
  * @brief   Header for the OLED Display Task (SSD1306).
  ******************************************************************************
  * @details Defines the "language" for Security and Net modules to communicate
  *          with the display. Queue-based: Ethernet puts Transaction_Data_t in
  *          g_display_queue; Display wakes, shows recipient, waits for User.
  ******************************************************************************
  * @author Gemini & User
  * @date 2026
  */

#ifndef __TASK_DISPLAY_H
#define __TASK_DISPLAY_H

#include "main.h"
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UI States for the OLED Display
 */
typedef enum {
    UI_STATE_WALLET,    /**< Shows Coin Type, Balance and pending Amount */
    UI_STATE_SECURITY,  /**< Shows Safe status (Locked/Unlocked) and Key Sign status */
    UI_STATE_NETWORK,   /**< Shows IP, MAC and HID Connection status */
    UI_STATE_LOG        /**< Shows system event logs */
} UI_State_t;

/**
 * @brief Structure representing a pending transaction for display
 */
typedef struct {
    char coin_name[8];     /**< e.g., "BTC" */
    double amount;        /**< e.g., 0.00123 */
    char recipient[35];    /**< BTC address */
    uint8_t is_pending;   /**< Flag to trigger "Confirm" screen */
} Transaction_Data_t;

/**
 * @brief Global UI State structure
 */
typedef struct {
    UI_State_t current_state;
    char ip_addr[16];
    char mac_addr[18];
    char last_log[32];
    uint8_t is_safe_locked; /**< 1 = Locked, 0 = Unlocked */
    Transaction_Data_t pending_tx;
} UI_Display_Data_t;

/* Function Prototypes */

/**
 * @brief Entry point for the Display Task (FreeRTOS task function).
 * @param argument FreeRTOS task argument (unused).
 * @return None.
 */
void StartDisplayTask(void *argument);

/**
 * @brief Create and start the display task (wrapper for StartDisplayTask).
 * @note  Call from main() before vTaskStartScheduler().
 * @return None.
 */
void Task_Display_Create(void);

/**
 * @brief Updates the display state via Queue.
 * @param new_data Pointer to the new UI data structure.
 * @return pdTRUE if sent, pdFALSE if queue full or timeout.
 */
BaseType_t UI_UpdateData(const UI_Display_Data_t *new_data);

/**
 * @brief Append a log string to the scrollable log (thread-safe).
 * @param msg  Null-terminated message (truncated if too long).
 * @return None.
 */
void Task_Display_Log(const char *msg);

/**
 * @brief Clear the pending transaction flag (e.g. after sign/reject).
 * @return None.
 */
void UI_ClearPending(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_DISPLAY_H */
