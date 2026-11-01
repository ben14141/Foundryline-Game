[CmdletBinding()]
param(
    [string]$EngineDirectory = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$ArchiveDirectory
)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (!$ArchiveDirectory) { $ArchiveDirectory = Join-Path $projectDirectory 'Saved\PlaytestRelease' }
$uat = Join-Path $EngineDirectory 'Engine\Build\BatchFiles\RunUAT.bat'
if (!(Test-Path -LiteralPath $uat)) { throw 'Unreal Engine 5.8 AutomationTool was not found.' }
if (!(Test-Path -LiteralPath (Join-Path $projectDirectory 'Content\Foundryline\Maps\L_MainMenu.umap'))) { throw 'Packaging requires the authorized full-content developer checkout.' }
& $uat BuildCookRun "-project=$projectDirectory\Foundryline.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -nocompileeditor -skipbuildeditor -cook -CookAll -stage -pak -iostore -compressed -nodebuginfo -archive "-archivedirectory=$ArchiveDirectory" -utf8output
if ($LASTEXITCODE -ne 0) { throw "Windows packaging failed (exit $LASTEXITCODE)." }
Write-Host "Packaged game: $ArchiveDirectory\Windows\Foundryline.exe"
