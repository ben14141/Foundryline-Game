param([switch]$Straight)
$ErrorActionPreference='Stop'
$officeRoot='C:/Users/curly/Documents/Foundryline Game'
$officeSettings=Join-Path $officeRoot 'Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini'
$officeReport=Join-Path $officeRoot $(if($Straight){'Saved/Logs/OfficeStoreysStraightNativeQA.json'}else{'Saved/Logs/OfficeStoreysNativeQA.json'})
if(Test-Path $officeReport){Remove-Item -LiteralPath $officeReport}
$officeOriginal=[IO.File]::ReadAllBytes($officeSettings)
[IO.File]::WriteAllBytes((Join-Path $officeRoot 'Saved/Logs/OfficeStoreysPlaySettings.before.ini'),$officeOriginal)
try {
 $officeConfig=[IO.File]::ReadAllText($officeSettings)
 $officeConfig=$officeConfig -replace '(?m)^PlayNetMode=.*$','PlayNetMode=PIE_ListenServer' -replace '(?m)^PlayNumberOfClients=.*$','PlayNumberOfClients=4' -replace '(?m)^RunUnderOneProcess=.*$','RunUnderOneProcess=True'
 [IO.File]::WriteAllText($officeSettings,$officeConfig)
 $officeArgs='"'+$officeRoot+'/Foundryline.uproject" -unattended -nosplash -NoSound -ExecutePythonScript="'+$officeRoot+'/Tools/Unreal/review_office_storeys.py" -abslog="'+$officeRoot+'/Saved/Logs/OfficeStoreysNative.log"'
 if($Straight){$officeArgs+=' -FoundrylineStraightReview'}
 $officeProcess=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $officeArgs -WindowStyle Hidden -PassThru
 if(!$officeProcess.WaitForExit(420000)){Stop-Process -Id $officeProcess.Id;throw 'Office review timeout; stopped only its own editor.'}
 if(!(Test-Path $officeReport)){throw 'Office review did not produce its acceptance report.'}
 $officeResult=Get-Content $officeReport -Raw | ConvertFrom-Json
 if(!$officeResult.completed -or $officeResult.errors.Count -gt 0 -or $officeResult.checks.PSObject.Properties.Value -contains $false){throw 'Office native acceptance failed. See Saved/Logs/OfficeStoreysNativeQA.json'}
 Write-Output "$(@($officeResult.checks.PSObject.Properties).Count) native checks passed."

} finally {[IO.File]::WriteAllBytes($officeSettings,$officeOriginal)}
