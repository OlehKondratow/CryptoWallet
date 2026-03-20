# 🐍 Python Virtual Environment Setup for CryptoWallet Testing

**Created:** 2026-03-20  
**Location:** `/data/projects/CryptoWallet/.venv-test`  
**Python:** 3.12.7  
**Status:** ✅ Ready to use

---

## 📋 What's Installed

Virtual environment `.venv-test` contains:

- **pyserial 3.5** — UART communication for RNG capture
- **requests 2.32.5** — HTTP client for signing tests
- **pip 26.0.1** — Package manager
- **setuptools 82.0.1** — Build utilities
- **wheel 0.46.3** — Distribution format

---

## 🚀 Quick Start in Cursor

### Method 1: Using Shell Commands

```bash
# Activate (one-time per terminal)
source .venv-test/bin/activate

# Run tests
python3 scripts/test_rng_signing_comprehensive.py --mode rng
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder
python3 scripts/test_rng_signing_comprehensive.py --mode signing

# Deactivate when done
deactivate
```

### Method 2: Using Helper Scripts

```bash
# One command to activate and show available tests
bash activate-tests.sh

# Run tests directly (auto-activates)
bash run-tests.sh --mode rng
bash run-tests.sh --mode dieharder
bash run-tests.sh --mode signing
```

### Method 3: Using Cursor Debugger

1. Open `.vscode/launch.json` in Cursor
2. Click on the test you want to run:
   - **Python: RNG Test (Capture)** — Capture 128 MiB RNG data
   - **Python: DIEHARDER Tests** — Run statistical tests
   - **Python: Transaction Signing Test** — Test signing
   - **Python: Verify All Systems** — Full verification

---

## 📁 Files Created

### Activation Scripts

| File | Purpose |
|------|---------|
| `activate-tests.sh` | Activate venv + show commands |
| `run-tests.sh` | Run tests with auto-activation |

### Configuration Files

| File | Purpose |
|------|---------|
| `.vscode/settings.json` | Python interpreter path for Cursor |
| `.vscode/launch.json` | Debug configurations for test modes |
| `requirements-test.txt` | Package list (flexible versions) |
| `requirements-test-lock.txt` | Pinned package versions |

### Virtual Environment

| Location | Purpose |
|----------|---------|
| `.venv-test/bin/python3` | Python interpreter |
| `.venv-test/lib/python3.12/site-packages/` | Installed packages |

---

## 🛠️ Common Tasks

### Activate Virtual Environment

```bash
source .venv-test/bin/activate

# Prompt changes to show activation
# (venv-test) user@machine:~/CryptoWallet$
```

### Run RNG Capture (30 minutes)

```bash
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

### Run DIEHARDER Tests (1-3 hours)

```bash
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --file rng.bin
```

### Run Signing Tests (10 minutes)

```bash
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
```

### Install Additional Packages

```bash
# Activate first
source .venv-test/bin/activate

# Install new package
pip install numpy

# List all packages
pip list

# Update package
pip install --upgrade requests
```

### Reinstall All Packages

```bash
# From locked versions
.venv-test/bin/pip install -r requirements-test-lock.txt

# Or from flexible requirements
.venv-test/bin/pip install -r requirements-test.txt
```

### Deactivate Virtual Environment

```bash
deactivate

# Prompt returns to normal
# user@machine:~/CryptoWallet$
```

---

## 🔧 Cursor Integration

### Automatic Python Selection

Cursor automatically uses `.venv-test/bin/python3` because of `.vscode/settings.json`:

```json
{
  "python.defaultInterpreterPath": "${workspaceFolder}/.venv-test/bin/python3"
}
```

**To verify:**
1. Press `Ctrl+Shift+P` → "Python: Select Interpreter"
2. Should show: `./.venv-test/bin/python3`

### Debug Configurations

Press `F5` in Cursor to run one of 5 pre-configured test modes:

1. **RNG Test (Capture)** — Capture UART RNG data
2. **DIEHARDER Tests** — Statistical analysis
3. **Transaction Signing Test** — HTTP signing tests
4. **Verify All Systems** — Complete verification
5. **Debug Mode** — Debug current Python file

---

## 📊 Environment Info

```bash
# Show Python info
python3 --version
which python3

# Show installed packages
pip list

# Show virtual environment location
echo $VIRTUAL_ENV

# Check if activated
[[ -n "$VIRTUAL_ENV" ]] && echo "✓ Activated" || echo "✗ Not activated"
```

---

## ⚠️ Troubleshooting

### "Command not found: python3"

**Solution:** Activate the virtual environment:
```bash
source .venv-test/bin/activate
```

### "ModuleNotFoundError: No module named 'pyserial'"

**Solution 1:** Ensure venv is activated
```bash
source .venv-test/bin/activate
```

**Solution 2:** Reinstall packages
```bash
.venv-test/bin/pip install -r requirements-test.txt
```

### "Permission denied" when running scripts

**Solution:** Make scripts executable
```bash
chmod +x activate-tests.sh run-tests.sh
```

### venv was deleted/corrupted

**Solution:** Recreate it
```bash
rm -rf .venv-test
python3 -m venv .venv-test
.venv-test/bin/pip install -r requirements-test.txt
```

---

## 🔒 Security Notes

- ✅ Virtual environment is **isolated** from system Python
- ✅ All changes are **local** to this project
- ✅ Dependencies are **version-pinned** in `requirements-test-lock.txt`
- ✅ No need for `sudo pip install`
- ⚠️ Never commit `.venv-test/` to git (already in `.gitignore`)

---

## 📚 Additional Resources

- **Test Documentation:** `docs_src/TESTING_GUIDE_RNG_SIGNING.md`
- **Cursor Python Support:** https://cursor.sh/docs/python
- **Virtual Environments:** https://docs.python.org/3/venv/
- **pyserial Documentation:** https://pyserial.readthedocs.io/
- **requests Documentation:** https://docs.requests.dev/

---

## ✅ Verification Checklist

After setup, verify everything works:

```bash
# 1. Check venv exists
[ -d ".venv-test" ] && echo "✓ venv exists"

# 2. Check Python accessible
.venv-test/bin/python3 --version

# 3. Check packages installed
.venv-test/bin/python3 -c "import serial, requests; print('✓ Packages OK')"

# 4. Check test script works
.venv-test/bin/python3 scripts/test_rng_signing_comprehensive.py --help

# 5. Check activation works
bash -c "source .venv-test/bin/activate && python3 --version"
```

---

## 🎯 Workflow Example

```bash
# Morning: Capture RNG
bash activate-tests.sh
python3 scripts/test_rng_signing_comprehensive.py --mode rng
# Output: rng.bin (128 MiB, ~30 min)

# Afternoon: Run DIEHARDER
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder
# Output: Statistical test results (1-3 hours)

# Evening: Test signing
make clean && make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4 && make flash
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
# Output: Signing verification results

# Cleanup
deactivate
```

---

**Status:** ✅ Ready  
**Version:** 1.0  
**Last Updated:** 2026-03-20
