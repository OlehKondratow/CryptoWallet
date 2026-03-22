/**
  ******************************************************************************
  * @file    task_net.c
  * @brief   LwIP + HTTP — DHCP/static IP, POST /tx, signing poll endpoints.
  ******************************************************************************
  * @details
  *          **Functional role:** Edge of the wallet facing the LAN — TCP/IP, HTTP
  *          server, JSON/form parsing **without** an external JSON library (size).
  *
  *          **HTTP:** Port 80. Body JSON or form: recipient, amount, currency.
  *          Validated via @c tx_request_validate.c , then @c wallet_tx_t to
  *          @c g_tx_queue → @c task_sign.c (not @c task_security.c ).
  *          HTTP API spec: @c docs_src/HTTP_API_ru.md , @c docs_src/HTTP_API_en.md .
  *
  *          **IP:** DHCP with static fallback (e.g. 192.168.0.10) in @c main.h .
  *          **Build:** @c USE_LWIP ; BSP from Cube / @c stm32_secure_boot template paths.
  *
  *          **Tests:** @c docs_src/testing-signing.md , @c scripts/bootloader_secure_signing_test.py .
  ******************************************************************************
  */

#include "task_net.h"
#include "main.h"
#include "hw_init.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "app_log.h"
#include "tx_request_validate.h"
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

#define NET_STACK_SIZE      1024U
#define NET_PRIORITY        (tskIDLE_PRIORITY + 4)
#define HTTP_PORT           80
#define REQ_BUF_SIZE        1024U
#define BODY_MAX            512U

#ifdef USE_LWIP
static struct netif g_netif;

/**
 * @brief Parse JSON body for recipient, amount, currency.
 */
static int parse_tx_json(const char *body, wallet_tx_t *tx);

/**
 * @brief Parse form-urlencoded body (recipient=...&amount=...&currency=...).
 */
static int parse_tx_form(const char *body, size_t body_len, wallet_tx_t *tx);

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
static int parse_tx_json(const char *body, wallet_tx_t *tx)
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

