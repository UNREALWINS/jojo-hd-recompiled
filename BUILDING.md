# Building on Windows

## Requirements

- Windows 10 or Windows 11 x64
- CMake 3.25+
- Ninja
- LLVM/Clang for the ReXGlue project
- MinGW-w64 with `g++.exe` and `windres.exe` for the standalone launcher/helper
- A compatible ReXGlue SDK/toolchain
- Files extracted locally from your own legally acquired copy of the game

## Local game data

Place your locally extracted files inside:

```text
extracted/
```

The workflow checks for:

```text
extracted/default.xex
```

The whole `extracted/` directory is ignored by Git and must not be committed or redistributed.

## Build the launcher

Open:

```text
launcher/
```

and run:

```text
COMPILAR_LAUNCHER.bat
```

This builds:

```text
JoJo HD Launcher.exe
JoJo HD Helper.exe
```

The launcher source uses only relative paths, so the finished files can live beside the recompiled game executable.

### Optional launcher artwork

The repository intentionally does not ship game/fan artwork.

You may locally provide:

```text
launcher/launcher_bg.png
launcher/jojo_launcher.ico
```

If the PNG is absent, the build script creates a neutral original fallback background, embeds it in the launcher executable, and removes the temporary PNG afterward.

## Build the ReXGlue project

The included preset targets Windows AMD64 Release:

```powershell
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release
```

Expected build directory:

```text
out/build/win-amd64-release/
```

Project-specific indirect-branch fixes are documented under `recomp/`.

## Create a portable local folder

After the recompiled executable and launcher/helper have been built, run:

```text
scripts/PREPARAR_JOJO_FINAL.bat
```

The script validates SHA-256 hashes for the important copied files and creates:

```text
dist/JoJo HD Recompiled v1.0 Beta/
```

Because that local output contains the user's own game data, **do not commit or redistribute the generated `dist/` directory**.
