# CryptoWallet Project Structure & Working Guide

## 📁 Complete Directory Map

```
/data/projects/CryptoWallet/
│
├── 📄 Makefile                      ← BUILD: make, make flash, make clean
├── 📄 Doxyfile                      ← Documentation generation config
├── 📄 FreeRTOSConfig.h              ← FreeRTOS configuration
├── 📄 requirements-test.txt         ← Python test dependencies
│
├── 📁 Core/                         ← ⭐ MAIN SOURCE CODE
│   ├── Inc/                         ← Header files (.h)
│   └── Src/                         ← Implementation files (.c)
│
├── 📁 Drivers/                      ← Hardware drivers
│   ├── libgcc/                      ← ARM runtime library
│   └── ssd1306/                     ← OLED display driver
│
├── 📁 ThirdParty/                   ← External libraries
│   └── trezor-crypto/               ← Trezor crypto library (ECC, RNG)
│
├── 📁 build/                        ← 🔨 BUILD OUTPUT (auto-generated)
│   ├── cryptowallet.bin             ← Final firmware (flash this!)
│   ├── cryptowallet.elf             ← ELF with debug symbols
│   ├── cryptowallet.dis             ← Disassembly listing
│   └── cryptowallet.map             ← Linker map
│
├── 📁 scripts/                      ← 🧪 TEST & UTILITY SCRIPTS
│   ├── capture_rng_uart.py          ← Capture entropy via UART
│   ├── run_dieharder.py             ← Run DIEHARDER statistical tests
│   ├── test_rng_signing_comprehensive.py  ← Full test suite
│   ├── bootloader_secure_signing_test.py  ← Bootloader tests
│   ├── test_commands_reference.sh   ← Shell command examples
│   ├── requirements.txt             ← Python dependencies for scripts
│   └── __pycache__/                 ← Compiled Python cache
│
├── 📁 docs_src/                     ← 📖 DOCUMENTATION SOURCE
│   ├── TEST_SCRIPTS_README.md       ← How to run tests
│   ├── TESTING_GUIDE_RNG_SIGNING.md ← Detailed testing guide
│   ├── architecture.md              ← Project architecture
│   ├── crypto/                      ← Crypto documentation
│   └── analysis/                    ← Code analysis docs
│
├── 📁 docs_doxygen/                 ← 📋 GENERATED DOXYGEN DOCS
│   ├── html/                        ← HTML documentation
│   ├── xml/                         ← XML source
│   └── md/                          ← Markdown files
│
├── 📁 .gitea/                       ← ⚙️  CI/CD CONFIGURATION
│   └── workflows/                   ← GitHub Actions / Gitea Actions
│       ├── test-runner.yml          ← Simple runner test
│       ├── build-test-hil.yml       ← Full build + HIL test
│       ├── stm32-make-workflow.yml  ← Makefile-based build
│       ├── stm32-manual-test.yml    ← Manual test phases
│       └── debug-runners.yml        ← Debug runner info
│
├── 📁 infra/                        ← 🐙 INFRASTRUCTURE
│   ├── docker-compose.yml           ← Gitea service config
│   ├── setup-host-runner.sh         ← Host runner installer
│   ├── .env.local                   ← Local environment variables
│   ├── .env                         ← Template env variables
│   ├── HOST_RUNNER_README.md        ← Runner setup guide
│   ├── HOST_RUNNER_WORKING_DIR.md   ← Working directories guide
│   ├── PATHS_REFERENCE.md           ← Full paths documentation
│   ├── QUICK_START_HOST_RUNNER.txt  ← Quick start (30 sec)
│   ├── act-runner-data/             ← Runner data (auto-generated)
│   ├── docker-compose/              ← Old configs (backup)
│   ├── gitea-config/                ← Gitea config backup
│   └── gitea-data/                  ← Gitea database backup
│
├── 📁 .venv-test/                   ← 🐍 Python venv for testing
│   ├── bin/                         ← Executables (python, pip)
│   ├── lib/                         ← Installed packages
│   └── include/
│
├── 📁 .venv-docs/                   ← 🐍 Python venv for docs
│   └── (similar structure)
│
├── 📁 .vscode/                      ← 💻 VS Code / Cursor config
│   ├── launch.json                  ← Debugger config
│   ├── tasks.json                   ← Build tasks
│   ├── c_cpp_properties.json        ← IntelliSense config
│   └── extensions.json              ← Recommended extensions
│
├── 📁 .git/                         ← 🔧 Git repository metadata
│
├── 📁 .cursor/                      ← 💡 Cursor IDE config
│   ├── rules/                       ← Cursor rules
│   ├── agent-transcripts/           ← Chat history
│   └── assets/                      ← Images & resources
│
├── 📁 udev/                         ← 🔌 USB device rules
│   └── (STM32 ST-LINK rules)
│
├── 📁 firmware_backup/              ← 💾 Backup binaries
│   └── (previous firmware versions)
│
└── 📁 Src/                          ← (Additional source)
```

---

## 🎯 What to Use Where

### 🔨 **Building Firmware**
```bash
cd /data/projects/CryptoWallet/

# Build variants
make                                  # Default (minimal-lwip)
make clean                           # Clean build artifacts
make USE_RNG_DUMP=1                  # Enable RNG debug output
make USE_CRYPTO_SIGN=1               # Enable signing tests
make USE_LWIP=1                      # Enable Ethernet stack
make -j$(nproc)                      # Parallel build

# Output: ./build/cryptowallet.bin (flash this)
```

