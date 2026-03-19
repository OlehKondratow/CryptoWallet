\page app_ethernet_cw "app_ethernet_cw: Ethernet link FSM and DHCP state machine"
\related ethernet_link_status_updated
\related DHCP_Thread
\related ethernet_set_led

# `app_ethernet_cw.c`

<brief>The `app_ethernet_cw` module provides Ethernet support: FSM for link-up/link-down states, DHCP client (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), IP address logging.</brief>

## Overview

The `app_ethernet_cw` module provides Ethernet support: FSM for link-up/link-down states, DHCP client (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), IP address logging.

## Logic Flow (Link + DHCP FSM)

### Link-up Callback

`ethernet_link_status_updated(netif)` called when cable connected/disconnected:
1. If link UP and was DOWN:
   - When `LWIP_DHCP=1`: transitions to `DHCP_START` (DHCP-thread starts)
   - When `LWIP_DHCP=0`: immediately enables LED2, starts `time_service_start()`
2. If link DOWN:
   - When `LWIP_DHCP=1`: transitions to `DHCP_LINK_DOWN`
   - When `LWIP_DHCP=0`: enables LED3 (warning)

### DHCP FSM (when `LWIP_DHCP=1`)

`DHCP_Thread(netif)` works as state machine:
1. **DHCP_START:** zeros IP/mask/gw, transitions to WAIT_ADDRESS, turns off both LEDs, calls `netifapi_dhcp_start()`
2. **DHCP_WAIT_ADDRESS:** polls `dhcp_supplied_address()`:
   - If IP received: logs address, enables LED2, starts `time_service_start()`, transitions to ASSIGNED
   - If retry counter exceeded (>12): fallback to static IP from `main.h`, logs, enables LED2, transitions to TIMEOUT
3. **DHCP_LINK_DOWN:** disables DHCP, enables LED3
4. Repeats every 500 ms

## Dependencies

Direct:
- LwIP netif callback mechanism
- DHCP client functions: `netifapi_dhcp_start()`, `dhcp_supplied_address()`
- LED control: LED2/LED3 GPIO writes
- Time service: `time_service_start()` on IP assignment

Indirect:
- `app_ethernet.h` (declares interfaces)
- `task_net.md` (uses IP connectivity)
- `task_io.md` (LED management)

## Module Relationships

- `app_ethernet.md` (declares interfaces)
- `task_net.md` (HTTP server depends on link)
- `task_io.md` (LED policy)
- `time_service.md` (NTP sync after DHCP)
