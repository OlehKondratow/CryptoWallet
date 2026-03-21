#!/bin/bash
# Quick setup script for Gitea + CI/CD infrastructure

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=================================================="
echo "CryptoWallet CI/CD Infrastructure Setup"
echo "=================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check prerequisites
echo "1️⃣  Checking prerequisites..."

if ! command -v podman &> /dev/null; then
    echo -e "${RED}✗ Podman not installed${NC}"
    echo "  Install Podman:"
    echo "    Ubuntu/Debian: sudo apt-get install -y podman podman-compose"
    echo "    macOS: brew install podman podman-compose"
    exit 1
fi

if ! command -v podman-compose &> /dev/null; then
    echo -e "${RED}✗ podman-compose not installed${NC}"
    echo "  Install: pip3 install podman-compose"
    exit 1
fi

echo -e "${GREEN}✓ Podman & podman-compose installed${NC}"
podman --version
podman-compose --version

# Check kernel modules for USB passthrough
echo ""
echo "2️⃣  Checking USB/device support..."

if grep -q "usb_usbfs" /proc/filesystems; then
    echo -e "${GREEN}✓ usbfs available${NC}"
else
    echo -e "${YELLOW}⚠ usbfs may not be available${NC}"
fi

if [ -e /dev/bus/usb ]; then
    echo -e "${GREEN}✓ /dev/bus/usb accessible${NC}"
else
    echo -e "${YELLOW}⚠ /dev/bus/usb not found - USB passthrough may not work${NC}"
fi

# Create necessary directories
echo ""
echo "3️⃣  Creating directories..."
mkdir -p "$REPO_ROOT/infra/gitea-data"
mkdir -p "$REPO_ROOT/infra/gitea-config"
mkdir -p "$REPO_ROOT/infra/act-runner-data"
mkdir -p "$REPO_ROOT/.gitea/workflows"
mkdir -p "$REPO_ROOT/.vscode"

echo -e "${GREEN}✓ Directories created${NC}"

# Build runner image
echo ""
echo "4️⃣  Preparing Act Runner..."

echo "ℹ Using official gitea/act_runner:nightly image"

# Generate runner token (if needed)
echo ""
echo "5️⃣  Setting up Act Runner registration..."

RUNNER_TOKEN="${GITEA_RUNNER_TOKEN:-}"

if [ -z "$RUNNER_TOKEN" ]; then
    echo -e "${YELLOW}ℹ No runner token provided${NC}"
    echo "  After Gitea starts, create token at:"
    echo "    http://localhost:3000/admin/runners"
    echo "  Then set: export GITEA_RUNNER_TOKEN=<token>"
else
    echo -e "${GREEN}✓ Runner token configured${NC}"
fi

# Start services
echo ""
echo "6️⃣  Starting services..."

cd "$REPO_ROOT"

podman-compose -f infra/docker-compose.yml up -d

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Services started${NC}"
else
    echo -e "${RED}✗ Failed to start services${NC}"
    podman-compose -f infra/docker-compose.yml logs
    exit 1
fi

# Wait for Gitea
echo ""
echo "7️⃣  Waiting for Gitea to be ready..."

for i in {1..30}; do
    if curl -f http://localhost:3000/api/v1/version &> /dev/null; then
        echo -e "${GREEN}✓ Gitea is running${NC}"
        break
    fi
    
    if [ $i -eq 30 ]; then
        echo -e "${RED}✗ Gitea did not start in time${NC}"
        podman-compose -f infra/docker-compose.yml logs gitea
        exit 1
    fi
    
    echo "  Waiting... ($i/30)"
    sleep 2
done

# Show status
echo ""
echo "=================================================="
echo -e "${GREEN}✓ Setup Complete!${NC}"
echo "=================================================="
echo ""
echo "📍 Gitea Web UI: http://localhost:3000"
echo "📍 Gitea SSH: ssh://git@localhost:2222"
echo ""
echo "Next steps:"
echo "  1. Open http://localhost:3000 in browser"
echo "  2. Initial setup will prompt you to create admin user"
echo "  3. Go to Admin -> Runners to create registration token"
echo "  4. Run: export GITEA_RUNNER_TOKEN=<token>"
echo "  5. Restart runner: podman-compose -f infra/docker-compose.yml restart gitea-runner"
echo "  6. Push code: git remote add gitea http://localhost:3000/<user>/<repo>.git"
echo ""
echo "Useful commands:"
echo "  podman-compose -f infra/docker-compose.yml logs -f gitea      # Gitea logs"
echo "  podman-compose -f infra/docker-compose.yml logs -f gitea-runner  # Runner logs"
echo "  podman-compose -f infra/docker-compose.yml down               # Stop all services"
echo ""
