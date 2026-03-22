# Installing Test Dependencies

This guide helps you install all required dependencies for running the CryptoWallet test suite.

## System Requirements

The testing suite requires:

- **dieharder** - Random number generator test suite (system package)
- **Python 3.8+** - Already installed with virtual environment
- **Serial connection** - USB/UART connection to CryptoWallet device

## Automatic Installation

The easiest way is to run the automated installer:

```bash
cd /data/projects/CryptoWallet
./install-test-deps.sh
```

This script:
1. Detects your operating system
2. Installs `dieharder` using the appropriate package manager
3. Activates the Python virtual environment
4. Installs/upgrades Python packages from `requirements-test.txt`

## Manual Installation by OS

### Linux (Debian/Ubuntu)

```bash
# Install system dependencies
sudo apt update
sudo apt install -y dieharder

# Activate venv and install Python packages
source .venv-test/bin/activate
pip install --upgrade pip
pip install -r requirements-test.txt
```

### Linux (Fedora/RHEL)

```bash
# Install system dependencies
sudo dnf install -y dieharder

# Activate venv and install Python packages
source .venv-test/bin/activate
pip install --upgrade pip
pip install -r requirements-test.txt
```

### Linux (Arch Linux)

```bash
# Install system dependencies
sudo pacman -S dieharder

# Activate venv and install Python packages
source .venv-test/bin/activate
pip install --upgrade pip
pip install -r requirements-test.txt
```

### macOS

```bash
# Install system dependencies (requires Homebrew)
brew install dieharder

# Activate venv and install Python packages
source .venv-test/bin/activate
pip install --upgrade pip
pip install -r requirements-test.txt
```

### Windows (using WSL2 with Ubuntu)

```bash
# Inside WSL terminal:
sudo apt update
sudo apt install -y dieharder

source .venv-test/bin/activate
pip install --upgrade pip
pip install -r requirements-test.txt
```

## Verifying Installation

After installation, verify everything is set up correctly:

```bash
# Activate venv
source .venv-test/bin/activate

# Check dieharder
dieharder --version

# Check Python packages
python3 -c "import serial; import requests; print('✓ All packages installed')"

# Run quick prerequisite check
python3 scripts/test_rng_signing_comprehensive.py --help
```

You should see a help message without any import errors.

## What Gets Installed

### System Packages

- **dieharder** - Random number generator test suite
  - Provides `dieharder` command-line tool
  - Used for statistical testing of RNG output
  - ~5-20 MB depending on OS

### Python Packages

From `requirements-test.txt`:

- **pyserial** - Serial port communication library
- **requests** - HTTP client library
- Plus standard build tools (pip, setuptools, wheel)

## Troubleshooting

### "dieharder: command not found"

The installation didn't complete successfully. Try:

```bash
# Check if dieharder is installed
which dieharder

# If not found, install it manually for your OS (see section above)
sudo apt install dieharder  # or equivalent for your OS
```

### "ModuleNotFoundError: No module named 'serial'"

The Python venv is not activated or packages aren't installed:

```bash
# Activate venv
source .venv-test/bin/activate

# Install packages
pip install -r requirements-test.txt

# Verify
python3 -c "import serial; print('✓ OK')"
```

### "Permission denied" on /dev/ttyUSB0 or /dev/ttyACM0

The user doesn't have permissions to access the serial port:

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and log back in, or run:
newgrp dialout

# Verify
ls -l /dev/ttyUSB0  # should show your user in the group
```

### Still Having Issues?

1. Check the venv is properly set up:
   ```bash
   ls -la .venv-test/bin/python3
   .venv-test/bin/python3 --version
   ```

2. Try reinstalling:
   ```bash
   source .venv-test/bin/activate
   pip install --force-reinstall -r requirements-test.txt
   ```

3. Check `docs_src/TESTING_GUIDE_RNG_SIGNING.md` for more detailed troubleshooting

4. Review `docs_src/VENV_SETUP.md` for venv-specific issues

## Next Steps

Once all dependencies are installed:

1. Check that your device is connected via USB/UART
2. Build and flash the CryptoWallet firmware to the device
3. Run the test suite:

```bash
source .venv-test/bin/activate
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

For full testing instructions, see `docs_src/TESTING_GUIDE_RNG_SIGNING.md`.

## Gitea Actions / CI (TRNG, UART)

Кратко: **`.gitea/workflows/simple-ci.yml`** — по умолчанию **`USE_RNG_DUMP=1`** (бинарный TRNG на UART), захват RNG в pipeline включён; текстовые маркеры загрузки в этом режиме не ждутся.

Подробная таблица режимов и переменные: **[CI_PIPELINE_ru.md](CI_PIPELINE_ru.md)** | **[CI_PIPELINE_en.md](CI_PIPELINE_en.md)**.  
Runner: **[INFRASTRUCTURE.md](INFRASTRUCTURE.md)**.

## Dependency Versions

See `requirements-test-lock.txt` for the exact versions used in the last successful test run.

Current minimum versions in `requirements-test.txt`:

- pyserial >= 3.5
- requests >= 2.28.0

## Getting Help

- **Test script usage**: `python3 scripts/test_rng_signing_comprehensive.py --help`
- **Testing guide**: Read `docs_src/TESTING_GUIDE_RNG_SIGNING.md`
- **Virtual environment setup**: Read `docs_src/VENV_SETUP.md`
- **Quick reference**: See `VENV_QUICKSTART.txt`
