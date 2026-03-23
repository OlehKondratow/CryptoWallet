#!/bin/bash
# Quick deployment script for running complete CI/CD infrastructure

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   CryptoWallet CI/CD Infrastructure Deployment            ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Menu
case "${1:-menu}" in
  up)
    echo -e "${GREEN}Starting services...${NC}"
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" up -d
    echo -e "${GREEN}✓ Services started${NC}"
    echo ""
    echo "Gitea Web UI: http://localhost:3000"
    ;;

  down)
    echo -e "${YELLOW}Stopping services...${NC}"
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" down
    echo -e "${GREEN}✓ Services stopped${NC}"
    ;;

  logs)
    echo -e "${BLUE}Service: ${2:-all}${NC}"
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" logs -f ${2}
    ;;

  status)
    echo -e "${BLUE}Container Status:${NC}"
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" ps
    echo ""
    echo -e "${BLUE}Resource Usage:${NC}"
    podman stats --no-stream \
      cryptowallet-gitea \
      cryptowallet-runner \
      2>/dev/null || echo "Containers not running"
    ;;

  build-runner)
    echo -e "${GREEN}Building runner image...${NC}"
    podman build \
      -t cryptowallet-runner:latest \
      -f "$REPO_ROOT/infra/Dockerfile.runner" \
      "$REPO_ROOT"
    echo -e "${GREEN}✓ Runner image built${NC}"
    
    echo -e "${YELLOW}Restarting runner...${NC}"
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" restart gitea-runner
    echo -e "${GREEN}✓ Runner restarted${NC}"
    ;;

  setup)
    echo -e "${YELLOW}Running full setup...${NC}"
    chmod +x "$REPO_ROOT/infra/setup-infrastructure.sh"
    "$REPO_ROOT/infra/setup-infrastructure.sh"
    ;;

  register-runner)
    if [ -z "$GITEA_RUNNER_TOKEN" ]; then
      echo -e "${RED}Error: GITEA_RUNNER_TOKEN not set${NC}"
      echo ""
      echo "Get token from: http://localhost:3000/admin/runners"
      echo "Then run:"
      echo "  export GITEA_RUNNER_TOKEN=<token>"
      echo "  ./deploy.sh register-runner"
      exit 1
    fi
    
    echo -e "${GREEN}Registering runner with token: ${GITEA_RUNNER_TOKEN:0:10}...${NC}"
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" restart gitea-runner
    sleep 3
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" logs gitea-runner | grep -i registered || echo "Check logs for details"
    ;;

  clean)
    echo -e "${YELLOW}⚠️  DANGEROUS: Removing all data and volumes...${NC}"
    read -p "Are you sure? Type 'yes' to confirm: " confirm
    if [ "$confirm" = "yes" ]; then
      podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" down -v
      rm -rf "$REPO_ROOT/infra/gitea-data" "$REPO_ROOT/infra/act-runner-data"
      echo -e "${GREEN}✓ All data removed${NC}"
    else
      echo "Aborted"
    fi
    ;;

  test)
    echo -e "${BLUE}Testing infrastructure...${NC}"
    
    echo ""
    echo "1. Testing Gitea connectivity..."
    if curl -f http://localhost:3000/api/v1/version 2>/dev/null | grep -q version; then
      echo -e "${GREEN}✓ Gitea responding${NC}"
    else
      echo -e "${RED}✗ Gitea not responding${NC}"
    fi
    
    echo ""
    echo "2. Testing runner health..."
    if podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" exec gitea-runner pgrep -f act >/dev/null 2>&1; then
      echo -e "${GREEN}✓ Runner is active${NC}"
    else
      echo -e "${RED}✗ Runner is not running${NC}"
    fi
    
    echo ""
    echo "3. Testing hardware detection..."
    if podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" exec gitea-runner lsusb 2>/dev/null | grep -q 0483; then
      echo -e "${GREEN}✓ ST-LINK detected${NC}"
    else
      echo -e "${YELLOW}⚠ ST-LINK not detected (may not be connected)${NC}"
    fi
    
    echo ""
    echo "4. Testing toolchain..."
    if podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" exec gitea-runner arm-none-eabi-gcc --version >/dev/null 2>&1; then
      echo -e "${GREEN}✓ ARM GCC available${NC}"
    else
      echo -e "${RED}✗ ARM GCC not available${NC}"
    fi
    ;;

  shell)
    echo -e "${BLUE}Entering runner shell...${NC}"
    podman-compose -f "$REPO_ROOT/infra/docker-compose.yml" exec gitea-runner /bin/bash
    ;;

  *)
    echo "CryptoWallet Infrastructure - Service Management (Podman)"
    echo ""
    echo "Usage: ./infra/deploy.sh <command>"
    echo ""
    echo "COMMANDS:"
    echo "  up                Start all services"
    echo "  down              Stop all services"
    echo "  status            Show container status"
    echo "  logs [service]    View logs (gitea, gitea-runner)"
    echo "  test              Health checks"
    echo "  shell             Enter container bash"
    echo "  register-runner   Register runner (needs GITEA_RUNNER_TOKEN)"
    echo "  build-runner      Rebuild runner image"
    echo "  clean             Remove all data (DESTRUCTIVE)"
    echo ""
    echo "See documentation/07-build-ci-infrastructure.md and infra/docker-compose.yml"
    echo ""
    ;;
esac
