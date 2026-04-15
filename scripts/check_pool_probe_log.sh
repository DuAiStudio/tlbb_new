#!/usr/bin/env bash
# E3: verify POOL-PROBE summary status lines.
# Usage:
#   bash scripts/check_pool_probe_log.sh /path/to/ShareMemory_YYYY-MM-DD.xxxxx.log
#   bash scripts/check_pool_probe_log.sh --allow-warn /path/to/ShareMemory_YYYY-MM-DD.xxxxx.log
set -euo pipefail

allow_warn=0
fail_on_invalid_tokens=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --allow-warn)
      allow_warn=1
      shift
      ;;
    --fail-on-invalid-tokens)
      fail_on_invalid_tokens=1
      shift
      ;;
    *)
      break
      ;;
  esac
done

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 [--allow-warn] [--fail-on-invalid-tokens] <ShareMemory_*.log>" >&2
  exit 2
fi

log_file="$1"
if [[ ! -f "$log_file" ]]; then
  echo "Not a file: $log_file" >&2
  exit 2
fi

ok=0
warn=0
fail=0
total=0
invalid_tokens=0
summary_matched=0

while IFS= read -r line || [[ -n "$line" ]]; do
  if [[ "$summary_matched" -eq 0 && "$line" == *"[POOL-PROBE] summary "* ]]; then
    if [[ "$line" =~ total=([0-9]+)([[:space:]]+filtered=([0-9]+))?[[:space:]]+ok=([0-9]+)[[:space:]]+warn=([0-9]+)[[:space:]]+fail=([0-9]+)([[:space:]]+invalid_tokens=([0-9]+))? ]]; then
      total="${BASH_REMATCH[1]}"
      ok="${BASH_REMATCH[4]}"
      warn="${BASH_REMATCH[5]}"
      fail="${BASH_REMATCH[6]}"
      if [[ -n "${BASH_REMATCH[8]:-}" ]]; then
        invalid_tokens="${BASH_REMATCH[8]}"
      fi
      summary_matched=1
    fi
    continue
  fi
  [[ "$summary_matched" -eq 1 ]] && continue
  [[ "$line" != *"[POOL-PROBE] status="* ]] && continue
  ((total+=1))
  if [[ "$line" == *"status=OK"* ]]; then
    ((ok+=1))
  elif [[ "$line" == *"status=WARN"* ]]; then
    ((warn+=1))
  elif [[ "$line" == *"status=FAIL"* ]]; then
    ((fail+=1))
  fi
done <"$log_file"

if [[ "$total" -eq 0 ]]; then
  echo "check_pool_probe_log: no [POOL-PROBE] status lines found" >&2
  exit 3
fi

echo "check_pool_probe_log: total=${total} ok=${ok} warn=${warn} fail=${fail} invalid_tokens=${invalid_tokens}"

if [[ "$fail" -gt 0 ]]; then
  echo "check_pool_probe_log: FAILED (status=FAIL present)" >&2
  exit 1
fi
if [[ "$allow_warn" -eq 0 && "$warn" -gt 0 ]]; then
  echo "check_pool_probe_log: FAILED (status=WARN present, use --allow-warn to ignore)" >&2
  exit 1
fi
if [[ "$fail_on_invalid_tokens" -eq 1 && "$invalid_tokens" -gt 0 ]]; then
  echo "check_pool_probe_log: FAILED (invalid_tokens present, remove bad probe_pool.types tokens)" >&2
  exit 1
fi

echo "check_pool_probe_log: OK (${log_file})"
exit 0
