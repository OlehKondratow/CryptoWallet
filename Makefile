# Secure Crypto Wallet - FreeRTOS firmware for NUCLEO-H743ZI2
# Dependencies: STM32CubeH7, stm32-ssd1306, stm32_secure_boot (linker, FreeRTOS)
#
# Documentation index: README.md (repo root). Module-level detail: file headers in
# Core/Src/*.c, Core/Inc/*.h, Src/task_net.c.
#
# ----- Common make variables (override: make VAR=1) -----
# USE_LWIP          - LwIP + Ethernet + HTTP task (default 1)
# USE_CRYPTO_SIGN   - trezor-crypto + signing stack (default 0)
# USE_TEST_SEED     - known test mnemonic in wallet_seed.c; forces USE_CRYPTO_SIGN
# USE_WEBUSB        - USB device WebUSB on PA11/PA12
# SKIP_OLED         - skip OLED/I2C paths when display bus hangs
# LWIP_ALIVE_LOG    - periodic UART alive from display/net tasks
#
# ----- Notable targets -----
# all, flash         - main firmware
# minimal-lwip       - staged LwIP bring-up build
# flash-minimal-lwip - flash minimal image
# boottest / flash-boottest - no FreeRTOS diagnostic
# secure-signing-test - Python smoke (see scripts/bootloader_secure_signing_test.py)
# docs / docs-serve  - MkDocs (regenerates testing-plan-signing-rng.md)
#
# USE_LWIP=1 enables LwIP Ethernet + HTTP server (default: enabled)

USE_LWIP ?= 1

