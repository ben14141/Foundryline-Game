# Launch the empty-lot road-only scenario without changing the default shop map.
$ProjectRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList ('"'+(Join-Path $ProjectRoot 'Foundryline.uproject')+'" /Game/Foundryline/Maps/L_RoadOnlyStarter -game -windowed -ResX=1920 -ResY=1080 -nosplash')
