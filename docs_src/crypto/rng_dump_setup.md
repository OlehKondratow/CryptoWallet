# Enabling RNG Dump in CryptoWallet Firmware

## Quick Solution

The RNG capture test is failing because the **firmware is not configured to output raw RNG data on UART**.

You need to **rebuild the firmware with `USE_RNG_DUMP=1`** flag.

## Step-by-Step Guide

### 1. Check Current Makefile

```bash
cd /data/projects/CryptoWallet
grep "USE_RNG_DUMP" Makefile
```

**Expected**: Empty output (not currently set)

### 2. Add RNG_DUMP to Makefile

Add this line after `USE_TEST_SEED` definition (around line 59):

```makefile
# USE_RNG_DUMP=1: output raw RNG data on UART (for Dieharder testing)
# WARNING: Disables normal UART output - only binary RNG data sent
USE_RNG_DUMP ?= 0
```

#### Location in Makefile:

Find this section (lines ~57-67):

```makefile
# USE_TEST_SEED=1: test mnemonic "abandon...about" for dev (NEVER for real funds)
# Implies USE_CRYPTO_SIGN=1
USE_TEST_SEED ?= 0
ifeq ($(USE_TEST_SEED),1)
USE_CRYPTO_SIGN := 1
```

Add your line after this, before the USE_CRYPTO_SIGN block.

### 3. Add Flag to CFLAGS

Find the CFLAGS section where USE_TEST_SEED is added (around line 60-70), and add:

```makefile
ifeq ($(USE_RNG_DUMP),1)
CFLAGS += -DUSE_RNG_DUMP
endif
```

This should go **after** the `USE_TEST_SEED` block and **before** the `USE_CRYPTO_SIGN` block.

### 4. Rebuild Firmware

```bash
cd /data/projects/CryptoWallet

# Option A: Clean rebuild with flag
make clean
make USE_RNG_DUMP=1 -j$(nproc)

# Option B: If you only want RNG dump (not crypto signing)
make clean
make USE_RNG_DUMP=1 -j$(nproc)

# Option C: With both crypto and RNG dump
make clean
make USE_CRYPTO_SIGN=1 USE_RNG_DUMP=1 -j$(nproc)
```

### 5. Flash to Device

```bash
# This depends on your setup. Common options:

# Option A: STM32 ST-Link
make flash

# Option B: DFU (if using DFU bootloader)
make dfu

# Option C: Manual using st-flash
st-flash write build/*.elf 0x08000000

# Option D: Using OpenOCD
openocd -f interface/stlink-v2-1.cfg -f target/stm32h7x.cfg \
  -c "program build/*.elf verify reset exit"
```

**Note**: Check your project's flash target:
```bash
grep -n "^flash:" Makefile
```

### 6. Verify Firmware Flashed

After flashing, the device should output **only binary data** on UART (no text):

```bash
# Quick test (stop with Ctrl+C)
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw

# You should see binary garbage - raw RNG data
# NOT text output, menu prompts, or nothing
```

## Firmware Code Integration

The firmware code should already have the RNG dump functionality. Check:

```bash
# Search for USE_RNG_DUMP in source
grep -r "USE_RNG_DUMP" /data/projects/CryptoWallet/Core/

# Should find UART/task configuration that:
# - Outputs raw RNG bytes when enabled
# - Bypasses normal UART messages
```

If not found, you may need to add it to `Core/Src/main.c` or the UART task.

## Common Issues

### Issue 1: "make: target flash not found"
Your project might use a different flash method. Check:
```bash
make help | grep -i flash
```

### Issue 2: "make: command not found"
You need to have the ARM embedded toolchain installed:
```bash
sudo apt install build-essential arm-none-eabi-gcc
```

### Issue 3: "No such file or directory: stm32cubeh7"
Dependencies not found. Check:
```bash
ls -d ../STM32CubeH7 ../stm32-ssd1306 ../stm32_secure_boot
```

If missing, clone them:
```bash
cd /data/projects
git clone https://github.com/STMicroelectronics/STM32CubeH7
# etc.
```

## Testing After Build

Once firmware is flashed, test the RNG dump:

```bash
# Test 1: Raw read
dd if=/dev/ttyACM0 bs=1 count=100 2>/dev/null | xxd

# Test 2: Run test script
source .venv-test/bin/activate
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

The test should now:
1. Open the serial port ✓
2. Detect data available ✓
3. **Successfully read binary data** ✓

## Expected Build Output

```
Compiling Core/Src/main.c ...
Compiling Core/Src/task_rng.c ...
Linking ...
Creating binary ...
arm-none-eabi-objcopy -O binary build/firmware.elf build/firmware.bin
Firmware ready: build/firmware.elf (128 KB)
```

## Verification

After flashing, verify the firmware build flags:

```bash
# Check if binary contains the flag in strings
arm-none-eabi-strings build/firmware.elf | grep -i rng

# Or check the build timestamp
ls -l build/firmware.elf
# Should show recent time
```

## Reverting (Back to Normal Firmware)

To go back to normal firmware without RNG dump:

```bash
cd /data/projects/CryptoWallet
make clean
make -j$(nproc)  # No flags - uses defaults
make flash
```

This will rebuild with:
- `USE_RNG_DUMP=0` (normal UART output)
- `USE_LWIP=1` (LwIP networking, default)
- `USE_CRYPTO_SIGN=0` (no signing, unless you set it)

## Next Steps

1. Edit Makefile and add USE_RNG_DUMP support
2. Rebuild: `make clean && make USE_RNG_DUMP=1 -j$(nproc)`
3. Flash to device: `make flash`
4. Verify: `python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw`
5. Test: `python3 scripts/test_rng_signing_comprehensive.py --mode rng`

---

**See Also:**
- `TROUBLESHOOT_RNG_CAPTURE.md` - Full troubleshooting guide
- `docs_src/TESTING_GUIDE_RNG_SIGNING.md` - Complete testing guide
- Firmware documentation in `docs_src/crypto/`
