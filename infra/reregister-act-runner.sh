#!/usr/bin/env bash
# Re-register act_runner on host with a new Gitea registration token.
#
# 1. Gitea -> Administration -> Actions -> Runners -> "Create runner" -> copy token.
# 2. Put token into infra/.env.local (line GITEA_RUNNER_TOKEN=...) or:
#     export GITEA_RUNNER_TOKEN='...'
# 3. Run:
#     ./infra/reregister-act-runner.sh
#
# Old runner in UI will become offline; delete it manually if duplicate appears.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER_HOME="${RUNNER_HOME:-${HOME}/gitea-runner}"
RUNNER_NAME="${RUNNER_NAME:-host-runner-$(hostname)}"
GITEA_URL="${GITEA_URL:-http://127.0.0.1:3000}"
# Label cryptowallet-host:host matches runs-on in .gitea/workflows/simple-ci.yml
# (unique name so jobs are not picked by docker runners like ubuntu-latest:docker).
GITEA_RUNNER_LABELS="${GITEA_RUNNER_LABELS:-cryptowallet-host:host}"

# Token: GITEA_RUNNER_TOKEN or GITEA_TOKEN from environment
TOKEN="${GITEA_RUNNER_TOKEN:-${GITEA_TOKEN:-}}"

if [[ -z "${TOKEN}" && -f "${SCRIPT_DIR}/.env.local" ]]; then
  TOKEN="$(grep -E '^[[:space:]]*GITEA_RUNNER_TOKEN=' "${SCRIPT_DIR}/.env.local" | tail -1 | cut -d= -f2- | tr -d '"' | tr -d "'" | tr -d '[:space:]')"
fi

if [[ -z "${TOKEN}" ]]; then
  echo "❌ No token found. Set GITEA_RUNNER_TOKEN in infra/.env.local or:"
  echo "   export GITEA_RUNNER_TOKEN='<registration token from Gitea>'"
  exit 1
fi

ACT_RUNNER="${RUNNER_HOME}/act_runner"
if [[ ! -x "${ACT_RUNNER}" ]]; then
  echo "❌ ${ACT_RUNNER} not found - install act_runner first or copy it from /usr/local/bin"
  exit 1
fi

echo "═══════════════════════════════════════════════════════"
echo "  🐙 Re-register Gitea act_runner"
echo "═══════════════════════════════════════════════════════"
echo "  RUNNER_HOME: ${RUNNER_HOME}"
echo "  GITEA_URL:   ${GITEA_URL}"
echo "  RUNNER_NAME: ${RUNNER_NAME}"
echo "  LABELS:      ${GITEA_RUNNER_LABELS}"
echo ""

echo "⏹  Stopping gitea-runner..."
systemctl --user stop gitea-runner.service 2>/dev/null || true

if [[ -f "${RUNNER_HOME}/.runner" ]]; then
  BAK="${RUNNER_HOME}/.runner.bak.$(date +%Y%m%d%H%M%S)"
  cp -a "${RUNNER_HOME}/.runner" "${BAK}"
  echo "📦 Backup: ${BAK}"
  rm -f "${RUNNER_HOME}/.runner"
fi

echo "📝 Registering..."
cd "${RUNNER_HOME}"
if "${ACT_RUNNER}" register \
  --instance "${GITEA_URL}" \
  --token "${TOKEN}" \
  --name "${RUNNER_NAME}" \
  --labels "${GITEA_RUNNER_LABELS}" \
  --no-interactive; then
  echo "✅ Registration successful"
else
  echo "❌ Registration failed. Check token and Gitea URL."
  exit 1
fi

echo "▶️  Starting gitea-runner..."
systemctl --user start gitea-runner.service
sleep 2
systemctl --user status gitea-runner.service --no-pager || true

echo ""
echo "Done. In Gitea: Admin -> Runners - verify the new runner; remove old one if needed."
echo "Logs: journalctl --user -f -u gitea-runner"