TOP         ?= $(CURDIR)
TREZOR_CRYPTO ?= $(abspath $(TOP)/ThirdParty/trezor-crypto)
CUBE_ROOT   ?= $(abspath $(TOP)/../STM32CubeH7)
SSD1306     ?= $(abspath $(TOP)/../stm32-ssd1306)
SECURE_BOOT ?= $(abspath $(TOP)/../stm32_secure_boot)
TEMPLATE    := $(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates
LWIP_CUBE   := $(SECURE_BOOT)/app/lwip_zero/cube
LWIP_APP    := $(SECURE_BOOT)/app/lwip_zero/cube_project

CC       = arm-none-eabi-gcc
LD       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
AS       = arm-none-eabi-gcc -x assembler-with-cpp

CFLAGS   = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2
CFLAGS  += -DSTM32H743xx -DUSE_HAL_DRIVER -DUSE_PWR_LDO_SUPPLY
ifeq ($(USE_LWIP),1)
CFLAGS  += -DUSE_LWIP
endif
CFLAGS  += -I$(TOP)/Core/Inc
CFLAGS  += -I$(TOP)/Drivers/ssd1306
CFLAGS  += -I$(SSD1306)/ssd1306
CFLAGS  += -I$(TOP)
CFLAGS  += -I$(SECURE_BOOT)/FreeRTOS/Source/include
CFLAGS  += -I$(SECURE_BOOT)/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
CFLAGS  += -I$(TEMPLATE)/Inc
# USE_TEST_SEED=1: test mnemonic "abandon...about" for dev (NEVER for real funds)
# Implies USE_CRYPTO_SIGN=1
USE_TEST_SEED ?= 0
ifeq ($(USE_TEST_SEED),1)
USE_CRYPTO_SIGN := 1
endif
# USE_CRYPTO_SIGN=1: full trezor-crypto + ECDSA (requires ThirdParty/trezor-crypto)
USE_CRYPTO_SIGN ?= 0
ifeq ($(USE_CRYPTO_SIGN),1)
CFLAGS  += -I$(TREZOR_CRYPTO)
CFLAGS  += -DHAL_RNG_MODULE_ENABLED -DUSE_CRYPTO_SIGN=1
CFLAGS  += -DRAND_PLATFORM_INDEPENDENT -DUSE_BIP32_25519_CURVES=0
endif
ifeq ($(USE_TEST_SEED),1)
CFLAGS  += -DUSE_TEST_SEED=1
endif
# USE_WEBUSB=1: USB device + WebUSB (PA11/PA12, CN13)
USE_WEBUSB ?= 0
ifeq ($(USE_WEBUSB),1)
CFLAGS  += -DUSE_WEBUSB=1 -DHAL_PCD_MODULE_ENABLED
CFLAGS  += -I$(CUBE_ROOT)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
endif

ifeq ($(USE_LWIP),1)
CFLAGS  += -I$(LWIP_CUBE)/Middlewares/Third_Party/LwIP/src/include
CFLAGS  += -I$(LWIP_CUBE)/Middlewares/Third_Party/LwIP/system
CFLAGS  += -I$(LWIP_CUBE)/Middlewares/Third_Party/LwIP/system/arch
CFLAGS  += -I$(LWIP_APP)/Inc
CFLAGS  += -I$(CUBE_ROOT)/Drivers/BSP/Components/lan8742
CFLAGS  += -I$(LWIP_CUBE)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
CFLAGS  += -I$(LWIP_CUBE)/Drivers/CMSIS/RTOS2/Include
CFLAGS  += -include $(TOP)/Core/Inc/lwipopts.h
endif

ifeq ($(USE_LWIP),1)
LDSCRIPT = $(TOP)/STM32H743ZITx_FLASH_LWIP.ld
else
LDSCRIPT = $(SECURE_BOOT)/app/step1/STM32H743ZITx_FLASH.ld
endif
STARTUP  = $(TEMPLATE)/STM32CubeIDE/Example/Startup/startup_stm32h743zitx.s
SYSTEM   = $(TEMPLATE)/Src/system_stm32h7xx.c
LDFLAGS  = -T$(LDSCRIPT) -Wl,--gc-sections -specs=nano.specs -lc -lm -lnosys

BUILD    = build
BINNAME  = cryptowallet
HAL_SRC  = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
FREERTOS     = $(SECURE_BOOT)/FreeRTOS/Source
FREERTOS_CUBE = $(SECURE_BOOT)/app/lwip_zero/cube/Middlewares/Third_Party/FreeRTOS/Source
PORT         = $(FREERTOS)/portable/GCC/ARM_CM4F
MEMMANG      = $(FREERTOS)/portable/MemMang

LWIP_SRC    = $(LWIP_CUBE)/Middlewares/Third_Party/LwIP/src
LWIP_API    = $(LWIP_SRC)/api
LWIP_CORE   = $(LWIP_SRC)/core
LWIP_NETIF  = $(LWIP_SRC)/netif
LWIP_IPV4   = $(LWIP_SRC)/core/ipv4
LWIP_APPS   = $(LWIP_SRC)/apps
LWIP_ARCH   = $(LWIP_CUBE)/Middlewares/Third_Party/LwIP/system/arch
CMSIS_RTOS2 = $(LWIP_CUBE)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
LAN8742_SRC = $(CUBE_ROOT)/Drivers/BSP/Components/lan8742

OBJ = $(BUILD)/main.o $(BUILD)/hw_init.o $(BUILD)/memzero.o $(BUILD)/stm32h7xx_hal_msp.o $(BUILD)/stm32h7xx_it.o \
      $(BUILD)/task_display.o $(BUILD)/task_security.o $(BUILD)/task_net.o $(BUILD)/task_io.o \
      $(BUILD)/startup.o $(BUILD)/system_stm32h7xx.o \
      $(BUILD)/hal.o $(BUILD)/hal_cortex.o $(BUILD)/hal_gpio.o $(BUILD)/hal_rcc.o $(BUILD)/hal_rcc_ex.o \
      $(BUILD)/hal_i2c.o $(BUILD)/hal_i2c_ex.o $(BUILD)/hal_uart.o $(BUILD)/hal_uart_ex.o $(BUILD)/hal_dma.o $(BUILD)/hal_pwr.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/event_groups.o \
      $(BUILD)/heap_4.o $(BUILD)/port.o \
      $(BUILD)/ssd1306.o $(BUILD)/ssd1306_fonts.o

ifeq ($(USE_LWIP),1)
OBJ += $(BUILD)/ethernetif.o $(BUILD)/app_ethernet_cw.o $(BUILD)/cmsis_os2.o $(BUILD)/timers.o $(BUILD)/hal_eth.o $(BUILD)/lan8742.o \
       $(BUILD)/time_service.o \
       $(BUILD)/lwip_sys_arch.o $(BUILD)/lwip_tcpip.o $(BUILD)/lwip_api_lib.o $(BUILD)/lwip_api_msg.o $(BUILD)/lwip_err.o \
       $(BUILD)/lwip_netbuf.o $(BUILD)/lwip_netdb.o $(BUILD)/lwip_netifapi.o $(BUILD)/lwip_sockets.o \
       $(BUILD)/lwip_def.o $(BUILD)/lwip_dns.o $(BUILD)/lwip_inet_chksum.o $(BUILD)/lwip_init.o $(BUILD)/lwip_ip.o \
       $(BUILD)/lwip_mem.o $(BUILD)/lwip_memp.o $(BUILD)/lwip_pbuf.o $(BUILD)/lwip_raw.o $(BUILD)/lwip_stats.o $(BUILD)/lwip_sys.o \
       $(BUILD)/lwip_tcp.o $(BUILD)/lwip_tcp_in.o $(BUILD)/lwip_tcp_out.o $(BUILD)/lwip_udp.o $(BUILD)/lwip_netif.o \
       $(BUILD)/lwip_timeouts.o $(BUILD)/lwip_ip4.o $(BUILD)/lwip_icmp.o $(BUILD)/lwip_etharp.o $(BUILD)/lwip_dhcp.o $(BUILD)/lwip_acd.o $(BUILD)/lwip_ip4_addr.o \
       $(BUILD)/lwip_ip4_frag.o $(BUILD)/lwip_ethernet.o $(BUILD)/lwip_slip.o $(BUILD)/lwip_sntp.o
endif

.PHONY: all clean flash docs docs-md docs-code-md docs-doxygen docs-serve minimal-lwip flash-minimal-lwip boottest flash-boottest secure-signing-test
.DEFAULT_GOAL := all

# Generate docs (MkDocs Material). Uses .venv-docs if present, else system mkdocs
MKDOCS = $(if $(wildcard .venv-docs/bin/mkdocs),.venv-docs/bin/mkdocs,mkdocs)
PYTHON3 ?= python3
docs:
	@$(MKDOCS) --version >/dev/null 2>&1 || { echo "Install: python3 -m venv .venv-docs && .venv-docs/bin/pip install -r requirements-docs.txt"; exit 1; }
	@echo "Regenerating docs_src/testing-plan-signing-rng.md ..."
	@$(PYTHON3) scripts/test_plan_signing_rng.py --write docs_src/testing-plan-signing-rng.md
	@echo "Regenerating docs_src/reference-code.md from source headers ..."
	@$(PYTHON3) scripts/generate_code_reference_md.py -o docs_src/reference-code.md
	$(MKDOCS) build
	@echo "Docs: docs/index.html"

# Markdown only: regenerate generated .md (no MkDocs / no docs/ HTML)
docs-md:
	@echo "Regenerating docs_src/testing-plan-signing-rng.md ..."
	@$(PYTHON3) scripts/test_plan_signing_rng.py --write docs_src/testing-plan-signing-rng.md
	@$(MAKE) docs-code-md
	@echo "OK: generated docs_src/*.md; run 'make docs' for HTML site."

# Code headers → docs_src/reference-code.md (see docs_src/code-doc-generation.md)
docs-code-md:
	@echo "Regenerating docs_src/reference-code.md ..."
	@$(PYTHON3) scripts/generate_code_reference_md.py -o docs_src/reference-code.md

# Doxygen: XML + HTML; then update README "Project Structure" and optionally emit .md per file
docs-doxygen:
	@command -v doxygen >/dev/null 2>&1 || { echo "Install doxygen (e.g. apt install doxygen)"; exit 1; }
	doxygen Doxyfile
	@$(PYTHON3) scripts/update_readme.py
	@$(PYTHON3) scripts/update_readme.py --md-dir docs_doxygen/md
	@$(PYTHON3) scripts/update_docs_src_index.py
	@$(PYTHON3) scripts/generate_readme_languages.py
	@echo "Doxygen: docs_doxygen/html, docs_doxygen/xml; README sections updated (Project Structure + docs_src index); docs_doxygen/md/*.md"
	@echo "README: Generated in English, Russian, Polish (README.md, README_ru.md, README_pl.md)"

# Serve docs locally (live reload)
docs-serve:
	$(MKDOCS) serve

# Minimal + LwIP: use lwip_zero startup+system (same as working lwip_zero)
LWIP_STARTUP = $(LWIP_CUBE)/Drivers/CMSIS/Device/ST/STM32H7xx/Source/Templates/gcc/startup_stm32h743xx.s
LWIP_SYSTEM  = $(LWIP_APP)/Src/system_stm32h7xx.c
CRYPTO_OBJ = $(BUILD)/crypto_wallet.o $(BUILD)/sha256_minimal.o
ifeq ($(USE_TEST_SEED),1)
CRYPTO_OBJ += $(BUILD)/wallet_seed.o
endif
ifeq ($(USE_CRYPTO_SIGN),1)
CRYPTO_OBJ += $(BUILD)/hal_rng.o
endif
ifeq ($(USE_CRYPTO_SIGN),1)
CRYPTO_OBJ += $(BUILD)/trezor_bip39.o $(BUILD)/trezor_bip32.o $(BUILD)/trezor_ecdsa.o $(BUILD)/trezor_secp256k1.o \
      $(BUILD)/trezor_sha2.o $(BUILD)/trezor_hmac.o $(BUILD)/trezor_pbkdf2.o $(BUILD)/trezor_bignum.o \
      $(BUILD)/trezor_hasher.o $(BUILD)/trezor_base58.o $(BUILD)/trezor_address.o \
      $(BUILD)/trezor_ripemd160.o $(BUILD)/trezor_rfc6979.o $(BUILD)/trezor_curves.o $(BUILD)/trezor_nist256p1.o \
      $(BUILD)/trezor_hmac_drbg.o $(BUILD)/trezor_blake256.o $(BUILD)/trezor_sha3.o $(BUILD)/trezor_groestl.o \
      $(BUILD)/trezor_blake2b.o $(BUILD)/trezor_blake2s.o \
      $(BUILD)/trezor_ed25519.o $(BUILD)/trezor_ed25519_sha3.o $(BUILD)/trezor_ed25519_keccak.o \
      $(BUILD)/trezor_curve25519_32bit.o $(BUILD)/trezor_curve25519_helpers.o $(BUILD)/trezor_modm_32bit.o \
      $(BUILD)/trezor_ed25519_basepoint.o $(BUILD)/trezor_ed25519_32bit_tables.o $(BUILD)/trezor_ed25519_impl.o \
      $(BUILD)/trezor_curve25519_scalarmult.o
endif
OBJ_MINIMAL_LWIP = $(BUILD)/main.o $(BUILD)/hw_init.o $(BUILD)/memzero.o $(BUILD)/stm32h7xx_hal_msp.o $(BUILD)/stm32h7xx_it_lwip.o $(BUILD)/stm32h7xx_it_systick.o \
      $(BUILD)/task_display_minimal.o $(BUILD)/task_net.o $(BUILD)/task_sign.o $(BUILD)/task_io.o $(BUILD)/task_user.o $(BUILD)/tx_request_validate.o $(BUILD)/time_service.o \
      $(CRYPTO_OBJ) \
      $(BUILD)/startup_minimal_lwip.o $(BUILD)/system_minimal_lwip.o $(BUILD)/stm32h7xx_hal_timebase_tim.o \
      $(BUILD)/hal.o $(BUILD)/hal_cortex.o $(BUILD)/hal_gpio.o $(BUILD)/hal_rcc.o $(BUILD)/hal_rcc_ex.o \
      $(BUILD)/hal_i2c.o $(BUILD)/hal_i2c_ex.o $(BUILD)/hal_uart.o $(BUILD)/hal_uart_ex.o $(BUILD)/hal_dma.o $(BUILD)/hal_pwr.o $(BUILD)/hal_tim.o $(BUILD)/hal_tim_ex.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/event_groups.o \
      $(BUILD)/heap_4.o $(BUILD)/port.o $(BUILD)/timers.o \
      $(BUILD)/ethernetif.o $(BUILD)/app_ethernet_cw.o $(BUILD)/cmsis_os2.o $(BUILD)/hal_eth.o $(BUILD)/lan8742.o \
      $(BUILD)/lwip_sys_arch.o $(BUILD)/lwip_tcpip.o $(BUILD)/lwip_api_lib.o $(BUILD)/lwip_api_msg.o $(BUILD)/lwip_err.o \
      $(BUILD)/lwip_netbuf.o $(BUILD)/lwip_netdb.o $(BUILD)/lwip_netifapi.o $(BUILD)/lwip_sockets.o \
      $(BUILD)/lwip_def.o $(BUILD)/lwip_dns.o $(BUILD)/lwip_inet_chksum.o $(BUILD)/lwip_init.o $(BUILD)/lwip_ip.o \
      $(BUILD)/lwip_mem.o $(BUILD)/lwip_memp.o $(BUILD)/lwip_pbuf.o $(BUILD)/lwip_raw.o $(BUILD)/lwip_stats.o $(BUILD)/lwip_sys.o \
      $(BUILD)/lwip_tcp.o $(BUILD)/lwip_tcp_in.o $(BUILD)/lwip_tcp_out.o $(BUILD)/lwip_udp.o $(BUILD)/lwip_netif.o \
      $(BUILD)/lwip_timeouts.o $(BUILD)/lwip_ip4.o $(BUILD)/lwip_icmp.o $(BUILD)/lwip_etharp.o $(BUILD)/lwip_dhcp.o $(BUILD)/lwip_acd.o $(BUILD)/lwip_ip4_addr.o \
      $(BUILD)/lwip_ip4_frag.o $(BUILD)/lwip_ethernet.o $(BUILD)/lwip_slip.o $(BUILD)/lwip_sntp.o \
      $(BUILD)/ssd1306.o $(BUILD)/ssd1306_fonts.o

USB_CUBE   := $(abspath $(TOP)/../STM32CubeH7)
USB_MW     := $(USB_CUBE)/Middlewares/ST/STM32_USB_Device_Library
WEBUSB_OBJ := $(BUILD)/usbd_conf_cw.o $(BUILD)/usbd_desc_cw.o $(BUILD)/usb_device.o \
      $(BUILD)/usb_webusb.o $(BUILD)/stm32h7xx_it_usb.o
ifeq ($(USE_WEBUSB),1)
OBJ_MINIMAL_LWIP += $(WEBUSB_OBJ) \
      $(BUILD)/hal_pcd.o $(BUILD)/hal_pcd_ex.o $(BUILD)/ll_usb.o \
      $(BUILD)/usbd_core.o $(BUILD)/usbd_ctlreq.o $(BUILD)/usbd_ioreq.o
endif

# Build: FreeRTOS + LwIP + display (single firmware)
minimal-lwip: CUBE_ROOT = $(LWIP_CUBE)
minimal-lwip: HAL_SRC = $(LWIP_CUBE)/Drivers/STM32H7xx_HAL_Driver/Src
minimal-lwip: LAN8742_SRC = $(LWIP_CUBE)/Drivers/BSP/Components/lan8742
minimal-lwip: TEMPLATE = $(LWIP_APP)
minimal-lwip: CFLAGS += -DUSE_STM32H7XX_NUCLEO_144_MB1364
minimal-lwip: config-copied $(BUILD)/$(BINNAME).bin
	@echo "Built: $(BUILD)/$(BINNAME).bin"

# Skip OLED if I2C hangs: make SKIP_OLED=1
SKIP_OLED ?= 0
# Alive heartbeat: make LWIP_ALIVE_LOG=1
LWIP_ALIVE_LOG ?= 0
CFLAGS += -DSKIP_OLED=$(SKIP_OLED) -DLWIP_ALIVE_LOG=$(LWIP_ALIVE_LOG)

# Diagnostic: no FreeRTOS — only HW_Init + LED blink + UART
boottest: CFLAGS += -DLWIP_NO_LINK_THREAD -DBOOT_TEST
boottest: minimal-lwip
	@echo "Built: $(BUILD)/$(BINNAME).bin — boot test (no RTOS)"

$(BUILD)/$(BINNAME).elf: $(OBJ_MINIMAL_LWIP)
	$(LD) $(CFLAGS) -o $@ $^ -T$(TOP)/STM32H743ZITx_FLASH_LWIP.ld -Wl,--gc-sections -specs=nano.specs -lc -lm -lnosys

$(BUILD)/$(BINNAME).bin: $(BUILD)/$(BINNAME).elf
	$(OBJCOPY) -O binary $< $@
$(BUILD)/task_display_minimal.o: $(TOP)/Core/Src/task_display_minimal.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

flash-minimal-lwip: minimal-lwip
	st-flash --connect-under-reset write $(BUILD)/$(BINNAME).bin 0x08000000

flash-boottest: boottest
	st-flash --connect-under-reset write $(BUILD)/$(BINNAME).bin 0x08000000

# Boot + flash + HTTP signing smoke test (see scripts/README.md)
secure-signing-test:
	python3 $(TOP)/scripts/bootloader_secure_signing_test.py

all: minimal-lwip

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/main.o: $(TOP)/Core/Src/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hw_init.o: $(TOP)/Core/Src/hw_init.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/time_service.o: $(TOP)/Core/Src/time_service.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/memzero.o: $(TOP)/Core/Src/memzero.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_hal_msp.o: $(TOP)/Core/Src/stm32h7xx_hal_msp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_it.o: $(TOP)/Core/Src/stm32h7xx_it.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/stm32h7xx_it_lwip.o: $(LWIP_APP)/Src/stm32h7xx_it.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_it_systick.o: $(TOP)/Core/Src/stm32h7xx_it_systick.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_hal_timebase_tim.o: $(LWIP_APP)/Src/stm32h7xx_hal_timebase_tim.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/task_display.o: $(TOP)/Core/Src/task_display.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/task_security.o: $(TOP)/Core/Src/task_security.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/task_net.o: $(TOP)/Src/task_net.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/task_sign.o: $(TOP)/Core/Src/task_sign.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/task_io.o: $(TOP)/Core/Src/task_io.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/task_user.o: $(TOP)/Core/Src/task_user.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/tx_request_validate.o: $(TOP)/Core/Src/tx_request_validate.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/crypto_wallet.o: $(TOP)/Core/Src/crypto_wallet.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/sha256_minimal.o: $(TOP)/Core/Src/sha256_minimal.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

ifeq ($(USE_TEST_SEED),1)
$(BUILD)/wallet_seed.o: $(TOP)/Core/Src/wallet_seed.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
endif

$(BUILD)/trezor_bip39.o: $(TREZOR_CRYPTO)/bip39.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_bip32.o: $(TREZOR_CRYPTO)/bip32.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_ecdsa.o: $(TREZOR_CRYPTO)/ecdsa.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_secp256k1.o: $(TREZOR_CRYPTO)/secp256k1.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_sha2.o: $(TREZOR_CRYPTO)/sha2.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_hmac.o: $(TREZOR_CRYPTO)/hmac.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_pbkdf2.o: $(TREZOR_CRYPTO)/pbkdf2.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_bignum.o: $(TREZOR_CRYPTO)/bignum.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_rand.o: $(TREZOR_CRYPTO)/rand.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_hasher.o: $(TREZOR_CRYPTO)/hasher.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_base58.o: $(TREZOR_CRYPTO)/base58.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_address.o: $(TREZOR_CRYPTO)/address.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_ripemd160.o: $(TREZOR_CRYPTO)/ripemd160.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_rfc6979.o: $(TREZOR_CRYPTO)/rfc6979.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_curves.o: $(TREZOR_CRYPTO)/curves.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_nist256p1.o: $(TREZOR_CRYPTO)/nist256p1.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_hmac_drbg.o: $(TREZOR_CRYPTO)/hmac_drbg.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_blake256.o: $(TREZOR_CRYPTO)/blake256.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_sha3.o: $(TREZOR_CRYPTO)/sha3.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_groestl.o: $(TREZOR_CRYPTO)/groestl.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_blake2b.o: $(TREZOR_CRYPTO)/blake2b.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/trezor_blake2s.o: $(TREZOR_CRYPTO)/blake2s.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
TREZOR_ED25519 := $(TREZOR_CRYPTO)/ed25519-donna
$(BUILD)/trezor_ed25519.o: $(TREZOR_ED25519)/ed25519.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_ed25519_sha3.o: $(TREZOR_ED25519)/ed25519-sha3.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_ed25519_keccak.o: $(TREZOR_ED25519)/ed25519-keccak.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_curve25519_32bit.o: $(TREZOR_ED25519)/curve25519-donna-32bit.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_curve25519_helpers.o: $(TREZOR_ED25519)/curve25519-donna-helpers.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_modm_32bit.o: $(TREZOR_ED25519)/modm-donna-32bit.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_ed25519_basepoint.o: $(TREZOR_ED25519)/ed25519-donna-basepoint-table.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_ed25519_32bit_tables.o: $(TREZOR_ED25519)/ed25519-donna-32bit-tables.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_ed25519_impl.o: $(TREZOR_ED25519)/ed25519-donna-impl-base.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<
$(BUILD)/trezor_curve25519_scalarmult.o: $(TREZOR_ED25519)/curve25519-donna-scalarmult-base.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TREZOR_ED25519) -c -o $@ $<

