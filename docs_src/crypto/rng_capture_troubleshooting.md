# RNG Capture Troubleshooting Guide

## Error Encountered

```
✗ Failed to capture RNG: device reports readiness to read but returned no data 
  (device disconnected or multiple access on port?)
```

This error occurs when the serial port opens successfully but returns **no data**. This usually means:

1. **Device not connected or disconnected**
2. **Multiple programs accessing the same port** (something is already reading)
3. **Firmware not built with `USE_RNG_DUMP=1`**
4. **Device not outputting RNG data on UART**

---

## Step-by-Step Troubleshooting

### 1. Verify Device Connection

First, check if the device is physically connected:

```bash
# List all USB/Serial devices
ls -la /dev/tty* | grep -E "(USB|ACM|FTDI|Serial)"

# Or use lsusb to see USB devices
lsusb | grep -i stm32

# Or check dmesg for recent connections
dmesg | tail -20 | grep -E "usb|tty"
```

**Expected output**: Should show `/dev/ttyACM0`, `/dev/ttyUSB0`, or similar

**If no device appears**:
- Check USB cable connection to your STM32H743 board
- Try different USB port
- Try different USB cable
- Restart the device (power cycle)

---

### 2. Verify Port Permissions

Check if you have permission to access the serial port:

```bash
# Check current permissions
ls -la /dev/ttyACM0

# You should see something like:
# crw-rw---- 1 root dialout 166, 0 ...

# If you're not in dialout group:
sudo usermod -a -G dialout $USER
newgrp dialout

# Logout and back in, then verify:
id | grep dialout
```

**If permission denied**:
```bash
# Temporary fix (current session)
sudo chmod 666 /dev/ttyACM0

# Permanent fix (add to dialout group - see above)
```

---

### 3. Check for Port Conflicts

Another program might be using the port (serial monitor, IDE debugger, etc.):

```bash
# See what's using the port
sudo lsof /dev/ttyACM0

# Or check if process is holding it
fuser /dev/ttyACM0

# Common culprits to close:
# - Arduino IDE Serial Monitor
# - VSCode/Cursor Serial Monitor
# - miniterm/picoterm
# - Other instances of this script
```

**If port is in use**:
- Close all IDE serial monitors
- Kill other test instances: `pkill -f test_rng_signing_comprehensive.py`
- Wait 2-3 seconds before retrying

---

### 4. Verify Firmware Configuration

**Critical**: The firmware **must be built with `USE_RNG_DUMP=1`**

This configures the STM32 to output **raw RNG data** on UART (binary only, no text).

#### Check Makefile or Build Configuration

```bash
# Look for USE_RNG_DUMP setting
grep -r "USE_RNG_DUMP" /data/projects/CryptoWallet/

# Should find something like:
# - Makefile: USE_RNG_DUMP=1
# - CMakeLists.txt or build config
# - Or set in Core/Src/main.h
```

#### If NOT Set

You need to **rebuild firmware** with this flag:

```bash
cd /data/projects/CryptoWallet

# Option 1: Set in Makefile
sed -i 's/USE_RNG_DUMP=0/USE_RNG_DUMP=1/' Makefile

# Option 2: Set as environment variable
export USE_RNG_DUMP=1

# Then rebuild
make clean
make -j$(nproc)

# Flash to device (if your project has a flash target)
make flash
```

#### Verify Firmware

After flashing, test UART manually:

```bash
# Install miniterm or similar
pip install pyserial

# Read raw UART data (stop with Ctrl+C)
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw

# You should see binary garbage (RNG data)
# NOT: text output, menu prompts, or nothing
```

---

### 5. Test UART Communication Manually

Before running the full test, verify basic serial communication:

```python
#!/usr/bin/env python3
# Simple serial test
import serial
import time

port = "/dev/ttyACM0"
try:
    ser = serial.Serial(port=port, baudrate=115200, timeout=2)
    time.sleep(0.5)
    
    print(f"Port {port} opened successfully")
    print(f"In waiting: {ser.in_waiting}")
    
    # Try to read some data
    data = ser.read(100)
    print(f"Read {len(data)} bytes")
    print(f"First 16 bytes (hex): {data[:16].hex()}")
    
    ser.close()
    print("✓ Serial communication working")
except Exception as e:
    print(f"✗ Error: {e}")
```

Save as `test_serial.py` and run:
```bash
python3 test_serial.py
```

**Expected**:
- Port opens successfully
- `in_waiting` shows some data available
- Reads 100+ bytes
- First bytes are binary (not ASCII text)

---

### 6. Baud Rate Mismatch

Ensure both device and script use the **same baud rate** (default 115200):

