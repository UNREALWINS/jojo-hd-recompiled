# JoJo's Bizarre Adventure HD Ver. — Recompiled Edition

Unofficial native Windows recompilation project for **JoJo's Bizarre Adventure HD Ver.**

> **Current release target:** v1.0 Beta

## Important

This repository **does not contain the original game**, extracted game data, Xbox 360 executable files, disc images, or other copyrighted assets from the commercial release.

You must provide files from your own legally acquired copy of the game.

## Project goals

- Run the game natively on Windows through recompilation.
- Provide a simple launcher for resolution and fullscreen options.
- Keep the project portable.
- Preserve the original game experience while fixing recompilation-specific crashes and compatibility issues.
- Make the recompilation process reproducible without distributing the original game data.

## What belongs in this repository

- Launcher source code.
- Helper source code.
- Recompilation manifests and configuration.
- Project-specific dispatcher/override fixes.
- Build and verification scripts.
- Documentation.

## What does NOT belong in this repository

Do **not** upload:

- `default.xex`
- extracted game files
- Xbox 360 package contents
- ISO/disc images
- original textures, audio, movies, fonts or other game assets
- generated binaries containing material that cannot legally be redistributed

The repository includes a defensive `.gitignore` to reduce the risk of accidentally committing these files.

## Expected local layout

After obtaining the project, place the files extracted from your own copy of the game inside:

```text
extracted/
```

The expected local project layout will look approximately like:

```text
jojo-hd-recompiled/
├─ extracted/              # supplied locally by the user; not tracked by Git
├─ launcher/
├─ scripts/
├─ src/
├─ README.md
├─ DISCLAIMER.md
└─ .gitignore
```

Additional build instructions and the exact required files will be documented as the public package is finalized.

## ReXGlue

This project uses **ReXGlue** as the recompilation framework/runtime.

ReXGlue is a separate project and is not maintained by this repository.

## Status

The current internal build has reached the first stable **v1.0 Beta** candidate used for full gameplay testing.

Known project work includes:

- fixes for generated indirect branch/jump-table dispatch cases;
- project-specific runtime overrides;
- launcher with resolution/fullscreen selection;
- an auxiliary helper for game-session behavior;
- portable relative-path launching.

## Legal notice

This is an unofficial fan/technical project.

**JoJo's Bizarre Adventure**, the original game, characters, trademarks and related intellectual property belong to their respective owners.

This project is not affiliated with, sponsored by, approved by, or endorsed by the original publisher, developer, licensors, rights holders, or ReXGlue maintainers.

See [DISCLAIMER.md](DISCLAIMER.md).

## Contributions

Please do not submit copyrighted game data in issues, pull requests, releases, attachments, or test packages.

When reporting a bug, prefer:

- logs;
- crash addresses;
- function names/addresses;
- reproduction steps;
- hashes of files where appropriate;

instead of uploading copyrighted game files.
