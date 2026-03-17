/**
  ******************************************************************************
  * @file    task_net.c
  * @brief   Connectivity module - LwIP stack, HTTP server for transaction data.
  ******************************************************************************
  * @details LwIP Ethernet integration. HTTP server on port 80. POST /tx with
  *          JSON: {"recipient":"addr","amount":"0.1","currency":"BTC"}.
  *          Decodes Recipient, Amount, Currency and sends to task_security
  *          via g_tx_queue. When USE_LWIP undefined, no task created.
  ******************************************************************************
  */

#include "task_net.h"
#include "main.h"
#include "hw_init.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_LWIP
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "lwip/sys.h"
#include "ethernetif.h"
#include "app_ethernet.h"
#endif

#define NET_STACK_SIZE      1024U   /* Same as lwip-uaid StartThread (256*4) */
#define NET_PRIORITY        (tskIDLE_PRIORITY + 4)  /* Higher than Disp — init first, like StartThread */
#define HTTP_PORT           80
#define REQ_BUF_SIZE        512U
#define BODY_MAX            256U

#ifdef USE_LWIP
static struct netif g_netif;

/**
 * @brief Parse JSON-like body for recipient, amount, currency.
 * @param body  Request body string.
 * @param tx    Output transaction struct.
 * @return 0 on success, -1 on parse error.
 */
static int parse_tx_body(const char *body, wallet_tx_t *tx);

/**
 * @brief HTTP server thread - accept connections, handle POST /tx.
 * @param arg  Unused.
 * @return None.
 */
static void http_server_thread(void *arg);

/**
 * @brief Network task - init LwIP, start HTTP server.
 * @param pvParameters  Unused.
 * @return None.
 */
static void net_task(void *pvParameters);
#endif

/**
 * @brief Create and start the network task.
 * @return None.
 */
void Task_Net_Create(void)
{
#ifdef USE_LWIP
    xTaskCreate(net_task, "Net", NET_STACK_SIZE, NULL, NET_PRIORITY, NULL);
#else
    (void)0;
#endif
}

#ifdef USE_LWIP
static int parse_tx_body(const char *body, wallet_tx_t *tx)
{
    if (body == NULL || tx == NULL) return -1;
    memset(tx, 0, sizeof(*tx));

    const char *r = strstr(body, "\"recipient\"");
    const char *a = strstr(body, "\"amount\"");
    const char *c = strstr(body, "\"currency\"");

    if (r != NULL) {
        const char *start = strchr(r, '"');
        if (start != NULL) {
            start = strchr(start + 1, '"');
            if (start != NULL) {
                start++;
                const char *end = strchr(start, '"');
                size_t len = (end != NULL) ? (size_t)(end - start) : 0;
                if (len >= TX_RECIPIENT_LEN) len = TX_RECIPIENT_LEN - 1;
                if (len > 0) memcpy(tx->recipient, start, len);
            }
        }
    }
    if (a != NULL) {
        const char *start = strchr(a, '"');
        if (start != NULL) {
            start = strchr(start + 1, '"');
            if (start != NULL) {
                start++;
                const char *end = strchr(start, '"');
                size_t len = (end != NULL) ? (size_t)(end - start) : 0;
                if (len >= TX_AMOUNT_LEN) len = TX_AMOUNT_LEN - 1;
                if (len > 0) memcpy(tx->amount, start, len);
            }
        }
    }
    if (c != NULL) {
        const char *start = strchr(c, '"');
        if (start != NULL) {
            start = strchr(start + 1, '"');
            if (start != NULL) {
                start++;
                const char *end = strchr(start, '"');
                size_t len = (end != NULL) ? (size_t)(end - start) : 0;
                if (len >= TX_CURRENCY_LEN) len = TX_CURRENCY_LEN - 1;
                if (len > 0) memcpy(tx->currency, start, len);
            }
        }
    }
    if (tx->currency[0] == '\0') (void)strncpy(tx->currency, "BTC", TX_CURRENCY_LEN - 1);
    return 0;
}

