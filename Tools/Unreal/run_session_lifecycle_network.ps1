param([string]$taskRoot='C:/Users/curly/Documents/Foundryline Game')
$ErrorActionPreference='Stop'
$taskSettings=Join-Path $taskRoot 'Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini'
$taskOriginal=[System.IO.File]::ReadAllBytes($taskSettings)
$taskScript=Join-Path $env:TEMP 'FoundrylineSessionLifecycle.py'
[System.IO.File]::WriteAllText($taskScript,"import runpy`nrunpy.run_path(r'$taskRoot/Tools/Unreal/validate_session_lifecycle_network.py')`n")
try {
 $taskConfig=[System.IO.File]::ReadAllText($taskSettings)
 $taskConfig=$taskConfig -replace '(?m)^PlayNetMode=.*$','PlayNetMode=PIE_ListenServer' -replace '(?m)^PlayNumberOfClients=.*$','PlayNumberOfClients=4' -replace '(?m)^RunUnderOneProcess=.*$','RunUnderOneProcess=True'
 [System.IO.File]::WriteAllText($taskSettings,$taskConfig)
 & 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' (Join-Path $taskRoot 'Foundryline.uproject') -unattended -NullRHI -nosound -nop4 "-ExecutePythonScript=$taskScript" '-abslog=C:/Users/curly/AppData/Local/Temp/SessionLifecycleNetwork.log'
 $taskExit=$LASTEXITCODE
 Copy-Item -LiteralPath 'C:/Users/curly/AppData/Local/Temp/SessionLifecycleNetwork.log' -Destination (Join-Path $taskRoot 'Saved/Logs/SessionLifecycleNetwork.log') -Force
} finally {[System.IO.File]::WriteAllBytes($taskSettings,$taskOriginal)}
exit $taskExit
