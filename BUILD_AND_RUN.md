# Build & Run

## Requirements

- **CMake ≥ 3.27** + a generator (Make or Ninja).
- **clang / clang++** and **clang-tidy** — the presets pin clang, and clang-tidy runs on every build.
- A **C++20** standard library.
- **Tests** (`test_runner`): **[uv](https://docs.astral.sh/uv/)** — it provisions Python ≥ 3.14; no other dependencies.

## Build — CLI (`sw_battle_test`)

Two presets (see `CMakePresets.json`); each produces `sw_battle_test` in `build/<preset>/`.

```bash
# Release (no sanitizers)
cmake --preset release && cmake --build build/release

# Debug (Address + UB sanitizers)
cmake --preset debug   && cmake --build build/debug
```

- 1st command = **configure** (generates the build system). Re-run it after editing `CMakeLists.txt`, toggling options, or **adding/removing source files** (sources are globbed at configure time).
- 2nd command = **compile/link**. Re-run it for ordinary edits to existing files.

## Run

```bash
# CLI: reads a command file, prints events to stdout
./build/release/sw_battle_test tests/01_example/commands.txt
```

## Tests (black-box suite)

```bash
uv run --project test_runner test_runner/main.py
```

- Auto-locates the binary (`build/debug` then `build/release`) and the `tests/` dir — build the CLI first.
- Override if needed: `--binary <path>` / `--tests <dir>`.
- Expected: `32 passed, 0 failed, 1 xfailed`.
