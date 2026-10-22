param(
    [string]$RexGlueSource = "",
    [switch]$SkipPortablePackage
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$PinnedRexGlueCommit = 'c94f5ebdcb3c9d1a460ca48e04f9758448f8d518'
$RexGlueRepo = 'https://github.com/rexglue/rexglue-sdk.git'
$Manifest = Join-Path $Root 'jojohd_manifest.toml'
$Extracted = Join-Path $Root 'extracted'
$DefaultXex = Join-Path $Extracted 'default.xex'
$Generated = Join-Path $Root 'generated'
$BuildDir = Join-Path $Root 'out\build\win-amd64-release'
$GameExe = Join-Path $BuildDir 'jojohd.exe'
$LauncherDir = Join-Path $Root 'launcher'
$LauncherExe = Join-Path $LauncherDir 'JoJo HD Launcher.exe'
$HelperExe = Join-Path $LauncherDir 'JoJo HD Helper.exe'

function Banner([string]$Text) {
    Write-Host ''
    Write-Host '============================================================' -ForegroundColor DarkCyan
    Write-Host ('  ' + $Text) -ForegroundColor Cyan
    Write-Host '============================================================' -ForegroundColor DarkCyan
}

function Fail([string]$Message, [int]$Code = 1) {
    Write-Host ''
    Write-Host 'ERRO:' -ForegroundColor Red
    Write-Host $Message -ForegroundColor Red
    exit $Code
}

function Need([string]$Command, [string]$Hint) {
    $cmd = Get-Command $Command -ErrorAction SilentlyContinue
    if (-not $cmd) {
        Fail ('Ferramenta ausente: ' + $Command + [Environment]::NewLine + $Hint) 10
    }
    return $cmd.Source
}

function Run([string]$Label, [scriptblock]$Action) {
    Write-Host ''
    Write-Host ('>> ' + $Label) -ForegroundColor Yellow
    & $Action
    if ($LASTEXITCODE -ne 0) {
        throw ($Label + ' falhou (codigo ' + $LASTEXITCODE + ').')
    }
}

Banner 'JOJO HD RECOMPILED - BUILD COMPLETO'

Write-Host "Projeto : $Root"
Write-Host "Manifest: $Manifest"

if (-not (Test-Path -LiteralPath $DefaultXex)) {
    Fail ('Nao encontrei extracted\default.xex.' + [Environment]::NewLine + [Environment]::NewLine +
          'Coloque os arquivos extraidos da SUA PROPRIA COPIA do jogo dentro de:' + [Environment]::NewLine +
          '  ' + $Extracted + [Environment]::NewLine + [Environment]::NewLine +
          'Nenhum arquivo original do jogo e baixado por este projeto.') 2
}

if (-not (Test-Path -LiteralPath $Manifest)) {
    Fail ('Manifest ausente: ' + $Manifest) 3
}

Banner '1/8 - VERIFICANDO FERRAMENTAS'

$Git = Need 'git.exe' 'Instale Git for Windows e deixe git.exe no PATH.'
$CMake = Need 'cmake.exe' 'Instale CMake 3.25 ou mais recente e deixe cmake.exe no PATH.'
$Ninja = Need 'ninja.exe' 'Instale Ninja e deixe ninja.exe no PATH.'
$Clang = Need 'clang.exe' 'Instale LLVM/Clang 18 ou mais recente e deixe clang.exe no PATH.'
$ClangXX = Need 'clang++.exe' 'Instale LLVM/Clang 18 ou mais recente e deixe clang++.exe no PATH.'
$Gpp = Need 'g++.exe' 'Instale MinGW-w64 e deixe g++.exe no PATH.'
$Windres = Need 'windres.exe' 'Instale MinGW-w64 completo e deixe windres.exe no PATH.'

Write-Host 'OK: Git, CMake, Ninja, Clang e MinGW-w64 encontrados.' -ForegroundColor Green

if ([string]::IsNullOrWhiteSpace($RexGlueSource)) {
    if ($env:REXGLUE_SOURCE) {
        $RexGlueSource = $env:REXGLUE_SOURCE
    } else {
        $RexGlueSource = Join-Path $Root 'tools\rexglue-sdk'
    }
}

$RexGlueSource = [System.IO.Path]::GetFullPath($RexGlueSource)
$RexParent = Split-Path -Parent $RexGlueSource

Banner '2/8 - PREPARANDO REXGLUE v0.10.0'

if (-not (Test-Path -LiteralPath $RexGlueSource)) {
    New-Item -ItemType Directory -Path $RexParent -Force | Out-Null
    Run 'Clonando ReXGlue oficial' {
        & $Git clone --recursive $RexGlueRepo $RexGlueSource
    }
}

if (-not (Test-Path -LiteralPath (Join-Path $RexGlueSource '.git'))) {
    Fail ('A pasta de ReXGlue nao e um checkout Git valido:' + [Environment]::NewLine + $RexGlueSource) 20
}

Push-Location $RexGlueSource
try {
    $CurrentRemote = (& $Git remote get-url origin 2>$null)
    if ($CurrentRemote -notmatch 'rexglue[\\/]rexglue-sdk') {
        Fail ('O checkout informado nao parece ser o ReXGlue oficial:' + [Environment]::NewLine + $RexGlueSource) 21
    }

    $Head = (& $Git rev-parse HEAD).Trim()
    if ($Head -ne $PinnedRexGlueCommit) {
        $Dirty = (& $Git status --porcelain)
        if ($Dirty) {
            Fail ('O checkout ReXGlue possui alteracoes locais e nao esta na revisao esperada.' + [Environment]::NewLine +
                  'Para proteger seus arquivos, o build automatico nao vai sobrescreve-los.' + [Environment]::NewLine +
                  'Use a pasta padrao tools\rexglue-sdk ou informe um checkout limpo.') 22
        }

        Run 'Obtendo revisao ReXGlue compativel' {
            & $Git fetch origin $PinnedRexGlueCommit --depth=1
        }
        Run 'Fixando revisao ReXGlue usada pela v1.0 Beta' {
            & $Git checkout --detach $PinnedRexGlueCommit
        }
    }

    Run 'Inicializando dependencias do ReXGlue' {
        & $Git submodule update --init --recursive
    }
}
finally {
    Pop-Location
}

$Patcher = Join-Path $Root 'scripts\APLICAR_PATCH_REXGLUE.ps1'
if (-not (Test-Path -LiteralPath $Patcher)) {
    Fail ('Script de patch ausente: ' + $Patcher) 23
}

Run 'Aplicando/verificando fix bctr -> CTR' {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Patcher -RexGlueSource $RexGlueSource
}

Banner '3/8 - COMPILANDO FERRAMENTA REXGLUE'

Push-Location $RexGlueSource
try {
    Run 'Configurando ReXGlue (Windows AMD64)' {
        & $CMake --preset win-amd64
    }
    Run 'Compilando ReXGlue Release' {
        & $CMake --build --preset win-amd64-release
    }
}
finally {
    Pop-Location
}

$RexExeCandidates = @(
    (Join-Path $RexGlueSource 'out\win-amd64\Release\rexglue.exe'),
    (Join-Path $RexGlueSource 'out\win-amd64\rexglue.exe')
)

$RexExe = $RexExeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if (-not $RexExe) {
    $RexExe = Get-ChildItem -LiteralPath (Join-Path $RexGlueSource 'out') -Recurse -File -Filter 'rexglue.exe' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}

if (-not $RexExe) {
    Fail 'rexglue.exe nao foi encontrado depois da compilacao.' 30
}

Write-Host "ReXGlue: $RexExe" -ForegroundColor Green

Banner '4/8 - GERANDO CODIGO DO JOGO'

Push-Location $Root
try {
    Run 'Executando ReXGlue codegen' {
        & $RexExe codegen $Manifest --ignore-stamp
    }
}
finally {
    Pop-Location
}

$GeneratedDefault = Join-Path $Generated 'default'
if (-not (Test-Path -LiteralPath $GeneratedDefault)) {
    Fail ('Codegen terminou, mas a pasta esperada nao existe:' + [Environment]::NewLine + $GeneratedDefault) 40
}

$Audit = Join-Path $Root 'scripts\AUDITAR_BCTR_GERADO.ps1'
Run 'Auditando dispatchers bctr gerados' {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Audit -GeneratedRoot $GeneratedDefault
}

Banner '5/8 - COMPILANDO JOJOHD.EXE'

Push-Location $Root
try {
    Run 'Configurando projeto JoJo HD' {
        & $CMake --preset win-amd64-release ("-DREXSDK_DIR=" + $RexGlueSource)
    }
    Run 'Compilando JoJo HD Release' {
        & $CMake --build --preset win-amd64-release
    }
}
finally {
    Pop-Location
}

if (-not (Test-Path -LiteralPath $GameExe)) {
    $FoundGame = Get-ChildItem -LiteralPath $BuildDir -Recurse -File -Filter 'jojohd.exe' -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if ($FoundGame) {
        Copy-Item -LiteralPath $FoundGame -Destination $GameExe -Force
    }
}

if (-not (Test-Path -LiteralPath $GameExe)) {
    Fail ('jojohd.exe nao foi encontrado em:' + [Environment]::NewLine + $BuildDir) 50
}

Write-Host "OK: $GameExe" -ForegroundColor Green

Banner '6/8 - COMPILANDO LAUNCHER E HELPER'

$LauncherBuild = Join-Path $LauncherDir 'BUILD_LAUNCHER.ps1'
Run 'Compilando launcher/helper' {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $LauncherBuild
}

if (-not (Test-Path -LiteralPath $LauncherExe)) {
    Fail 'JoJo HD Launcher.exe nao foi gerado.' 60
}
if (-not (Test-Path -LiteralPath $HelperExe)) {
    Fail 'JoJo HD Helper.exe nao foi gerado.' 61
}

Copy-Item -LiteralPath $LauncherExe -Destination (Join-Path $Root 'JoJo HD Launcher.exe') -Force
Copy-Item -LiteralPath $HelperExe -Destination (Join-Path $Root 'JoJo HD Helper.exe') -Force

Banner '7/8 - VALIDANDO ARQUIVOS'

$Required = @(
    $DefaultXex,
    $GameExe,
    (Join-Path $Root 'JoJo HD Launcher.exe'),
    (Join-Path $Root 'JoJo HD Helper.exe')
)

foreach ($File in $Required) {
    if (-not (Test-Path -LiteralPath $File)) {
        Fail ('Arquivo final ausente:' + [Environment]::NewLine + $File) 70
    }
    $Hash = (Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash
    Write-Host ('OK  ' + (Split-Path -Leaf $File) + '  ' + $Hash.Substring(0,16) + '...') -ForegroundColor Green
}

if (-not $SkipPortablePackage) {
    Banner '8/8 - MONTANDO PASTA PORTATIL FINAL'

    $Prepare = Join-Path $Root 'scripts\PREPARAR_JOJO_FINAL.ps1'
    Run 'Criando dist\JoJo HD Recompiled v1.0 Beta' {
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Prepare
    }
} else {
    Banner '8/8 - PACOTE PORTATIL IGNORADO'
}

Write-Host ''
Write-Host '============================================================' -ForegroundColor Green
Write-Host '  JOJO HD RECOMPILED v1.0 BETA - BUILD CONCLUIDO' -ForegroundColor Green
Write-Host '============================================================' -ForegroundColor Green
Write-Host ''
Write-Host "Jogo:     $GameExe"
Write-Host ("Launcher: " + (Join-Path $Root "JoJo HD Launcher.exe"))
Write-Host ("Helper:   " + (Join-Path $Root "JoJo HD Helper.exe"))
if (-not $SkipPortablePackage) {
    Write-Host ("Final:    " + (Join-Path $Root "dist\JoJo HD Recompiled v1.0 Beta"))
}
Write-Host ''
Write-Host 'Os arquivos originais dentro de extracted/ continuam locais e ignorados pelo Git.' -ForegroundColor Yellow

exit 0
