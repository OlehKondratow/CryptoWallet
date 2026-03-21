#!/bin/bash
# Setup script for Gitea Act Runner on Host (Ubuntu)
# This script downloads, configures, and registers the runner

set -e

RUNNER_VERSION="v0.3.0"
RUNNER_ARCH="linux_amd64"
RUNNER_HOME="${HOME}/gitea-runner"
RUNNER_NAME="host-runner-$(hostname)"
GITEA_URL="${GITEA_URL:-http://localhost:3000}"
GITEA_TOKEN="${GITEA_TOKEN:-}"

echo "═══════════════════════════════════════════════════════"
echo "  🐙 Gitea Act Runner Host Setup"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "Configuration:"
echo "  Version: ${RUNNER_VERSION}"
echo "  Architecture: ${RUNNER_ARCH}"
echo "  Installation dir: ${RUNNER_HOME}"
echo "  Runner name: ${RUNNER_NAME}"
echo "  Gitea URL: ${GITEA_URL}"
echo ""

# Check if runner directory exists
if [ -d "${RUNNER_HOME}" ]; then
    echo "⚠️  Runner directory already exists at ${RUNNER_HOME}"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
else
    echo "📁 Creating runner directory..."
    mkdir -p "${RUNNER_HOME}"
fi

cd "${RUNNER_HOME}"

# Step 1: Check for existing Act Runner binary
echo ""
if [ -f "/usr/local/bin/act_runner" ]; then
    echo "✅ Found existing Act Runner at /usr/local/bin/act_runner"
    cp /usr/local/bin/act_runner "${RUNNER_HOME}/act_runner"
    chmod +x "${RUNNER_HOME}/act_runner"
    echo "✅ Copied to ${RUNNER_HOME}/"
else
    echo "📥 Downloading Act Runner..."
    DOWNLOAD_URL="https://dl.gitea.io/act_runner/${RUNNER_VERSION}/act_runner-${RUNNER_VERSION}-${RUNNER_ARCH}.tar.gz"
    echo "   URL: ${DOWNLOAD_URL}"

    if ! wget -q "${DOWNLOAD_URL}" -O "act_runner-${RUNNER_VERSION}-${RUNNER_ARCH}.tar.gz"; then
        echo "❌ Failed to download Act Runner"
        exit 1
    fi

    echo "✅ Downloaded"

    # Step 2: Extract
    echo ""
    echo "📦 Extracting..."
    tar -xzf "act_runner-${RUNNER_VERSION}-${RUNNER_ARCH}.tar.gz"
    rm "act_runner-${RUNNER_VERSION}-${RUNNER_ARCH}.tar.gz"
    chmod +x act_runner
    echo "✅ Extracted"
fi

# Step 3: Generate config
echo ""
echo "⚙️  Generating config..."
if [ -f "config.yaml" ]; then
    echo "   Config already exists, backing up..."
    mv config.yaml config.yaml.bak
fi

${RUNNER_HOME}/act_runner generate-config > config.yaml

# Step 4: Configure
echo ""
echo "🔧 Configuring runner..."
cat > config.yaml << 'EOF'
log:
  level: debug

runner:
  file_uid: 1000
  file_gid: 1000
  capacity: 1
  timeout: 3h
  privileged: false
  insecure: false
  fetch_timeout: 5s
  fetch_interval: 2s
  labels:
    - ubuntu-latest
    - host
    - x86_64

cache:
  enabled: true
  dir: /tmp/gitea_runner/cache
  ttl: 259200000000000

container:
  network: bridge
  policy: pull
  force_pull: false
  force_rebuild: false
  buildkit_parallel_load: false
  log_driver: ""
EOF

echo "✅ Config created"

# Step 5: Check if token is provided
echo ""
if [ -z "${GITEA_TOKEN}" ]; then
    echo "⚠️  GITEA_TOKEN not set!"
    echo ""
    echo "To register the runner, you need a registration token:"
    echo "  1. Go to: ${GITEA_URL}/admin/runners"
    echo "  2. Click 'Create new Runner'"
    echo "  3. Copy the registration token"
    echo ""
    echo "Then run:"
    echo "  export GITEA_TOKEN='your_token_here'"
    echo "  ${RUNNER_HOME}/act_runner register \\"
    echo "    --instance ${GITEA_URL} \\"
    echo "    --token \${GITEA_TOKEN} \\"
    echo "    --name ${RUNNER_NAME} \\"
    echo "    --no-interactive"
    echo ""
    read -p "Enter token (or press Enter to skip): " TOKEN_INPUT
    if [ -n "${TOKEN_INPUT}" ]; then
        GITEA_TOKEN="${TOKEN_INPUT}"
    else
        echo "Skipping registration. You can register later manually."
        exit 0
    fi
fi

# Step 6: Register
echo ""
echo "📝 Registering runner..."
if ${RUNNER_HOME}/act_runner register \
    --instance "${GITEA_URL}" \
    --token "${GITEA_TOKEN}" \
    --name "${RUNNER_NAME}" \
    --no-interactive; then
    echo "✅ Runner registered successfully!"
else
    echo "⚠️  Registration failed. Check the token and try again."
    exit 1
fi

# Step 7: Create systemd service
echo ""
echo "🔧 Creating systemd service..."
SYSTEMD_DIR="${HOME}/.config/systemd/user"
mkdir -p "${SYSTEMD_DIR}"

cat > "${SYSTEMD_DIR}/gitea-runner.service" << EOF
[Unit]
Description=Gitea Act Runner (Host)
After=network.target
Documentation=https://docs.gitea.io/en-us/act/

[Service]
Type=simple
WorkingDirectory=${RUNNER_HOME}
ExecStart=${RUNNER_HOME}/act_runner daemon
Restart=on-failure
RestartSec=10s
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=default.target
EOF

echo "✅ Service file created at ${SYSTEMD_DIR}/gitea-runner.service"

# Step 8: Enable and start service
echo ""
echo "🚀 Enabling and starting service..."
systemctl --user daemon-reload
systemctl --user enable gitea-runner.service
systemctl --user start gitea-runner.service

# Check status
sleep 2
if systemctl --user is-active --quiet gitea-runner; then
    echo "✅ Service is running!"
else
    echo "⚠️  Service failed to start. Check logs:"
    echo "   journalctl --user -xe -u gitea-runner"
    exit 1
fi

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  ✨ Setup Complete!"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "Runner info:"
echo "  Home: ${RUNNER_HOME}"
echo "  Name: ${RUNNER_NAME}"
echo "  Status: $(systemctl --user is-active gitea-runner)"
echo ""
echo "Useful commands:"
echo "  View logs:     journalctl --user -f -u gitea-runner"
echo "  Stop service:  systemctl --user stop gitea-runner"
echo "  Start service: systemctl --user start gitea-runner"
echo "  Restart:       systemctl --user restart gitea-runner"
echo "  Status:        systemctl --user status gitea-runner"
echo ""
echo "Next: Check Gitea Admin → Runners to verify registration"
echo ""
