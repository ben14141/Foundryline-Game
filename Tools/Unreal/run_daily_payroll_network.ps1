$ErrorActionPreference='Stop'
$rackRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$rackSettings=Join-Path $rackRoot 'Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini'
$rackReport=Join-Path $rackRoot 'Saved/Logs/DailyPayrollNetworkQA.json'
if(Test-Path $rackReport){Remove-Item -LiteralPath $rackReport}
$rackOriginal=[IO.File]::ReadAllBytes($rackSettings)
try {
 $rackConfig=[IO.File]::ReadAllText($rackSettings)
 $rackConfig=$rackConfig -replace '(?m)^PlayNetMode=.*$','PlayNetMode=PIE_ListenServer' -replace '(?m)^PlayNumberOfClients=.*$','PlayNumberOfClients=4' -replace '(?m)^RunUnderOneProcess=.*$','RunUnderOneProcess=True'
 [IO.File]::WriteAllText($rackSettings,$rackConfig)
 $rackArgs='"'+$rackRoot+'/Foundryline.uproject" -unattended -nosplash -NoSound -ExecutePythonScript="'+$rackRoot+'/Tools/Unreal/review_daily_payroll_network.py" -abslog="'+$rackRoot+'/Saved/Logs/DailyPayrollNetwork.log"'
 $rackProcess=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $rackArgs -WindowStyle Hidden -PassThru
 if(!$rackProcess.WaitForExit(360000)){Stop-Process -Id $rackProcess.Id;throw 'Rack review timeout; stopped only its own editor.'}
 if(!(Test-Path $rackReport)){throw 'Rack review did not produce its report.'}
 $rackResult=Get-Content $rackReport -Raw | ConvertFrom-Json
 if(!$rackResult.completed -or $rackResult.errors.Count -gt 0 -or $rackResult.checks.PSObject.Properties.Value -contains $false){throw 'Rack acceptance failed. See Saved/Logs/DailyPayrollNetworkQA.json'}
 Write-Output "$(@($rackResult.checks.PSObject.Properties).Count) native daily payroll checks passed."
} finally {[IO.File]::WriteAllBytes($rackSettings,$rackOriginal)}
