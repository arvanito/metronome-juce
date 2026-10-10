# Metronome (JUCE)

The native version of the [Metronome web app](https://github.com/arvanito/metronome). It shows the
**same web UI** in a native window, while the audio comes from a **C++/[JUCE](https://juce.com)
engine** with sample-accurate timing. Windows comes first; Android is next.

## How it fits together

```
┌──────────────── Metronome.exe ────────────────┐
│  WebView2 window                              │
│  └─ the web app's UI (web/ submodule),        │
│     embedded in the exe as a zip              │
│          ▲  "metronome" events  │             │
│          │  beat / bar / tempo  ▼ update /    │
│                                   start / stop│
│  MainComponent (bridge)                       │
│  └─ MetronomeEngine (audio callback)          │
│     └─ Scheduler + sounds → sound card        │
└───────────────────────────────────────────────┘
```

- **One UI codebase.** The UI is the web app, included as a git submodule in [`web/`](web). In the
  browser it uses its Web Audio engine. Inside this app it detects JUCE (`window.__JUCE__`) and uses
  [`NativeEngine`](web/src/engine/nativeEngine.ts) instead, which forwards everything to C++. Change
  the UI in the web repo, then update the submodule here.
- **Native audio.** The [`Scheduler`](Source/engine/Scheduler.h) runs inside the audio callback and
  starts every click on its exact sample, with no drift (tested to within one sample over ten minutes).
  The audio thread never locks or allocates. The speed trainer steps the tempo there too, exactly on
  the bar line.
- **The bridge** ([`MainComponent`](Source/ui/MainComponent.cpp)) serves the embedded UI files to the
  WebView and passes messages both ways: settings, start/stop and "audio settings" come from the page,
  while heard beats, bars and trainer tempo changes go back to it.

## Building (Windows)

Requirements:
- Visual Studio 2022 or newer, with the "Desktop development with C++" workload (provides CMake and Ninja)
- Node.js 20 or newer (builds the web UI)

JUCE and the WebView2 SDK are downloaded automatically.

```bat
git clone --recurse-submodules https://github.com/arvanito/metronome-juce.git
build.cmd            :: debug build + tests
build.cmd release    :: optimised build
```

The app is at `%LOCALAPPDATA%\metronome-juce\build\Metronome_artefacts\<Debug|Release>\Metronome.exe`.
Build output is kept outside the source folder (see [CMakePresets.json](CMakePresets.json)), so that
OneDrive doesn't sync gigabytes of build files.

Updating the UI to the latest web app:

```bat
git submodule update --remote web
```

**VS Code:** install the recommended extensions (CMake Tools, C/C++). Open the folder, pick the
`windows` preset, then build, run or debug from the status bar.

**Debugging the UI:** set `WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=9229`
before starting the app, then open `edge://inspect` in Edge to get DevTools for the app's page.

## Project structure

```
Source/
  core/     Pure logic: rhythm, tempo, tap tempo, speed trainer, volume, settings (+ JSON)
  engine/   Scheduler, synthesised sounds, MetronomeEngine (the audio callback)
  ui/       MainComponent: WebView host and bridge
web/        The web app (submodule): the UI
Tests/      Unit tests (core, scheduler, sounds, engine with a fake audio device)
```

The sounds exist in both repositories: in TypeScript for the browser, and in C++ here. The C++ port
in [`Sounds.cpp`](Source/engine/Sounds.cpp) follows `web/src/engine/sounds.ts` line by line, so keep
the two in step when changing a sound.

## Licence note

JUCE is dual-licensed: AGPLv3, or a commercial/Starter licence (free below a revenue limit). Building
the app for your own use is fine either way. Distributing binaries requires following one of those
licences.
