/**
  ******************************************************************************
  * @file    ssd1306_conf.h
  * @brief   SSD1306 config for CryptoWallet - I2C1, 128x32.
  ******************************************************************************
  * @details Set SSD1306_USE_ADDR_3D to 1 if your module uses 0x3D (SA0 high).
  ******************************************************************************
  */

#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

#define STM32H7
#define SSD1306_USE_I2C
#define SSD1306_I2C_PORT        hi2c1
#define SSD1306_I2C_ADDR        (0x3C << 1)  /* Same as lwip-uaid-SSD1306 */
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          32
#define SSD1306_INCLUDE_FONT_6x8

#endif /* __SSD1306_CONF_H__ */
