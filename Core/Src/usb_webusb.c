/**
  ******************************************************************************
  * @file    usb_webusb.c
  * @brief   WebUSB vendor-specific class implementation.
  ******************************************************************************
  */

#include "usb_webusb.h"
#include "usbd_ctlreq.h"
#include "wallet_shared.h"
#include "tx_request_validate.h"
#include <string.h>

#include "task_display.h"

#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)

#define WEBUSB_CMD_PING       0x00U
#define WEBUSB_CMD_PONG       0x00U
#define WEBUSB_CMD_SIGN_REQ   0x01U
#define WEBUSB_CMD_SIGN_RESP  0x02U

static uint8_t USBD_WEBUSB_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_WEBUSB_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_WEBUSB_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_WEBUSB_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_WEBUSB_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_WEBUSB_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_WEBUSB_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t *USBD_WEBUSB_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_WEBUSB_GetDeviceQualifierDesc(uint16_t *length);

USBD_ClassTypeDef USBD_WEBUSB_ClassDriver = {
    USBD_WEBUSB_Init,
    USBD_WEBUSB_DeInit,
    USBD_WEBUSB_Setup,
    USBD_WEBUSB_EP0_TxReady,
    USBD_WEBUSB_EP0_RxReady,
    USBD_WEBUSB_DataIn,
    USBD_WEBUSB_DataOut,
    NULL,
    NULL,
    NULL,
    USBD_WEBUSB_GetCfgDesc,
    USBD_WEBUSB_GetCfgDesc,
    USBD_WEBUSB_GetCfgDesc,
    USBD_WEBUSB_GetDeviceQualifierDesc,
};

#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t USBD_WEBUSB_CfgDesc[WEBUSB_CONFIG_DESC_SIZ] __ALIGN_END = {
    0x09, 0x02, (uint8_t)(WEBUSB_CONFIG_DESC_SIZ & 0xFF), (uint8_t)(WEBUSB_CONFIG_DESC_SIZ >> 8),
    0x01, 0x01, 0x00, 0xC0, 0x32,
    /* Interface 0: Vendor-specific */
    0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00, 0x00,
    /* EP1 IN Bulk */
    0x07, 0x05, WEBUSB_EPIN_ADDR, 0x02, (uint8_t)(WEBUSB_EP_SIZE & 0xFF), (uint8_t)(WEBUSB_EP_SIZE >> 8), 0x00,
    /* EP2 OUT Bulk */
    0x07, 0x05, WEBUSB_EPOUT_ADDR, 0x02, (uint8_t)(WEBUSB_EP_SIZE & 0xFF), (uint8_t)(WEBUSB_EP_SIZE >> 8), 0x00,
};

#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t USBD_WEBUSB_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

static uint8_t s_tx_buf[WEBUSB_EP_SIZE + 1];  /* 1 byte cmd + 64 bytes sig */
static uint8_t s_rx_buf[WEBUSB_EP_SIZE];
static USBD_HandleTypeDef *s_pdev = NULL;

static uint8_t USBD_WEBUSB_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    s_pdev = pdev;
    USBD_LL_OpenEP(pdev, WEBUSB_EPIN_ADDR, USBD_EP_TYPE_BULK, WEBUSB_EP_SIZE);
    USBD_LL_OpenEP(pdev, WEBUSB_EPOUT_ADDR, USBD_EP_TYPE_BULK, WEBUSB_EP_SIZE);
    USBD_LL_PrepareReceive(pdev, WEBUSB_EPOUT_ADDR, s_rx_buf, WEBUSB_EP_SIZE);
    return (uint8_t)USBD_OK;
}

static uint8_t USBD_WEBUSB_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_LL_CloseEP(pdev, WEBUSB_EPIN_ADDR);
    USBD_LL_CloseEP(pdev, WEBUSB_EPOUT_ADDR);
    return (uint8_t)USBD_OK;
}

static uint8_t USBD_WEBUSB_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    (void)pdev;
    (void)req;
    return (uint8_t)USBD_OK;
}

static uint8_t USBD_WEBUSB_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)pdev;
    (void)epnum;
    return (uint8_t)USBD_OK;
}

static void send_pong(void)
{
    if (s_pdev == NULL) return;
    static const char pong[] = "pong";
    memcpy(s_tx_buf, pong, 4);
    USBD_LL_Transmit(s_pdev, WEBUSB_EPIN_ADDR, s_tx_buf, 4);
}

