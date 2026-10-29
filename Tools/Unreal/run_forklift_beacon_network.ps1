# Listen-server forklift beacon check with N players (default 2). Restores the editor play settings afterwards.
param([int]$Clients=2,[switch]$Render)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$settings=Join-Path $root 'Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini'
$original=[System.IO.File]::ReadAllBytes($settings)
try {
 $config=[System.IO.File]::ReadAllText($settings)
 $config=$config -replace '(?m)^PlayNetMode=.*$','PlayNetMode=PIE_ListenServer' -replace '(?m)^PlayNumberOfClients=.*$',"PlayNumberOfClients=$Clients" -replace '(?m)^RunUnderOneProcess=.*$','RunUnderOneProcess=True'
 [System.IO.File]::WriteAllText($settings,$config)
 $rhi=if($Render){@('-windowed','-ResX=1600','-ResY=900')}else{@('-NullRHI')}
 & 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' (Join-Path $root 'Foundryline.uproject') -unattended @rhi -nosound -nop4 -nosplash "-ExecutePythonScript=$(Join-Path $PSScriptRoot 'review_forklift_beacon.py')" "-abslog=$(Join-Path $root 'Saved/Logs/ForkliftBeaconNetwork.log')"
 $code=$LASTEXITCODE
} finally {[System.IO.File]::WriteAllBytes($settings,$original)}
exit $code
