#!/usr/bin/env bash
# A3: verify a captured ShareMemory log contains expected startup substrings.
# Usage: bash scripts/check_sharememory_log.sh /path/to/ShareMemory_YYYY-MM-DD.xxxxx.log
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KEYWORDS="${SCRIPT_DIR}/sharememory_startup_keywords.txt"

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <ShareMemory_*.log>" >&2
  exit 2
fi

LOG_FILE="$1"
if [[ ! -f "$LOG_FILE" ]]; then
  echo "Not a file: $LOG_FILE" >&2
  exit 2
fi

missing=0
while IFS= read -r line || [[ -n "$line" ]]; do
  [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
  pattern="${line//$'\r'/}"
  if ! grep -qF "$pattern" "$LOG_FILE"; then
    echo "MISSING: $pattern" >&2
    missing=1
  fi
done <"$KEYWORDS"

if [[ "$missing" -ne 0 ]]; then
  echo "check_sharememory_log: FAILED" >&2
  exit 1
fi

echo "check_sharememory_log: OK ($LOG_FILE)"
exit 0
