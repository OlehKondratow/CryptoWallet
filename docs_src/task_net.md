\page task_net "task_net: LwIP + HTTP server for transaction signing API"
\related Task_Net_Create
\related http_server_thread
\related parse_tx_json
\related parse_tx_form

# `task_net.c` + `task_net.h`

<brief>The `task_net` module is the network facade of the application: it brings up LwIP/Ethernet, starts an HTTP server on port 80, parses JSON/form POST requests (`POST /tx`), validates transactions, and sends them for signing via `g_tx_queue`.</brief>

## Overview

The `task_net` module is the entry point for all external signing requests: a host (PC, mobile, web interface) connects via Ethernet, sends JSON or form data with address/amount/currency, and the module parses, validates, shows on SSD1306, and queues for the signing task. Without LwIP (`USE_LWIP=0`), the module is a no-op. Business role: be the "gateway to the microcontroller" for the network client.

## Logic Flow (HTTP Server FSM)

1. **Initialization:**
   - `Task_Net_Create()` from `main.c` creates a FreeRTOS task with priority `NET_PRIORITY`
   - The `net_task()` initializes `tcpip_init()` (LwIP core)
   - Configures netif: DHCP (if `LWIP_DHCP=1`) or static IP (from `main.h`)
   - Sets callback on `ethernet_link_status_updated()`
   - Starts separate `http_server_thread()` with 16K stack

2. **HTTP Server FSM:**
   - Creates TCP listener on port 80
   - Loop: `netconn_accept()` waits for incoming connection
   - Receives all data in single `netconn_recv()` call (2-sec timeout)

3. **Request Parsing:**
   - Parses HTTP first line: `GET /path` or `POST /path`
   - For `GET /ping`: returns "pong"
   - For `GET /`: returns HTML form for interface
   - For `GET /tx/signed`: returns JSON with last signature (`g_last_sig` + flag `g_last_sig_ready`)
   - For `POST /tx`:
     - Parses body as JSON (looks for `"recipient"`, `"amount"`, `"currency"`) or form (`recipient=...&amount=...&currency=...`)
     - Validates via `tx_request_validate()`
     - If valid, sends to `g_tx_queue` for signing and updates display via `g_display_queue`

4. **Responses:**
   - HTTP 200 with HTML/JSON/text
   - HTTP 404 for unknown paths

## Interrupts and Registers

`task_net` does not touch ISR/registers directly. Uses LwIP API (netconn, netbuf). Interaction with HAL only through `ethernet_link_status_updated()` (from `app_ethernet_cw.c`).

## Timings and Branching Conditions

| Endpoint | Method | Behavior |
|----------|--------|----------|
| `/ping` | GET | Immediately return "pong" |
| `/` | GET | Send HTML form |
| `/tx/signed` | GET | Send JSON with last signature or status |
| `/tx` | POST | Parse body (JSON or form), validate, send to `g_tx_queue` |
| Other | GET/POST | HTTP 404 |

Timing:
- HTTP request timeout: 2 sec (netconn_set_recvtimeout)
- Queue send timeout: 100 ms (pdMS_TO_TICKS(100))
- Task main loop: 2 sec delay for display update (IP after DHCP)

## Dependencies

Direct dependencies:
- **FreeRTOS:** xTaskCreate, xQueueSend, vTaskDelay, xSemaphoreTake/Give
- **LwIP:** netconn API, tcpip_init, netif
- **wallet_shared.h:** `wallet_tx_t`, `g_tx_queue`
- **tx_request_validate.h:** validation before queue
- **task_display.h:** `Task_Display_Log()`, `g_display_queue`, `g_display_ctx_mutex`
- **app_ethernet.h:** `ethernet_link_status_updated()` callback
- Globals from other modules: `g_last_sig[]`, `g_last_sig_ready` (from `task_sign`)

## Module Relationships

- `wallet_shared.md` (wallet_tx_t structure and queue)
- `tx_request_validate.md` (validation before enqueueing)
- `task_sign.md` (consumer from g_tx_queue)
- `task_display.md` (display updates)
- `app_ethernet_cw.md` (link callbacks and LED feedback)