$(BUILD)/startup.o: $(STARTUP) | $(BUILD)
	$(AS) $(filter-out -include $(TOP)/Core/Inc/lwipopts.h,$(CFLAGS)) -c -o $@ $<
$(BUILD)/system_stm32h7xx.o: $(SYSTEM) | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/startup_minimal_lwip.o: $(LWIP_STARTUP) | $(BUILD)
	$(AS) $(filter-out -include $(TOP)/Core/Inc/lwipopts.h,$(CFLAGS)) -c -o $@ $<
$(BUILD)/system_minimal_lwip.o: $(LWIP_SYSTEM) | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/hal.o: $(HAL_SRC)/stm32h7xx_hal.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_cortex.o: $(HAL_SRC)/stm32h7xx_hal_cortex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_gpio.o: $(HAL_SRC)/stm32h7xx_hal_gpio.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_rcc.o: $(HAL_SRC)/stm32h7xx_hal_rcc.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_rcc_ex.o: $(HAL_SRC)/stm32h7xx_hal_rcc_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_i2c.o: $(HAL_SRC)/stm32h7xx_hal_i2c.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_i2c_ex.o: $(HAL_SRC)/stm32h7xx_hal_i2c_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_uart.o: $(HAL_SRC)/stm32h7xx_hal_uart.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_uart_ex.o: $(HAL_SRC)/stm32h7xx_hal_uart_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_dma.o: $(HAL_SRC)/stm32h7xx_hal_dma.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_pwr.o: $(HAL_SRC)/stm32h7xx_hal_pwr.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
ifeq ($(USE_CRYPTO_SIGN),1)
$(BUILD)/hal_rng.o: $(HAL_SRC)/stm32h7xx_hal_rng.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
endif
$(BUILD)/hal_tim.o: $(HAL_SRC)/stm32h7xx_hal_tim.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_tim_ex.o: $(HAL_SRC)/stm32h7xx_hal_tim_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tasks.o: $(FREERTOS)/tasks.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $(FREERTOS)/tasks.c
$(BUILD)/queue.o: $(FREERTOS)/queue.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $(FREERTOS)/queue.c
$(BUILD)/list.o: $(FREERTOS)/list.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $(FREERTOS)/list.c
$(BUILD)/event_groups.o: $(FREERTOS_CUBE)/event_groups.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $(FREERTOS_CUBE)/event_groups.c
$(BUILD)/heap_4.o: $(MEMMANG)/heap_4.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $(MEMMANG)/heap_4.c
$(BUILD)/port.o: $(PORT)/port.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $(PORT)/port.c

