param(
    [Parameter(Mandatory=$false)]
    [string]$GeneratedRoot = "."
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path $GeneratedRoot).Path

$CppFiles = Get-ChildItem -LiteralPath $Root -Recurse -File -Filter '*.cpp'
if (-not $CppFiles) {
    throw "Nenhum arquivo .cpp encontrado em: $Root"
}

$Suspicious = @()

foreach ($File in $CppFiles) {
    $Text = Get-Content -Raw -LiteralPath $File.FullName

    # Old/problematic ReXGlue bctr codegen pattern:
    # switch (ctx.rN.u32) + numeric case indices + out-of-range trap.
    if (
        $Text -match 'switch\s*\(\s*ctx\.r\d+\.u32\s*\)' -and
        $Text -match 'case\s+\d+\s*:' -and
        $Text -match '__builtin_trap\(\);\s*//\s*Switch case out of range'
    ) {
        $MatchesFound = [regex]::Matches(
            $Text,
            'switch\s*\(\s*(ctx\.r\d+\.u32)\s*\)'
        )

        foreach ($M in $MatchesFound) {
            $Suspicious += [pscustomobject]@{
                File = $File.FullName.Substring($Root.Length).TrimStart('\')
                Switch = $M.Groups[1].Value
            }
        }
    }
}

Write-Host ''
Write-Host '=== Auditoria bctr ===' -ForegroundColor Cyan
Write-Host "Arquivos .cpp analisados: $($CppFiles.Count)"

if ($Suspicious.Count -eq 0) {
    Write-Host 'Nenhum dispatcher bctr antigo/suspeito foi encontrado.' -ForegroundColor Green
    exit 0
}

Write-Host ''
Write-Host "ATENCAO: $($Suspicious.Count) dispatcher(s) suspeito(s) encontrado(s)." -ForegroundColor Yellow
$Suspicious | Sort-Object File,Switch | Format-Table -AutoSize

Write-Host ''
Write-Host 'O codegen provavelmente foi feito com um ReXGlue sem o patch CTR.' -ForegroundColor Yellow
exit 3
