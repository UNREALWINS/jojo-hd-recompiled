# Scripts

## Final local package

`PREPARAR_JOJO_FINAL.bat` validates and assembles the user's local portable game folder.

## ReXGlue v0.10.0 bctr fix

Run:

```text
APLICAR_PATCH_REXGLUE.bat "C:\path\to\rexglue-sdk"
```

This safely patches the ReXGlue v0.10.0 `build_bctr` generator used by this project. It verifies the expected source layout first and creates a backup before writing.

After rebuilding ReXGlue and regenerating JoJo source, run:

```text
AUDITAR_BCTR_GERADO.bat "C:\path\to\generated"
```

The audit searches for the old high-risk generated pattern: a jump-table switch on `ctx.rN.u32` that ends in the ReXGlue out-of-range trap.

No script downloads or uploads original game files.
