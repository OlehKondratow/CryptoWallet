/**
  ******************************************************************************
  * @file    wallet_shared.h
  * @brief   Shared types and handles for inter-task communication.
  ******************************************************************************
  */

#ifndef __WALLET_SHARED_H
#define __WALLET_SHARED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"

/*-----------------------------------------------------------------------------
 * Transaction data (task_net -> task_security via Queue)
 *-----------------------------------------------------------------------------*/
#define TX_RECIPIENT_LEN   42U
#define TX_AMOUNT_LEN      24U
#define TX_CURRENCY_LEN    8U

typedef struct {
    char recipient[TX_RECIPIENT_LEN];
    char amount[TX_AMOUNT_LEN];
    char currency[TX_CURRENCY_LEN];
} wallet_tx_t;

/*-----------------------------------------------------------------------------
 * Display state (task_display state machine)
 *-----------------------------------------------------------------------------*/
typedef enum {
    STATE_WALLET = 0,
    STATE_SECURITY,
    STATE_NETWORK,
    STATE_LOG,
    STATE_COUNT
} display_state_t;

/*-----------------------------------------------------------------------------
 * Display context (updated by other tasks, read by task_display)
 *-----------------------------------------------------------------------------*/
typedef struct {
    char currency[TX_CURRENCY_LEN];
    char amount[TX_AMOUNT_LEN];
    bool safe_locked;
    bool signature_valid;
    char ip_addr[16];
    char mac_addr[18];
    bool hid_connected;
    char log_line[64];
    uint16_t log_scroll;
} display_context_t;

/*-----------------------------------------------------------------------------
 * Signing FSM states (Trezor-style)
 *-----------------------------------------------------------------------------*/
typedef enum {
    SIGNING_IDLE = 0,
    SIGNING_RECEIVED,
    SIGNING_WAIT_CONFIRM,
    SIGNING_IN_PROGRESS,
    SIGNING_DONE,
    SIGNING_REJECTED,
    SIGNING_ERROR
} signing_state_t;

/*-----------------------------------------------------------------------------
 * Event Group bits (task_io -> task_security)
 *-----------------------------------------------------------------------------*/
#define EVENT_USER_CONFIRMED    (1U << 0)
#define EVENT_USER_REJECTED     (1U << 1)

/*-----------------------------------------------------------------------------
 * Global handles (created in main, used by tasks)
 *-----------------------------------------------------------------------------*/
extern QueueHandle_t     g_tx_queue;
extern QueueHandle_t     g_display_queue;  /**< Net -> Display: Transaction_Data_t */
extern EventGroupHandle_t g_user_event_group;
extern SemaphoreHandle_t g_i2c_mutex;
extern SemaphoreHandle_t g_ui_mutex;  /**< task_display s_ui_data protection */
extern display_context_t g_display_ctx;
extern SemaphoreHandle_t g_display_ctx_mutex;
/** @brief Set by task_security when alert condition (e.g. sign error). */
extern volatile uint8_t  g_security_alert;

#ifdef __cplusplus
}
#endif

#endif /* __WALLET_SHARED_H */
