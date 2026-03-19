/**
  ******************************************************************************
  * @file    task_net.h
  * @brief   LwIP + HTTP — Ethernet stack and port 80 API for transactions.
  ******************************************************************************
  * @details
  *          Parses @c POST /tx (JSON or form), validates, enqueues @c wallet_tx_t to
  *          @c g_tx_queue for @c task_sign.c . Pushes display updates via
  *          @c g_display_queue . May call @c Task_Display_Log() (direct log — common
  *          in embedded). Without @c USE_LWIP , @c Task_Net_Create() is empty.
  ******************************************************************************
  */

#ifndef __TASK_NET_H
#define __TASK_NET_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and start the network task (LwIP HTTP server).
 * @note  Call from main() before vTaskStartScheduler().
 * @note  Requires USE_LWIP, LwIP and ethernetif.
 * @return None.
 */
void Task_Net_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_NET_H */
