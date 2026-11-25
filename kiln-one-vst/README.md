# Kiln One

One-knob mastering VST3. The big LOUD knob moves threshold, ratio and makeup gain together.
Includes Thick / Clean / Tape colour keys, Multi, Smooth, Calm (x2), Boost / True Peak / Unity output modes,
mid/side Wide, three-band tone, delay-matched Bypass, and 17 genre and artist-type presets (host program list).

## Build (needs CMake 3.22+ and a C++17 compiler)

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target KilnOne_VST3

Output: `build/KilnOne_artefacts/Release/VST3/Kiln One.vst3`

## Install for Cubase

- Windows: copy `Kiln One.vst3` to `C:\Program Files\Common Files\VST3\` (run the copy as administrator).
- macOS: copy it to `/Library/Audio/Plug-Ins/VST3/` (or `~/Library/Audio/Plug-Ins/VST3/`).

Then in Cubase: Studio > VST Plug-in Manager > Update, and insert Kiln One in the Master bus Inserts.
Turn Bypass off before you bounce. Latency is reported to the host.

## Notes
- True Peak mode uses a -1.0 dB ceiling with a hard clip. It is not an oversampled true-peak limiter.
- The browser demo's Monitor section (Cube, Phone, Radio) is not in the plugin on purpose, so it can't end up in a bounce.

## Browser version

`web/kiln-one.html` is the standalone browser demo of the same plugin. Open it in any modern browser.
