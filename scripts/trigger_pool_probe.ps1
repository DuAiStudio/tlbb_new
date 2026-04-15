# F1: create probe_pool.cmd and optional probe_pool.types in runtime directory.
# Usage examples:
#   powershell -ExecutionPolicy Bypass -File scripts\trigger_pool_probe.ps1 -RuntimeDir D:\Server
#   powershell -ExecutionPolicy Bypass -File scripts\trigger_pool_probe.ps1 -RuntimeDir D:\Server -Types "GlobalData,ItemSerial"
#   powershell -ExecutionPolicy Bypass -File scripts\trigger_pool_probe.ps1 -RuntimeDir D:\Server -UseTemplate

param(
    [string] $RuntimeDir = ".",
    [string] $Types = "",
    [switch] $UseTemplate
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $RuntimeDir)) {
    Write-Error "RuntimeDir not found: $RuntimeDir"
    exit 2
}

$runtimePath = (Resolve-Path -LiteralPath $RuntimeDir).Path
$typesPath = Join-Path $runtimePath "probe_pool.types"
$cmdPath = Join-Path $runtimePath "probe_pool.cmd"
$templatePath = Join-Path $PSScriptRoot "sample_probe_pool.types"

if ($UseTemplate) {
    if (-not (Test-Path -LiteralPath $templatePath)) {
        Write-Error "Template not found: $templatePath"
        exit 2
    }
    Copy-Item -LiteralPath $templatePath -Destination $typesPath -Force
    Write-Host "Wrote probe types from template: $typesPath"
} elseif ($Types.Trim().Length -gt 0) {
    Set-Content -LiteralPath $typesPath -Value $Types -Encoding UTF8
    Write-Host "Wrote probe types: $typesPath"
} else {
    if (Test-Path -LiteralPath $typesPath) {
        Remove-Item -LiteralPath $typesPath -Force
        Write-Host "Removed stale probe types: $typesPath"
    }
}

Set-Content -LiteralPath $cmdPath -Value "probe" -Encoding ASCII
Write-Host "Triggered probe command: $cmdPath"
exit 0
