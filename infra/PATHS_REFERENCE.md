# Full Paths Reference - Host Runner & CI

## Act runner: root vs regular user

- If **act_runner runs as root** (including launch from project directory): home directory
  is `/root/`, config may be next to the binary or in `/root/gitea-runner/`;
  jobs with label `cryptowallet-host:host` (see `simple-ci.yml`) run **as root** on host.
- If runner runs as a **regular user** (example below uses `pilgrim`): paths are like
  `/home/pilgrim/gitea-runner/`, and for Podman use `DOCKER_HOST=unix:///run/user/UID/...`.

Below is an example for a user-level runner; replace `/home/pilgrim` with your layout.

## 🏠 Local Machine Paths

### Host Runner Installation
```
/home/pilgrim/gitea-runner/
├── act_runner                    # Binary executable
├── config.yaml                   # Configuration file
├── .runner                        # Registration metadata
└── logs/                         # (through systemd)
```

**Systemd service file:**
```
/home/pilgrim/.config/systemd/user/gitea-runner.service
```

### Project Repository
```
/data/projects/CryptoWallet/
├── Makefile                      # Build configuration
├── requirements-test.txt         # Python dependencies
├── scripts/                      # Test & utility scripts
├── build/                        # Build outputs
├── Core/                         # Source code
├── .gitea/workflows/            # CI/CD workflows
└── documentation/                # Canonical manual + generated snippets
```

### Build Artifacts (Local)
```
/data/projects/CryptoWallet/build/
├── cryptowallet.bin             # Final binary (205 KB)
├── cryptowallet.elf             # ELF with symbols (602 KB)
├── cryptowallet.dis             # Disassembly
└── cryptowallet.map             # Linker map
```

### Test Scripts
```
/data/projects/CryptoWallet/scripts/
├── capture_rng_uart.py          # UART data capture
├── run_dieharder.py             # Statistical tests
├── test_rng_signing_comprehensive.py  # Full test suite
├── bootloader_secure_signing_test.py
└── requirements.txt             # (scripts dependencies)
```

### UART/Hardware
```
/dev/ttyACM0                      # Primary UART (ST-LINK)
/dev/ttyACM1                      # Secondary (if available)
/dev/ttyUSB0                      # USB Serial 0 (if available)
/dev/ttyUSB1                      # USB Serial 1 (if available)
```

---

## 🚀 CI Execution Paths (During Workflow)

When a workflow job runs, the runner clones the repo to a **dynamic workspace**:

### Dynamic Base Paths (Set by Runner)
```
RUNNER_WORKSPACE    = /run/user/1000/podman/containers/[HASH]/work/
                      (or similar - varies by execution)

RUNNER_TEMP         = /tmp/runner-tmp-[HASH]/
RUNNER_TOOL_CACHE   = /opt/runner-tool-cache/
```

### Repository After Checkout
```
$RUNNER_WORKSPACE/admin/CryptoWallet/
├── .git/
├── Makefile
├── Core/
├── scripts/
├── build/                        (doesn't exist yet)
├── requirements-test.txt
└── .gitea/workflows/
```

### Build Output During Job
```
$RUNNER_WORKSPACE/admin/CryptoWallet/build/
├── cryptowallet.bin
├── cryptowallet.elf
├── cryptowallet.dis
└── cryptowallet.map
```

### Temp Files During Job
```
$RUNNER_WORKSPACE/admin/CryptoWallet/
├── rng.bin                       # Captured UART data
├── dieharder_results.txt         # Test results
├── .venv/                        # Python venv (if created)
└── (other temporary files)
```

---

## 📝 In Workflow YAML - How to Use Paths

### Relative Paths (Recommended - easier)
```yaml
steps:
  - uses: actions/checkout@v3      # Clones to repo root
  
  - run: make -j$(nproc)           # Builds ./build/
  
  - run: ls -lh build/             # Check ./build/
  
  - run: cat requirements-test.txt  # Read ./requirements-test.txt
```

### Absolute Paths with Variables
```yaml
steps:
  - run: echo $RUNNER_WORKSPACE    # Print workspace path
  
  - run: ls -lh $RUNNER_WORKSPACE/admin/CryptoWallet/build/
  
  - run: pwd                       # Should show $RUNNER_WORKSPACE/admin/CryptoWallet/
```

### Device Paths (Always Absolute)
```yaml
steps:
  - run: ls -la /dev/ttyACM0       # Must use /dev/ prefix
  
  - run: |
      python3 scripts/capture_rng_uart.py \
        --port /dev/ttyACM0 \
        --out ./rng.bin
```

