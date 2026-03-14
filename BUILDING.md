# building ksynth-desktop

## requirements

| Tool | Minimum version |
|------|----------------|
| CMake | 3.16 |
| C compiler | C11 (gcc 9+, clang 11+, MSVC 2019+) |
| C++ compiler | C++17 (same versions) |
| git | any recent version |

FLTK and miniaudio are vendored. No other libraries need to be installed except for the platform audio and display dependencies listed below.

---

## linux (fedora / rhel)

Install display and audio development headers:

```sh
sudo dnf install cmake gcc-c++ \
    libX11-devel libXext-devel libXft-devel \
    libXinerama-devel libXcursor-devel libXrender-devel \
    freetype-devel fontconfig-devel
```

Build:

```sh
git clone --recurse-submodules https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Binaries: `build/ksynth-desktop` and `build/ksplay`.

Audio backend is selected at runtime via `miniaudio`'s auto-detection. On a modern Fedora system it will use PipeWire (via the PulseAudio compatibility layer) without any extra configuration. If audio fails to initialise, check that PipeWire or PulseAudio is running.

---

## linux (debian / ubuntu)

```sh
sudo apt install cmake g++ \
    libx11-dev libxext-dev libxft-dev \
    libxinerama-dev libxcursor-dev libxrender-dev \
    libfreetype-dev libfontconfig-dev
```

Then the same `cmake` / `make` steps as above.

---

## macos

Install Xcode Command Line Tools if you haven't already:

```sh
xcode-select --install
```

Install CMake — either from [cmake.org](https://cmake.org/download/) or via Homebrew:

```sh
brew install cmake
```

Build:

```sh
git clone --recurse-submodules https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.logicalcpu)
```

Audio uses CoreAudio automatically. No extra packages needed.

---

## windows

Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload, and [CMake](https://cmake.org/download/).

In a **Developer Command Prompt**:

```cmd
git clone --recurse-submodules https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Binaries: `build\Release\ksynth-desktop.exe` and `build\Release\ksplay.exe`.

Audio uses the Windows Multimedia API (WinMM) automatically.

---

## build targets

| Target | Description |
|--------|-------------|
| `ksynth-desktop` | The full GUI application |
| `ksplay` | Headless CLI — runs `.ks` files and plays audio |

### ksplay usage

```sh
./ksplay patch.ks
./ksplay kick.ks snare.ks hihat.ks   # loads into slots 0, 1, 2 and plays each
```

---

## vendored submodules

The `vendor/` directory contains:

| Path | What it is |
|------|-----------|
| `vendor/fltk/` | FLTK 1.4 GUI toolkit (git submodule) |
| `vendor/miniaudio.h` | miniaudio single-header audio library |
| `vendor/ksynth.c` | ksynth interpreter |
| `vendor/ksynth.h` | ksynth API header |

If you cloned without `--recurse-submodules`, initialise the FLTK submodule:

```sh
git submodule update --init --recursive
```

---

## cmake options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | (none) | Set to `Release` for an optimised build |
| `FLTK_BUILD_TEST` | OFF | Disabled automatically |
| `FLTK_BUILD_FLUID` | OFF | Disabled automatically |

For a release build:

```sh
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## troubleshooting

**No audio on Linux** — check that your audio server is running:
```sh
systemctl --user status pipewire
# or
pulseaudio --check
```

**FLTK configure errors on Linux** — ensure X11 development headers are installed. The error message will name the missing header.

**`fatal error: FL/Fl.H: No such file or directory`** — FLTK submodule was not initialised. Run `git submodule update --init --recursive`.

**Wayland** — FLTK 1.4 runs on Wayland via XWayland automatically on most distributions. If you see rendering issues, set `GDK_BACKEND=x11` before launching.
