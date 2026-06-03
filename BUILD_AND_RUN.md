# Build & Run

## Requirements

- **CMake ≥ 3.27** + a generator (Make or Ninja).
- **clang / clang++** and **clang-tidy** — the presets pin clang, and clang-tidy runs on every build.
- A **C++20** standard library.
- **Tests** (`test_runner`): **[uv](https://docs.astral.sh/uv/)** — it provisions Python ≥ 3.14; no other dependencies.
- **Visualizer** (`sw_visualizer`, opt-in): extra system + build-time deps — see [Visualizer](#visualizer-sw_visualizer-opt-in). The CLI and tests build **without** them.

## Build — CLI (`sw_battle_test`)

Three presets (see `CMakePresets.json`); `debug`/`release` each produce `sw_battle_test` in `build/<preset>/`. Use the **workflow preset** to configure + build in one step:

```bash
# Release (no sanitizers)
cmake --workflow --preset release

# Debug (Address + UB sanitizers)
cmake --workflow --preset debug
```

Equivalently, the two phases by hand:

```bash
cmake --preset release && cmake --build build/release
```

- **configure** generates the build system. Re-run it after editing `CMakeLists.txt`, toggling options, or **adding/removing source files** (sources are globbed at configure time).
- **build** compiles/links. Re-run it for ordinary edits to existing files.

The CLI presets keep `BUILD_VISUALIZER=OFF`, so **they never fetch or build the visualizer's dependencies** — a plain CLI/tests build needs only CMake + clang.

## Run

```bash
# CLI: reads a command file, prints events to stdout
./build/release/sw_battle_test tests/01_example/commands.txt
```

## Visualizer (`sw_visualizer`, opt-in)

A raylib + Dear ImGui GUI that loads a command file and plays the simulation interactively. It is **off by default** (`BUILD_VISUALIZER=OFF`); the `vis` preset turns it on and builds into `build/vis/`.

### Extra dependencies

The visualizer pulls **raylib 5.5**, **Dear ImGui** (`docking`) and **rlImGui** via CMake `FetchContent`, so the first configure needs:

- **git** + **network access** — `FetchContent` clones the three repos.
- A **GPU/OpenGL 3.3** context and a display (X11 or Wayland; WSLg works) to actually open the window.
- **System dev packages** that raylib/GLFW link against. On Debian/Ubuntu:

  ```bash
  sudo apt install git libasound2-dev libgl1-mesa-dev libglu1-mesa-dev \
      libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
      libwayland-dev libxkbcommon-dev
  ```

The configure step prints which display backend it found (e.g. `Including X11 support`); if these packages are missing it fails there.

### Build & run

```bash
# Configure + build (fetches raylib/imgui/rlImGui on first run — minutes)
cmake --workflow --preset vis

# Run: load a command file, then drive it from the toolbar (play / step / reload)
./build/vis/sw_visualizer commands_test.txt
```

Assets (toolbar icons) are copied next to the binary at build time, so run it from anywhere.

## Tests (black-box suite)

```bash
uv run --project test_runner test_runner/main.py
```

- Auto-locates the binary (`build/debug` then `build/release`) and the `tests/` dir — build the CLI first.
- Override if needed: `--binary <path>` / `--tests <dir>`.
- Expected: `32 passed, 0 failed, 1 xfailed`.