### 🧪 **Testing**
```bash
# Capture RNG data
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --bytes 1000000

# Run DIEHARDER tests
dieharder -f rng.bin -a

# Full test suite
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# Signing tests
python3 scripts/test_rng_signing_comprehensive.py --mode signing
```

### 📖 **Documentation**
```bash
# View documentation
cat docs_src/architecture.md
cat docs_src/TESTING_GUIDE_RNG_SIGNING.md

# Generate Doxygen docs
doxygen Doxyfile                     # Generates docs_doxygen/

# View HTML docs
open docs_doxygen/html/index.html
```

### ⚙️  **Running Workflows**
```bash
cd /data/projects/CryptoWallet/

# Manual workflow trigger (via Gitea UI)
# http://localhost:3000/admin/CryptoWallet/actions

# Or push to trigger automatically
git push
```

### 🔌 **Hardware Flashing**
```bash
# Flash built firmware
st-flash write ./build/cryptowallet.bin 0x08000000

# Read device ID
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "init; targets; exit"
```

---

## 🛠️ Essential Commands

### From Project Root: `/data/projects/CryptoWallet/`

| Task | Command |
|------|---------|
| **Build** | `make -j$(nproc)` |
| **Clean** | `make clean` |
| **Flash** | `st-flash write build/cryptowallet.bin 0x08000000` |
| **Check UART** | `ls -la /dev/ttyACM0` |
| **Capture RNG** | `python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --bytes 1000000` |
| **Run Tests** | `python3 scripts/test_rng_signing_comprehensive.py --mode rng` |
| **View Logs** | `journalctl --user -f -u gitea-runner` |
| **Git Push** | `git push` |
| **Check Status** | `git status` |

---

## 📊 Key File Reference

### Configuration & Build
| File | Purpose | Location |
|------|---------|----------|
| `Makefile` | Build orchestration | `./Makefile` |
| `Doxyfile` | Doc generation config | `./Doxyfile` |
| `FreeRTOSConfig.h` | FreeRTOS tuning | `./FreeRTOSConfig.h` |
| `.vscode/launch.json` | Debugger config | `./.vscode/launch.json` |

### Source Code
| Directory | Contents |
|-----------|----------|
| `Core/Inc/` | Header files (`*.h`) |
| `Core/Src/` | Implementation (`*.c`) |
| `Drivers/` | Hardware drivers |
| `ThirdParty/` | External libraries |

### Build Output
| File | Size | Purpose |
|------|------|---------|
| `build/cryptowallet.bin` | ~205 KB | **Flash this to device** |
| `build/cryptowallet.elf` | ~602 KB | Debug symbols, analysis |
| `build/cryptowallet.dis` | ~5 MB | Disassembly listing |

### Testing
| File | Purpose | Runtime |
|------|---------|---------|
| `scripts/capture_rng_uart.py` | Capture entropy | 5-30 min |
| `scripts/run_dieharder.py` | Statistical tests | 1-3 hours |
| `scripts/test_rng_signing_comprehensive.py` | Full suite | 2+ hours |

### CI/CD Workflows
| File | Trigger | Duration |
|------|---------|----------|
| `test-runner.yml` | Manual | <1 min |
| `stm32-make-workflow.yml` | Manual | 5 min |
| `build-test-hil.yml` | Manual | 30+ min |
| `stm32-manual-test.yml` | Manual | Varies |

---

## 🚀 Quick Start from Project Root

```bash
# 1. Navigate
cd /data/projects/CryptoWallet

# 2. Build
make clean
make USE_RNG_DUMP=1 -j$(nproc)

# 3. Check output
ls -lh build/cryptowallet.bin

# 4. Flash (if device connected)
st-flash write build/cryptowallet.bin 0x08000000

# 5. Test (if UART available)
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --bytes 1000000

# 6. Push to Gitea
git push

# 7. Monitor in Gitea UI
# http://localhost:3000/admin/CryptoWallet/actions
```

---

## 💾 Working with Build Artifacts

All builds output to: **`/data/projects/CryptoWallet/build/`**

```bash
# Check what was built
ls -lh ./build/

# Binary to flash
./build/cryptowallet.bin           # 205 KB

# For debugging
./build/cryptowallet.elf           # Contains symbols
./build/cryptowallet.dis           # Disassembly

# For analysis
./build/cryptowallet.map           # Linker map file
```

---

## 🔌 Hardware Connections

From project: `/data/projects/CryptoWallet/`

```bash
# Check UART device
ls -la /dev/ttyACM0

# Check if device recognized
lsusb | grep "STM"

# Access UART in Python
python3 -c "import serial; s = serial.Serial('/dev/ttyACM0', 115200); print('UART OK')"
```

---

## 📋 Current Status Check

```bash
cd /data/projects/CryptoWallet

echo "=== Project Status ==="
pwd

echo -e "\n=== Build Status ==="
[ -f build/cryptowallet.bin ] && echo "✓ Binary ready" || echo "✗ Not built"

echo -e "\n=== UART Status ==="
[ -c /dev/ttyACM0 ] && echo "✓ UART available" || echo "✗ No UART"

echo -e "\n=== Git Status ==="
git status

echo -e "\n=== Runner Status ==="
systemctl --user is-active gitea-runner && echo "✓ Runner active" || echo "✗ Runner inactive"
```

---

## 📝 Next Steps

All work is done in **`/data/projects/CryptoWallet/`**:

1. ✅ Build: `make -j$(nproc)` → `./build/cryptowallet.bin`
2. ✅ Test: `python3 scripts/capture_rng_uart.py ...`
3. ✅ Push: `git push`
4. ✅ Monitor: Gitea UI Actions tab
5. ✅ Debug: Check logs via `journalctl` or Gitea logs
