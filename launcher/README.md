# Launcher and session helper

This directory contains the public Windows launcher and helper source used by the Recompiled Edition workflow.

## Files

- `JoJo_HD_Launcher.cpp` — portable Windows launcher.
- `JoJo_HD_Helper.cpp` — invisible game-session helper for F12 exit confirmation and cursor handling.
- `BUILD_LAUNCHER.ps1` — MinGW-w64 build script.
- `COMPILAR_LAUNCHER.bat` — double-click build entry point.

## Runtime behavior

The launcher:

- looks for `extracted/default.xex`;
- looks for `jojohd.exe` in the project root or the standard ReXGlue Windows release output directory;
- launches with `--gpu_plugin xenos`;
- passes `--license_mask=1`;
- passes the selected resolution and fullscreen state;
- starts the helper with the game PID;
- closes itself after the game starts.

The helper:

- runs without a visible console window;
- polls F12 with `GetAsyncKeyState`;
- asks before closing the game;
- keeps the cursor out of the play area;
- exits when the game process exits.

## Artwork

The public source intentionally does **not** include copyrighted JoJo/game/anime artwork or the private themed background used by the local build.

A public distribution should use original/licensed artwork or the clean launcher UI included here.
