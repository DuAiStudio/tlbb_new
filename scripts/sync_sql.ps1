param(
    [string]$SourceDir = "D:\MyCode\Tlbb\Sql",
    [string]$TargetDir = ".\sql"
)

New-Item -ItemType Directory -Force $TargetDir | Out-Null
Copy-Item "$SourceDir\*.sql" $TargetDir -Force
Write-Output "SQL files synced to $TargetDir"
