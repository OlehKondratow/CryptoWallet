# STM32H743ZI2 Reset - Complete Guide

## 🔄 Reset Methods

### 1️⃣ **st-flash reset** (Simplest)
```bash
# Perform a soft reset via ST-LINK
st-flash reset

# With connection under reset (for stuck devices)
st-flash --connect-under-reset reset
```

**Pros:** Fast, simple, reliable  
**Cons:** Requires ST-LINK connected  
**Use case:** After flashing, to start fresh

---

### 2️⃣ **st-flash with --reset** (Auto-reset after flash)
```bash
# Flash AND reset automatically
st-flash --reset write build/cryptowallet.bin 0x08000000

# With debug info
st-flash --debug --reset write build/cryptowallet.bin 0x08000000

# Under-reset mode (for unresponsive device)
st-flash --reset --connect-under-reset write build/cryptowallet.bin 0x08000000
```

**Pros:** One-liner, automatic  
**Cons:** Only after flash operation  
**Use case:** CI/CD pipelines, automated flashing

---

### 3️⃣ **OpenOCD reset** (Full control)
```bash
# Connect to STM32H7x via ST-LINK
openocd \
  -f interface/stlink.cfg \
  -f target/stm32h7x.cfg \
  -c "init" \
  -c "reset" \
  -c "exit"

# Or with Telnet for manual control
openocd \
  -f interface/stlink.cfg \
  -f target/stm32h7x.cfg &

# Then in another terminal:
telnet localhost 4444

# In Telnet CLI:
> reset               # Soft reset
> reset halt          # Reset and stop CPU
> reset run           # Reset and run
> exit

# Kill OpenOCD
pkill openocd
```

**Pros:** Full debug control, multiple reset modes  
**Cons:** More complex, requires config files  
**Use case:** Debugging, advanced scenarios

---

### 4️⃣ **Manual Reset** (Hardware button)
```bash
# Just press the RESET button on Nucleo board
# (Black button on board - hold 1 second)
```

**Pros:** Always works, no software needed  
**Cons:** Manual, not for CI/CD  
**Use case:** Emergency, connectivity issues

---

## 📋 Reset Types Explained

| Type | Command | Effect | Use Case |
|------|---------|--------|----------|
| **Soft Reset** | `st-flash reset` | CPU restart only | Normal operation |
| **Hard Reset** | RESET button | Full device reset | Emergency |
| **Connect-under-Reset** | `--connect-under-reset` | For stuck/bricked devices | Recovery |
| **Halt & Reset** | `reset halt` (OpenOCD) | Stop CPU after reset | Debugging |
| **Run & Reset** | `reset run` (OpenOCD) | Resume execution | Normal |

---

## 🛠️ Practical Scenarios

### After Flashing New Firmware
```bash
# Option 1: Combined (recommended)
st-flash --reset write build/cryptowallet.bin 0x08000000

# Option 2: Separate steps
st-flash write build/cryptowallet.bin 0x08000000
sleep 1
st-flash reset
```

### If Device Becomes Unresponsive
```bash
# Method 1: st-flash with force
st-flash --connect-under-reset --reset write build/cryptowallet.bin 0x08000000

# Method 2: OpenOCD recovery
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "init; reset halt; exit"
sleep 2

# Method 3: Manual
# Press RESET button for 1-2 seconds on Nucleo board
```

### During Development (Iterative flashing)
```bash
# Bash script for rapid iteration
#!/bin/bash
echo "Building..."
make clean && make -j$(nproc) || exit 1

echo "Flashing..."
st-flash --reset write build/cryptowallet.bin 0x08000000

echo "✓ Done - Device reset"
```

### Before Reading UART Output
```bash
# Flash with reset, then immediately listen
st-flash --reset write build/cryptowallet.bin 0x08000000
sleep 0.5
python3 -c "
import serial
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
while True:
    if ser.in_waiting:
        print(ser.read(1).decode('utf-8', errors='ignore'), end='')
"
```

---

## 🧪 Workflow Integration

### In `.gitea/workflows/`

