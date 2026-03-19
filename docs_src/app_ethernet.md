\page app_ethernet "app_ethernet: Ethernet glue interfaces (link callback + DHCP FSM)"

# `Core/Inc/app_ethernet.h`

<brief>The `app_ethernet` header defines Ethernet "glue" layer interfaces: it declares callback `ethernet_link_status_updated()`, LED state reading functions, and when `LWIP_DHCP` enabled, declares `DHCP_Thread()` and set of DHCP FSM state constants used by implementation in `Src/app_ethernet_cw.c`.</brief>

## Overview

The `app_ethernet` header defines Ethernet "glue" layer interfaces: it declares callback `ethernet_link_status_updated()`, LED state reading functions, and when `LWIP_DHCP` enabled, declares `DHCP_Thread()` and set of DHCP FSM state constants used by implementation in `Src/app_ethernet_cw.c`.

## Logic Flow (Contract + State Model)

Run-time state machine implemented in `app_ethernet_cw.c`, but state formalized here as constants:

| State | Value | Meaning |
|-------|-------|---------|
| `DHCP_OFF` | 0 | DHCP not active |
| `DHCP_START` | 1 | transition to start DHCP after link-up |
| `DHCP_WAIT_ADDRESS` | 2 | waiting for lease/address |
| `DHCP_ADDRESS_ASSIGNED` | 3 | address received |
| `DHCP_TIMEOUT` | 4 | DHCP failed, static fallback chosen |
| `DHCP_LINK_DOWN` | 5 | link down, DHCP must stop/reset |

## Interrupts and Registers

Header contains no ISR and registers — only declarations.

## Dependencies

Declaration of functions and state constants:
- `ethernet_link_status_updated()` — called by LwIP on link change
- `DHCP_Thread()` — thread main loop for DHCP state machine
- State enum values used by `app_ethernet_cw.c`

## Module Relationships

- `app_ethernet_cw.md` (implementation of Ethernet glue)
- `task_net.md` (uses link callbacks)
- `task_io.md` (LED management based on link state)
