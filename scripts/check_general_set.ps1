# C2: verify t_general_set dirty keys after ShareMemory save flow.
# Usage:
# powershell -ExecutionPolicy Bypass -File scripts\check_general_set.ps1 `
#   -Host 127.0.0.1 -Port 3306 -User root -Password yourpass -Database tlbbdb [-Expected 1]

param(
    [Parameter(Mandatory = $true)][string] $Host,
    [Parameter(Mandatory = $true)][int] $Port,
    [Parameter(Mandatory = $true)][string] $User,
    [Parameter(Mandatory = $true)][string] $Password,
    [Parameter(Mandatory = $true)][string] $Database,
    [int] $Expected = 1
)

$ErrorActionPreference = "Stop"
$sql = "SELECT sKey,nVal FROM t_general_set WHERE sKey IN ('GUILD_NEW','PSHOP_NEW','CITY_NEW') ORDER BY sKey;"
$args = @("-N", "-B", "-h", $Host, "-P", "$Port", "-u", $User, "-p$Password", $Database, "-e", $sql)
$rows = & mysql @args
if ($LASTEXITCODE -ne 0) {
    Write-Error "mysql command failed. Ensure mysql client is installed and credentials are correct."
    exit 2
}
if (-not $rows) {
    Write-Host "check_general_set: FAILED (no rows)" -ForegroundColor Red
    exit 1
}

$text = ($rows | Out-String).Trim()
Write-Host $text
$pairs = @{}
foreach ($line in $rows) {
    $parts = $line -split "\s+"
    if ($parts.Length -ge 2) {
        $pairs[$parts[0]] = [int]$parts[1]
    }
}

$missing = @()
foreach ($key in @("GUILD_NEW", "PSHOP_NEW", "CITY_NEW")) {
    if (-not $pairs.ContainsKey($key) -or $pairs[$key] -ne $Expected) {
        $missing += $key
    }
}

if ($missing.Count -gt 0) {
    foreach ($m in $missing) {
        Write-Host "MISSING_OR_MISMATCH: $m expected_nVal=$Expected" -ForegroundColor Red
    }
    Write-Host "check_general_set: FAILED" -ForegroundColor Red
    exit 1
}

Write-Host "check_general_set: OK (expected_nVal=$Expected)" -ForegroundColor Green
exit 0
