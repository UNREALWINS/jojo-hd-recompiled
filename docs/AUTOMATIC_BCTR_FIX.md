# Automatic bctr/CTR compatibility fix

The v1.0 Beta crash series was traced to a ReXGlue v0.10.0 code-generation behavior in `build_bctr`.

Stock v0.10.0 can generate a jump-table dispatcher that switches on the original jump-table index register. In JoJo HD, that register is often overwritten/scaled/loaded with the resolved destination before `bctr`, so the generated switch can fall into `__builtin_trap()`.

The project previously worked around this with 257 project-specific overrides.

## Preferred reproducible workflow

The public workflow now fixes the code generator itself before regenerating the game.

### 1. Get ReXGlue SDK source v0.10.0

Use the official ReXGlue SDK source corresponding to release v0.10.0.

The JoJo project manifest targets:

```text
sdk_version = "0.10.0"
```

### 2. Apply the project patch

From this repository:

```text
scripts\APLICAR_PATCH_REXGLUE.bat "C:\path\to\rexglue-sdk"
```

The script:

- verifies the expected v0.10.0 `control_flow.cpp` layout;
- refuses to modify an unknown layout;
- creates `control_flow.cpp.jojo-bctr-backup`;
- changes generated jump-table dispatch to switch on resolved `CTR` target addresses;
- deduplicates repeated destination addresses;
- changes the old out-of-range trap into an indirect CTR fallback;
- verifies the result after writing.

A raw reference patch is also included at:

```text
patches/rexglue-v0.10.0-bctr-ctr.patch
```

### 3. Rebuild ReXGlue

Build the patched ReXGlue source using its official build instructions.

### 4. Regenerate JoJo code

Run codegen again from the user's own locally extracted `default.xex`.

Do not commit generated game-derived source.

### 5. Audit the generated code

Run:

```text
scripts\AUDITAR_BCTR_GERADO.bat "C:\path\to\generated"
```

The audit scans generated C++ for the old high-risk pattern:

```text
switch (ctx.rN.u32)
...
__builtin_trap(); // Switch case out of range
```

A clean result exits with code 0.

## Historical validation

The manual development series reached 257 overrides by v1.0 Beta.

Known milestones included:

- V3.8: 244 affected routines;
- V3.10: 256 total;
- V3.11: 257 total.

The final V3.11 crash involved:

- function: `sub_822C6420`
- `bctr`: `0x822C6444`
- table: `0x8305A13C`
- incorrect generated source: `ctx.r7.u32`
- correct dispatch source: resolved `ctx.ctr.u32`

The generator-level patch replaces the need to maintain hundreds of per-function overrides for this same class of codegen failure.