static int parse_tx_form(const char *body, size_t body_len, wallet_tx_t *tx)
{
    if (body == NULL || tx == NULL) return -1;
    memset(tx, 0, sizeof(*tx));

    static const char key_recipient[] = "recipient=";
    static const char key_amount[] = "amount=";
    static const char key_currency[] = "currency=";

    const char *p = body;
    const char *end = body + body_len;

    while (p < end) {
        if (strncmp(p, key_recipient, sizeof(key_recipient) - 1) == 0) {
            p += sizeof(key_recipient) - 1;
            const char *v_end = p;
            while (v_end < end && *v_end != '&' && *v_end != '\0') v_end++;
            size_t len = (size_t)(v_end - p);
            if (len >= TX_RECIPIENT_LEN) len = TX_RECIPIENT_LEN - 1;
            if (len > 0) {
                memcpy(tx->recipient, p, len);
                tx->recipient[len] = '\0';
            }
            p = v_end;
        } else if (strncmp(p, key_amount, sizeof(key_amount) - 1) == 0) {
            p += sizeof(key_amount) - 1;
            const char *v_end = p;
            while (v_end < end && *v_end != '&' && *v_end != '\0') v_end++;
            size_t len = (size_t)(v_end - p);
            if (len >= TX_AMOUNT_LEN) len = TX_AMOUNT_LEN - 1;
            if (len > 0) {
                memcpy(tx->amount, p, len);
                tx->amount[len] = '\0';
            }
            p = v_end;
        } else if (strncmp(p, key_currency, sizeof(key_currency) - 1) == 0) {
            p += sizeof(key_currency) - 1;
            const char *v_end = p;
            while (v_end < end && *v_end != '&' && *v_end != '\0') v_end++;
            size_t len = (size_t)(v_end - p);
            if (len >= TX_CURRENCY_LEN) len = TX_CURRENCY_LEN - 1;
            if (len > 0) {
                memcpy(tx->currency, p, len);
                tx->currency[len] = '\0';
            }
            p = v_end;
        } else {
            while (p < end && *p != '&') p++;
        }
        if (p < end && *p == '&') p++;
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

    APP_LOG_INFO("[NET] HTTP server listening :80");

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
                size_t copy = (len < REQ_BUF_SIZE - 1) ? (size_t)len : REQ_BUF_SIZE - 1;
                memcpy(req, buf, copy);
                req[copy] = '\0';

                int is_get = (strstr(req, "GET ") == req);
                int is_post = (strstr(req, "POST ") == req);
                char *path = strchr(req, ' ');
                char *path_end = path ? strchr(path + 1, ' ') : NULL;
                size_t path_len = path_end ? (size_t)(path_end - path - 1) : 0;

                if (is_get && path != NULL && path_len >= 1) {
                    const char *p = path + 1;

                    if (path_len >= 5 && strncmp(p, "/ping", 5) == 0) {
                        /* GET /ping — simple connectivity test */
                        APP_LOG_INFO("[HTTP] GET /ping -> pong");
                        static const char pong[] =
                            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
                            "Connection: close\r\nContent-Length: 4\r\n\r\npong";
                        (void)netconn_write(conn, pong, sizeof(pong) - 1, NETCONN_COPY);
                    } else if (path_len == 1 && p[0] == '/') {
                        /* GET / — HTML form */
                        APP_LOG_INFO("[HTTP] GET /");
                        static const char html[] =
                            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                            "Connection: close\r\n\r\n"
                            "<!DOCTYPE html><html><head><meta charset=utf-8>"
                            "<title>CryptoWallet — Sign TX</title></head><body>"
                            "<h1>Sign Transaction</h1>"
                            "<form method=post action=/tx>"
                            "<p>Recipient: <input name=recipient size=42 placeholder=1A1zP1...></p>"
                            "<p>Amount: <input name=amount size=16 placeholder=0.001></p>"
                            "<p>Currency: <input name=currency size=8 value=BTC></p>"
                            "<p><button type=submit>Submit for signing</button></p>"
                            "</form>"
                            "<hr><h2>Last signed signature</h2>"
                            "<p><a href=/tx/signed>Refresh</a></p>"
                            "<pre id=sig>—</pre>"
                            "<script>fetch('/tx/signed').then(r=>r.json()).then(d=>{"
                            "document.getElementById('sig').textContent=d.sig||d.status||'—';"
                            "}).catch(()=>{});</script>"
                            "</body></html>";
                        (void)netconn_write(conn, html, sizeof(html) - 1, NETCONN_COPY);
                    } else if (path_len >= 10 && strncmp(p, "/tx/signed", 10) == 0) {
                        /* GET /tx/signed — JSON with signature hex */
                        Task_Display_Log("[HTTP] GET /tx/signed");
                        char json[256];
                        if (g_last_sig_ready) {
                            char hex[132];
                            for (int i = 0; i < 64; i++)
                                (void)snprintf(hex + i * 2, 4, "%02X", g_last_sig[i]);
                            hex[128] = '\0';
                            (void)snprintf(json, sizeof(json),
                                "{\"status\":\"signed\",\"sig\":\"%s\"}", hex);
                        } else {
                            (void)snprintf(json, sizeof(json),
                                "{\"status\":\"pending\"}");
                        }
                        (void)snprintf(req, REQ_BUF_SIZE,
                            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
                            "Connection: close\r\nContent-Length: %u\r\n\r\n%s",
                            (unsigned)strlen(json), json);
                        (void)netconn_write(conn, req, strlen(req), NETCONN_COPY);
                    } else {
                        APP_LOG_WARN("[HTTP] 404");
                        static const char notfound[] =
                            "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
                        (void)netconn_write(conn, notfound, sizeof(notfound) - 1, NETCONN_COPY);
                    }
                } else if (is_post && path != NULL && path_len >= 3 &&
                           strncmp(path + 1, "/tx", 3) == 0) {
                    /* POST /tx */
                    char *body = strstr(req, "\r\n\r\n");
                    wallet_tx_t tx;
                    int parsed = 0;
                    if (body != NULL) {
                        body += 4;
                        size_t body_len = (size_t)(req + copy - body);
                        if (strstr(req, "application/json") != NULL) {
                            parsed = (parse_tx_json(body, &tx) == 0);
                        } else {
                            parsed = (parse_tx_form(body, body_len, &tx) == 0);
                        }
                    }
                    if (parsed) {
                        if (tx_request_validate(&tx) != TX_VALID_OK) {
                            APP_LOG_WARN("[HTTP] TX invalid");
                        } else if (xQueueSend(g_tx_queue, &tx, pdMS_TO_TICKS(100)) == pdTRUE) {
                            APP_LOG_INFO("[HTTP] TX enqueued");
                            Transaction_Data_t disp_tx;
                            memset(&disp_tx, 0, sizeof(disp_tx));
                            (void)snprintf(disp_tx.coin_name, sizeof(disp_tx.coin_name), "%s",
                                           tx.currency[0] ? tx.currency : "BTC");
                            disp_tx.amount = (tx.amount[0] != '\0') ? atof(tx.amount) : 0.0;
                            (void)snprintf(disp_tx.recipient, sizeof(disp_tx.recipient), "%s", tx.recipient);
                            disp_tx.is_pending = 1;
                            (void)xQueueSend(g_display_queue, &disp_tx, pdMS_TO_TICKS(100));
                        }
                    } else {
                        APP_LOG_WARN("[HTTP] POST /tx invalid body");
                    }
                    static const char resp[] =
                        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                        "Connection: close\r\n\r\n"
                        "<!DOCTYPE html><html><head><meta charset=utf-8>"
                        "<meta http-equiv=refresh content=2;url=/>"
                        "</head><body><p>Transaction submitted. Confirm on device. "
                        "<a href=/>Back</a></p></body></html>";
                    (void)netconn_write(conn, resp, sizeof(resp) - 1, NETCONN_COPY);
                } else {
                    static const char resp[] =
                        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
                        "Connection: close\r\nContent-Length: 2\r\n\r\nOK";
                    (void)netconn_write(conn, resp, sizeof(resp) - 1, NETCONN_COPY);
                }
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
    APP_LOG_INFO("[NET] task started");
    APP_LOG_INFO("[NET] tcpip_init...");
    tcpip_init(NULL, NULL);
    APP_LOG_INFO("[NET] tcpip OK");

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

    sys_thread_new("HTTP", http_server_thread, NULL, 16384, 2);  /* 16K stack for HTTP (was 2K, 8x increase) */
    APP_LOG_INFO("[NET] HTTP stack ready");
    APP_LOG_WALLET_SUB_INFO("NET");

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
            APP_LOG_INFO("[NET] alive");
            last_log = xTaskGetTickCount();
        }
#endif
    }
}
#endif
