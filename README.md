# Griddy

A topographic drum sequencer MIDI effect inspired by Mutable Instruments Grids, built in C++ with JUCE and a GPU-accelerated [Visage](https://github.com/danielraffel/visage) fork.<br>
[💾 Download macOS Installer (PKG)](https://github.com/danielraffel/Griddy-MIDI-Effect-Plugin/releases/download/1.0.65/Griddy_1.0.65.pkg) • [All Releases](https://github.com/danielraffel/Griddy-MIDI-Effect-Plugin/releases)

<img width="630" height="503" alt="image" src="https://github.com/user-attachments/assets/889a0d06-8cc9-4ff8-9acf-cd80bfef791d" />


## Overview

Griddy generates evolving drum patterns by interpolating across a 5x5 map of rhythm nodes. Drag the XY pad to move through the terrain, shape each voice with density and velocity controls, and animate the whole pattern with LFO routing, pattern resets, and transport-aware timing.

Brief demo video:

[![Watch the video](https://img.youtube.com/vi/6K_gBFbkRlU/0.jpg)](https://youtu.be/6K_gBFbkRlU)

### Features

- **Pattern Morphing**: Explore 25 rhythm regions from a single XY pad
- **Three Drum Voices**: Bass drum, snare drum, and hi-hat channels
- **Density and Velocity Control**: Shape probability and intensity per voice
- **Chaos and Swing**: Add controlled randomness and timing feel
- **Modulation Matrix**: Route built-in LFOs to XY, densities, swing, chaos, velocities, reset, and note mappings
- **Pattern Utilities**: Reset modes, Euclidean mode, and pattern chaining
- **Plugin Formats**: AU, VST3, and standalone macOS app
- **iOS App**: Shared Visage-based UI with native multi-touch support from the forked renderer

## Requirements

- macOS 15 or later
- Apple Silicon Mac for the current release builds
- A DAW that supports AU or VST3 plugins
- Xcode 15+ and CMake 3.24+ for local builds

## Building from Source

### Quick Build

1. Clone the repository:

```bash
git clone https://github.com/danielraffel/Griddy-MIDI-Effect-Plugin.git
cd Griddy-MIDI-Effect-Plugin
```

2. Create your local config:

```bash
cp .env.example .env
```

3. Generate the Xcode project:

```bash
./scripts/generate_and_open_xcode.sh
```

JUCE is downloaded automatically on first build. Visage is already vendored in the repo.

### Build Options

- **Standalone app**: `./scripts/build.sh standalone`
- **AU + VST3 + Standalone local build**: `./scripts/build.sh all local`
- **Signed/notarized package**: `./scripts/build.sh all pkg`
- **Publish release**: `./scripts/build.sh all publish`
- **iOS project generation**: `cmake -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS`

### Installation

After local builds, the macOS targets land in:

- **AU**: `~/Library/Audio/Plug-Ins/Components/Griddy.component`
- **VST3**: `~/Library/Audio/Plug-Ins/VST3/Griddy.vst3`
- **Standalone**: `build/Griddy_artefacts/Release/Standalone/Griddy.app`

## Usage

### Basic Operation

1. **Pattern Selection**: Drag the X/Y pad to move through the rhythm map
2. **Density**: Adjust BD, SD, and HH densities to change trigger probability
3. **Velocity**: Shape accent and output feel per voice
4. **Chaos / Swing**: Add instability or groove
5. **Settings / Modulation**: Open the settings panel for MIDI mapping, LFO routing, reset behavior, and live mode

### Default MIDI Mapping

- Bass Drum: MIDI Note C1 (36)
- Snare Drum: MIDI Note D1 (38)
- Hi-Hat: MIDI Note F#1 (42)

### Pattern Grid

The 5x5 grid blends between different pattern families:

- **Top-Left**: Sparse, minimal patterns
- **Top-Right**: Denser hat and percussion motion
- **Bottom-Left**: Heavier kick emphasis
- **Bottom-Right**: More balanced groove patterns
- **Center**: General-purpose mid-density rhythms

## License

This project is licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE) for the project license.

Griddy incorporates pattern data and algorithmic ideas from [Mutable Instruments Grids](https://github.com/pichenettes/eurorack/tree/master/grids), which is also GPL v3.0.

For third-party attributions and shipped notices, see [LICENSES.md](LICENSES.md). The plugin also ships an in-app acknowledgements page generated from the current license bundle.

## Acknowledgments

- Emilie Gillet / Mutable Instruments for the original Grids module and rhythm maps
- The JUCE team for the plugin/application framework
- Matt Tytel for Visage, with this project using a fork that adds the iOS touch support needed by the shared UI

## Support

For issues, questions, or suggestions, open an issue on GitHub or contact [thegenerouscorp@gmail.com](mailto:thegenerouscorp@gmail.com).

## Build Status

- macOS AU/VST3/Standalone: ✅ Supported
- iOS app target: ✅ Supported
- Windows/Linux: not currently shipped

---

Made by The Generous Corp
