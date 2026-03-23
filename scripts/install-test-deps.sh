#!/bin/bash
#
# Install system and Python dependencies for CryptoWallet testing
#

set -e

echo "╔══════════════════════════════════════════════════════════════════════╗"
echo "║         CryptoWallet Testing Dependencies Installer                  ║"
echo "╚══════════════════════════════════════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT" || exit 1

if [ ! -f "$REPO_ROOT/.venv-test/bin/python3" ]; then
    echo "❌ Error: Python venv not found at $REPO_ROOT/.venv-test"
    echo ""
    echo "Please create the venv first:"
    echo "  python3 -m venv .venv-test"
    echo "  source .venv-test/bin/activate"
    echo "  pip install -r requirements-test.txt"
    exit 1
fi

echo "📦 Installing system dependencies..."
echo ""

# Detect OS and install dieharder
if command -v apt-get &> /dev/null; then
    echo "   Debian/Ubuntu detected"
    echo "   Installing: dieharder"
    sudo apt-get update
    sudo apt-get install -y dieharder
elif command -v dnf &> /dev/null; then
    echo "   Fedora/RHEL detected"
    echo "   Installing: dieharder"
    sudo dnf install -y dieharder
elif command -v brew &> /dev/null; then
    echo "   macOS detected"
    echo "   Installing: dieharder"
    brew install dieharder
elif command -v pacman &> /dev/null; then
    echo "   Arch Linux detected"
    echo "   Installing: dieharder"
    sudo pacman -S --noconfirm dieharder
else
    echo "   ⚠️  Could not detect package manager"
    echo "   Please install dieharder manually:"
    echo "   - Debian/Ubuntu: sudo apt install dieharder"
    echo "   - Fedora/RHEL:   sudo dnf install dieharder"
    echo "   - macOS:         brew install dieharder"
    echo "   - Arch Linux:    sudo pacman -S dieharder"
fi

echo ""
echo "📦 Installing Python dependencies..."
echo ""

source "$REPO_ROOT/.venv-test/bin/activate"

# Install/upgrade pip packages
echo "   Upgrading pip, setuptools, wheel..."
pip install --upgrade pip setuptools wheel

echo "   Installing from requirements-test.txt..."
pip install -r "$SCRIPT_DIR/requirements-test.txt"

echo ""
echo "╔══════════════════════════════════════════════════════════════════════╗"
echo "║                    ✅ Installation Complete                          ║"
echo "╚══════════════════════════════════════════════════════════════════════╝"
echo ""
echo "✅ System dependencies installed"
echo "✅ Python venv configured"
echo "✅ Python packages installed"
echo ""
echo "Next steps:"
echo ""
echo "  1. Activate the venv:"
echo "     source .venv-test/bin/activate"
echo ""
echo "  2. Run tests:"
echo "     python3 scripts/test_rng_signing_comprehensive.py --mode rng"
echo ""
echo "  3. Or use the helper script:"
echo "     ./scripts/run-tests.sh"
echo ""
