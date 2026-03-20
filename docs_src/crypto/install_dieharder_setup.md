╔══════════════════════════════════════════════════════════════════════╗
║                                                                      ║
║                  ⚠️  MISSING SYSTEM DEPENDENCY                       ║
║                                                                      ║
║                         dieharder not found                          ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝

The test suite requires 'dieharder' - a random number generator test tool.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✅ AUTOMATIC INSTALLATION (Recommended):

   ./install-test-deps.sh

   This script automatically:
   - Detects your OS
   - Installs dieharder
   - Configures Python packages

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📋 MANUAL INSTALLATION by OS:

   Linux (Debian/Ubuntu):
   $ sudo apt update && sudo apt install -y dieharder

   Linux (Fedora/RHEL):
   $ sudo dnf install -y dieharder

   Linux (Arch):
   $ sudo pacman -S dieharder

   macOS (with Homebrew):
   $ brew install dieharder

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📚 For more details, see:
   docs_src/INSTALL_TEST_DEPS.md

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

After installation, verify:

   dieharder --version

Then try running tests again:

   python3 scripts/test_rng_signing_comprehensive.py --mode rng

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
