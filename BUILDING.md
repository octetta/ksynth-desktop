# building ksynth-desktop

## requirements

| Tool | Minimum version |
|------|----------------|
| CMake | 3.16 |
| C compiler | C11 (gcc 9+, clang 11+, MSVC 2019+) |
| C++ compiler | C++17 (same versions) |
| git | any recent version |

FLTK, miniaudio, and ksynth are all vendored in the repository. No other libraries need to be installed except for the platform audio and display dependencies listed below.

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
git clone https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Binaries: `build/ksynth-desktop` and `build/ksplay`.

Audio backend is selected at runtime via miniaudio's auto-detection. On a modern Fedora system it will use PipeWire (via the PulseAudio compatibility layer) without any extra configuration. If audio fails to initialise, check that PipeWire or PulseAudio is running.

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
git clone https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.logicalcpu)
```

Audio uses CoreAudio automatically. No extra packages needed.

---

## windows

### with visual studio

Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload, and [CMake](https://cmake.org/download/).

In a **Developer Command Prompt**:

```cmd
git clone https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Binaries: `build\Release\ksynth-desktop.exe` and `build\Release\ksplay.exe`.

### with zig cc

[Zig](https://ziglang.org/download/) bundles a clang-compatible C/C++ compiler that works well as a drop-in. This approach requires neither Visual Studio nor MinGW — just Zig and Ninja.

Install Zig (from [ziglang.org](https://ziglang.org/download/) or `winget install zig.zig`) and Ninja:

```cmd
winget install Ninja-build.Ninja
```

Then:

```cmd
git clone https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_C_COMPILER="zig cc" -DCMAKE_CXX_COMPILER="zig c++"
cmake --build .
```

Binaries: `build\ksynth-desktop.exe` and `build\ksplay.exe`.

Ninja is a single small executable with no installer side-effects. If `zig cc` is not on your PATH as a bare command, use the full path: `-DCMAKE_C_COMPILER="C:\path\to\zig.exe cc"` — note the space between the executable and `cc` is intentional, that is how zig exposes its C compiler frontend.

Audio uses the Windows Multimedia API (WinMM) automatically on all three Windows build paths.

### with mingw-w64 (msys2)

[MSYS2](https://www.msys2.org/) provides a MinGW-w64 toolchain with a Unix-like shell, which is the most straightforward way to get `gcc` and `make` on Windows.

Install MSYS2, then open the **MSYS2 MinGW64** shell and install the required packages:

```sh
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-ninja \
          git
```

Build:

```sh
git clone https://github.com/octetta/ksynth-desktop
cd ksynth-desktop
mkdir build && cd build
cmake .. -G Ninja
ninja
```

Binaries: `build/ksynth-desktop.exe` and `build/ksplay.exe`.

A few things to be aware of with MinGW:

- **Runtime DLLs** — the binaries link against `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, and `libwinpthread-1.dll` from the MinGW runtime. These are in your MSYS2 `mingw64/bin` directory. Either run the app from the MSYS2 shell (where they are on `PATH`), or copy them alongside the executable for distribution.
- **Console window** — MinGW builds will open a console window behind the GUI by default. To suppress it, add `-mwindows` to the link flags in CMakeLists.txt, or use the Ninja/VS generator with a `WIN32` executable target.
- **FLTK and WinSock** — FLTK pulls in `ws2_32` on Windows. CMake handles this automatically; no manual flag is needed.

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

## vendored libraries

The `vendor/` directory contains everything needed to build:

| Path | What it is |
|------|-----------|
| `vendor/fltk/` | FLTK 1.4 GUI toolkit |
| `vendor/miniaudio.h` | miniaudio single-header audio library |
| `vendor/ksynth.c` | ksynth interpreter |
| `vendor/ksynth.h` | ksynth API header |

`ksynth.c` and `ksynth.h` are checked in directly. If you need to update ksynth to a newer version from [octetta/k-synth](https://github.com/octetta/k-synth), replace those two files manually.

> **Note:** FLTK is included as a git submodule. A plain `git clone` is sufficient since the submodule is already initialised in the repository. If for any reason FLTK is missing from `vendor/fltk/`, run:
> ```sh
> git submodule update --init --recursive
> ```

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

**`fatal error: FL/Fl.H: No such file or directory`** — FLTK submodule content is missing. Run `git submodule update --init --recursive`.

**Wayland** — FLTK 1.4 runs on Wayland via XWayland automatically on most distributions. If you see rendering issues, set `GDK_BACKEND=x11` before launching.
