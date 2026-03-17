/**
  ******************************************************************************
  * @file    usbd_desc_cw.c
  * @brief   USB device descriptors for CryptoWallet WebUSB.
  ******************************************************************************
  */

#include "usbd_core.h"
#include "usbd_ctlreq.h"
#include "usbd_desc_cw.h"
#include "usbd_conf_cw.h"

#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)

#define USBD_VID          0x1209U
#define USBD_PID          0xC0DEU
#define USBD_LANGID       0x0409U
#define USBD_MANUFACTURER "CryptoWallet"
#define USBD_PRODUCT      "CryptoWallet WebUSB"
#define USBD_CONFIG_STR   "WebUSB Config"
#define USBD_INTERFACE_STR "WebUSB Interface"

#define USB_SIZ_STRING_SERIAL 0x1AU

#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t USBD_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12, USB_DESC_TYPE_DEVICE, 0x00, 0x02,
    0x00, 0x00, 0x00, USB_MAX_EP0_SIZE,
    LOBYTE(USBD_VID), HIBYTE(USBD_VID),
    LOBYTE(USBD_PID), HIBYTE(USBD_PID),
    0x00, 0x01, 0x01, 0x02, 0x03, USBD_MAX_NUM_CONFIGURATION
};

#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC, USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID), HIBYTE(USBD_LANGID)
};

#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL, USB_DESC_TYPE_STRING,
};

/* BOS + WebUSB Platform Capability (UUID 3408b638-09a9-47a0-8bfd-a0768815b665) */
#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t USBD_BOSDesc[] __ALIGN_END = {
    0x05, 0x0F, 0x1D, 0x00, 0x01,
    0x18, 0x10, 0x05, 0x00,
    0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47,
    0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
    0x00, 0x01, 0x01, 0x01
};

static void Get_SerialNum(void);
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

#define DEVICE_ID1 (0x1FF1E800U)
#define DEVICE_ID2 (0x1FF1E804U)
#define DEVICE_ID3 (0x1FF1E808U)

static uint8_t *USBD_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    *length = sizeof(USBD_DeviceDesc);
    return USBD_DeviceDesc;
}

static uint8_t *USBD_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    *length = sizeof(USBD_LangIDDesc);
    return USBD_LangIDDesc;
}

static uint8_t *USBD_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t *)USBD_MANUFACTURER, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t *)USBD_PRODUCT, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    *length = USB_SIZ_STRING_SERIAL;
    Get_SerialNum();
    return USBD_StringSerial;
}

static uint8_t *USBD_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t *)USBD_CONFIG_STR, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t *)USBD_INTERFACE_STR, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_BOSDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    *length = sizeof(USBD_BOSDesc);
    return USBD_BOSDesc;
}

static void Get_SerialNum(void)
{
    uint32_t d0 = *(uint32_t *)DEVICE_ID1;
    uint32_t d1 = *(uint32_t *)DEVICE_ID2;
    uint32_t d2 = *(uint32_t *)DEVICE_ID3;
    d0 += d2;
    if (d0 != 0) {
        IntToUnicode(d0, &USBD_StringSerial[2], 8);
        IntToUnicode(d1, &USBD_StringSerial[18], 4);
    }
}

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        uint8_t v = (value >> 28) & 0x0F;
        pbuf[2 * i] = (v < 10) ? (v + '0') : (v + 'A' - 10);
        value <<= 4;
        pbuf[2 * i + 1] = 0;
    }
}

USBD_DescriptorsTypeDef WEBUSB_Desc = {
    USBD_DeviceDescriptor,
    USBD_LangIDStrDescriptor,
    USBD_ManufacturerStrDescriptor,
    USBD_ProductStrDescriptor,
    USBD_SerialStrDescriptor,
    USBD_ConfigStrDescriptor,
    USBD_InterfaceStrDescriptor,
    USBD_BOSDescriptor
};

#endif /* USE_WEBUSB */
