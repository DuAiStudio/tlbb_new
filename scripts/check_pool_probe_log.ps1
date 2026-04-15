# E3: verify POOL-PROBE summary status lines.
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts\check_pool_probe_log.ps1 D:\Log\ShareMemory_2026-04-14.xxxxx.log
#   powershell -ExecutionPolicy Bypass -File scripts\check_pool_probe_log.ps1 D:\Log\ShareMemory_2026-04-14.xxxxx.log -AllowWarn

param(
    [Parameter(Mandatory = $true)]
    [string] $LogPath,
    [switch] $AllowWarn,
    [switch] $FailOnInvalidTokens
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path -LiteralPath $LogPath)) {
    Write-Error "Log file not found: $LogPath"
    exit 2
}

$ok = 0
$warn = 0
$fail = 0
$total = 0
$invalidTokens = 0
$summaryMatched = $false

Get-Content -LiteralPath $LogPath -Encoding UTF8 | ForEach-Object {
    $line = $_
    if (-not $summaryMatched -and $line.IndexOf("[POOL-PROBE] summary ", [StringComparison]::Ordinal) -ge 0) {
        if ($line -match "total=(\d+)\s+(?:filtered=(\d+)\s+)?ok=(\d+)\s+warn=(\d+)\s+fail=(\d+)(?:\s+invalid_tokens=(\d+))?") {
            $total = [int]$Matches[1]
            $ok = [int]$Matches[3]
            $warn = [int]$Matches[4]
            $fail = [int]$Matches[5]
            if ($Matches[6]) {
                $invalidTokens = [int]$Matches[6]
            }
            $summaryMatched = $true
        }
        return
    }
    if ($summaryMatched) { return }
    if ($line.IndexOf("[POOL-PROBE] status=", [StringComparison]::Ordinal) -lt 0) { return }
    $total++
    if ($line.IndexOf("status=OK", [StringComparison]::Ordinal) -ge 0) {
        $ok++
    } elseif ($line.IndexOf("status=WARN", [StringComparison]::Ordinal) -ge 0) {
        $warn++
    } elseif ($line.IndexOf("status=FAIL", [StringComparison]::Ordinal) -ge 0) {
        $fail++
    }
}

if ($total -eq 0) {
    Write-Host "check_pool_probe_log: no [POOL-PROBE] status lines found" -ForegroundColor Yellow
    exit 3
}

Write-Host "check_pool_probe_log: total=$total ok=$ok warn=$warn fail=$fail invalid_tokens=$invalidTokens"

if ($fail -gt 0) {
    Write-Host "check_pool_probe_log: FAILED (status=FAIL present)" -ForegroundColor Red
    exit 1
}
if (-not $AllowWarn -and $warn -gt 0) {
    Write-Host "check_pool_probe_log: FAILED (status=WARN present, use -AllowWarn to ignore)" -ForegroundColor Red
    exit 1
}
if ($FailOnInvalidTokens -and $invalidTokens -gt 0) {
    Write-Host "check_pool_probe_log: FAILED (invalid_tokens present, remove bad probe_pool.types tokens)" -ForegroundColor Red
    exit 1
}

Write-Host "check_pool_probe_log: OK ($LogPath)" -ForegroundColor Green
exit 0
