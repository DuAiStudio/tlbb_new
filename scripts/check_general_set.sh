#!/usr/bin/env bash
# C2: verify t_general_set dirty keys after ShareMemory save flow.
# Usage:
#   bash scripts/check_general_set.sh <host> <port> <user> <password> <database> [expected_nVal]
set -euo pipefail

if [[ $# -lt 5 ]]; then
  echo "Usage: $0 <host> <port> <user> <password> <database> [expected_nVal]" >&2
  exit 2
fi

host="$1"
port="$2"
user="$3"
pass="$4"
db="$5"
expected="${6:-1}"

sql="SELECT sKey,nVal FROM t_general_set WHERE sKey IN ('GUILD_NEW','PSHOP_NEW','CITY_NEW') ORDER BY sKey;"
rows="$(mysql -N -B -h "$host" -P "$port" -u "$user" "-p$pass" "$db" -e "$sql")"

if [[ -z "$rows" ]]; then
  echo "check_general_set: FAILED (no rows)" >&2
  exit 1
fi

echo "$rows"
missing=0
for key in GUILD_NEW PSHOP_NEW CITY_NEW; do
  if ! awk -v k="$key" -v v="$expected" '$1==k && $2==v {found=1} END{exit found?0:1}' <<<"$rows"; then
    echo "MISSING_OR_MISMATCH: $key expected_nVal=$expected" >&2
    missing=1
  fi
done

if [[ $missing -ne 0 ]]; then
  echo "check_general_set: FAILED" >&2
  exit 1
fi

echo "check_general_set: OK (expected_nVal=$expected)"
