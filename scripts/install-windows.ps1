param([string]$PluginPath = '', [string]$Destination = '')
$ErrorActionPreference = 'Stop'
if (!$PluginPath) {
    $PluginPath = Join-Path $PSScriptRoot 'FugScope.dll'
    if (!(Test-Path $PluginPath)) {
        $PluginPath = Join-Path (Split-Path $PSScriptRoot -Parent) 'dist/FugScope-windows-x64/FugScope.dll'
    }
}
if (!(Test-Path -LiteralPath $PluginPath -PathType Leaf)) { throw 'Build or extract FugScope.dll first.' }
$Documents = [Environment]::GetFolderPath('MyDocuments')
if (!$Destination) { $Destination = Join-Path $Documents 'Resolume/Extra Effects' }
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$Target = Join-Path $Destination 'FugScope.dll'
if (Test-Path -LiteralPath $Target) {
    $Backup = Join-Path $Documents ('FugScope Backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $Backup -Force | Out-Null
    Copy-Item -LiteralPath $Target -Destination $Backup
}
Copy-Item -LiteralPath $PluginPath -Destination $Target -Force
Write-Host "Installed: $Target"
Write-Host 'Add this directory in Resolume: Preferences > Video > FFGL Plugin Directories.'
Write-Host 'Restart Resolume and find FugScope in Sources. Enable Test Signal to check rendering.'
