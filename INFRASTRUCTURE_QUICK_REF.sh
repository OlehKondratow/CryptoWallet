#!/bin/bash
# CryptoWallet Infrastructure - Quick Reference

cat << 'EOF'

╔══════════════════════════════════════════════════════════════════════════════╗
║                          CryptoWallet Infrastructure                        ║
║               Production-Ready CI/CD + HIL Testing (Podman)                 ║
╚══════════════════════════════════════════════════════════════════════════════╝

📚 DOCUMENTATION
════════════════════════════════════════════════════════════════════════════════

Complete guide: docs_src/infrastructure.md (all-in-one, 500+ lines)

Topics covered:
  ✓ Quick start (5 minutes)
  ✓ Full installation
  ✓ Local development (VS Code/Cursor)
  ✓ CI/CD pipeline details
  ✓ Hardware-in-the-Loop testing
  ✓ Docker alternatives (Podman, LXD, Native)
  ✓ Troubleshooting
  ✓ Command reference


🚀 QUICK START
════════════════════════════════════════════════════════════════════════════════

1. Run setup (5 minutes)
   $ cd /data/projects/CryptoWallet
   $ chmod +x infra/setup-infrastructure.sh
   $ ./infra/setup-infrastructure.sh

2. Open Gitea
   → http://localhost:3000
   → Create admin user

3. Get runner token
   → Admin → Runners → Create Runner
   → Copy token

4. Register runner
   $ export GITEA_RUNNER_TOKEN=<your_token>
   $ ./infra/deploy.sh register-runner
   $ ./infra/deploy.sh test

5. Trigger pipeline
   $ git push -u gitea main


💻 LOCAL DEVELOPMENT
════════════════════════════════════════════════════════════════════════════════

Build:    Ctrl+Shift+B
Debug:    F5
Tasks:    Cmd+Shift+P → Run Task

Available tasks:
  • build-firmware              Standard build
  • build-firmware-rng          With RNG_DUMP
  • flash-firmware              Program device
  • run-rng-test                Capture TRNG
  • run-signing-test            Test signatures
  • static-analysis-cppcheck    Code analysis
  • openocd-server              GDB server


🛠️ SERVICE MANAGEMENT
════════════════════════════════════════════════════════════════════════════════

./infra/deploy.sh up              Start services
./infra/deploy.sh down            Stop services
./infra/deploy.sh status          Container status
./infra/deploy.sh logs gitea      View Gitea logs
./infra/deploy.sh logs gitea-runner   View runner logs
./infra/deploy.sh test            Health check
./infra/deploy.sh shell           Debug container
./infra/deploy.sh clean           Remove all data


🔗 IMPORTANT LINKS
════════════════════════════════════════════════════════════════════════════════

Gitea Web UI:          http://localhost:3000
Gitea SSH:             ssh://git@localhost:2222
OpenOCD GDB:           localhost:3333


❓ NEED HELP?
════════════════════════════════════════════════════════════════════════════════

1. Read documentation:
   $ cat docs_src/infrastructure.md

2. Check status:
   $ ./infra/deploy.sh test

3. View logs:
   $ ./infra/deploy.sh logs gitea-runner

4. Enter container:
   $ ./infra/deploy.sh shell


✅ VERIFICATION
════════════════════════════════════════════════════════════════════════════════

After setup, verify:
  [ ] Services running: ./infra/deploy.sh status
  [ ] Gitea accessible: http://localhost:3000
  [ ] Admin user created
  [ ] Runner registered
  [ ] ./infra/deploy.sh test shows all ✓
  [ ] Code pushed: git push -u gitea main
  [ ] Pipeline triggered (check Actions in Gitea)


═══════════════════════════════════════════════════════════════════════════════

For complete documentation, see: docs_src/infrastructure.md

EOF
