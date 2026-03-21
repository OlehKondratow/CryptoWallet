# Host Runner - Working Directory & Test Setup

## 📁 Working Directory Structure

### Host Runner Home
```
~/gitea-runner/
├── act_runner                    # Binary (from /usr/local/bin)
├── config.yaml                   # Runner configuration
├── .runner                        # Registration data (auto-generated)
└── logs/                         # (implicit, through systemd)
```

**Current directory during CI job execution:**
- Git repo is cloned to: `$RUNNER_WORKSPACE/*/admin/CryptoWallet/`
- Jobs execute in: `$PWD` = repo root

### Project Structure (for CI)
```
/tmp/runner-work/admin/CryptoWallet/  (dynamic during job)
├── Makefile
├── Core/                         # Source code
├── scripts/                      # Test & utility scripts
├── .gitea/workflows/            # Pipeline configs
├── requirements-test.txt         # Python deps
├── build/                        # Compiler output
└── (other files)
```

**In workflows, use relative paths:**
```yaml
- run: make -j$(nproc)           # Builds to ./build/
- run: ls -lh build/             # Check artifacts
```

---

## 🔌 UART & Hardware Access (Host Runner)

### Available UART Ports
```
/dev/ttyACM0    ← Primary (ST-LINK Virtual COM Port)
/dev/ttyACM1    ← Secondary (if multiple devices)
/dev/ttyUSB0    ← USB Serial adapter 0
/dev/ttyUSB1    ← USB Serial adapter 1
```

**Status on this system:**
```
✓ /dev/ttyACM0 - Available (crw-rw-rw-+)
✓ pyserial 3.5 - Installed
✓ dieharder - Available
```

### Python Test Scripts Available
```
scripts/capture_rng_uart.py      # Capture entropy via UART
scripts/run_dieharder.py         # Run DIEHARDER tests
scripts/test_rng_signing_comprehensive.py  # Full test suite
scripts/bootloader_secure_signing_test.py  # Bootloader tests
```

### Requirements
```bash
# For UART & test scripts
python3 -m pip install -r requirements-test.txt

# Installs:
- pyserial>=3.5       (UART communication)
- requests>=2.32      (HTTP client)
- colorama>=0.4       (Terminal colors)
- pytest>=7.0         (Unit tests)
```

### Setup in Workflow

```yaml
hil-test:
  runs-on: host              # MUST be "host" for hardware access
  steps:
    - uses: actions/checkout@v3
    
    - name: Setup Python venv
      run: |
        python3 -m venv .venv
        .venv/bin/pip install -r requirements-test.txt
    
    - name: Capture UART data
      run: |
        .venv/bin/python3 scripts/capture_rng_uart.py \
          --port /dev/ttyACM0 \
          --out rng.bin \
          --bytes 134217728  # 128 MiB
    
    - name: Run DIEHARDER
      run: |
        dieharder -f rng.bin -a | tee dieharder.log
```

---

## 📦 Build Artifacts

### Where They Go
```
./build/                         # During job execution
├── cryptowallet.bin            # Final binary (upload this)
├── cryptowallet.elf            # ELF with symbols
├── cryptowallet.dis            # Disassembly (from objdump)
└── cryptowallet.map            # Link map
```

### Upload in Workflow
```yaml
- name: Upload artifacts
  uses: actions/upload-artifact@v3
  with:
    name: firmware-build
    path: |
      build/cryptowallet.bin
      build/cryptowallet.elf
      build/cryptowallet.dis
    retention-days: 30
```

### Download in Workflow
```yaml
- name: Download artifacts
  uses: actions/download-artifact@v3
  with:
    name: firmware-build
    path: build/

# After download:
- run: ls -lh build/
```

---

## 🚀 Key Environment Variables (Host Runner)

### Set Automatically by Gitea
```
RUNNER_WORKSPACE        # Base work directory
RUNNER_TEMP            # Temp directory
RUNNER_TOOL_CACHE      # Cache directory
GITHUB_WORKSPACE       # Alias for RUNNER_WORKSPACE
GITHUB_ACTION_PATH     # Path to action
```

### Custom in Systemd
```ini
Environment="DOCKER_HOST=unix:///run/user/1000/podman/podman.sock"
```

### Available on Host
```bash
pwd                    # Shows current job dir
env | grep RUNNER      # See all RUNNER_* vars
ls $RUNNER_WORKSPACE   # Browse workspace
```

---

## 📊 Test Workflow Examples

### Minimal UART Test (5 min)
```yaml
uart-test:
  runs-on: host
  steps:
    - uses: actions/checkout@v3
    - run: python3 -c "import serial; print('✓ UART ready')"
    - run: ls -la /dev/ttyACM0
    - run: echo 'UART test passed'
```

### Full RNG+DIEHARDER (2+ hours)
```yaml
hil-full-test:
  runs-on: host
  timeout-minutes: 180
  steps:
    - uses: actions/checkout@v3
    - run: python3 -m pip install -r requirements-test.txt
    - run: make USE_RNG_DUMP=1 -j$(nproc)
    - run: st-flash write build/cryptowallet.bin 0x08000000
    - run: python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --bytes 134217728
    - run: dieharder -f rng.bin -a
```

---

## 🔍 Debugging Workflow Issues

### Check Host Runner Status
```bash
# From your local machine
systemctl --user status gitea-runner

# View logs
journalctl --user -f -u gitea-runner

# Check config
cat ~/gitea-runner/config.yaml
```

### During Workflow Execution
- Gitea UI shows live logs: Actions → Job → View logs
- Download artifacts for inspection after job
- Use `run: ls -lha` to verify files exist
- Use `run: pwd` to see working directory

### Common Issues

| Issue | Fix |
|-------|-----|
| UART device not found | Check `/dev/ttyACM*` exists locally first |
| Permission denied on `/dev/ttyACM0` | User must be in `plugdev` group |
| Python import error | Run `pip install -r requirements-test.txt` |
| Build outputs missing | Check `./build/` exists after `make` |
| Artifacts not uploading | Verify path is correct relative to job dir |

---

## 📝 Current Setup

### Host Runner Info
```
Name: host-runner-ws8
Status: ACTIVE (systemd service)
Version: v0.3.0
Labels: ubuntu-latest, ubuntu-24.04, ubuntu-22.04
Capacity: 1 job at a time
Podman Socket: /run/user/1000/podman/podman.sock
```

### For Workflows
- Use `runs-on: ubuntu-latest` for any job (available on host runner)
- Use `runs-on: host` if you want to be explicit
- Build jobs: ~5-10 minutes
- HIL tests: 30 minutes to 3+ hours depending on scope

---

## 🎯 Next Steps

1. **Test basic workflow**: Push to trigger `.gitea/workflows/test-runner.yml`
2. **Test build**: Trigger `.gitea/workflows/stm32-make-workflow.yml`
3. **Test UART**: Create custom workflow with `capture_rng_uart.py`
4. **Full HIL**: When hardware is connected

All test scripts are in `scripts/` and ready to use!
