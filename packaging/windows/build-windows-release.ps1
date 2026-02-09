param(
  [string]$SourceDir = ".",
  [string]$BuildDir = "build-win",
  [string]$InstallDir = "build-win/install",
  [string]$Generator = "Ninja",
  [string]$QtPrefixPath = $env:QT_PREFIX_PATH,
  [string]$MpvRuntimeDll = "",
  [string]$CmakeToolchainFile = "",
  [string]$PackageGenerators = "NSIS;ZIP",
  [switch]$EnableSigning,
  [string]$SignToolPath = $env:SIGNTOOL_PATH,
  [string]$CertThumbprint = $env:SIGN_CERT_THUMBPRINT,
  [string]$TimestampUrl = "http://timestamp.digicert.com",
  [ValidateSet("sha1", "sha256", "sha384", "sha512")]
  [string]$FileDigest = "sha256",
  [ValidateSet("sha1", "sha256", "sha384", "sha512")]
  [string]$TimestampDigest = "sha256"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path -Path $SourceDir
$buildPath = Join-Path $repoRoot $BuildDir
$installPath = Join-Path $repoRoot $InstallDir

New-Item -Path $buildPath -ItemType Directory -Force | Out-Null
New-Item -Path $installPath -ItemType Directory -Force | Out-Null

$cmakeConfigureArgs = @(
  "-S", $repoRoot,
  "-B", $buildPath,
  "-G", $Generator,
  "-DCMAKE_BUILD_TYPE=Release",
  "-DCMAKE_INSTALL_PREFIX=$installPath"
)

if ($QtPrefixPath) {
  $cmakeConfigureArgs += "-DCMAKE_PREFIX_PATH=$QtPrefixPath"
}

if ($MpvRuntimeDll) {
  $cmakeConfigureArgs += "-DMPV_RUNTIME_DLL=$MpvRuntimeDll"
}

if ($CmakeToolchainFile) {
  $cmakeConfigureArgs += "-DCMAKE_TOOLCHAIN_FILE=$CmakeToolchainFile"
}

if ($EnableSigning) {
  if (-not $CertThumbprint) {
    throw "EnableSigning was set but no certificate thumbprint was provided. Use -CertThumbprint or SIGN_CERT_THUMBPRINT."
  }

  if (-not $SignToolPath) {
    $signtoolCommand = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($signtoolCommand) {
      $SignToolPath = $signtoolCommand.Source
    }
  }

  if (-not $SignToolPath) {
    throw "EnableSigning was set but signtool.exe was not found. Use -SignToolPath or SIGNTOOL_PATH."
  }

  $cmakeConfigureArgs += "-DVPFM_WINDOWS_SIGNING=ON"
  $cmakeConfigureArgs += "-DVPFM_SIGNTOOL_PATH=$SignToolPath"
  $cmakeConfigureArgs += "-DVPFM_SIGN_CERT_THUMBPRINT=$CertThumbprint"
  $cmakeConfigureArgs += "-DVPFM_SIGN_TIMESTAMP_URL=$TimestampUrl"
  $cmakeConfigureArgs += "-DVPFM_SIGN_FILE_DIGEST=$FileDigest"
  $cmakeConfigureArgs += "-DVPFM_SIGN_TIMESTAMP_DIGEST=$TimestampDigest"
}

Write-Host "==> Configure"
& cmake @cmakeConfigureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

Write-Host "==> Build"
& cmake --build $buildPath --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }

Write-Host "==> Install"
& cmake --install $buildPath --config Release --prefix $installPath
if ($LASTEXITCODE -ne 0) { throw "CMake install failed." }

$installedExe = Join-Path $installPath "bin/VideoPlayerForMe.exe"
if (Test-Path $installedExe) {
  $windeployqt = $null
  if ($QtPrefixPath) {
    $candidate = Join-Path $QtPrefixPath "bin/windeployqt.exe"
    if (Test-Path $candidate) {
      $windeployqt = $candidate
    }
  }

  if (-not $windeployqt) {
    $cmd = Get-Command windeployqt.exe -ErrorAction SilentlyContinue
    if ($cmd) { $windeployqt = $cmd.Source }
  }

  if ($windeployqt) {
    Write-Host "==> Running windeployqt"
    & $windeployqt --release --no-translations --dir (Join-Path $installPath "bin") $installedExe
    if ($LASTEXITCODE -ne 0) {
      throw "windeployqt failed."
    }
  } else {
    Write-Warning "windeployqt.exe not found. Qt runtime deployment depends on CMake deploy script only."
  }
}

if ($EnableSigning) {
  Write-Host "==> Signing installed binaries"
  $signScript = Join-Path $repoRoot "packaging/windows/sign-windows-artifacts.ps1"
  $binaryFiles = @(Get-ChildItem -Path (Join-Path $installPath "bin") -File -Recurse |
    Where-Object { $_.Extension -in ".exe", ".dll" } |
    Select-Object -ExpandProperty FullName)

  if ($binaryFiles.Count -gt 0) {
    & $signScript `
      -Files $binaryFiles `
      -SignToolPath $SignToolPath `
      -CertThumbprint $CertThumbprint `
      -TimestampUrl $TimestampUrl `
      -FileDigest $FileDigest `
      -TimestampDigest $TimestampDigest

    if ($LASTEXITCODE -ne 0) {
      throw "Signing installed binaries failed."
    }
  }
}

Write-Host "==> Package"
Push-Location $buildPath
try {
  & cpack -C Release -G $PackageGenerators
  if ($LASTEXITCODE -ne 0) {
    throw "CPack failed."
  }
} finally {
  Pop-Location
}

if ($EnableSigning) {
  Write-Host "==> Signing generated installers"
  $signScript = Join-Path $repoRoot "packaging/windows/sign-windows-artifacts.ps1"
  $packageFiles = @(Get-ChildItem -Path $buildPath -File |
    Where-Object { $_.Extension -in ".exe", ".msi" } |
    Select-Object -ExpandProperty FullName)

  if ($packageFiles.Count -gt 0) {
    & $signScript `
      -Files $packageFiles `
      -SignToolPath $SignToolPath `
      -CertThumbprint $CertThumbprint `
      -TimestampUrl $TimestampUrl `
      -FileDigest $FileDigest `
      -TimestampDigest $TimestampDigest

    if ($LASTEXITCODE -ne 0) {
      throw "Signing generated packages failed."
    }
  }
}

Write-Host "Done. Build directory: $buildPath"
Write-Host "Install directory: $installPath"
