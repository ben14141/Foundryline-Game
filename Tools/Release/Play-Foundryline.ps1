[CmdletBinding()]
param(
    [string]$ReleaseTag = 'windows-playtest-2026-10-31',
    [string]$InstallDirectory = (Join-Path $env:LOCALAPPDATA 'Foundryline\Playtest'),
    [switch]$DownloadOnly
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$headers = @{ 'User-Agent' = 'Foundryline-Playtest'; 'Accept' = 'application/vnd.github+json' }
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/ben14141/Foundryline-Game/releases/tags/$ReleaseTag" -Headers $headers
$archive = @($release.assets | Where-Object name -eq 'Foundryline-Windows.zip')
$checksum = @($release.assets | Where-Object name -eq 'Foundryline-Windows.zip.sha256')
if ($archive.Count -ne 1 -or $checksum.Count -ne 1) { throw 'The Windows playtest download is not available on this release.' }
$versionDirectory = Join-Path $InstallDirectory ([string]$release.id)
$executable = Join-Path $versionDirectory 'Windows\Foundryline.exe'
$marker = Join-Path $versionDirectory 'download-asset-id.txt'
$installed = (Test-Path -LiteralPath $executable) -and (Test-Path -LiteralPath $marker) -and ((Get-Content -LiteralPath $marker -Raw).Trim() -eq [string]$archive[0].id)
if (!$installed) {
    $temporaryDirectory = Join-Path ([IO.Path]::GetTempPath()) ('Foundryline-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $temporaryDirectory -Force | Out-Null
    $zipPath = Join-Path $temporaryDirectory 'Foundryline-Windows.zip'
    $hashPath = Join-Path $temporaryDirectory 'Foundryline-Windows.zip.sha256'
    Write-Host 'Downloading the complete Windows game, including cooked game content...'
    Invoke-WebRequest -UseBasicParsing -Uri $checksum[0].browser_download_url -OutFile $hashPath
    Invoke-WebRequest -UseBasicParsing -Uri $archive[0].browser_download_url -OutFile $zipPath
    $expectedHash = ((Get-Content -LiteralPath $hashPath -Raw).Trim() -split '\s+')[0]
    if ($expectedHash -notmatch '^[0-9a-fA-F]{64}$' -or (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash -ne $expectedHash) { throw 'Download verification failed. The game was not launched.' }
    New-Item -ItemType Directory -Path $versionDirectory -Force | Out-Null
    Expand-Archive -LiteralPath $zipPath -DestinationPath $versionDirectory -Force
    if (!(Test-Path -LiteralPath $executable)) { throw 'The downloaded archive does not contain the game executable.' }
    Set-Content -LiteralPath $marker -Value ([string]$archive[0].id) -Encoding ascii
    # Leave failed downloads available for diagnosis; only clean our verified temporary files.
    Remove-Item -LiteralPath $zipPath,$hashPath
    Remove-Item -LiteralPath $temporaryDirectory
}
Write-Host "Ready: $executable"
if (!$DownloadOnly) { Start-Process -FilePath $executable -WorkingDirectory (Split-Path -Parent $executable) }
