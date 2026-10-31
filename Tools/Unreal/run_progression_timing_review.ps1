$ErrorActionPreference='Stop'
$reviewRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$reviewSettings=Join-Path $reviewRoot 'Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini'
$reviewReport=Join-Path $reviewRoot 'Saved/Logs/ProgressionTimingNativeQA.json'
$reviewOriginal=[IO.File]::ReadAllBytes($reviewSettings)
try {
 $reviewConfig=[IO.File]::ReadAllText($reviewSettings)
 $reviewConfig=$reviewConfig -replace '(?m)^PlayNetMode=.*$','PlayNetMode=PIE_Standalone' -replace '(?m)^PlayNumberOfClients=.*$','PlayNumberOfClients=1' -replace '(?m)^RunUnderOneProcess=.*$','RunUnderOneProcess=True'
 [IO.File]::WriteAllText($reviewSettings,$reviewConfig)
 if(Test-Path $reviewReport){Remove-Item -LiteralPath $reviewReport}
 $reviewArgs='"'+$reviewRoot+'/Foundryline.uproject" -unattended -nosplash -NoSound -StarterReviewQuit -ExecutePythonScript="'+$reviewRoot+'/Tools/Unreal/review_progression_timing.py" -abslog="'+$reviewRoot+'/Saved/Logs/ProgressionTimingNative.log"'
 $reviewProcess=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $reviewArgs -WindowStyle Hidden -PassThru
 if(!$reviewProcess.WaitForExit(1860000)){Stop-Process -Id $reviewProcess.Id;throw 'Starter review timed out; stopped only its own editor.'}
 if(!(Test-Path $reviewReport)){throw 'Starter review did not produce its report.'}
 $reviewResult=Get-Content $reviewReport -Raw | ConvertFrom-Json
 if(!$reviewResult.completed -or $reviewResult.errors.Count -gt 0 -or $reviewResult.checks.PSObject.Properties.Value -contains $false){throw 'Starter acceptance failed. See Saved/Logs/ProgressionTimingNativeQA.json'}
 Write-Output "$(@($reviewResult.checks.PSObject.Properties).Count) native progression checks passed."
} finally {[IO.File]::WriteAllBytes($reviewSettings,$reviewOriginal)}
