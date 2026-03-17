/**
  ******************************************************************************
  * @file    app_ethernet.h
  * @brief   Ethernet link callback and DHCP thread interface.
  ******************************************************************************
  * @details Implemented in app_ethernet_cw.c. Used by ethernetif and task_net.
  ******************************************************************************
  */

#ifndef __APP_ETHERNET_H
#define __APP_ETHERNET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"

/** @brief DHCP FSM states */
#define DHCP_OFF              ((uint8_t)0)
#define DHCP_START            ((uint8_t)1)
#define DHCP_WAIT_ADDRESS     ((uint8_t)2)
#define DHCP_ADDRESS_ASSIGNED ((uint8_t)3)
#define DHCP_TIMEOUT          ((uint8_t)4)
#define DHCP_LINK_DOWN        ((uint8_t)5)

/**
 * @brief LwIP link status callback. Called when Ethernet link goes up/down.
 * @param netif LwIP netif.
 */
void ethernet_link_status_updated(struct netif *netif);

/** @brief Get LED2 (network) state. */
uint8_t ethernet_led2_is_on(void);

/** @brief Get LED3 (link down) state. */
uint8_t ethernet_led3_is_on(void);

#if LWIP_DHCP
/**
 * @brief DHCP client thread. FSM: START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT.
 * @param argument Pointer to struct netif.
 */
void DHCP_Thread(void *argument);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APP_ETHERNET_H */
