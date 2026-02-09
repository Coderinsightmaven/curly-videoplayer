# VideoPlayerForMe

Cross-platform Qt/C++ show-control video app inspired by ProVideoPlayer workflows.

## Implemented Feature Set

- Cue list with metadata:
  - target screen
  - target set (`Selected Screen`, per-screen set, or `All Screens`)
  - layer
  - loop
  - preload flag
  - hotkey
  - timecode trigger
  - MIDI note mapping
  - DMX channel/value trigger mapping
  - cue-level transition override
  - transition styles (`Cut`, `Fade`, `Dip To Black`, `Wipe Left`, `Dip To White`)
  - cue filter chain (`mpv vf`)
  - cue filter preset binding
  - live input source URL
  - auto-follow action (follow row + delay)
  - playlist actions (playlist id, auto-advance, loop, delay)
  - auto-stop timer
- Program output engine:
  - multi-screen full-screen outputs
  - per-screen layered playback
  - transitions (`Cut`, `Fade`, `Dip To Black`)
  - per-screen edge blend + keystone controls
  - per-screen output mask controls (left/top/right/bottom)
  - fallback slate on errors/stopped state
  - operator text overlay (`OSC /text`)
- Preview/program workflow:
  - preview window
  - `Take` to program with selected transition
- Project persistence:
  - save/load `.show` (JSON structure)
  - cues, transition settings, control config, calibrations
  - relative media path mode for portable projects
- Control inputs:
  - OSC UDP server
  - Art-Net DMX input listener (OpDmx/universe routing)
  - MIDI input (optional RtMidi build)
  - timecode trigger routing (from OSC `/timecode` or MIDI MTC quarter-frame)
  - DMX-style trigger input via OSC `/dmx <channel> <value>`
- Backup trigger:
  - optional HTTP POST when a cue goes live
  - optional UDP failover sync (cue-live/stop-all/overlay replication with shared-key auth)
- Utility workflow:
  - add color-bars test-pattern cues
  - relink missing media files in loaded projects
- Optional NDI hook:
  - compile-time abstraction with SDK detection
- Optional Syphon/SDI hooks:
  - compile-time bridge toggles with SDK/header detection
- Packaging/deployment baseline:
  - CPack config for macOS DMG and Windows ZIP/NSIS

## Dependencies

Required:
- CMake `>= 3.21`
- Qt 6 (`Core`, `Gui`, `Widgets`, `Network`)
- `libmpv`

Optional:
- RtMidi (`HAVE_RTMIDI`) for direct MIDI input
- NewTek NDI SDK (`HAVE_NDI_SDK`) for NDI initialization path

### macOS (Homebrew)

```bash
brew install cmake qt mpv pkg-config
# optional
brew install rtmidi
```

### Windows (suggested)

- Install Qt 6 via Qt Maintenance Tool.
- Install mpv dev package (or use `vcpkg` + CMake toolchain).
- Optional: install/build RtMidi and NDI SDK.
- For signed releases, install Windows SDK `signtool.exe` and import your signing cert.

## Build

```bash
cmake -S . -B build-default -DCMAKE_BUILD_TYPE=Debug
cmake --build build-default -j
```

Preset-based builds:

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug

cmake --preset sanitizer-debug
cmake --build --preset sanitizer-debug
```

Run tests:

```bash
ctest --preset dev-debug
# or
ctest --preset sanitizer-debug
```

Current smoke test:
- `project_serializer_smoke` validates save/load roundtrip for cues, calibration, and app config.

## Repro Workflow

- Use the bug report form at `/Users/liammarincik/projects/liams-projects/videoplayerforme/.github/ISSUE_TEMPLATE/bug_repro.yml` when filing issues.
- Follow `/Users/liammarincik/projects/liams-projects/videoplayerforme/docs/repro-checklist.md` to gather reproducible steps, environment details, and validation command output.

## Run

macOS bundle executable:

```bash
./build-default/VideoPlayerForMe.app/Contents/MacOS/VideoPlayerForMe
```

## OSC Commands

Server listens on configured UDP port (default `9000`).

Supported addresses:
- `/cue/play <row>`
- `/cue/preview <row>`
- `/cue/preload <row>`
- `/cue/take`
- `/cue/stop_all`
- `/timecode <HH:MM:SS:FF>`
- `/dmx <channel> <value>`
- `/text <message>` (empty to clear)

Text-command fallback (non-binary OSC datagrams) is also accepted:
- `play 3`
- `preview 3`
- `preload 3`
- `take`
- `stopall`
- `timecode 01:00:00:00`
- `dmx 1 255`
- `text Live lyrics`

## Hotkeys

- `Space`: preview selected cue
- `Enter`: take preview live
- `G`: play selected cue live
- `Ctrl+L`: preload selected cue
- `S`: stop selected layer
- `Shift+S`: stop all
- `Up/Down`: selection
- `1..9`: preview cue row 0..8
- `Shift+1..9`: play cue row 0..8 live
- Cue-specific hotkeys can be set in cue properties.

## Package

```bash
cmake --build build-default --target package
```

Generated artifacts depend on OS and installed packaging tools.

## Windows Release Pipeline

Use the PowerShell automation in `/Users/liammarincik/projects/liams-projects/videoplayerforme/packaging/windows`.

Unsigned build/package example:

```powershell
pwsh -ExecutionPolicy Bypass -File packaging/windows/build-windows-release.ps1 `
  -SourceDir . `
  -BuildDir build-win `
  -InstallDir build-win/install `
  -Generator Ninja `
  -QtPrefixPath "C:/Qt/6.8.2/msvc2022_64" `
  -MpvRuntimeDll "C:/mpv/mpv-2.dll"
```

Signed build/package example:

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

See `/Users/liammarincik/projects/liams-projects/videoplayerforme/packaging/windows/README.md` for details.
