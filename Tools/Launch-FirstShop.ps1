# Plasma-only starter shop, independent of the empty shell and larger factory maps.
$ProjectRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -WindowStyle Normal -ArgumentList ('"'+(Join-Path $ProjectRoot 'Foundryline.uproject')+'" /Game/Foundryline/Maps/L_FirstShopStarter -game -windowed -ResX=1920 -ResY=1080 -nosplash')
