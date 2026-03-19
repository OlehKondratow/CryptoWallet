/**
  ******************************************************************************
  * @file    wallet_shared.h
  * @brief   Shared types and IPC: queues, events, mutexes, display context.
  ******************************************************************************
  * @details
  *          **Data flow (architecture overview):**
  *          - @c task_net : HTTP @c POST /tx → validate → @c g_tx_queue → @c task_sign .
  *          - @c task_net : UI hints → @c g_display_queue → @c task_display .
  *          - @c task_user : USER (PC13) → @c g_user_event_group → @c task_sign .
  *          - @c task_sign : status/result → @c g_display_ctx → @c task_display .
  *          - @c task_display : SSD1306 I2C under @c g_i2c_mutex .
  *
  *          **Queues:** @c g_tx_queue (@c wallet_tx_t ); @c g_display_queue
  *          (@c Transaction_Data_t in @c task_display.h ).
  *
  *          **Events:** @c EVENT_USER_CONFIRMED (short press),
  *          @c EVENT_USER_REJECTED (long ~2.5 s).
  *
  *          **Mutexes:** @c g_i2c_mutex ; @c g_display_ctx_mutex ; @c g_ui_mutex
  *          (display task internal UI merge).
  *
  *          **Display states ( @c display_state_t ):** WALLET, SECURITY, NETWORK, LOG.
  *          **Signing FSM ( @c signing_state_t )** — used by @c task_sign.c (and
  *          @c task_security.c if enabled).
  *          **Output signature:** @c g_last_sig[64] , @c g_last_sig_ready .
  *
  *          Diagram + coding notes: @c docs_src/architecture.md .
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
 * Transaction data (task_net -> task_sign via g_tx_queue)
 *-----------------------------------------------------------------------------*/
/** @brief Max recipient string length including NUL (host address / identifier). */
#define TX_RECIPIENT_LEN   42U
/** @brief Max amount string length (decimal text, e.g. "0.001"). */
#define TX_AMOUNT_LEN      24U
/** @brief Max currency code length (e.g. "BTC"). */
#define TX_CURRENCY_LEN    8U

/**
 * @brief Host-facing transaction fields validated by @c tx_request_validate() before signing.
 * @details Filled by @c task_net.c (HTTP) or WebUSB path, consumed by @c task_sign.c .
 *          Not a raw Bitcoin transaction — signing hashes a deterministic string built from these fields.
 */
typedef struct {
    char recipient[TX_RECIPIENT_LEN]; /**< Null-terminated payee identifier (e.g. Base58 BTC address). */
    char amount[TX_AMOUNT_LEN];       /**< Null-terminated decimal amount string. */
    char currency[TX_CURRENCY_LEN];   /**< Null-terminated ticker; empty defaults to "BTC" in signing. */
} wallet_tx_t;

/*-----------------------------------------------------------------------------
 * Display state (task_display state machine)
 *-----------------------------------------------------------------------------*/
/**
 * @brief High-level OLED screen mode (legacy / parallel to @c UI_State_t in task_display.h).
 * @details WALLET shows amount/currency; SECURITY lock + signature flag; NETWORK IP/MAC; LOG scroll line.
 */
typedef enum {
    STATE_WALLET = 0,   /**< Currency and amount summary. */
    STATE_SECURITY,     /**< Locked state and signature validity. */
    STATE_NETWORK,      /**< IP, MAC, connection hints. */
    STATE_LOG,          /**< Scrollable log text. */
    STATE_COUNT         /**< Sentinel: number of states (not a visible mode). */
} display_state_t;

/*-----------------------------------------------------------------------------
 * Display context (updated by other tasks, read by task_display)
 *-----------------------------------------------------------------------------*/
/**
 * @brief Shared snapshot of wallet/network/signing status for the display task.
 * @details Writers must hold @c g_display_ctx_mutex except where noted. Readers in @c task_display.c
 *          merge this with queue-driven @c Transaction_Data_t .
 */
