# Metronome (JUCE)

A native C++ version of the [Metronome web app](https://github.com/arvanito/metronome), built with
[JUCE](https://juce.com). It has the same features, the same sounds and the same modular layout.
Windows comes first; Android is next.

## Features

- 20–300 BPM: click the number to type a tempo, −/+ (hold to repeat), slider, arrow keys (Shift = ±5)
- Italian tempo names, tap tempo (button or `T`), `Space` to start and stop
- Time signatures 1–16 beats per bar (over 2/4/8/16); click a beat light to set accent, normal or mute
- Subdivisions: 8ths, triplets, 16ths
- Nine synthesised sounds (ported from the web app), volume up to 150% with a soft limiter
- Speed trainer, presets, audio device settings (WASAPI / ASIO)
- Remembers everything between runs (`%APPDATA%\Metronome\Metronome.settings`)

## Timing

The web app has to schedule clicks ahead of time from a JavaScript timer. Here the
[`Scheduler`](Source/engine/Scheduler.h) runs **inside the audio callback**: each block, it works out
which ticks fall in that block and starts each click on its exact sample. Tick times are kept as
fractional sample positions, so there is no drift. The tests check that playback stays within one
sample over ten minutes. The audio thread never locks or allocates. The UI talks to it through atomics,
and beat lights are fed by a lock-free FIFO, adjusted for output latency.

## Building (Windows)

Requirements: Visual Studio 2022 or newer with the "Desktop development with C++" workload. CMake and
Ninja come with Visual Studio. JUCE is downloaded automatically on the first build.

```bat
build.cmd            :: debug build + tests
build.cmd release    :: optimised build
```

The app is at `%LOCALAPPDATA%\metronome-juce\build\Metronome_artefacts\<Debug|Release>\Metronome.exe`.
Build output is kept outside the source folder (see [CMakePresets.json](CMakePresets.json)), so that
OneDrive doesn't sync gigabytes of build files.

**VS Code:** install the recommended extensions (CMake Tools, C/C++). Open the folder, pick the
`windows` preset, then build, run or debug from the status bar.

Developer aid: `Metronome.exe --screenshot out.png [--expanded]` renders the UI to an image and quits.

## Project structure

```
Source/
  core/       Pure logic: rhythm, tempo, tap tempo, speed trainer, volume, settings (+ JSON)
  engine/     Scheduler, synthesised sounds, MetronomeEngine (the audio callback)
  ui/         AppState, Theme, Feature base class, MainComponent (window, timer, shortcuts)
  features/   One file per feature; the list in Features.cpp decides what appears
Tests/        Unit tests (core, scheduler, sounds, engine with a fake audio device)
```

### Adding a feature

1. Create `Source/features/MyFeature.cpp` with a class deriving from `Feature`. Implement
   `preferredHeight()`, and optionally `stateChanged()`, `tickHeard()`, `frame()` or `shortcut()`.
   Read and write settings through `ctx.state`.
2. Declare `createMyFeature()` in `Features.h`, add it to the list in `Features.cpp`, and add the
   file to `CMakeLists.txt`.

## Licence note

JUCE is dual-licensed: AGPLv3, or a commercial/Starter licence (free below a revenue limit). Building
the app for your own use is fine either way. Distributing binaries requires following one of those
licences.