$(BUILD)/ssd1306.o: $(SSD1306)/ssd1306/ssd1306.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TOP)/Drivers/ssd1306 -c -o $@ $<
$(BUILD)/ssd1306_fonts.o: $(SSD1306)/ssd1306/ssd1306_fonts.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(TOP)/Drivers/ssd1306 -c -o $@ $<

ifeq ($(USE_WEBUSB),1)
$(BUILD)/usbd_conf_cw.o: $(TOP)/Core/Src/usbd_conf_cw.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(USB_MW)/Core/Inc -c -o $@ $<
$(BUILD)/usbd_desc_cw.o: $(TOP)/Core/Src/usbd_desc_cw.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(USB_MW)/Core/Inc -c -o $@ $<
$(BUILD)/usb_device.o: $(TOP)/Core/Src/usb_device.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(USB_MW)/Core/Inc -c -o $@ $<
$(BUILD)/usb_webusb.o: $(TOP)/Core/Src/usb_webusb.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(USB_MW)/Core/Inc -c -o $@ $<
$(BUILD)/stm32h7xx_it_usb.o: $(TOP)/Core/Src/stm32h7xx_it_usb.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_pcd.o: $(USB_CUBE)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_pcd_ex.o: $(USB_CUBE)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/ll_usb.o: $(USB_CUBE)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/usbd_core.o: $(USB_MW)/Core/Src/usbd_core.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(USB_MW)/Core/Inc -c -o $@ $<
$(BUILD)/usbd_ctlreq.o: $(USB_MW)/Core/Src/usbd_ctlreq.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(USB_MW)/Core/Inc -c -o $@ $<
$(BUILD)/usbd_ioreq.o: $(USB_MW)/Core/Src/usbd_ioreq.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(USB_MW)/Core/Inc -c -o $@ $<
endif

