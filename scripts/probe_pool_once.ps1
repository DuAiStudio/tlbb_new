# F3: trigger pool probe then wait latest log for summary and check.
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts\probe_pool_once.ps1 -RuntimeDir D:\Server -LogDir D:\Log
#   powershell -ExecutionPolicy Bypass -File scripts\probe_pool_once.ps1 -RuntimeDir D:\Server -LogDir D:\Log -Types "GlobalData,ItemSerial" -AllowWarn
#   powershell -ExecutionPolicy Bypass -File scripts\probe_pool_once.ps1 -RuntimeDir D:\Server -LogDir D:\Log -UseTemplate -TimeoutSec 30 -FailOnInvalidTokens

param(
    [string] $RuntimeDir = ".",
    [string] $LogDir = "D:\Log",
    [string] $Types = "",
    [switch] $UseTemplate,
    [switch] $AllowWarn,
    [switch] $FailOnInvalidTokens,
    [int] $TimeoutSec = 20,
    [int] $PollMs = 1000
)

$ErrorActionPreference = "Stop"
if ($TimeoutSec -lt 1) { $TimeoutSec = 1 }
if ($PollMs -lt 200) { $PollMs = 200 }

$trigger = Join-Path $PSScriptRoot "trigger_pool_probe.ps1"
$checkLatest = Join-Path $PSScriptRoot "check_pool_probe_latest.ps1"

$triggerArgs = @("-ExecutionPolicy", "Bypass", "-File", $trigger, "-RuntimeDir", $RuntimeDir)
if ($UseTemplate) { $triggerArgs += "-UseTemplate" }
elseif ($Types.Trim().Length -gt 0) { $triggerArgs += @("-Types", $Types) }

Write-Host "probe_pool_once: trigger probe"
& powershell @triggerArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$deadline = (Get-Date).AddSeconds($TimeoutSec)
while ((Get-Date) -lt $deadline) {
    $latest = Get-ChildItem -LiteralPath $LogDir -Filter "ShareMemory_*.log" -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($null -ne $latest) {
        $text = Get-Content -LiteralPath $latest.FullName -Raw -Encoding UTF8
        if ($text.IndexOf("[POOL-PROBE] summary ", [StringComparison]::Ordinal) -ge 0) {
            $checkArgs = @("-ExecutionPolicy", "Bypass", "-File", $checkLatest, "-LogDir", $LogDir)
            if ($AllowWarn) { $checkArgs += "-AllowWarn" }
            if ($FailOnInvalidTokens) { $checkArgs += "-FailOnInvalidTokens" }
            Write-Host "probe_pool_once: summary found, checking"
            & powershell @checkArgs
            exit $LASTEXITCODE
        }
    }
    Start-Sleep -Milliseconds $PollMs
}

Write-Error "probe_pool_once: timeout waiting for [POOL-PROBE] summary (TimeoutSec=$TimeoutSec)"
exit 4
