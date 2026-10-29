# Four-player listen-server welding-station check (plus a late joiner). Restores the editor play settings afterwards.
param([string]$Script='validate_welding_station_network.py',[string]$Name='WeldingStationNetwork',[int]$Clients=4,[switch]$Render)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$settings=Join-Path $root 'Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini'
$original=[System.IO.File]::ReadAllBytes($settings)
try {
 $config=[System.IO.File]::ReadAllText($settings)
 $mode=if($Clients -gt 1){'PIE_ListenServer'}else{'PIE_Standalone'}
 $config=$config -replace '(?m)^PlayNetMode=.*$',"PlayNetMode=$mode" -replace '(?m)^PlayNumberOfClients=.*$',"PlayNumberOfClients=$Clients" -replace '(?m)^RunUnderOneProcess=.*$','RunUnderOneProcess=True'
 [System.IO.File]::WriteAllText($settings,$config)
 $rhi=if($Render){@('-windowed','-ResX=1920','-ResY=1080')}else{@('-NullRHI')}
 & 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' (Join-Path $root 'Foundryline.uproject') -unattended @rhi -nosound -nop4 -nosplash "-ExecutePythonScript=$(Join-Path $PSScriptRoot $Script)" "-abslog=$(Join-Path $root "Saved/Logs/$Name.log")"
 $code=$LASTEXITCODE
} finally {[System.IO.File]::WriteAllBytes($settings,$original)}
exit $code
