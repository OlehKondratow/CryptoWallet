#!/bin/bash
# -*- mode: markdown -*-

# CryptoWallet Testing Setup - Complete Guide

## ✅ What's Been Done

All Python dependencies have been installed and configured for the CryptoWallet testing suite.

### Files Created

| File | Size | Purpose |
|------|------|---------|
| `requirements-test.txt` | 696 B | Python package specifications with version ranges |
| `requirements-test-lock.txt` | 705 B | Exact versions of all installed packages |
| `install-test-deps.sh` | 3.4 K | Automated installer for system and Python dependencies |
| `INSTALL_DIEHARDER.txt` | 2.6 K | Quick reference for dieharder installation |
| `docs_src/INSTALL_TEST_DEPS.md` | ~7 K | Complete installation guide with troubleshooting |

### Python Packages Installed (22 total)

#### Core Dependencies (Required)
- **pyserial 3.5** - Serial port communication with CryptoWallet device
- **requests 2.32.5** - HTTP client for REST API communication
- **colorama 0.4.6** - Colored terminal output

#### Development Tools (Optional but Recommended)
- **pytest 7.4.4** - Unit testing framework
- **black 24.10.0** - Code formatter
- **flake8 7.3.0** - Code linter

#### Build & Package Management
- **setuptools 82.0.1**, **pip 26.0.1**, **wheel 0.46.3**

#### Transitive Dependencies
- charset-normalizer, idna, urllib3 (HTTP support)
- click, pathspec, platformdirs (CLI tools)
- And others for linting/formatting

## 🐛 Issue Fixed

**Problem:** `pytest-serial>=0.0.9` - This package doesn't exist in PyPI

**Root Cause:** Erroneous package specification in original requirements

**Solution:** Removed non-existent package, kept only valid dependencies

## 🚀 Quick Start

### 1. Install System Dependencies

**Automatic (Recommended):**
```bash
cd /data/projects/CryptoWallet
./install-test-deps.sh
```

**Manual (Choose your OS):**

Linux (Debian/Ubuntu):
```bash
sudo apt update && sudo apt install -y dieharder
```

Linux (Fedora/RHEL):
```bash
sudo dnf install -y dieharder
```

macOS:
```bash
brew install dieharder
```

### 2. Activate Virtual Environment

```bash
source .venv-test/bin/activate
```

### 3. Verify Installation

```bash
# Check dieharder
dieharder --version

# Check Python packages
python3 -c "import serial, requests, colorama, pytest; print('✓ All packages OK')"

# Test the test script
python3 scripts/test_rng_signing_comprehensive.py --help
```

## 📖 Documentation

- **Installation Details**: `docs_src/INSTALL_TEST_DEPS.md`
- **Virtual Environment Setup**: `docs_src/VENV_SETUP.md`
- **Testing Guide**: `docs_src/TESTING_GUIDE_RNG_SIGNING.md`
- **Quick Start**: `VENV_QUICKSTART.txt`
- **Requirements**: `requirements-test.txt` and `requirements-test-lock.txt`

## 🎯 Available Test Modes

Once everything is set up, you can run:

```bash
# RNG Test (capture and analyze entropy)
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# Dieharder Analysis (statistical tests)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder

# Transaction Signing Test
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10

# Comprehensive Test Suite
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
```

## 🔧 Using with Cursor IDE

### Method 1: Terminal
```bash
source .venv-test/bin/activate
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

### Method 2: Run with Helper Script
```bash
bash activate-tests.sh
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

### Method 3: Cursor Debugger (F5)
1. Open `.vscode/launch.json` - debug configs are pre-configured
2. Press F5 or Ctrl+Shift+D
3. Select test mode from dropdown
4. Runs with debugger attached

## ✅ Verification Checklist

- [ ] `dieharder --version` shows version number
- [ ] `.venv-test/bin/python3 --version` shows Python 3.12
- [ ] `python3 -c "import serial"` succeeds (no ImportError)
- [ ] `python3 -c "import requests"` succeeds
- [ ] `python3 scripts/test_rng_signing_comprehensive.py --help` shows full help
- [ ] USB/UART device is connected (for `--mode rng`)

## 📝 Requirements Files

### requirements-test.txt
Contains flexible version constraints for development:
- Use when: Installing in new environment, flexible versions acceptable
- Install with: `pip install -r requirements-test.txt`

### requirements-test-lock.txt
Contains exact pinned versions for reproducibility:
- Use when: Exact reproduction needed, deployment to production
- Install with: `pip install -r requirements-test-lock.txt`
- Generated: `pip freeze > requirements-test-lock.txt`

## 🐛 Troubleshooting

### "dieharder: command not found"
```bash
# Check if installed
which dieharder

# Install for your OS (see Quick Start section)
```

### "ModuleNotFoundError: No module named 'serial'"
```bash
# Activate venv
source .venv-test/bin/activate

# Reinstall packages
pip install -r requirements-test.txt

# Verify
python3 -c "import serial; print('OK')"
```

### "Permission denied" on /dev/ttyUSB0
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER
newgrp dialout

# Verify
ls -l /dev/ttyUSB0
```

### Still having issues?
See `docs_src/INSTALL_TEST_DEPS.md` for complete troubleshooting guide.

## 📊 Environment Information

- **Python Version**: 3.12.7
- **Virtual Environment**: `.venv-test/`
- **Total Packages**: 22
- **Setup Date**: 2026-03-20
- **Status**: ✅ Ready

## 🔄 Updating Dependencies

To update packages while maintaining version constraints:

```bash
source .venv-test/bin/activate
pip install --upgrade -r requirements-test.txt
pip freeze > requirements-test-lock.txt
```

## 📚 Related Files

```
.
├── .venv-test/                          # Virtual environment
├── requirements-test.txt                # Package specs
├── requirements-test-lock.txt           # Locked versions
├── install-test-deps.sh                 # Auto-installer
├── INSTALL_DIEHARDER.txt               # Dieharder reference
├── activate-tests.sh                    # Venv activation helper
├── run-tests.sh                         # Test runner script
├── VENV_QUICKSTART.txt                 # Quick start
├── docs_src/
│   ├── INSTALL_TEST_DEPS.md            # Complete setup guide
│   ├── VENV_SETUP.md                   # Venv details
│   └── TESTING_GUIDE_RNG_SIGNING.md    # Testing guide
└── scripts/
    └── test_rng_signing_comprehensive.py  # Main test script
```

---

**Last Updated**: 2026-03-20  
**Status**: ✅ Complete and Verified
