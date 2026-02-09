# Windows Packaging + Signing

This folder contains a release pipeline for Windows installers and signed binaries.

## Scripts

- `build-windows-release.ps1`
  - configures, builds, installs, deploys runtime files, packages with CPack
  - optional signing for staged binaries and generated installer artifacts
- `sign-windows-artifacts.ps1`
  - signs one or more files with `signtool.exe`

## Prerequisites

- Visual Studio Build Tools or full Visual Studio (MSVC + Windows SDK)
- CMake
- Qt 6 (with `windeployqt.exe`)
- mpv runtime DLL (`mpv-2.dll`) reachable via CMake detection or `-MpvRuntimeDll`
- Optional for signing:
  - code-signing certificate in certificate store
  - certificate thumbprint
  - `signtool.exe`

## Example: Unsigned Package Build

```powershell
pwsh -ExecutionPolicy Bypass -File packaging/windows/build-windows-release.ps1 `
  -SourceDir . `
  -BuildDir build-win `
  -InstallDir build-win/install `
  -Generator Ninja `
  -QtPrefixPath "C:/Qt/6.8.2/msvc2022_64"
```

## Example: Signed Release Build

```powershell
$env:SIGNTOOL_PATH = "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/signtool.exe"
$env:SIGN_CERT_THUMBPRINT = "<YOUR_CERT_THUMBPRINT>"

pwsh -ExecutionPolicy Bypass -File packaging/windows/build-windows-release.ps1 `
  -SourceDir . `
  -BuildDir build-win `
  -InstallDir build-win/install `
  -Generator Ninja `
  -QtPrefixPath "C:/Qt/6.8.2/msvc2022_64" `
  -MpvRuntimeDll "C:/mpv/mpv-2.dll" `
  -EnableSigning
```

## Output Artifacts

Generated files are written under the selected build directory (`build-win` by default), for example:

- `VideoPlayerForMe-<version>-win64.exe` (NSIS installer)
- `VideoPlayerForMe-<version>-win64.zip` (portable package)
