[CmdletBinding()]
param(
    [string]$ContentRepository,
    [switch]$Publish
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (!$ContentRepository) { $ContentRepository = Join-Path (Split-Path -Parent $projectRoot) 'Foundryline Content' }
$ContentRepository = [IO.Path]::GetFullPath($ContentRepository)
if ($ContentRepository.StartsWith($projectRoot + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or $ContentRepository -eq $projectRoot) { throw 'Use a separate sibling folder for the private content repository.' }
if (!(Test-Path -LiteralPath (Join-Path $projectRoot 'Foundryline.uproject'))) { throw 'Run this helper from a Foundryline source checkout.' }
& git lfs version
if ($LASTEXITCODE -ne 0) { throw 'Install Git LFS before synchronizing content.' }
$running = Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^(UnrealEditor|UnrealEditor-Cmd|Foundryline)\.exe$' -and $_.CommandLine -and $_.CommandLine.IndexOf($projectRoot,[StringComparison]::OrdinalIgnoreCase) -ge 0 }
if ($running) { throw 'Close this Foundryline editor/game before synchronizing content.' }
if (!(Test-Path -LiteralPath (Join-Path $ContentRepository '.git'))) {
    if ($Publish) { throw 'Restore the private repository before publishing edited assets.' }
    & git clone https://github.com/ben14141/Foundryline-Content.git $ContentRepository
    if ($LASTEXITCODE -ne 0) { throw 'Private content clone failed. Accept the CalebM61 invitation and sign Git in to an authorized GitHub account.' }
}
$remote = & git -C $ContentRepository remote get-url origin
if ($LASTEXITCODE -ne 0 -or $remote -notmatch 'github\.com[:/]ben14141/Foundryline-Content(?:\.git)?$') { throw 'This folder is not the Foundryline private content repository.' }
$dirty = & git -C $ContentRepository status --porcelain
if ($LASTEXITCODE -ne 0 -or $dirty) { throw 'Private content has uncommitted changes. Review/commit or preserve those edits before syncing.' }
if (!$Publish) {
    & git -C $ContentRepository pull --ff-only
    if ($LASTEXITCODE -ne 0) { throw 'Private content pull failed; no files were copied into the game.' }
    & git -C $ContentRepository lfs pull
    if ($LASTEXITCODE -ne 0) { throw 'Git LFS content download failed; no files were copied into the game.' }
}
$source = Join-Path $(if ($Publish) { $projectRoot } else { $ContentRepository }) 'Content'
$destination = Join-Path $(if ($Publish) { $ContentRepository } else { $projectRoot }) 'Content'
$source = (Resolve-Path -LiteralPath $source).Path
# Never copy pointer stubs into Unreal or replace real assets with them.
$sourceFiles = @(Get-ChildItem -LiteralPath $source -Recurse -File)
foreach ($file in $sourceFiles) {
    if ($file.Length -lt 512 -and $file.Extension -in '.uasset','.umap','.png','.wav','.ttf','.otf') {
        if ((Get-Content -LiteralPath $file.FullName -TotalCount 1) -eq 'version https://git-lfs.github.com/spec/v1') { throw "Unresolved Git LFS pointer: $($file.FullName). Run git lfs pull first." }
    }
}
$backupRoot = Join-Path $projectRoot ('Saved\ContentSyncBackups\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
$copied = 0; $backedUp = 0
foreach ($file in $sourceFiles) {
    $relative = $file.FullName.Substring($source.Length + 1)
    $target = Join-Path $destination $relative
    if (Test-Path -LiteralPath $target) {
        if ((Get-Item -LiteralPath $target).Length -eq $file.Length -and (Get-FileHash -LiteralPath $target).Hash -eq (Get-FileHash -LiteralPath $file.FullName).Hash) { continue }
        $backup = Join-Path $backupRoot $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
        Copy-Item -LiteralPath $target -Destination $backup
        $backedUp++
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    $copied++
}
Write-Host "Copied $copied files. No destination files were deleted."
if ($backedUp) { Write-Host "Preserved $backedUp replaced files in $backupRoot" }
if ($Publish) {
    Write-Host "Review, commit and push edited assets ONLY in the private repository: $ContentRepository"
    & git -C $ContentRepository status --short
} else {
    Write-Host 'Content restored. Open Foundryline.uproject with Unreal Engine 5.8.'
}
