param([Parameter(Mandatory=$true)][string]$Stage,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference = 'Stop'
$compiler = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
if (!(Test-Path $compiler)) { throw 'Install Inno Setup 6: winget install JRSoftware.InnoSetup' }
New-Item -ItemType Directory -Force -Path $Output | Out-Null
& $compiler "/DStage=$Stage" "/DOutput=$Output" "$PSScriptRoot\G-Boy.iss"
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed' }
