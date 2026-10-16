param([string]$Installer,[string]$Output)
$ErrorActionPreference='Stop'
$target=Join-Path $env:RUNNER_TEMP 'G Boy Installer Smoke'
$profile=Join-Path $env:APPDATA 'G Boy\Library'
New-Item -ItemType Directory -Force -Path $profile | Out-Null
$marker=Join-Path $profile 'installer-preserve-test.txt';Set-Content $marker 'User library must survive uninstall'
$process=Start-Process -FilePath $Installer -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/DIR=`"$target`"") -Wait -PassThru
if ($process.ExitCode -ne 0) { throw "Install failed: $($process.ExitCode)" }
$binary=Join-Path $env:CommonProgramFiles 'VST3\G Boy.vst3\Contents\x86_64-win\G Boy.vst3'
if (!(Test-Path $binary) -or !(Test-Path "$target\G Boy.exe")) { throw 'Installer payload missing' }
$manifest=Join-Path $env:ProgramData 'G Boy\Factory Content\gboy-library.json'
if (!(Test-Path $manifest)) { throw 'Factory content not installed' }
$process=Start-Process -FilePath "$target\unins000.exe" -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -Wait -PassThru
if ($process.ExitCode -ne 0 -or (Test-Path $binary) -or !(Test-Path $marker)) { throw 'Uninstall or profile-preservation failed' }
New-Item -ItemType Directory -Force -Path $Output | Out-Null
@{passed=$true;installed_and_uninstalled=$true;user_profile_preserved=$true} | ConvertTo-Json | Set-Content (Join-Path $Output 'installer-smoke.json')
