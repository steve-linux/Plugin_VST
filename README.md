# Channel Strip VST

Channel strip audio plugin (VST3 / AU / CLAP) built with JUCE/C++, derived from an existing Reaper (JSFX) channel strip and extended.

## Goal

Port the existing Reaper channel strip to a cross-DAW plugin format and improve it both in sound/features and in UI.

### Current modules (from the Reaper version)
- **Input / HPF**: trim, high-pass filter with selectable slope
- **Preamp**: saturation models (e.g. "1073 style"), drive, color, output
- **Gate / Expander**: threshold, ratio, attack, release, range, in/out curve and GR meter
- **Equalizer**: 4 bands (LF, LM, HM, HF), pre/post compressor placement
- **Compressor**: VCA-style models, threshold, ratio, attack, release, knee, makeup, mix, sidechain HPF, auto release/makeup
- **Limiter / Output**: ceiling, release, output, LUFS target, loudness metering (momentary, short-term, integrated)
- **Analyzer**: spectrum + EQ response + waveform, oversampling up to 4x, factory presets

### Planned
- VST3, AU and CLAP builds (Windows, macOS, Linux)
- Analysis of graphical and functional improvements (to be defined)

## Status

Project setup. Development has not started yet.

## Build

To be defined (CMake + JUCE).
