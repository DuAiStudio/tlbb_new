# A3: verify a captured ShareMemory log contains expected startup substrings.
# Usage: powershell -File scripts\check_sharememory_log.ps1 D:\Log\ShareMemory_2026-04-14.xxxxx.log

param(
    [Parameter(Mandatory = $true)]
    [string] $LogPath
)

$ErrorActionPreference = "Stop"
$keywordsFile = Join-Path $PSScriptRoot "sharememory_startup_keywords.txt"
if (-not (Test-Path -LiteralPath $LogPath)) {
    Write-Error "Log file not found: $LogPath"
    exit 2
}
$text = Get-Content -LiteralPath $LogPath -Raw -Encoding UTF8
$missing = @()
Get-Content -LiteralPath $keywordsFile | ForEach-Object {
    $line = $_.Trim()
    if ($line.Length -eq 0 -or $line.StartsWith("#")) { return }
    if ($text.IndexOf($line, [StringComparison]::Ordinal) -lt 0) {
        $missing += $line
    }
}
if ($missing.Count -gt 0) {
    foreach ($m in $missing) { Write-Host "MISSING: $m" -ForegroundColor Red }
    Write-Host "check_sharememory_log: FAILED" -ForegroundColor Red
    exit 1
}
Write-Host "check_sharememory_log: OK ($LogPath)" -ForegroundColor Green
exit 0
