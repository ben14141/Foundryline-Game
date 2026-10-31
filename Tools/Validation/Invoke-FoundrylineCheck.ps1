param(
 [ValidateSet('Presentation','Commerce','Employees','Construction','Full')][string]$Profile='Presentation',
 [switch]$NoBuild,
 [switch]$Assets,
 [int]$TimeoutSeconds=600
)
$ErrorActionPreference='Stop'
$ProjectRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$EngineRoot='C:\Program Files\Epic Games\UE_5.8'
$LogRoot=Join-Path $ProjectRoot 'Saved/Logs'
New-Item -ItemType Directory -Force -Path $LogRoot | Out-Null
$Watch=[Diagnostics.Stopwatch]::StartNew()
$Timing=[ordered]@{profile=$Profile;buildSeconds=0;testSeconds=0;success=$false}
try {
 if(-not $NoBuild){
  $Sources=Get-ChildItem (Join-Path $ProjectRoot 'Source') -Recurse -File | Where-Object {$_.Extension -in '.h','.cpp','.cs'}
  $Newest=($Sources | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).LastWriteTimeUtc
  $Dll=Join-Path $ProjectRoot 'Binaries/Win64/UnrealEditor-Foundryline.dll'
  if(!(Test-Path $Dll) -or (Get-Item $Dll).LastWriteTimeUtc -lt $Newest){
   & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') FoundrylineEditor Win64 Development (Join-Path $ProjectRoot 'Foundryline.uproject') -WaitMutex -NoHotReloadFromIDE *> (Join-Path $LogRoot 'FocusedBuild.log')
   if($LASTEXITCODE -ne 0){throw 'Build failed: Saved/Logs/FocusedBuild.log'}
  }
 }
 $Timing.buildSeconds=[Math]::Round($Watch.Elapsed.TotalSeconds,2)
 $Filters=@{Presentation='StartsWith:Foundryline.Commerce+StartsWith:Foundryline.Employees+StartsWith:Foundryline.Flatbed';Commerce='StartsWith:Foundryline.Commerce';Employees='StartsWith:Foundryline.Employees';Construction='StartsWith:Foundryline.Construction';Full='StartsWith:Foundryline.'}
 $Report=Join-Path $ProjectRoot ('Saved/Automation/Focused-'+$Profile)
 $Log=Join-Path $LogRoot ('Focused-'+$Profile+'.log')
 $Arguments='"'+(Join-Path $ProjectRoot 'Foundryline.uproject')+'" -unattended -nop4 -nosplash -NullRHI -NoSound -nosourcecontrol -ReportExportPath="'+$Report+'" -abslog="'+$Log+'" -ExecCmds="Automation RunTests '+$Filters[$Profile]+'" -TestExit="Automation Test Queue Empty"'
 if($Assets){$Arguments+=' -ExecutePythonScript="'+((Join-Path $ProjectRoot 'Tools/Unreal/audit_presentation_assets.py').Replace('\','/'))+'"'}
 if(Test-Path (Join-Path $Report 'index.json')){Remove-Item -LiteralPath (Join-Path $Report 'index.json')}
 if($Assets -and (Test-Path (Join-Path $LogRoot 'PresentationAssetQA.json'))){Remove-Item -LiteralPath (Join-Path $LogRoot 'PresentationAssetQA.json')}
 $Process=Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $Arguments -WindowStyle Hidden -PassThru
 if(!$Process.WaitForExit($TimeoutSeconds*1000)){Stop-Process -Id $Process.Id;throw 'Focused checks exceeded timeout; only this runner process was stopped.'}
 $Timing.testSeconds=[Math]::Round($Watch.Elapsed.TotalSeconds-$Timing.buildSeconds,2)
 $Index=Join-Path $Report 'index.json'
 if(!(Test-Path $Index)){throw 'Automation did not export a report.'}
 $Result=Get-Content $Index -Raw | ConvertFrom-Json
 $Passed=[int]$Result.succeeded+[int]$Result.succeededWithWarnings
 $Timing.testsPassed=$Passed
 if($Result.failed -gt 0 -or $Passed -lt 1){throw "Automation: $($Result.failed) failed, $Passed passed. See $Report"}
 if($Assets){$Audit=Get-Content (Join-Path $LogRoot 'PresentationAssetQA.json') -Raw | ConvertFrom-Json;if($Audit.errors.Count -gt 0 -or $Audit.checks.PSObject.Properties.Value -contains $false){throw 'Presentation asset audit failed.'}}
 $Timing.success=$true
 Write-Output "$Passed checks passed; build $($Timing.buildSeconds)s, tests $($Timing.testSeconds)s."
} finally {
 $Timing.totalSeconds=[Math]::Round($Watch.Elapsed.TotalSeconds,2)
 $Timing | ConvertTo-Json | Set-Content (Join-Path $LogRoot 'FocusedCheckTiming.json')
}