```bash
# Check device firmware baud rate
grep -r "115200\|UART.*BAUD\|BAUDRATE" /data/projects/CryptoWallet/Core/Src/ | head -5

# If different, either:
# - Rebuild firmware with correct baud
# - Or specify in test script:
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyACM0 --baud 115200
```

---

## Quick Diagnostic Checklist

Run these checks in order:

```bash
# 1. Device connected?
ls /dev/ttyACM0
# Expected: /dev/ttyACM0 (or /dev/ttyUSB0)

# 2. Can read from port?
sudo dd if=/dev/ttyACM0 bs=1 count=10 2>/dev/null | xxd
# Expected: Some bytes (binary data)

# 3. Python can open it?
python3 -c "import serial; s=serial.Serial('/dev/ttyACM0'); print('✓')"
# Expected: ✓ and no error

# 4. Firmware has USE_RNG_DUMP=1?
grep -i "USE_RNG_DUMP\|RNG.*DUMP" /data/projects/CryptoWallet/Makefile
# Expected: USE_RNG_DUMP=1

# 5. Firmware recently flashed?
# Check build date/time
ls -la /data/projects/CryptoWallet/build/*.elf
# If older than expected, need to rebuild and flash
```

---

## Common Issues & Solutions

| Issue | Symptoms | Solution |
|-------|----------|----------|
| **Device not connected** | "No such file or directory: /dev/ttyACM0" | Check USB cable and port |
| **Multiple access** | "Failed to capture RNG: no data returned" | Close serial monitors, kill other instances |
| **Firmware issue** | "No data returned" (but port opens) | Rebuild with `USE_RNG_DUMP=1` and reflash |
| **Baud rate mismatch** | "Garbage or no data" | Verify baud rate matches firmware (115200) |
| **Permission denied** | "Permission denied" | Add user to dialout group: `sudo usermod -a -G dialout $USER` |
| **Different device path** | Works for someone else but not you | Use `--port /dev/ttyUSB0` or check `ls /dev/tty*` |

---

## If Still Not Working

Try these additional steps:

### Reset Device
```bash
# Power cycle (disconnect and reconnect USB)
# Or use ST-Link reset if available

# Then wait 2-3 seconds and try again
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

### Rebuild Everything
```bash
cd /data/projects/CryptoWallet

# Clean build
make distclean 2>/dev/null || true

# Rebuild with explicit RNG dump
make USE_RNG_DUMP=1 -j$(nproc)

# Flash (depends on your project)
make flash  # or similar for your setup
```

### Test with Different Tool
```bash
# Try alternative tools to verify serial works

# Option 1: miniterm (simple)
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw

# Option 2: screen
screen /dev/ttyACM0 115200

# Option 3: picocom
sudo apt install picocom
picocom -b 115200 /dev/ttyACM0
```

If these show binary data, the device and port are working. Then the issue is firmware configuration.

### Check Logs
```bash
# Enable debug output in test script
python3 scripts/test_rng_signing_comprehensive.py --mode rng -v 2>&1 | tee test_debug.log

# Review the log for more details
cat test_debug.log | grep -i "error\|failed\|timeout"
```

---

## Next Steps

1. **Run diagnostic checklist above** - identify which step fails
2. **Fix the specific issue** - use solution from the table
3. **Retry test**:
   ```bash
   source .venv-test/bin/activate
   python3 scripts/test_rng_signing_comprehensive.py --mode rng
   ```

4. **If still stuck** - check:
   - Device log: `dmesg | tail -20`
   - Firmware source: `/data/projects/CryptoWallet/Core/Src/` for RNG configuration
   - Build output: Check `make` output for warnings/errors

---

## Success Indicators

When working correctly, you should see:

```
✓ Checking prerequisites...
  ✓ pyserial
  ✓ dieharder
  ✓ serial port

RNG Data Capture from UART
ℹ Port: /dev/ttyACM0, Baud: 115200
ℹ Target: 128 MiB
ℹ Capturing RNG data...

Captured: 10 MiB (7.8%)  [████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] ...
Captured: 20 MiB (15.6%) [████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] ...
...
Captured: 128 MiB (100%) [████████████████████████████████████████]

✓ Successfully captured 128 MiB RNG data to rng.bin
✓ Saved analysis to rng_analysis.txt
```

The capture should take **5-15 minutes** depending on UART baud rate and device RNG speed.

---

**Questions?** Check:
- `docs_src/TESTING_GUIDE_RNG_SIGNING.md` - Complete testing guide
- `docs_src/INSTALL_TEST_DEPS.md` - Installation and setup
- Firmware documentation in `/data/projects/CryptoWallet/docs_src/crypto/`
