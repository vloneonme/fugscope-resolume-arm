param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
$Build = Join-Path $Root 'build/windows-x64'
$Package = Join-Path $Root 'dist/FugScope-windows-x64'
function Invoke-Checked { param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE)" }
}
Invoke-Checked cmake @('-S', $Root, '-B', $Build, '-G', 'Visual Studio 17 2022', '-A', 'x64')
Invoke-Checked cmake @('--build', $Build, '--config', $Configuration, '--parallel')
Invoke-Checked ctest @('--test-dir', $Build, '-C', $Configuration, '--output-on-failure')
Invoke-Checked cmake @('--install', $Build, '--config', $Configuration, '--prefix', $Package)
Copy-Item (Join-Path $PSScriptRoot 'install-windows.ps1') $Package -Force
Compress-Archive -Path "$Package/*" -DestinationPath (Join-Path $Root 'dist/FugScope-windows-x64.zip') -Force
Write-Host "Built: $Package/FugScope.dll"
