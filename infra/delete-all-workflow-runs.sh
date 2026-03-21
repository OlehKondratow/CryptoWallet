#!/usr/bin/env bash
# Удаляет все записи о workflow runs в Gitea (SQLite), без затронутых runner'ов.
# Требуется: контейнер cryptowallet-gitea (podman/docker) и доступ к /data/gitea/gitea.db внутри.
#
# Использование:
#   ./infra/delete-all-workflow-runs.sh
# Или с другим именем контейнера:
#   GITEA_CONTAINER=my-gitea ./infra/delete-all-workflow-runs.sh

set -euo pipefail

GITEA_CONTAINER="${GITEA_CONTAINER:-cryptowallet-gitea}"
DB_PATH="${DB_PATH:-/data/gitea/gitea.db}"

if ! podman exec "$GITEA_CONTAINER" test -r "$DB_PATH" 2>/dev/null; then
  if docker exec "$GITEA_CONTAINER" test -r "$DB_PATH" 2>/dev/null; then
    RUNTIME=docker
  else
    echo "❌ Контейнер не найден или нет БД: $GITEA_CONTAINER ($DB_PATH)"
    exit 1
  fi
else
  RUNTIME=podman
fi

echo "Используется: $RUNTIME exec $GITEA_CONTAINER → $DB_PATH"

$RUNTIME exec "$GITEA_CONTAINER" sqlite3 "$DB_PATH" "
BEGIN IMMEDIATE;
DELETE FROM action_task_step WHERE task_id IN (SELECT id FROM action_task);
DELETE FROM action_task_output WHERE task_id IN (SELECT id FROM action_task);
DELETE FROM action_task;
DELETE FROM action_run_job;
DELETE FROM action_artifact;
DELETE FROM action_run;
DELETE FROM action_run_index;
COMMIT;
SELECT 'action_run', COUNT(*) FROM action_run
UNION ALL SELECT 'action_run_job', COUNT(*) FROM action_run_job;
"

echo "✅ Все workflow runs удалены из БД (runner'ы и токены не трогались)."
