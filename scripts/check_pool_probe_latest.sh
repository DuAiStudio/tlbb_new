#!/usr/bin/env bash
# F2: auto-select latest ShareMemory log then run pool-probe checker.
# Usage:
#   bash scripts/check_pool_probe_latest.sh --log-dir ./Log
#   bash scripts/check_pool_probe_latest.sh --log-dir ./Log --allow-warn --fail-on-invalid-tokens
set -euo pipefail

log_dir="./Log"
allow_warn=0
fail_on_invalid_tokens=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --log-dir)
      log_dir="${2:-}"
      shift 2
      ;;
    --allow-warn)
      allow_warn=1
      shift
      ;;
    --fail-on-invalid-tokens)
      fail_on_invalid_tokens=1
      shift
      ;;
    *)
      echo "Unknown arg: $1" >&2
      echo "Usage: $0 [--log-dir DIR] [--allow-warn] [--fail-on-invalid-tokens]" >&2
      exit 2
      ;;
  esac
done

if [[ ! -d "$log_dir" ]]; then
  echo "Log dir not found: $log_dir" >&2
  exit 2
fi

latest="$(ls -1t "$log_dir"/ShareMemory_*.log 2>/dev/null | head -n 1 || true)"
if [[ -z "$latest" ]]; then
  echo "No ShareMemory_*.log found in: $log_dir" >&2
  exit 3
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
checker="${script_dir}/check_pool_probe_log.sh"
cmd=(bash "$checker")
if [[ "$allow_warn" -eq 1 ]]; then cmd+=(--allow-warn); fi
if [[ "$fail_on_invalid_tokens" -eq 1 ]]; then cmd+=(--fail-on-invalid-tokens); fi
cmd+=("$latest")

echo "check_pool_probe_latest: using $latest"
"${cmd[@]}"