typedef struct {
    char currency[TX_CURRENCY_LEN]; /**< Current or pending currency label for UI. */
    char amount[TX_AMOUNT_LEN];      /**< Amount string shown on WALLET / confirm screens. */
    bool safe_locked;                /**< true = treat wallet UI as locked / idle-safe. */
    bool signature_valid;            /**< true after last successful sign until cleared. */
    char ip_addr[16];                /**< IPv4 dotted string for NETWORK screen. */
    char mac_addr[18];               /**< MAC string (e.g. "AA:BB:...") for NETWORK screen. */
    bool hid_connected;              /**< Hint for USB/WebUSB or HID link status in UI. */
    char log_line[64];               /**< Last line appended via @c Task_Display_Log() path. */
    uint16_t log_scroll;             /**< Horizontal scroll offset for log rendering. */
} display_context_t;

/*-----------------------------------------------------------------------------
 * Signing FSM states (used by task_sign and legacy task_security)
 *-----------------------------------------------------------------------------*/
/**
 * @brief Signing workflow states for UI updates and optional legacy FSM.
 * @details @c task_sign.c uses a subset: IDLE, RECEIVED, WAIT_CONFIRM; returns to IDLE after completion.
 */
typedef enum {
    SIGNING_IDLE = 0,          /**< No transaction in progress. */
    SIGNING_RECEIVED,          /**< Request accepted from queue, shown to user. */
    SIGNING_WAIT_CONFIRM,      /**< Waiting for USER confirm/reject or timeout. */
    SIGNING_IN_PROGRESS,       /**< Crypto operations running (legacy path). */
    SIGNING_DONE,              /**< Success terminal state (legacy). */
    SIGNING_REJECTED,          /**< User or policy rejected. */
    SIGNING_ERROR              /**< Validation or crypto failure. */
} signing_state_t;

/*-----------------------------------------------------------------------------
 * Event Group bits (task_user -> task_sign)
 *-----------------------------------------------------------------------------*/
/** @brief Short USER press: user confirmed signing (@c task_user.c sets this bit). */
#define EVENT_USER_CONFIRMED    (1U << 0)
/** @brief Long USER hold: user rejected signing. */
#define EVENT_USER_REJECTED     (1U << 1)

/*-----------------------------------------------------------------------------
 * Global handles (created in main.c, used by tasks)
 *-----------------------------------------------------------------------------*/
/**
 * @brief Queue of @c wallet_tx_t from network/WebUSB to signing task.
 * @details Depth 4 in @c main.c . Producer: @c task_net.c ; consumer: @c task_sign.c .
 */
extern QueueHandle_t     g_tx_queue;
/**
 * @brief Queue of @c Transaction_Data_t for display updates (e.g. pending tx from net).
 * @details Producer: @c task_net.c ; consumer: @c task_display.c .
 */
extern QueueHandle_t     g_display_queue;
/**
 * @brief USER button events for @c task_sign.c (@c EVENT_USER_CONFIRMED / @c EVENT_USER_REJECTED ).
 * @details Set by @c task_user.c ; waited on in @c task_sign.c with auto-clear.
 */
extern EventGroupHandle_t g_user_event_group;
/**
 * @brief Mutex for SSD1306 I2C bus (exclusive access from display task).
 */
extern SemaphoreHandle_t g_i2c_mutex;
/**
 * @brief Protects @c UI_Display_Data_t merge inside @c task_display.c .
 */
extern SemaphoreHandle_t g_ui_mutex;
/** @brief Live display fields updated by net/sign tasks; read under @c g_display_ctx_mutex . */
extern display_context_t g_display_ctx;
/** @brief Serializes access to @c g_display_ctx . */
extern SemaphoreHandle_t g_display_ctx_mutex;
/**
 * @brief Non-zero when any task signals a security fault (validation fail, no seed, sign error).
 * @details @c task_io.c drives LED3 from this flag. Cleared after successful sign in @c task_sign.c .
 */
extern volatile uint8_t  g_security_alert;

/** @brief Last ECDSA compact signature (64 bytes: r||s), valid if @c g_last_sig_ready is set. */
extern uint8_t  g_last_sig[64];
/** @brief 1 = @c g_last_sig contains a fresh signature for HTTP/WebUSB polling; 0 otherwise. */
extern volatile uint8_t  g_last_sig_ready;

#ifdef __cplusplus
}
#endif

#endif /* __WALLET_SHARED_H */
