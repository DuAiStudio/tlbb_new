#!/usr/bin/env bash
# F1: create probe_pool.cmd and optional probe_pool.types in runtime directory.
# Usage examples:
#   bash scripts/trigger_pool_probe.sh --runtime-dir /opt/tlbb/server
#   bash scripts/trigger_pool_probe.sh --runtime-dir /opt/tlbb/server --types "GlobalData,ItemSerial"
#   bash scripts/trigger_pool_probe.sh --runtime-dir /opt/tlbb/server --use-template
set -euo pipefail

runtime_dir="."
types=""
use_template=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runtime-dir)
      runtime_dir="${2:-}"
      shift 2
      ;;
    --types)
      types="${2:-}"
      shift 2
      ;;
    --use-template)
      use_template=1
      shift
      ;;
    *)
      echo "Unknown arg: $1" >&2
      echo "Usage: $0 [--runtime-dir DIR] [--types CSV] [--use-template]" >&2
      exit 2
      ;;
  esac
done

if [[ ! -d "$runtime_dir" ]]; then
  echo "Runtime dir not found: $runtime_dir" >&2
  exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
types_path="${runtime_dir}/probe_pool.types"
cmd_path="${runtime_dir}/probe_pool.cmd"
template_path="${script_dir}/sample_probe_pool.types"

if [[ "$use_template" -eq 1 ]]; then
  if [[ ! -f "$template_path" ]]; then
    echo "Template not found: $template_path" >&2
    exit 2
  fi
  cp "$template_path" "$types_path"
  echo "Wrote probe types from template: $types_path"
elif [[ -n "${types//[[:space:]]/}" ]]; then
  printf "%s\n" "$types" >"$types_path"
  echo "Wrote probe types: $types_path"
else
  if [[ -f "$types_path" ]]; then
    rm -f "$types_path"
    echo "Removed stale probe types: $types_path"
  fi
fi

printf "probe\n" >"$cmd_path"
echo "Triggered probe command: $cmd_path"
exit 0
