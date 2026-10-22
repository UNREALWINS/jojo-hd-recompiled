param(
    [Parameter(Mandatory=$true)]
    [string]$RexGlueSource
)

$ErrorActionPreference = 'Stop'

$SourceRoot = (Resolve-Path $RexGlueSource).Path
$Target = Join-Path $SourceRoot 'src\codegen\builders\control_flow.cpp'

if (-not (Test-Path $Target)) {
    throw "Arquivo nao encontrado: $Target"
}

$Original = Get-Content -Raw -LiteralPath $Target

$AlreadyPatched =
    $Original -match 'JoJo HD Recompiled compatibility fix' -and
    $Original -match 'switch \(\{\}\.u32\).*ctx\.ctr\(\)' -and
    $Original -match 'REX_CALL_INDIRECT_FUNC\(\{\}\.u32\).*ctx\.ctr\(\)'

if ($AlreadyPatched) {
    Write-Host 'Patch bctr ja esta aplicado.' -ForegroundColor Green
    exit 0
}

$ExpectedSwitch = '    ctx.println("\tswitch ({}.u32) {{", ctx.r(jt->indexRegister));'
$ExpectedCase   = 'ctx.println("\tcase {}:", i);'
$ExpectedTrap   = '    ctx.println("\t\t__builtin_trap(); // Switch case out of range");'

foreach ($Needle in @($ExpectedSwitch, $ExpectedCase, $ExpectedTrap)) {
    if (-not $Original.Contains($Needle)) {
        throw @"
O arquivo control_flow.cpp nao corresponde ao layout esperado do ReXGlue v0.10.0.

Trecho esperado nao encontrado:
$Needle

Nenhuma alteracao foi feita.
"@
    }
}

$Backup = "$Target.jojo-bctr-backup"
if (-not (Test-Path $Backup)) {
    Copy-Item -LiteralPath $Target -Destination $Backup
}

$Modified = $Original

$Modified = $Modified.Replace(
    $ExpectedSwitch,
@'
    // JoJo HD Recompiled compatibility fix:
    // dispatch from the resolved CTR target instead of assuming the
    // jump-table index register still contains 0..N-1 at bctr time.
    ctx.println("\tswitch ({}.u32) {{", ctx.ctr());
'@.TrimEnd()
)

$Modified = $Modified.Replace(
@'
    for (size_t i = 0; i < jt->targets.size(); i++) {
      ctx.println("\tcase {}:", i);
      auto label = jt->targets[i];
'@,
@'
    for (size_t i = 0; i < jt->targets.size(); i++) {
      auto label = jt->targets[i];

      // Several indices can legally resolve to the same destination.
      // When switching on CTR, emit each target address only once.
      bool duplicate = false;
      for (size_t j = 0; j < i; j++) {
        if (jt->targets[j] == label) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) continue;

      ctx.println("\tcase 0x{:X}:", label);
'@
)

$Modified = $Modified.Replace(
    $ExpectedTrap,
@'
    ctx.println("\t\tREX_CALL_INDIRECT_FUNC({}.u32);", ctx.ctr());
    ctx.println("\t\treturn;");
'@.TrimEnd()
)

if ($Modified -eq $Original) {
    throw 'O patch nao alterou o arquivo. Nenhuma mudanca foi salva.'
}

Set-Content -LiteralPath $Target -Value $Modified -Encoding UTF8

$Check = Get-Content -Raw -LiteralPath $Target
if (
    $Check -notmatch 'JoJo HD Recompiled compatibility fix' -or
    $Check -notmatch 'ctx\.println\("\\tswitch \(\{\}\.u32\) \{\{", ctx\.ctr\(\)\);' -or
    $Check -notmatch 'ctx\.println\("\\tcase 0x\{:\X\}:", label\);' -or
    $Check -notmatch 'REX_CALL_INDIRECT_FUNC\(\{\}\.u32\)'
) {
    Copy-Item -LiteralPath $Backup -Destination $Target -Force
    throw 'A verificacao final falhou. O backup original foi restaurado.'
}

Write-Host ''
Write-Host 'Patch bctr aplicado com sucesso.' -ForegroundColor Green
Write-Host "Arquivo: $Target"
Write-Host "Backup:  $Backup"
Write-Host ''
Write-Host 'Agora recompile o ReXGlue e gere novamente o codigo do jogo.'