ifeq ($(USE_LWIP),1)
$(BUILD)/ethernetif.o: $(LWIP_APP)/Src/ethernetif.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/app_ethernet_cw.o: $(TOP)/Src/app_ethernet_cw.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/cmsis_os2.o: $(CMSIS_RTOS2)/cmsis_os2.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/timers.o: $(FREERTOS_CUBE)/timers.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_eth.o: $(HAL_SRC)/stm32h7xx_hal_eth.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lan8742.o: $(LAN8742_SRC)/lan8742.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_sys_arch.o: $(LWIP_ARCH)/sys_arch.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcpip.o: $(LWIP_API)/tcpip.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_api_lib.o: $(LWIP_API)/api_lib.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_api_msg.o: $(LWIP_API)/api_msg.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_err.o: $(LWIP_API)/err.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netbuf.o: $(LWIP_API)/netbuf.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netdb.o: $(LWIP_API)/netdb.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netifapi.o: $(LWIP_API)/netifapi.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_sockets.o: $(LWIP_API)/sockets.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_def.o: $(LWIP_CORE)/def.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_dns.o: $(LWIP_CORE)/dns.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_inet_chksum.o: $(LWIP_CORE)/inet_chksum.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_init.o: $(LWIP_CORE)/init.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip.o: $(LWIP_CORE)/ip.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_mem.o: $(LWIP_CORE)/mem.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_memp.o: $(LWIP_CORE)/memp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_pbuf.o: $(LWIP_CORE)/pbuf.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_raw.o: $(LWIP_CORE)/raw.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_stats.o: $(LWIP_CORE)/stats.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_sys.o: $(LWIP_CORE)/sys.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcp.o: $(LWIP_CORE)/tcp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcp_in.o: $(LWIP_CORE)/tcp_in.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcp_out.o: $(LWIP_CORE)/tcp_out.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_udp.o: $(LWIP_CORE)/udp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netif.o: $(LWIP_CORE)/netif.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_timeouts.o: $(LWIP_CORE)/timeouts.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip4.o: $(LWIP_IPV4)/ip4.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_icmp.o: $(LWIP_IPV4)/icmp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_etharp.o: $(LWIP_IPV4)/etharp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_dhcp.o: $(LWIP_IPV4)/dhcp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_acd.o: $(LWIP_IPV4)/acd.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip4_addr.o: $(LWIP_IPV4)/ip4_addr.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip4_frag.o: $(LWIP_IPV4)/ip4_frag.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ethernet.o: $(LWIP_NETIF)/ethernet.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_slip.o: $(LWIP_NETIF)/slipif.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_sntp.o: $(LWIP_APPS)/sntp/sntp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
endif

ifeq ($(USE_LWIP),1)
FREERTOS_CONFIG_SRC = $(TOP)/FreeRTOSConfig_lwip.h
else
FREERTOS_CONFIG_SRC = $(TOP)/FreeRTOSConfig.h
endif

config-copied: $(FREERTOS_CONFIG_SRC)
	@cp "$(FREERTOS)/include/FreeRTOSConfig.h" "$(FREERTOS)/include/FreeRTOSConfig.h.bak" 2>/dev/null || true
	@cp "$(FREERTOS_CONFIG_SRC)" "$(FREERTOS)/include/FreeRTOSConfig.h"
	@touch config-copied

flash: all
	st-flash --connect-under-reset write $(BUILD)/$(BINNAME).bin 0x08000000

clean:
	@mv "$(FREERTOS)/include/FreeRTOSConfig.h.bak" "$(FREERTOS)/include/FreeRTOSConfig.h" 2>/dev/null || true
	@rm -f config-copied
	rm -rf $(BUILD)
