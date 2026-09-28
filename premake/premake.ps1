# Run premake5 on Windows, downloading a pinned build on first use.
#
# Usage (from GenerateProject.bat):
#   powershell -NoProfile -ExecutionPolicy Bypass -File premake\premake.ps1 vs2022
#
# - The binary is cached in bin\tools\premake-<version>\ (git-ignored) and verified by SHA256.
# - Set PREMAKE5=C:\path\to\premake5.exe to use an existing binary instead.
# - Keep PremakeVersion in sync with GenerateProject.sh when upgrading.
#
# This file is intentionally ASCII-only: Windows PowerShell 5.1 reads BOM-less files as ANSI.

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$PremakeVersion = '5.0.0-beta7'
$PremakeSha256  = '8baec7b265fd43050f006ef47000457fa93b6fafbb1ead7e333e862a467c2e95'

$repoRoot = Split-Path -Parent $PSScriptRoot

function Get-Premake
{
    if ($env:PREMAKE5)
    {
        return $env:PREMAKE5
    }

    $dir = Join-Path $repoRoot "bin\tools\premake-$PremakeVersion"
    $exe = Join-Path $dir 'premake5.exe'
    if (Test-Path -LiteralPath $exe)
    {
        return $exe
    }

    $url = "https://github.com/premake/premake-core/releases/download/v$PremakeVersion/premake-$PremakeVersion-windows.zip"
    $tmp = Join-Path ([IO.Path]::GetTempPath()) ([Guid]::NewGuid().ToString())
    New-Item -ItemType Directory -Path $tmp | Out-Null

    try
    {
        $zip = Join-Path $tmp 'premake.zip'
        Write-Host "Downloading premake $PremakeVersion (windows)..."
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing

        $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $zip).Hash.ToLowerInvariant()
        if ($actual -ne $PremakeSha256)
        {
            throw "premake checksum mismatch (expected $PremakeSha256, got $actual)"
        }

        Expand-Archive -LiteralPath $zip -DestinationPath $tmp -Force
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Move-Item -LiteralPath (Join-Path $tmp 'premake5.exe') -Destination $exe -Force
    }
    finally
    {
        Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }

    return $exe
}

$premake = Get-Premake
Set-Location -LiteralPath $repoRoot
& $premake @args
exit $LASTEXITCODE
