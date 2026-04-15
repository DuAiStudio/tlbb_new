# F2: auto-select latest ShareMemory log then run pool-probe checker.
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts\check_pool_probe_latest.ps1 -LogDir D:\Log
#   powershell -ExecutionPolicy Bypass -File scripts\check_pool_probe_latest.ps1 -LogDir D:\Log -AllowWarn -FailOnInvalidTokens

param(
    [string] $LogDir = "D:\Log",
    [switch] $AllowWarn,
    [switch] $FailOnInvalidTokens
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path -LiteralPath $LogDir)) {
    Write-Error "LogDir not found: $LogDir"
    exit 2
}

$latest = Get-ChildItem -LiteralPath $LogDir -Filter "ShareMemory_*.log" -File |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if ($null -eq $latest) {
    Write-Error "No ShareMemory_*.log found in: $LogDir"
    exit 3
}

$checker = Join-Path $PSScriptRoot "check_pool_probe_log.ps1"
$args = @("-ExecutionPolicy", "Bypass", "-File", $checker, $latest.FullName)
if ($AllowWarn) { $args += "-AllowWarn" }
if ($FailOnInvalidTokens) { $args += "-FailOnInvalidTokens" }

Write-Host "check_pool_probe_latest: using $($latest.FullName)"
& powershell @args
exit $LASTEXITCODE