static void http_server_thread(void *arg)
{
    struct netconn *listener = NULL;
    struct netconn *conn = NULL;
    (void)arg;

    listener = netconn_new(NETCONN_TCP);
    if (listener == NULL) return;
    if (netconn_bind(listener, NULL, HTTP_PORT) != ERR_OK) {
        netconn_delete(listener);
        return;
    }
    if (netconn_listen(listener) != ERR_OK) {
        netconn_delete(listener);
        return;
    }

    Task_Display_Log("HTTP :80");

    for (;;) {
        err_t err = netconn_accept(listener, &conn);
        if (err != ERR_OK || conn == NULL) continue;

        netconn_set_recvtimeout(conn, 2000);
        struct netbuf *inbuf = NULL;
        err = netconn_recv(conn, &inbuf);
        if (err == ERR_OK && inbuf != NULL) {
            void *buf = NULL;
            u16_t len = 0;
            netbuf_data(inbuf, &buf, &len);
            if (buf != NULL && len > 0 && len < REQ_BUF_SIZE) {
                char req[REQ_BUF_SIZE];
                size_t copy = (len < REQ_BUF_SIZE - 1) ? len : REQ_BUF_SIZE - 1;
                memcpy(req, buf, copy);
                req[copy] = '\0';

                if (strstr(req, "POST") != NULL && strstr(req, "/tx") != NULL) {
                    char *body = strstr(req, "\r\n\r\n");
                    if (body != NULL) {
                        body += 4;
                        wallet_tx_t tx;
                        if (parse_tx_body(body, &tx) == 0) {
                            if (xQueueSend(g_tx_queue, &tx, pdMS_TO_TICKS(100)) == pdTRUE) {
                                Task_Display_Log("TX enqueued");
                            }
                            /* Push to Display queue: Transaction_Data_t */
                            Transaction_Data_t disp_tx;
                            memset(&disp_tx, 0, sizeof(disp_tx));
                            (void)snprintf(disp_tx.coin_name, sizeof(disp_tx.coin_name), "%s",
                                           tx.currency[0] ? tx.currency : "BTC");
                            disp_tx.amount = (tx.amount[0] != '\0') ? atof(tx.amount) : 0.0;
                            (void)snprintf(disp_tx.recipient, sizeof(disp_tx.recipient), "%s", tx.recipient);
                            disp_tx.is_pending = 1;
                            (void)xQueueSend(g_display_queue, &disp_tx, pdMS_TO_TICKS(100));
                        }
                    }
                }

                static const char resp[] =
                    "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
                    "Connection: close\r\nContent-Length: 2\r\n\r\nOK";
                (void)netconn_write(conn, resp, sizeof(resp) - 1, NETCONN_COPY);
            }
            netbuf_delete(inbuf);
        }
        netconn_close(conn);
        netconn_delete(conn);
        conn = NULL;
    }
}

static void net_task(void *pvParameters)
{
    (void)pvParameters;
    ip_addr_t ipaddr, netmask, gw;

    /* Same order as lwip-uaid-SSD1306 StartThread: tcpip_init FIRST, no delay */
    Task_Display_Log("Net: start");
    Task_Display_Log("Net: tcpip_init...");
    tcpip_init(NULL, NULL);
    Task_Display_Log("Net: tcpip OK");

#if LWIP_DHCP
    ip_addr_set_zero_ip4(&ipaddr);
    ip_addr_set_zero_ip4(&netmask);
    ip_addr_set_zero_ip4(&gw);
#else
    IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
    IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif

    netif_add(&g_netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
    netif_set_default(&g_netif);
#if !LWIP_NETIF_LINK_CALLBACK
    netif_set_up(&g_netif);
    netif_set_link_up(&g_netif);
#endif
    ethernet_link_status_updated(&g_netif);

#if LWIP_NETIF_LINK_CALLBACK
    netif_set_link_callback(&g_netif, ethernet_link_status_updated);
    sys_thread_new("EthLink", ethernet_link_thread, &g_netif, DEFAULT_THREAD_STACKSIZE, 1);
#endif

#if LWIP_DHCP
    sys_thread_new("DHCP", DHCP_Thread, &g_netif, 1024, 1);  /* 1024 stack: 512 was too small */
#endif

    if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        const ip4_addr_t *ip = netif_ip4_addr(&g_netif);
        (void)snprintf(g_display_ctx.ip_addr, sizeof(g_display_ctx.ip_addr),
                      "%d.%d.%d.%d", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
        (void)snprintf(g_display_ctx.mac_addr, sizeof(g_display_ctx.mac_addr),
                      "%02X:%02X:%02X", (unsigned)g_netif.hwaddr[0], (unsigned)g_netif.hwaddr[1],
                      (unsigned)g_netif.hwaddr[2]);
        g_display_ctx.hid_connected = false;
        xSemaphoreGive(g_display_ctx_mutex);
    }

    sys_thread_new("HTTP", http_server_thread, NULL, 2048, 2);  /* 2048 stack for HTTP request handling */
    Task_Display_Log("Net: HTTP ready");

#if LWIP_ALIVE_LOG
    TickType_t last_log = xTaskGetTickCount();
#endif
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        /* Update display context when DHCP assigns IP */
        if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            const ip4_addr_t *ip = netif_ip4_addr(&g_netif);
            if (!ip4_addr_isany_val(*ip)) {
                (void)snprintf(g_display_ctx.ip_addr, sizeof(g_display_ctx.ip_addr),
                              "%d.%d.%d.%d", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
            }
            xSemaphoreGive(g_display_ctx_mutex);
        }
#if LWIP_ALIVE_LOG
        if ((xTaskGetTickCount() - last_log) >= pdMS_TO_TICKS(10000U)) {
            Task_Display_Log("Net: alive");
            last_log = xTaskGetTickCount();
        }
#endif
    }
}
#endif
