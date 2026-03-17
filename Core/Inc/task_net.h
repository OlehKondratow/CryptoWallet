/**
  ******************************************************************************
  * @file    task_net.h
  * @brief   Connectivity module - LwIP stack, HTTP server for transaction data.
  ******************************************************************************
  * @details LwIP Ethernet integration. HTTP server thread receives POST /tx
  *          with JSON body, decodes and sends to task_security via g_tx_queue.
  *          When USE_LWIP is undefined, Task_Net_Create is a no-op.
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
