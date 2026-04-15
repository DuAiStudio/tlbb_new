#!/usr/bin/env bash
# F3: trigger pool probe then wait latest log for summary and check.
# Usage:
#   bash scripts/probe_pool_once.sh --runtime-dir /path/to/server --log-dir ./Log
#   bash scripts/probe_pool_once.sh --runtime-dir /path/to/server --log-dir ./Log --types "GlobalData,ItemSerial" --allow-warn
#   bash scripts/probe_pool_once.sh --runtime-dir /path/to/server --log-dir ./Log --use-template --timeout-sec 30 --fail-on-invalid-tokens
set -euo pipefail

runtime_dir="."
log_dir="./Log"
types=""
use_template=0
allow_warn=0
fail_on_invalid_tokens=0
timeout_sec=20
poll_ms=1000

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runtime-dir) runtime_dir="${2:-}"; shift 2 ;;
    --log-dir) log_dir="${2:-}"; shift 2 ;;
    --types) types="${2:-}"; shift 2 ;;
    --use-template) use_template=1; shift ;;
    --allow-warn) allow_warn=1; shift ;;
    --fail-on-invalid-tokens) fail_on_invalid_tokens=1; shift ;;
    --timeout-sec) timeout_sec="${2:-20}"; shift 2 ;;
    --poll-ms) poll_ms="${2:-1000}"; shift 2 ;;
    *)
      echo "Unknown arg: $1" >&2
      echo "Usage: $0 --runtime-dir DIR --log-dir DIR [--types CSV|--use-template] [--allow-warn] [--fail-on-invalid-tokens] [--timeout-sec N] [--poll-ms N]" >&2
      exit 2
      ;;
  esac
done

if [[ "$timeout_sec" -lt 1 ]]; then timeout_sec=1; fi
if [[ "$poll_ms" -lt 200 ]]; then poll_ms=200; fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
trigger="${script_dir}/trigger_pool_probe.sh"
check_latest="${script_dir}/check_pool_probe_latest.sh"

cmd=(bash "$trigger" --runtime-dir "$runtime_dir")
if [[ "$use_template" -eq 1 ]]; then
  cmd+=(--use-template)
elif [[ -n "${types//[[:space:]]/}" ]]; then
  cmd+=(--types "$types")
fi

echo "probe_pool_once: trigger probe"
"${cmd[@]}"

deadline=$(( $(date +%s) + timeout_sec ))
while [[ "$(date +%s)" -lt "$deadline" ]]; do
  latest="$(ls -1t "$log_dir"/ShareMemory_*.log 2>/dev/null | head -n 1 || true)"
  if [[ -n "$latest" ]] && rg -q "\[POOL-PROBE\] summary " "$latest"; then
    check_cmd=(bash "$check_latest" --log-dir "$log_dir")
    if [[ "$allow_warn" -eq 1 ]]; then check_cmd+=(--allow-warn); fi
    if [[ "$fail_on_invalid_tokens" -eq 1 ]]; then check_cmd+=(--fail-on-invalid-tokens); fi
    echo "probe_pool_once: summary found, checking"
    "${check_cmd[@]}"
    exit $?
  fi
  sleep "$(awk "BEGIN { printf \"%.3f\", $poll_ms/1000 }")"
done

echo "probe_pool_once: timeout waiting for [POOL-PROBE] summary (timeout_sec=${timeout_sec})" >&2
exit 4
