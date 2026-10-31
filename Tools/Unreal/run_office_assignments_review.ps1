$ErrorActionPreference='Stop'
$officeRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$officeSettings=Join-Path $officeRoot 'Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini'
$officeReport=Join-Path $officeRoot 'Saved/Logs/OfficeAssignmentsNativeQA.json'
if(Test-Path $officeReport){Remove-Item -LiteralPath $officeReport}
$officeOriginal=[IO.File]::ReadAllBytes($officeSettings)
try {
 $officeConfig=[IO.File]::ReadAllText($officeSettings)
 $officeConfig=$officeConfig -replace '(?m)^PlayNetMode=.*$','PlayNetMode=PIE_ListenServer' -replace '(?m)^PlayNumberOfClients=.*$','PlayNumberOfClients=4' -replace '(?m)^RunUnderOneProcess=.*$','RunUnderOneProcess=True'
 [IO.File]::WriteAllText($officeSettings,$officeConfig)
 $officeArgs='"'+$officeRoot+'/Foundryline.uproject" -unattended -nosplash -NoSound -ExecutePythonScript="'+$officeRoot+'/Tools/Unreal/review_office_assignments.py" -abslog="'+$officeRoot+'/Saved/Logs/OfficeAssignmentsNative.log"'
 $officeProcess=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $officeArgs -WindowStyle Hidden -PassThru
 if(!$officeProcess.WaitForExit(360000)){Stop-Process -Id $officeProcess.Id;throw 'Office review timeout; stopped only its own editor.'}
 if(!(Test-Path $officeReport)){throw 'Office review did not produce its report.'}
 $officeResult=Get-Content $officeReport -Raw | ConvertFrom-Json
 if(!$officeResult.completed -or $officeResult.errors.Count -gt 0 -or $officeResult.checks.PSObject.Properties.Value -contains $false){throw 'Office acceptance failed. See Saved/Logs/OfficeAssignmentsNativeQA.json'}
 Write-Output "$(@($officeResult.checks.PSObject.Properties).Count) native office checks passed."
} finally {[IO.File]::WriteAllBytes($officeSettings,$officeOriginal)}
