$ProjectRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -WindowStyle Normal -ArgumentList ('"'+(Join-Path $ProjectRoot 'Foundryline.uproject')+'" /Game/Foundryline/Maps/L_MainMenu -game -windowed -ResX=1280 -ResY=720 -nosplash')
