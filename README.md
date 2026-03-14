# ksynth-desktop

A native desktop application for the [ksynth](https://github.com/octetta/k-synth) live-coding synthesis language. Write short vector-oriented programs, hear the result immediately, build kits and instruments, play them from a pad grid.

Sessions are compatible with [ksynth web](https://github.com/octetta/k-synth) — save from the browser, load on the desktop, or vice versa.

---

## quick start

```sh
git clone https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build && cd build
cmake ..
make -j$(nproc)
./ksynth-desktop
```

Type into the editor, press `Ctrl+J` to run. A cell appears in the notebook with a waveform preview. Click `→0` to bank to slot 0. Click the slot to play it.

See [BUILDING.md](BUILDING.md) for full platform instructions and [GUIDE.md](GUIDE.md) for a complete tutorial.

---

## what it is

ksynth-desktop is a two-pane live-coding environment:

- **left — notebook** — append-only log of every run, waveforms, bank buttons
- **right — editor** — write ksynth code, run it, inspect results in the REPL strip at the bottom

A **slot strip** across the top holds 16 audio buffers ready to trigger. A **pad window** turns them into a playable instrument. Sessions save and load as `.json` files that round-trip with the web app.

There is also a headless CLI tool (`ksplay`) for running `.ks` files directly from the command line.

---

## dependencies

- [FLTK 1.4](https://www.fltk.org/) — vendored as a git submodule
- [miniaudio](https://miniaud.io/) — single-header, vendored
- [ksynth](https://github.com/octetta/k-synth) — vendored (`ksynth.c` / `ksynth.h`)
- CMake 3.16+, a C11 compiler, a C++17 compiler

No other dependencies. No package manager required.

---

## license

MIT — see [LICENSE](LICENSE).