static void process_sign_request(const uint8_t *data, uint16_t len)
{
    /* Ping: "ping" (4 bytes) → respond "pong" (4 bytes) */
    if (len == 4 && memcmp(data, "ping", 4) == 0) {
        Task_Display_Log("WebUSB ping");
        send_pong();
        return;
    }
    if (len < 10) return;
    wallet_tx_t tx;
    memset(&tx, 0, sizeof(tx));
    const char *p = (const char *)data;
    const char *r = strstr(p, "\"recipient\"");
    const char *a = strstr(p, "\"amount\"");
    const char *c = strstr(p, "\"currency\"");
    if (r && a) {
        const char *vr = strchr(r, ':');
        if (vr) {
            vr = strchr(vr, '"');
            if (vr) { vr++; const char *er = strchr(vr, '"'); if (er) { size_t l = (size_t)(er - vr); if (l >= TX_RECIPIENT_LEN) l = TX_RECIPIENT_LEN - 1; if (l > 0) memcpy(tx.recipient, vr, l); } }
        }
        const char *va = strchr(a, ':');
        if (va) {
            va = strchr(va, '"');
            if (va) { va++; const char *ea = strchr(va, '"'); if (ea) { size_t l = (size_t)(ea - va); if (l >= TX_AMOUNT_LEN) l = TX_AMOUNT_LEN - 1; if (l > 0) memcpy(tx.amount, va, l); } }
        }
        if (c) {
            const char *vc = strchr(c, ':');
            if (vc) { vc = strchr(vc, '"'); if (vc) { vc++; const char *ec = strchr(vc, '"'); if (ec) { size_t l = (size_t)(ec - vc); if (l >= TX_CURRENCY_LEN) l = TX_CURRENCY_LEN - 1; if (l > 0) memcpy(tx.currency, vc, l); } } }
        }
        if (tx.currency[0] == '\0') (void)strncpy(tx.currency, "BTC", TX_CURRENCY_LEN - 1);
        if (tx_request_validate(&tx) == TX_VALID_OK && g_tx_queue != NULL) {
            if (xQueueSend(g_tx_queue, &tx, pdMS_TO_TICKS(100)) == pdTRUE) {
                Task_Display_Log("WebUSB TX");
            }
        }
    }
}

void WebUSB_NotifySignatureReady(void)
{
    if (s_pdev == NULL || !g_last_sig_ready) return;
    s_tx_buf[0] = WEBUSB_CMD_SIGN_RESP;
    memcpy(s_tx_buf + 1, g_last_sig, 64);
    USBD_LL_Transmit(s_pdev, WEBUSB_EPIN_ADDR, s_tx_buf, 65);
}

static uint8_t USBD_WEBUSB_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    if (epnum != (WEBUSB_EPOUT_ADDR & 0x7F)) return (uint8_t)USBD_OK;
    uint32_t len = USBD_LL_GetRxDataSize(pdev, WEBUSB_EPOUT_ADDR);
    if (len > 0 && len <= WEBUSB_EP_SIZE) {
        USBD_LL_PrepareReceive(pdev, WEBUSB_EPOUT_ADDR, s_rx_buf, WEBUSB_EP_SIZE);
        process_sign_request(s_rx_buf, (uint16_t)len);
    } else {
        USBD_LL_PrepareReceive(pdev, WEBUSB_EPOUT_ADDR, s_rx_buf, WEBUSB_EP_SIZE);
    }
    return (uint8_t)USBD_OK;
}

static uint8_t USBD_WEBUSB_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    (void)pdev;
    return (uint8_t)USBD_OK;
}

static uint8_t USBD_WEBUSB_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
    (void)pdev;
    return (uint8_t)USBD_OK;
}

static uint8_t *USBD_WEBUSB_GetCfgDesc(uint16_t *length)
{
    *length = sizeof(USBD_WEBUSB_CfgDesc);
    return USBD_WEBUSB_CfgDesc;
}

static uint8_t *USBD_WEBUSB_GetDeviceQualifierDesc(uint16_t *length)
{
    *length = sizeof(USBD_WEBUSB_DeviceQualifierDesc);
    return USBD_WEBUSB_DeviceQualifierDesc;
}

#endif /* USE_WEBUSB */