---

## 🔄 Artifact Upload/Download Paths

### During Upload
```yaml
- name: Upload artifacts
  uses: actions/upload-artifact@v3
  with:
    name: firmware-build
    path: |
      build/cryptowallet.bin       # Relative to job working dir
      build/cryptowallet.elf
      build/cryptowallet.dis
```

**Stored on Gitea:**
```
http://localhost:3000/admin/CryptoWallet/runs/[RUN_ID]/artifacts/firmware-build/
```

### During Download
```yaml
- name: Download artifacts
  uses: actions/download-artifact@v3
  with:
    name: firmware-build
    path: build/                   # Downloads to ./build/

# After this, files exist at:
# ./build/cryptowallet.bin
# ./build/cryptowallet.elf
# ./build/cryptowallet.dis
```

---

## 🎯 Complete Workflow Path Example

```yaml
hil-test:
  runs-on: cryptowallet-host
  steps:
    # After checkout, working dir is at repo root
    # Current working directory implicitly: $RUNNER_WORKSPACE/admin/CryptoWallet/
    
    - uses: actions/checkout@v3
    
    - name: Show paths
      run: |
        echo "pwd: $(pwd)"                          # Shows: /...admin/CryptoWallet/
        echo "ls:"; ls -lha                         # Shows repo contents
        echo "RUNNER_WORKSPACE: $RUNNER_WORKSPACE"
        echo "build dir: $(pwd)/build"              # Will be: /...admin/CryptoWallet/build
    
    - name: Build
      run: make -j$(nproc)                         # Creates ./build/
    
    - name: Check build output
      run: |
        test -f ./build/cryptowallet.bin || { echo "Build failed!"; exit 1; }
        ls -lh ./build/
    
    - name: Setup Python
      run: |
        python3 -m venv .venv                      # Creates ./venv
        .venv/bin/pip install -r requirements-test.txt
    
    - name: Capture UART
      run: |
        .venv/bin/python3 scripts/capture_rng_uart.py \
          --port /dev/ttyACM0 \
          --out ./rng.bin \
          --bytes 1000000
    
    - name: Upload results
      if: always()
      uses: actions/upload-artifact@v3
      with:
        name: hil-results
        path: |
          ./build/cryptowallet.bin
          ./rng.bin
          dieharder_results.txt
```

---

## 📊 Reference Table

| Path Type | Local | In CI Job |
|-----------|-------|-----------|
| **Runner Home** | `/home/pilgrim/gitea-runner/` | N/A |
| **Repo Root** | `/data/projects/CryptoWallet/` | `$RUNNER_WORKSPACE/admin/CryptoWallet/` |
| **Build Dir** | `/data/projects/CryptoWallet/build/` | `./build/` (relative) |
| **Scripts** | `/data/projects/CryptoWallet/scripts/` | `./scripts/` (relative) |
| **UART Device** | `/dev/ttyACM0` | `/dev/ttyACM0` (same) |
| **Python Deps** | `/data/projects/CryptoWallet/requirements-test.txt` | `./requirements-test.txt` |
| **Temp Files** | (N/A) | `$RUNNER_TEMP/...` |
| **Cached Files** | (N/A) | `$RUNNER_TOOL_CACHE/...` |

---

## 💡 Best Practices

✅ **Use relative paths in workflows** - portable across different runners
```yaml
make -j$(nproc)              # Good
./build/cryptowallet.bin     # Good
```

❌ **Avoid hardcoded absolute paths** - fails if paths change
```yaml
make -C /data/projects/...   # Bad
/data/projects/.../build/    # Bad
```

✅ **Device paths are always absolute**
```yaml
/dev/ttyACM0                 # Must be absolute
```

✅ **Use environment variables when needed**
```yaml
$RUNNER_WORKSPACE/admin/CryptoWallet/build/
$RUNNER_TEMP/myfile.txt
```

---

## 🔍 Debug: Print All Paths in Workflow

```yaml
- name: Debug paths
  run: |
    echo "=== Environment ==="
    env | grep RUNNER | sort
    echo ""
    echo "=== Working Directory ==="
    pwd
    echo ""
    echo "=== Repository Contents ==="
    ls -la
    echo ""
    echo "=== UART Devices ==="
    ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "None"
```

When you run this job, check the logs in Gitea to see all actual paths!
