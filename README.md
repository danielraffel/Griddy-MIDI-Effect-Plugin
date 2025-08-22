# Griddy

A topographic drum sequencer MIDI Effect plugin (VST3/AU) inspired by Mutable Instruments Grids, built in C++ with JUCE.<br>
[Signed macOS Apple Silicon Installer](https://github.com/danielraffel/Griddy-MIDI-Effect-Plugin/releases/tag/1.0.1)

<img width="630" height="573" alt="image" src="https://github.com/user-attachments/assets/dc2e8f45-2b1b-4c44-9771-f8c14caad771" />

## Overview

Griddy is an algorithmic drum sequencer that generates evolving drum patterns using machine learning-derived rhythm maps. Navigate through a 5x5 grid of patterns using X/Y controls to morph between different rhythmic styles in real-time. Brief Demo Video:

[![Watch the video](https://img.youtube.com/vi/6K_gBFbkRlU/0.jpg)](https://youtu.be/6K_gBFbkRlU)

### Features

- **Pattern Morphing**: Smoothly interpolate between 25 different drum patterns using X/Y pad control
- **Three Drum Voices**: Bass drum, snare drum, and hi-hat channels
- **Density Controls**: Adjust the density/probability for each drum voice
- **Chaos Control**: Add controlled randomness to patterns
- **Swing Control**: Apply swing timing to patterns
- **MIDI Output**: Sends MIDI notes for integration with DAWs and drum machines
- **LED Matrix Display**: Visual feedback showing current pattern activity
- **Modulation**: Built in LFOs to modulate all settings 

## Requirements

- macOS 10.13 or later (Intel or Apple Silicon)
- A DAW that supports AU or VST3 plugins
- Xcode 14+ (for building from source)
- CMake 3.24 or later
- JUCE Framework (included as submodule)

## Building from Source

### Quick Build

1. Clone the repository:
```bash
git clone https://github.com/danielraffel/Griddy-MIDI-Effect-Plugin.git
cd Griddy-MIDI-Effect-Plugin
git submodule update --init --recursive
```

2. Build the project:
```bash
./generate_and_open_xcode.sh
```

This will configure CMake, generate the Xcode project, and open it automatically.

3. In Xcode, select your target (Standalone, AU, or VST3) and build

### Build Options

- **Debug build** (default): `./generate_and_open_xcode.sh`
- **Release build**: `./generate_and_open_xcode.sh release`
- **Clean build**: `rm -rf build/ && ./generate_and_open_xcode.sh`

### Installation

After building, the plugins will be located in:
- **AU**: `~/Library/Audio/Plug-Ins/Components/Griddy.component`
- **VST3**: `~/Library/Audio/Plug-Ins/VST3/Griddy.vst3`
  
## Usage

### Basic Operation

1. **Pattern Selection**: Click and drag on the X/Y pad to navigate through the pattern space
2. **Density Controls**: Adjust the three knobs to control the density of each drum voice
3. **Chaos**: Add randomness to make patterns less predictable
4. **Swing**: Apply swing timing for a more human feel

### MIDI Mapping

- Bass Drum: MIDI Note C1 (36)
- Snare Drum: MIDI Note D1 (38)
- Hi-Hat: MIDI Note F#1 (42)

### Pattern Grid

The 5x5 grid represents different pattern styles:
- **Top-Left**: Minimal, sparse patterns
- **Top-Right**: Complex hi-hat patterns
- **Bottom-Left**: Heavy kick patterns
- **Bottom-Right**: Balanced, groove-oriented patterns
- **Center**: Versatile, mid-density patterns

## License

This project is licensed under the **GNU General Public License v3.0** - see the [LICENSE](LICENSE) file for details.

### Important License Information

Griddy incorporates drum patterns and algorithms from [Mutable Instruments Grids](https://github.com/pichenettes/eurorack/tree/master/grids), which is licensed under GPL v3.0.

For detailed third-party attributions and licenses, please see <a href="LICENSES.md" target="_blank">LICENSES.md</a>.

## Contributing

Contributions are welcome! Please feel free to submit pull requests.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Acknowledgments
Details in 
- Emilie Gillet (Mutable Instruments) for the original Grids module and algorithm
- The JUCE team for the excellent cross-platform audio framework
- [Matt Tytel](https://tytel.org) for the Visage UI framework (which I am working on integrating correctly)

## Support

For issues, questions, or suggestions, please open an issue on GitHub or contact [mailto:thegenerouscorp@gmail.com](thegenerouscorp@gmail.com)

## Build Status

- macOS (Apple Silicon):  Supported

---

Made by The Generous Corp
