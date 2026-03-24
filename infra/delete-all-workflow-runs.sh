#!/usr/bin/env bash
# Deletes all workflow run records in Gitea (SQLite), without touching runners.
# Requires: cryptowallet-gitea container (podman/docker) and access to /data/gitea/gitea.db inside.
#
# Usage:
#   ./infra/delete-all-workflow-runs.sh
# Or with a different container name:
#   GITEA_CONTAINER=my-gitea ./infra/delete-all-workflow-runs.sh

set -euo pipefail

GITEA_CONTAINER="${GITEA_CONTAINER:-cryptowallet-gitea}"
DB_PATH="${DB_PATH:-/data/gitea/gitea.db}"

if ! podman exec "$GITEA_CONTAINER" test -r "$DB_PATH" 2>/dev/null; then
  if docker exec "$GITEA_CONTAINER" test -r "$DB_PATH" 2>/dev/null; then
    RUNTIME=docker
  else
    echo "❌ Container not found or DB missing: $GITEA_CONTAINER ($DB_PATH)"
    exit 1
  fi
else
  RUNTIME=podman
fi

echo "Using: $RUNTIME exec $GITEA_CONTAINER -> $DB_PATH"

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

echo "✅ All workflow runs deleted from DB (runners and tokens were not touched)."