```yaml
hil-test:
  runs-on: host
  steps:
    - uses: actions/checkout@v3
    
    - name: Build firmware
      run: make -j$(nproc)
    
    - name: Flash with reset
      run: |
        echo "Flashing to STM32H743ZI2..."
        st-flash --reset write build/cryptowallet.bin 0x08000000 || \
        echo "⚠️ Flash failed (device may not be connected)"
    
    - name: Wait for device stabilization
      run: sleep 2
    
    - name: Verify reset
      run: |
        echo "Checking device status..."
        st-flash --info || echo "Device checking skipped"
    
    - name: Capture UART after reset
      run: |
        python3 scripts/capture_rng_uart.py \
          --port /dev/ttyACM0 \
          --out rng.bin \
          --bytes 1000000
```

---

## 📡 Read UART After Reset

### Python Script
```python
#!/usr/bin/env python3
import serial
import time
import subprocess

# Reset device
print("Resetting device...")
subprocess.run(["st-flash", "reset"])

# Wait for device to restart
time.sleep(1)

# Connect UART
print("Connecting to UART...")
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
time.sleep(0.5)

# Read output
print("Device output:")
timeout = time.time() + 10  # 10 second timeout
while time.time() < timeout:
    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        print(data.decode('utf-8', errors='ignore'), end='')

ser.close()
print("\n✓ Done")
```

### Shell Script
```bash
#!/bin/bash

# Reset
echo "Resetting STM32..."
st-flash reset

# Wait
sleep 1

# Read UART with timeout
echo "Reading UART output (10 seconds)..."
timeout 10 python3 -c "
import serial
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
try:
    data = ser.read(10000)
    print(data.decode('utf-8', errors='ignore'))
finally:
    ser.close()
" || echo "UART read timeout or error"
```

---

## 🔍 Troubleshooting Reset

| Problem | Solution |
|---------|----------|
| `Device not found` | Check USB cable, press RESET button, try `--connect-under-reset` |
| `Permission denied /dev/...` | Add user to `plugdev` group: `sudo usermod -aG plugdev $USER` |
| `Cannot claim USB device` | Another process has it; `lsof \| grep tty` to find, or `pkill -f openocd` |
| `Reset doesn't work` | Try OpenOCD: `openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "init; reset; exit"` |
| `Device stuck/bricked` | Use `st-flash --connect-under-reset` or press RESET button manually |

---

## ⚡ Quick Commands Reference

```bash
# Basic reset
st-flash reset

# Flash + reset (recommended)
st-flash --reset write build/cryptowallet.bin 0x08000000

# Recovery reset (for stuck devices)
st-flash --connect-under-reset reset

# OpenOCD interactive (advanced)
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg

# Manual hardware reset
# Press RESET button on Nucleo-H743ZI2 board (black button)
```

---

## 📊 Reset Timing

```
Flash Operation:     ~2-5 seconds
Reset Execution:     <1 second
Device Boot:         ~1-2 seconds
UART Ready:          ~500ms after boot

Total (Flash+Reset):  ~5 seconds
```

---

## 🎯 Recommended Reset Strategy for CI/CD

### Simple (Recommended for most cases)
```bash
# In .gitea/workflows:
st-flash --reset write build/cryptowallet.bin 0x08000000
sleep 1  # Wait for device to stabilize
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --bytes 1000000
```

### Robust (With error handling)
```bash
# Flash
st-flash --reset write build/cryptowallet.bin 0x08000000 || {
  echo "Initial flash failed, trying recovery..."
  st-flash --connect-under-reset --reset write build/cryptowallet.bin 0x08000000
}

# Verify
sleep 2
st-flash --info || echo "Warning: Could not verify device"

# Test
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --bytes 1000000
```

### Production (With retry logic)
```bash
#!/bin/bash
MAX_RETRIES=3
RETRY=0

while [ $RETRY -lt $MAX_RETRIES ]; do
  echo "Flash attempt $((RETRY+1))/$MAX_RETRIES..."
  
  st-flash --reset write build/cryptowallet.bin 0x08000000 && {
    echo "✓ Flash successful"
    sleep 2
    exit 0
  }
  
  RETRY=$((RETRY+1))
  if [ $RETRY -lt $MAX_RETRIES ]; then
    echo "⚠️ Attempting recovery..."
    sleep 1
  fi
done

echo "✗ Flash failed after $MAX_RETRIES attempts"
exit 1
```

---

## 🚀 Next Steps

1. **For immediate use**: Use `st-flash --reset write ...`
2. **For debugging**: Use `openocd` for manual control
3. **For CI/CD**: Integrate into workflows with error handling
4. **For testing**: Reset before reading UART output
