param(
  [Parameter(Mandatory = $true)]
  [string[]]$Files,

  [string]$SignToolPath = $env:SIGNTOOL_PATH,
  [Parameter(Mandatory = $true)]
  [string]$CertThumbprint,
  [string]$TimestampUrl = "http://timestamp.digicert.com",
  [ValidateSet("sha1", "sha256", "sha384", "sha512")]
  [string]$FileDigest = "sha256",
  [ValidateSet("sha1", "sha256", "sha384", "sha512")]
  [string]$TimestampDigest = "sha256",
  [string]$Description = "VideoPlayerForMe"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $SignToolPath) {
  $signtoolCommand = Get-Command signtool.exe -ErrorAction SilentlyContinue
  if ($signtoolCommand) {
    $SignToolPath = $signtoolCommand.Source
  }
}

if (-not $SignToolPath) {
  throw "signtool.exe not found. Pass -SignToolPath or set SIGNTOOL_PATH."
}

if (-not (Test-Path -Path $SignToolPath)) {
  throw "signtool.exe path does not exist: $SignToolPath"
}

if (-not $Files -or $Files.Count -eq 0) {
  throw "No files were provided to sign."
}

foreach ($rawPath in $Files) {
  if (-not $rawPath) {
    continue
  }

  $resolvedPath = Resolve-Path -Path $rawPath -ErrorAction SilentlyContinue
  if (-not $resolvedPath) {
    Write-Warning "Skipping missing file: $rawPath"
    continue
  }

  $path = $resolvedPath.Path
  $extension = [System.IO.Path]::GetExtension($path).ToLowerInvariant()

  if ($extension -eq ".zip") {
    Write-Host "Skipping unsigned archive format: $path"
    continue
  }

  Write-Host "Signing: $path"

  & $SignToolPath sign `
    /sha1 $CertThumbprint `
    /fd $FileDigest `
    /td $TimestampDigest `
    /tr $TimestampUrl `
    /d $Description `
    /v `
    $path

  if ($LASTEXITCODE -ne 0) {
    throw "signtool failed for $path (exit code $LASTEXITCODE)."
  }
}

Write-Host "Signing completed."
