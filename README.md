
# BassSynth

**BassSynth** is a next-generation wavetable bass synthesizer plugin (VST3/AU) built with C++ and the JUCE framework. Designed for modern electronic music producers, it combines earth-shattering low-end with crystalline top-end textures, featuring an intuitive Ableton-style dark UI.

<img src="ScreenShots/Screenshot_1.png" width="600">

👉 **[Watch the Demo Video on X (動作デモ動画はこちら！)]([https://x.com/kijyoumusic/**](https://x.com/kijyoumusic/status/2047948478392021324?s=20)）

## ✨ Key Features

### 🌊 Advanced Wavetable Engine
- **10 Math-Generated Factory Morphing Tables**: 64-frame fluid morphing waves (Basic Morph, PWM Sweep, Sync Sweep, Vowel Sweep, etc.) calculated in real-time at zero-latency.
- **Custom Wavetable Import**: Drag & Drop or browse your own `.wav` files with high-quality bandlimited interpolation.
- **Smart Browser**: Instantly filter through Factory, User, and Favorite (★) wavetables. Includes a real-time search bar.
- **Spectral Morphing**: 13 morph modes (Bend, PWM, Sync, Mirror, Vocode, Comb, etc.) across 3 independent morphing slots (A, B, C).

### 🎨 Color IR Engine & Sparkle Arp
- **Auto-Chord Learning**: Play a MIDI chord, and the synth learns it to generate a custom Impulse Response (IR) matching the harmony.
- **Convolution Resonator**: Apply 5 unique IR types (Crystal Saw, Shimmer PWM, Harmonic Bell, Stacked Shimmer, Crystal Pluck) for metallic or ethereal top-end layers.
- **Sparkle Arp**: A dedicated, dynamic arpeggiator that runs through the learned chord structure to create high-frequency rhythmic textures.

### 🎛️ Dual Filter & Shaping
- **State-Variable Filters**: Two independent filters (LPF, HPF, BPF, Notch, Comb, Analog LPF) that can be routed in Serial or Parallel.
- **Distortion Unit**: Built-in Drive, Sine Shaper, Downsampler (Rate), and Bitcrusher for aggressive tone shaping.

### 🧬 Modulation Powerhouse
- **MSEG (Multi-Stage Envelope Generator)**: 2 custom visual envelope editors for complex, repeating rhythmic shapes.
- **LFOs & Mod Envelopes**: 3 LFOs and 3 Mod Envelopes with comprehensive sync and trigger options.
- **10-Slot Matrix**: Route any modulator to Pitch, FM, Morph A/B/C, Filter Cutoff/Reso, or Master Gain with pinpoint accuracy.

### 🎚️ Master Dynamics
- **True OTT**: Built-in 3-band upward/downward multiband compression.
- **Soothe-Style Resonance Suppression**: Instantly tame harsh frequencies (Selectivity, Sharpness, Focus controls).
- **Zero-Latency Peak Limiter**: Transparent, latency-free brickwall limiter with adjustable ceiling to keep your mix clipping-free.

---

## 🛠 Installation

### For Users (Download)
1. Download the latest release from the [Releases](https://github.com/yourusername/BassSynth/releases) page. 
2. Move the `BassSynth.vst3` (Windows)  to your DAW's plugin folder.
   - **Windows VST3**: `C:\Program Files\Common Files\VST3`
![Downloads](https://img.shields.io/github/downloads/OTODESK4193/OTODESK-Rhythm-Matrix/total.svg)


---

## 🎹 Quick Start Guide
1. **Browse Wavetables**: Click the `BROWSE` button top left. Use the search bar or click the `★` icon to favorite shapes.
2. **Learn a Chord (Color Mode)**: Click `COL` to open the Color panel. Hit `LEARN CHORD`, play a chord on your MIDI keyboard, and hear the harmonic resonance build up over your bass.
3. **Edit MSEGs**: Navigate to the `MSEGs` tab. Double-click to add/remove points, and Alt+Drag (or Option+Drag) to curve the lines.

---

## 📝 License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**. 

Since this plugin is built using the [JUCE framework](https://juce.com/) under its Personal/Open-Source tier, it must be distributed as open-source under the GPLv3 license. 
You are free to use, modify, and distribute this software, provided that any derivative works are also open-sourced under the same license. 

For commercial distribution without open-sourcing, a commercial JUCE license is required.

---

## 👨‍💻 Credits
- Developed by　OTODESK　https://x.com/kijyoumusic
- Built with [JUCE](https://juce.com/)
- UI Inspired by modern digital workstations.

