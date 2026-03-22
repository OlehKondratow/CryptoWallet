#!/usr/bin/env bash
# Перерегистрация act_runner на хосте с новым Registration Token из Gitea.
#
# 1. Gitea → Администрирование → Actions → Runners → «Создать runner» → скопировать токен.
# 2. Положить токен в infra/.env.local (строка GITEA_RUNNER_TOKEN=...) или:
#     export GITEA_RUNNER_TOKEN='...'
# 3. Запуск:
#     ./infra/reregister-act-runner.sh
#
# Старый runner в UI станет offline — удалите его вручную, если появится дубликат.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER_HOME="${RUNNER_HOME:-${HOME}/gitea-runner}"
RUNNER_NAME="${RUNNER_NAME:-host-runner-$(hostname)}"
GITEA_URL="${GITEA_URL:-http://127.0.0.1:3000}"
# Метка cryptowallet-host:host — совпадает с runs-on в .gitea/workflows/simple-ci.yml
# (уникальное имя, чтобы не перехватывал docker-runner с ubuntu-latest:docker).
GITEA_RUNNER_LABELS="${GITEA_RUNNER_LABELS:-cryptowallet-host:host}"

# Токен: GITEA_RUNNER_TOKEN или GITEA_TOKEN из окружения
TOKEN="${GITEA_RUNNER_TOKEN:-${GITEA_TOKEN:-}}"

if [[ -z "${TOKEN}" && -f "${SCRIPT_DIR}/.env.local" ]]; then
  TOKEN="$(grep -E '^[[:space:]]*GITEA_RUNNER_TOKEN=' "${SCRIPT_DIR}/.env.local" | tail -1 | cut -d= -f2- | tr -d '"' | tr -d "'" | tr -d '[:space:]')"
fi

if [[ -z "${TOKEN}" ]]; then
  echo "❌ Нет токена. Укажите GITEA_RUNNER_TOKEN в infra/.env.local или:"
  echo "   export GITEA_RUNNER_TOKEN='<registration token из Gitea>'"
  exit 1
fi

ACT_RUNNER="${RUNNER_HOME}/act_runner"
if [[ ! -x "${ACT_RUNNER}" ]]; then
  echo "❌ Не найден ${ACT_RUNNER} — сначала установите act_runner или скопируйте из /usr/local/bin"
  exit 1
fi

echo "═══════════════════════════════════════════════════════"
echo "  🐙 Перерегистрация Gitea act_runner"
echo "═══════════════════════════════════════════════════════"
echo "  RUNNER_HOME: ${RUNNER_HOME}"
echo "  GITEA_URL:   ${GITEA_URL}"
echo "  RUNNER_NAME: ${RUNNER_NAME}"
echo "  LABELS:      ${GITEA_RUNNER_LABELS}"
echo ""

echo "⏹  Останавливаю gitea-runner..."
systemctl --user stop gitea-runner.service 2>/dev/null || true

if [[ -f "${RUNNER_HOME}/.runner" ]]; then
  BAK="${RUNNER_HOME}/.runner.bak.$(date +%Y%m%d%H%M%S)"
  cp -a "${RUNNER_HOME}/.runner" "${BAK}"
  echo "📦 Резервная копия: ${BAK}"
  rm -f "${RUNNER_HOME}/.runner"
fi

echo "📝 Регистрация..."
cd "${RUNNER_HOME}"
if "${ACT_RUNNER}" register \
  --instance "${GITEA_URL}" \
  --token "${TOKEN}" \
  --name "${RUNNER_NAME}" \
  --labels "${GITEA_RUNNER_LABELS}" \
  --no-interactive; then
  echo "✅ Регистрация успешна"
else
  echo "❌ Ошибка регистрации. Проверьте токен и URL Gitea."
  exit 1
fi

echo "▶️  Запуск gitea-runner..."
systemctl --user start gitea-runner.service
sleep 2
systemctl --user status gitea-runner.service --no-pager || true

echo ""
echo "Готово. В Gitea: Admin → Runners — проверьте новый runner; старый удалите при необходимости."
echo "Логи: journalctl --user -f -u gitea-runner"
