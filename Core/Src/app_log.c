/**
 ******************************************************************************
 * @file    app_log.c
 * @brief   Implements @c App_Log_*Msg — level prefix + truncation for @c UART_Log .
 ******************************************************************************
 */

#include "app_log.h"
#include <stdio.h>
#include <string.h>

#ifndef APP_LOG_DYNAMIC_MAX
#define APP_LOG_DYNAMIC_MAX 200
#endif

static void app_log_prefix(const char *prefix, const char *msg)
{
    char buf[APP_LOG_DYNAMIC_MAX];

    if (msg == NULL) {
        return;
    }
    {
        int n = snprintf(buf, sizeof(buf), "%s%s", prefix, msg);
        if (n < 0 || n >= (int)sizeof(buf)) {
            buf[sizeof(buf) - 1U] = '\0';
        }
    }
    Task_Display_Log(buf);
}

void App_Log_InfoMsg(const char *msg)
{
    app_log_prefix("[INFO] ", msg);
}

void App_Log_WarnMsg(const char *msg)
{
    app_log_prefix("[WARN] ", msg);
}

void App_Log_ErrMsg(const char *msg)
{
    app_log_prefix("[ERR] ", msg);
}

#if APP_LOG_ENABLE_DBG
void App_Log_DbgMsg(const char *msg)
{
    app_log_prefix("[DBG] ", msg);
}
#endif
