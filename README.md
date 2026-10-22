# JoJo's Bizarre Adventure HD Ver. — Recompiled Edition

Unofficial native Windows recompilation project for **JoJo's Bizarre Adventure HD Ver.**

> **Current milestone:** v1.0 Beta

## Important

This repository **does not contain the original game**, extracted game data, Xbox 360 executable files, disc images, or other copyrighted assets from the commercial release.

You must provide files from your own legally acquired copy of the game.

## v1.0 Beta status

The internal v1.0 Beta build reached full-game testing without the previous illegal-instruction crashes.

The public repository currently includes:

- portable Windows launcher source;
- game-session helper source;
- MinGW launcher/helper build scripts;
- local game-data verification scripts;
- documentation of the verified recompilation compatibility fixes;
- a machine-readable compatibility-fix catalog;
- defensive Git ignore rules that block common game dumps and extracted data.

The large generated recompilation source derived from the commercial executable is intentionally **not** committed.

The public reproduction path now includes an automatic **ReXGlue v0.10.0 bctr/CTR codegen fix** plus a post-codegen audit, so users do not need to hand-edit hundreds of generated files.

## Repository layout

```text
jojo-hd-recompiled/
├─ docs/
│  ├─ BUILDING.md
│  └─ COMPATIBILITY_FIXES.md
├─ extracted/
│  └─ .gitkeep
├─ launcher/
│  ├─ JoJo_HD_Launcher.cpp
│  ├─ JoJo_HD_Helper.cpp
│  ├─ BUILD_LAUNCHER.ps1
│  └─ COMPILAR_LAUNCHER.bat
├─ scripts/
│  ├─ PREPARAR_JOGO.ps1
│  └─ PREPARAR_JOGO.bat
├─ src/
│  ├─ README.md
│  └─ compatibility_fixes.toml
├─ CONTRIBUTING.md
├─ DISCLAIMER.md
└─ README.md
```

## One-click Windows build

For the normal Windows workflow, after placing files from your own copy of the game in `extracted/`, simply run:

```text
BUILD_JOJO.bat
```

The build script performs the complete v1.0 Beta pipeline:

1. validates `extracted/default.xex`;
2. checks Git, CMake, Ninja, LLVM/Clang and MinGW-w64;
3. clones the pinned official ReXGlue source into `tools/rexglue-sdk` when needed;
4. applies/verifies the project bctr -> CTR generator fix with backup;
5. builds the ReXGlue tool;
6. generates JoJo C++ locally from the user's own XEX;
7. audits generated bctr dispatchers;
8. builds `jojohd.exe`;
9. builds the launcher and helper;
10. validates important outputs by SHA-256;
11. creates `dist/JoJo HD Recompiled v1.0 Beta/`.

The `tools/rexglue-sdk/`, `generated/`, `out/`, `dist/` and extracted game data remain local and are ignored by Git.

## Preparing your local copy

Clone or download the repository and place files extracted from **your own copy** of the game in:

```text
extracted/
```

The verification script expects at least:

```text
extracted/default.xex
```

Run:

```text
scripts\PREPARAR_JOGO.bat
```

The script only checks local files. It does not download or upload game data.

## Launcher

The public launcher is built from source in `launcher/`.

Run:

```text
launcher\COMPILAR_LAUNCHER.bat
```

It builds:

```text
JoJo HD Launcher.exe
JoJo HD Helper.exe
```

The launcher build is forced to **x64** and statically links the MinGW runtime. The build script also checks the resulting imports and rejects executables that still depend on `libwinpthread-1.dll`, `libstdc++-6.dll` or `libgcc_s_*.dll`.

The normal tested runtime arguments include:

```text
--game_data_root "<project>\extracted" --gpu_plugin xenos --license_mask=1
```

The launcher passes the selected resolution/fullscreen state and closes after starting the game.

The helper remains active only for the game session and handles F12 exit confirmation/cursor behavior.

## Compatibility work

The v1.0 Beta stability work focused on generated PowerPC `bctr` dispatch cases where the generated host dispatcher could select the destination using a general-purpose register instead of the emulated `CTR`.

The development series included fixes for routines such as:

- `sub_8212B868`
- `sub_82121D20`
- `sub_821704D0`
- `sub_822BA690`
- `sub_822C6420`

The original development build reached 257 project-specific overrides. The public reproduction method now fixes the generator itself so those same bad dispatchers are not emitted.

See [docs/AUTOMATIC_BCTR_FIX.md](docs/AUTOMATIC_BCTR_FIX.md) for the automatic workflow and the compatibility notes under `recomp/` for the historical development path.

## Performance note

Heavy trace logging caused small in-fight stalls during development.

Normal gameplay should not use trace logging. Diagnostic mode exists specifically for troubleshooting.

## Public artwork policy

The public repository does not include the themed JoJo background/logo artwork from the local/private launcher build.

This avoids using the repository as a distribution channel for third-party artwork. The public launcher source works without those assets.

## ReXGlue

This project uses **ReXGlue** as the recompilation framework/runtime.

ReXGlue is a separate project and is not maintained by this repository.

## Legal notice

This is an unofficial fan/technical project.

**JoJo's Bizarre Adventure**, the original game, characters, trademarks and related intellectual property belong to their respective owners.

This project is not affiliated with, sponsored by, approved by, or endorsed by the original publisher, developer, licensors, rights holders, or ReXGlue maintainers.

See [DISCLAIMER.md](DISCLAIMER.md).

## Reporting bugs

Please provide:

- reproduction steps;
- Windows version;
- GPU;
- crash address/function when available;
- diagnostic logs when useful.

Do **not** upload `default.xex`, ISOs, extracted game data or copyrighted game assets.

See [CONTRIBUTING.md](CONTRIBUTING.md).
