$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root 'out\build\win-amd64-release'
$GameExe = Join-Path $Build 'jojohd.exe'
$Extracted = Join-Path $Root 'extracted'
$Launcher = Join-Path $Root 'JoJo HD Launcher.exe'
$Helper = Join-Path $Root 'JoJo HD Helper.exe'
$Dest = Join-Path $Root 'dist\JoJo HD Recompiled v1.0 Beta'

function Require-Path([string]$Path, [string]$Label) {
    if (-not (Test-Path -LiteralPath $Path)) {
        throw ('Missing: ' + $Label + [Environment]::NewLine + $Path)
    }
}

Write-Host ''
Write-Host '=== JOJO HD RECOMPILED - PREPARE PORTABLE BUILD ===' -ForegroundColor Cyan
Write-Host "Project: $Root"
Write-Host "Output : $Dest"
Write-Host ''

Require-Path $GameExe 'jojohd.exe'
Require-Path (Join-Path $Extracted 'default.xex') 'extracted\default.xex'
Require-Path $Launcher 'JoJo HD Launcher.exe'
Require-Path $Helper 'JoJo HD Helper.exe'

if (Test-Path -LiteralPath $Dest) {
    $stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
    $backup = $Dest + '_BACKUP_' + $stamp
    Write-Host "Existing output preserved as:" -ForegroundColor Yellow
    Write-Host "  $backup"
    Move-Item -LiteralPath $Dest -Destination $backup
}

New-Item -ItemType Directory -Path $Dest -Force | Out-Null

Copy-Item -LiteralPath $GameExe -Destination (Join-Path $Dest 'jojohd.exe') -Force
Copy-Item -LiteralPath $Launcher -Destination (Join-Path $Dest 'JoJo HD Launcher.exe') -Force
Copy-Item -LiteralPath $Helper -Destination (Join-Path $Dest 'JoJo HD Helper.exe') -Force

Get-ChildItem -LiteralPath $Build -File -Filter '*.dll' -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $Dest $_.Name) -Force
}

foreach ($name in @('gamecontrollerdb.txt','dxcompiler.dll','dxil.dll')) {
    $p = Join-Path $Build $name
    if (Test-Path -LiteralPath $p) {
        Copy-Item -LiteralPath $p -Destination (Join-Path $Dest $name) -Force
    }
}

Copy-Item -LiteralPath $Extracted -Destination (Join-Path $Dest 'extracted') -Recurse -Force

foreach ($name in @('JoJo_HD_Launcher.ini','jojo.toml','jojohd.toml','gamecontrollerdb.txt')) {
    $p = Join-Path $Root $name
    if (Test-Path -LiteralPath $p) {
        Copy-Item -LiteralPath $p -Destination (Join-Path $Dest $name) -Force
    }
}

New-Item -ItemType Directory -Path (Join-Path $Dest 'logs') -Force | Out-Null

$checks = @(
    @{ Name='jojohd.exe'; Source=$GameExe; Dest=(Join-Path $Dest 'jojohd.exe') },
    @{ Name='JoJo HD Launcher.exe'; Source=$Launcher; Dest=(Join-Path $Dest 'JoJo HD Launcher.exe') },
    @{ Name='JoJo HD Helper.exe'; Source=$Helper; Dest=(Join-Path $Dest 'JoJo HD Helper.exe') },
    @{ Name='default.xex'; Source=(Join-Path $Extracted 'default.xex'); Dest=(Join-Path $Dest 'extracted\default.xex') }
)

foreach ($c in $checks) {
    Require-Path $c.Dest $c.Name
    $srcHash = (Get-FileHash -LiteralPath $c.Source -Algorithm SHA256).Hash
    $dstHash = (Get-FileHash -LiteralPath $c.Dest -Algorithm SHA256).Hash
    if ($srcHash -ne $dstHash) {
        throw "SHA-256 validation failed: $($c.Name)"
    }
    Write-Host "OK: $($c.Name)" -ForegroundColor Green
}

Write-Host ''
Write-Host 'Portable folder prepared successfully:' -ForegroundColor Green
Write-Host $Dest
Write-Host ''
Write-Host 'NOTE: extracted/ contains files from YOUR local game copy and must never be committed or redistributed.' -ForegroundColor Yellow

Start-Process explorer.exe -ArgumentList ('"' + $Dest + '"')
