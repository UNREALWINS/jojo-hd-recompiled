# Recompilation compatibility work

This directory contains public metadata and documentation for the project-specific compatibility fixes used by the Recompiled Edition.

## Current public contents

- `compatibility_fixes.toml` — machine-readable catalog of the verified compatibility fixes.
- See `../docs/COMPATIBILITY_FIXES.md` for the technical history.

## Why the full generated source is not committed

The recompilation pipeline can generate a large amount of source from the commercial Xbox 360 executable.

That generated output is intentionally excluded from the public repository.

The public project records and distributes the independently authored compatibility logic, addresses, tooling and documentation needed to reproduce the work without using GitHub as a distribution channel for the original game's code or assets.

The v1.0 Beta fix series addressed incorrect generated indirect-branch dispatch behavior where `bctr` paths could select a destination from a GPR instead of the emulated `CTR`.

Future public tooling should regenerate/apply these fixes locally against files produced from the user's own legally acquired copy.
