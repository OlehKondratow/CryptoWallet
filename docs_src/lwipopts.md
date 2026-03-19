\page lwipopts "lwipopts: LwIP compile-time options (DHCP + SNTP)"

# `Core/Inc/lwipopts.h`

<brief>Header `lwipopts` defines compile-time configuration for LwIP: enables IPv4/TCP/DHCP/DNS/SNTP, configures heap size (`LWIP_RAM_HEAP_POINTER` and `MEM_SIZE`), TCP buffer/window parameters, and binds SNTP time updates to `time_service_set_epoch()`.</brief>

## Overview

<brief>Header `lwipopts` defines compile-time configuration for LwIP: enables IPv4/TCP/DHCP/DNS/SNTP, configures heap size (`LWIP_RAM_HEAP_POINTER` and `MEM_SIZE`), TCP buffer/window parameters, and binds SNTP time updates to `time_service_set_epoch()`.</brief>

## Abstract (Logic Synthesis)

An embedded network stack is not just "what packets are sent" but also "what resources and structures are available". `lwipopts.h` fixes these resources and behaviors:

- Which LwIP subsystems are compiled
- Where the heap is located in memory
- How PBUF and TCP window buffers are structured
- How SNTP integrates with the application via `SNTP_SET_SYSTEM_TIME(sec)`

The config file's business goal: reconcile LwIP with STM32H7 constraints (D2 SRAM memory and cache/MPU interaction) and with the application's `time_service`.

## Logic Flow (Compile-time "State Machine")

This is not a runtime state machine. Logic is in sets of macros that define subsystem behavior:

| Parameter Group | Key Macros | Purpose |
|---|---|---|
| UART alive logging | `LWIP_ALIVE_LOG` | Print "alive" periodically in LwIP path |
| Heap/Memory | `MEM_ALIGNMENT`, `MEM_SIZE`, `LWIP_RAM_HEAP_POINTER` | Heap size and placement |
| Threading | `TCPIP_THREAD_*`, `DEFAULT_THREAD_STACKSIZE` | Internal TCP/IP thread parameters |
| Protocol enable | `LWIP_IPV4`, `LWIP_TCP`, `LWIP_DHCP`, `LWIP_SNTP`, `LWIP_DNS` | Which subsystems compile |
| Timing integration | `SNTP_SET_SYSTEM_TIME(sec)` | Calls `time_service_set_epoch(sec)` |
| Link callback | `LWIP_NETIF_LINK_CALLBACK` (via `LWIP_NO_LINK_THREAD`) | How LwIP reports link changes |

## Interrupts/Registers

None. This is compile-time configuration.

## Timings and Critical Conditions

| Parameter | Value | Context |
|---|---|---|
| `SNTP_UPDATE_DELAY` | `15*60*1000` ms | Time update period |
| `MEM_SIZE` | `14*1024` bytes | Affects LwIP connection serving and buffer allocation |
| TCP buffer/window | `TCP_SND_BUF`, `TCP_WND` | Impacts throughput |

## Dependencies

Direct:
- `time_service_set_epoch()` declaration from `time_service.h`

Indirect:
- `hw_init` for proper MPU/cache in `USE_LWIP` branch (LwIP heap placed at `0x30004000` as part of MPU setup)

## Relations

- `hw_init.md` — Why heap/caches must align with LwIP
- `time_service.md` — SNTP epoch handling via `time_service_set_epoch`
- `task_net.md` / `app_ethernet_cw.md` — Runtime DHCP and SNTP startup logic
