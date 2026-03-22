/**
 ******************************************************************************
 * @file    app_log.h
 * @brief   Единый UART-лог: уровни [ERR]/[WARN]/[INFO]/[DBG] и тег подсистемы.
 ******************************************************************************
 * @details
 *          Строковые литералы: @c APP_LOG_ERR("...") , @c APP_LOG_INFO("[NET] ...") .
 *          Динамические строки: @c App_Log_InfoMsg(buf) — добавляет префикс уровня.
 *          @c APP_LOG_DBG отключён, если @c APP_LOG_ENABLE_DBG=0 (по умолчанию).
 *
 *          См. также @c Task_Display_Log() — низкоуровневый вывод без префикса уровня.
 ******************************************************************************
 */

#ifndef APP_LOG_H
#define APP_LOG_H

#include "task_display.h"

#ifndef APP_LOG_ENABLE_DBG
#define APP_LOG_ENABLE_DBG 0
#endif

/** Литералы: префикс уровня склеивается со строкой в compile-time */
#define APP_LOG_ERR(lit) Task_Display_Log("[ERR] " lit)
#define APP_LOG_WARN(lit) Task_Display_Log("[WARN] " lit)
#define APP_LOG_INFO(lit) Task_Display_Log("[INFO] " lit)
#if APP_LOG_ENABLE_DBG
#define APP_LOG_DBG(lit) Task_Display_Log("[DBG] " lit)
#else
#define APP_LOG_DBG(lit) ((void)0)
#endif

/** Динамическая строка (полное сообщение без префикса уровня) */
void App_Log_InfoMsg(const char *msg);
void App_Log_WarnMsg(const char *msg);
void App_Log_ErrMsg(const char *msg);
#if APP_LOG_ENABLE_DBG
void App_Log_DbgMsg(const char *msg);
#else
#define App_Log_DbgMsg(msg) ((void)(msg))
#endif

/**
 * @brief Сводные строки «кошелёк» для UART и CI (`scripts/ci/uart_boot_markers.txt`).
 * @details MAIN — готовность ядра до @c osKernelStart ; остальные — после старта задач.
 *          Формат: @c [INFO] [WALLET] MAIN ok | @c [INFO] [WALLET] &lt;NAME&gt; info .
 */
#define APP_LOG_WALLET_MAIN_OK() APP_LOG_INFO("[WALLET] MAIN ok")
#define APP_LOG_WALLET_SUB_INFO(name_lit) APP_LOG_INFO("[WALLET] " name_lit " info")

#endif /* APP_LOG_H */
