# Focused automation for the welding-station work. Usage: run_welding_tests.ps1 <Filter+Filter> <LogName>
param([string]$Filter='Foundryline.Factory.WeldingStation',[string]$Name='WeldingTests')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$log=Join-Path $root "Saved/Logs/$Name.log"
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' (Join-Path $root 'Foundryline.uproject') -unattended -NullRHI -nosound -nop4 -nosplash "-ExecCmds=Automation RunTests $Filter;Quit" "-TestExit=Automation Test Queue Empty" "-ReportExportPath=$(Join-Path $root "Saved/Automation/$Name")" "-abslog=$log"
exit $LASTEXITCODE
