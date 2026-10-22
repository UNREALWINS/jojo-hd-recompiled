# Patches

## `rexglue-v0.10.0-bctr-ctr.patch`

Reference patch for the ReXGlue SDK v0.10.0 `build_bctr` generator.

It changes jump-table dispatch from the original index register to the resolved CTR target address, deduplicates repeated targets, and uses an indirect CTR fallback instead of the generated out-of-range trap.

For normal use, prefer the guarded Windows script:

```text
scripts\APLICAR_PATCH_REXGLUE.bat "C:\path\to\rexglue-sdk"
```

The script verifies the expected v0.10.0 source layout and creates a backup before changing anything.
