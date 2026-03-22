/**
  ******************************************************************************
  * @file    app_ethernet_cw.c
  * @brief   Ethernet glue — link callbacks, DHCP state machine, LED feedback.
  ******************************************************************************
  * @details
  *          **Support module:** Bridges LwIP/netif to board LEDs and user feedback.
  *          @c ethernet_link_status_updated() on link change; DHCP thread when
  *          @c LWIP_DHCP . LED2 = network activity hint, LED3 = link-down warning.
  *          Logs through @c Task_Display_Log() . Starts @c time_service_start() when
  *          appropriate. Ported from lwip_zero.
  ******************************************************************************
  */
#include "main.h"
#include "hw_init.h"
#include "task_display.h"
#include "app_log.h"
#include "time_service.h"
#include "cmsis_os2.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/netifapi.h"
#include "app_ethernet.h"
#include "ethernetif.h"
#include <stdio.h>

#if LWIP_DHCP
#include "lwip/dhcp.h"
#define MAX_DHCP_TRIES 12   /* 4 was too low; DHCP can need 15–30 s on slow networks */
__IO uint8_t DHCP_state = DHCP_OFF;
#endif

static volatile uint8_t s_led2_on = 0U;
static volatile uint8_t s_led3_on = 0U;
static int8_t s_last_link_up = -1;

/**
 * @brief Set LED2 (network) and LED3 (link down) state.
 * @param led2_on 1 = LED2 on.
 * @param led3_on 1 = LED3 on.
 */
static void ethernet_set_led(uint8_t led2_on, uint8_t led3_on)
{
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, led2_on ? LED2_ON_LEVEL : LED2_OFF_LEVEL);
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, led3_on ? LED3_ON_LEVEL : LED3_OFF_LEVEL);
    s_led2_on = led2_on;
    s_led3_on = led3_on;
}

/**
 * @brief Get LED2 (network) state.
 * @return 1 if on, 0 otherwise.
 */
uint8_t ethernet_led2_is_on(void)
{
    return s_led2_on;
}

/**
 * @brief Get LED3 (link down) state.
 * @return 1 if on, 0 otherwise.
 */
uint8_t ethernet_led3_is_on(void)
{
    return s_led3_on;
}

/**
 * @brief Log netif IP, netmask, gateway to UART.
 * @param prefix Prefix string (e.g. "[DHCP] Assigned").
 * @param netif  LwIP netif.
 */
static void ethernet_log_netif_addr(const char *prefix, const struct netif *netif)
{
    char ip_buf[16], mask_buf[16], gw_buf[16], line[128];

    if (ip4addr_ntoa_r(netif_ip4_addr(netif), ip_buf, sizeof(ip_buf)) == NULL)
        (void)snprintf(ip_buf, sizeof(ip_buf), "0.0.0.0");
    if (ip4addr_ntoa_r(netif_ip4_netmask(netif), mask_buf, sizeof(mask_buf)) == NULL)
        (void)snprintf(mask_buf, sizeof(mask_buf), "0.0.0.0");
    if (ip4addr_ntoa_r(netif_ip4_gw(netif), gw_buf, sizeof(gw_buf)) == NULL)
        (void)snprintf(gw_buf, sizeof(gw_buf), "0.0.0.0");

    (void)snprintf(line, sizeof(line), "[INFO] %s IP=%s MASK=%s GW=%s", prefix, ip_buf, mask_buf, gw_buf);
    Task_Display_Log(line);
}

/**
 * @brief LwIP link callback. Called when Ethernet link goes up/down.
 * @param netif LwIP netif.
 * @return None.
 */
void ethernet_link_status_updated(struct netif *netif)
{
    const int8_t link_up = (netif_is_link_up(netif) != 0U) ? 1 : 0;

    if (s_last_link_up == link_up) return;
    s_last_link_up = link_up;

    if (link_up) {
        APP_LOG_INFO("[ETH] Link up");
        APP_LOG_WALLET_SUB_INFO("ETH");
#if LWIP_DHCP
        DHCP_state = DHCP_START;
#else
        ethernet_set_led(1, 0);
        time_service_start();
#endif
    } else {
        APP_LOG_INFO("[ETH] Link down");
#if LWIP_DHCP
        DHCP_state = DHCP_LINK_DOWN;
#else
        ethernet_set_led(0, 1);
#endif
    }
}

#if LWIP_DHCP
/**
 * @brief DHCP client thread. FSM: START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT.
 * @param argument Pointer to struct netif.
 * @return None.
 */
void DHCP_Thread(void *argument)
{
    struct netif *netif = (struct netif *)argument;
    ip_addr_t ipaddr, netmask, gw;
    struct dhcp *dhcp;

    for (;;) {
        switch (DHCP_state) {
        case DHCP_START:
            APP_LOG_INFO("[DHCP] Start");
            ip_addr_set_zero_ip4(&netif->ip_addr);
            ip_addr_set_zero_ip4(&netif->netmask);
            ip_addr_set_zero_ip4(&netif->gw);
            DHCP_state = DHCP_WAIT_ADDRESS;
            ethernet_set_led(0, 0);
            netifapi_dhcp_start(netif);
            break;

        case DHCP_WAIT_ADDRESS:
            if (dhcp_supplied_address(netif)) {
                DHCP_state = DHCP_ADDRESS_ASSIGNED;
                ethernet_log_netif_addr("[DHCP] Assigned", netif);
                time_service_start();
                ethernet_set_led(1, 0);
            } else {
                dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
                if (dhcp != NULL && dhcp->tries > MAX_DHCP_TRIES) {
                    DHCP_state = DHCP_TIMEOUT;
                    IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
                    IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
                    IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
                    netifapi_netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
                    ethernet_log_netif_addr("[DHCP] Timeout, static", netif);
                    time_service_start();
                    ethernet_set_led(1, 0);
                }
            }
            break;

        case DHCP_LINK_DOWN:
            DHCP_state = DHCP_OFF;
            netifapi_dhcp_stop(netif);
            ethernet_set_led(0, 1);
            break;

        default:
            break;
        }
        osDelay(500);
    }
}
#endif /* LWIP_DHCP */
