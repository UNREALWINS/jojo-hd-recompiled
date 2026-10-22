# Recompilation fixes

The stable v1.0 Beta candidate was reached after a sequence of project-specific fixes for generated PowerPC indirect-branch dispatchers.

## Stable override count

The tested build contains **257 targeted overrides**:

| Stage | Overrides | Purpose |
| --- | ---: | --- |
| V3.8 | 244 | Repair one repeated family of incorrectly generated terminal `bctr` dispatchers |
| V3.9 | 4 | Replace remaining restrictive dispatcher implementations |
| V3.10 | 8 | Repair a second verified dispatcher family |
| V3.11 | 1 | Repair the final confirmed crash in `sub_822C6420` |
| **Total** | **257** | Stable v1.0 Beta candidate |

## Root cause

The confirmed crashes ended in host `UD2` / Windows exception `0xC000001D`.

In the affected generated functions, the PowerPC code loaded a branch destination into CTR, but the generated C++ dispatcher selected an unrelated table-base register instead of dispatching the actual CTR destination. The project fixes keep the original preparation logic and dispatch through the already-loaded guest CTR target.

## Scope

The fixes are deliberately narrow:

- no blanket removal of traps;
- no NOP patching;
- no fabricated success returns;
- no modification of the user's extracted game data;
- no replacement of the ReXGlue SDK;
- only confirmed dispatcher families are overridden.

## Historical stages

The source tree will keep the individual V3.8, V3.9, V3.10 and V3.11 material separated so regressions can be traced to the stage that introduced a change.

The large V3.8 family is stored in a compressed text representation in this repository to keep the public tree manageable. A local expansion script reconstructs the source file before compiling.
