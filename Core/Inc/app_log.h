/**
 ******************************************************************************
 * @file    app_log.h
 * @brief   Unified UART log: levels [ERR]/[WARN]/[INFO]/[DBG] and subsystem tag.
 ******************************************************************************
 * @details
 *          String literals: @c APP_LOG_ERR("...") , @c APP_LOG_INFO("[NET] ...") .
 *          Dynamic strings: @c App_Log_InfoMsg(buf) — prepends the level prefix.
 *          @c APP_LOG_DBG is disabled when @c APP_LOG_ENABLE_DBG=0 (default).
 *
 *          See also @c Task_Display_Log() — low-level output without a level prefix.
 ******************************************************************************
 */

#ifndef APP_LOG_H
#define APP_LOG_H

#include "task_display.h"

#ifndef APP_LOG_ENABLE_DBG
#define APP_LOG_ENABLE_DBG 0
#endif

/** Literals: level prefix concatenated with the string at compile time */
#define APP_LOG_ERR(lit) Task_Display_Log("[ERR] " lit)
#define APP_LOG_WARN(lit) Task_Display_Log("[WARN] " lit)
#define APP_LOG_INFO(lit) Task_Display_Log("[INFO] " lit)
#if APP_LOG_ENABLE_DBG
#define APP_LOG_DBG(lit) Task_Display_Log("[DBG] " lit)
#else
#define APP_LOG_DBG(lit) ((void)0)
#endif

/** Dynamic string (full message without a level prefix) */
void App_Log_InfoMsg(const char *msg);
void App_Log_WarnMsg(const char *msg);
void App_Log_ErrMsg(const char *msg);
#if APP_LOG_ENABLE_DBG
void App_Log_DbgMsg(const char *msg);
#else
#define App_Log_DbgMsg(msg) ((void)(msg))
#endif

/**
 * @brief Wallet summary lines for UART and CI (`scripts/ci/uart_boot_markers.txt`).
 * @details MAIN — core ready before @c osKernelStart; the rest after tasks start.
 *          Format: @c [INFO] [WALLET] MAIN ok | @c [INFO] [WALLET] &lt;NAME&gt; info .
 */
#define APP_LOG_WALLET_MAIN_OK() APP_LOG_INFO("[WALLET] MAIN ok")
#define APP_LOG_WALLET_SUB_INFO(name_lit) APP_LOG_INFO("[WALLET] " name_lit " info")

#endif /* APP_LOG_H */
