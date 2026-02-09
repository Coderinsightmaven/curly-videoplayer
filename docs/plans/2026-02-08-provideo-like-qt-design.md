# ProVideo-Like Qt v1 Design

Date: 2026-02-08

## Product Goal

Build a cross-platform desktop video playback operator tool for macOS and Windows with a ProVideoPlayer-like workflow focused on:

1. Cue list trigger workflow.
2. Multi-screen output routing.
3. Layered playback on each output.

## Scope Guardrails (v1)

### In Scope

- Manual cue loading from files.
- Per-cue metadata: name, media path, target screen, target layer, loop.
- Full-screen output window per screen.
- Layer surfaces where each layer has an independent player instance.
- Playback commands: play selected cue, stop selected layer, stop all.

### Out of Scope

- Show control protocols (MIDI/OSC/LTC).
- Timeline editing and cue transitions.
- Advanced effects/compositing.
- Project save/load.
- Network sync and collaboration.

## Architecture

- `MainWindow`: operator UI.
- `CueListModel`: cue data table model.
- `DisplayManager`: screen discovery and refresh.
- `PlaybackController`: orchestration from selected cue to routed output.
- `OutputRouter`: manages one `OutputWindow` per display.
- `OutputWindow`: full-screen output shell.
- `LayerSurface`: stacked layer host, one player per active layer.
- `IPlayer`: backend abstraction interface.
- `MpvPlayer`: `libmpv` implementation.

## Data Flow

1. Operator adds cue in UI.
2. Cue is inserted into `CueListModel`.
3. Operator selects cue and triggers play.
4. `PlaybackController` validates and forwards cue to `OutputRouter`.
5. `OutputRouter` ensures output window exists for target display.
6. `LayerSurface` ensures player exists for target layer.
7. `MpvPlayer` loads cue media and starts playback.

## Error Handling

- Invalid row/cue selection is blocked by controller checks.
- Routing failures and backend playback errors emit UI status messages.
- Missing displays gracefully fall back to primary screen when possible.

## Test Strategy (next phase)

- Unit tests for `CueListModel` and controller validation logic.
- Integration test with fake player backend for routing/layer behavior.
- Manual matrix:
  - macOS + Windows.
  - single and dual display setups.
  - mixed codecs and alpha media samples.

## Backend Decision

Chosen backend for v1: `libmpv`.

Reason: best path to broad codec reliability and alpha-capable playback without building a custom media pipeline in v1.
