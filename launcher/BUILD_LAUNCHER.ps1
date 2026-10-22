$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$gpp = (Get-Command g++.exe -ErrorAction Stop).Source
$windres = (Get-Command windres.exe -ErrorAction Stop).Source

$bg = Join-Path $PSScriptRoot 'launcher_bg.png'
$icon = Join-Path $PSScriptRoot 'jojo_launcher.ico'
$generatedBg = $false

if (-not (Test-Path -LiteralPath $bg)) {
    Write-Host 'launcher_bg.png not found. Generating a neutral fallback background...' -ForegroundColor Yellow
    Add-Type -AssemblyName System.Drawing
    $bmp = New-Object System.Drawing.Bitmap 1031,648
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::FromArgb(20,15,28))

    $panel = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(34,31,43))
    $field = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(42,48,61))
    $button = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(49,43,59))
    $textBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(235,235,235))
    $mutedBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(180,180,190))
    $accent = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(220,190,120)),2
    $titleFont = New-Object System.Drawing.Font 'Segoe UI',22,[System.Drawing.FontStyle]::Bold
    $font = New-Object System.Drawing.Font 'Segoe UI',11,[System.Drawing.FontStyle]::Regular
    $small = New-Object System.Drawing.Font 'Segoe UI',9,[System.Drawing.FontStyle]::Regular

    $g.FillRectangle($panel,230,0,801,648)
    $g.DrawString("JoJo's Bizarre Adventure HD Ver.",$titleFont,$textBrush,270,72)
    $g.DrawString('Recompiled Edition - v1.0 Beta',$font,$mutedBrush,274,112)

    $g.DrawString('RESOLUTION',$small,$mutedBrush,414,323)
    $g.FillRectangle($field,439,348,177,32)
    $g.DrawString('FULLSCREEN',$font,$textBrush,450,389)
    $g.DrawString('V-SYNC',$font,$textBrush,450,422)

    $g.FillRectangle($button,249,503,457,72)
    $g.DrawRectangle($accent,251,505,453,68)
    $g.DrawString('PLAY',$titleFont,$textBrush,420,520)

    $g.FillRectangle($button,725,505,291,70)
    $g.DrawString('DIAGNOSTIC MODE',$font,$textBrush,790,530)

    $g.FillRectangle($button,823,344,193,46)
    $g.DrawString('OPEN SAVES',$font,$textBrush,858,358)
    $g.FillRectangle($button,823,395,193,45)
    $g.DrawString('OPEN LOGS',$font,$textBrush,862,408)
    $g.FillRectangle($button,823,446,193,45)
    $g.DrawString('ADVANCED',$font,$textBrush,863,459)

    $g.FillRectangle($button,249,590,219,47)
    $g.DrawString('CREDITS',$font,$textBrush,320,604)
    $g.FillRectangle($button,477,590,213,47)
    $g.DrawString('VERIFY FILES',$font,$textBrush,530,604)
    $g.FillRectangle($button,702,590,314,47)
    $g.DrawString('EXIT',$font,$textBrush,835,604)

    $bmp.Save($bg,[System.Drawing.Imaging.ImageFormat]::Png)

    $titleFont.Dispose()
    $font.Dispose()
    $small.Dispose()
    $accent.Dispose()
    $panel.Dispose()
    $field.Dispose()
    $button.Dispose()
    $textBrush.Dispose()
    $mutedBrush.Dispose()
    $g.Dispose()
    $bmp.Dispose()
    $generatedBg = $true
}

$rc = 'jojo_launcher.rc'
$res = 'jojo_launcher_res.o'
$rcLines = @()
if (Test-Path -LiteralPath $icon) {
    $rcLines += '1 ICON "jojo_launcher.ico"'
}
$rcLines += '101 RCDATA "launcher_bg.png"'
$rcLines | Set-Content -Encoding ASCII $rc

& $windres $rc '-O' 'coff' '-o' $res
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to compile launcher resources.'
}

$launcherArgs = @(
    '-std=c++23','-O2','-s','-DUNICODE','-D_UNICODE','-municode','-mwindows',
    'JoJo_HD_Launcher.cpp',$res,'-o','JoJo HD Launcher.exe',
    '-lgdiplus','-lgdi32','-luser32','-lshell32','-lole32','-luuid',
    '-static-libgcc','-static-libstdc++'
)
& $gpp @launcherArgs
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to compile launcher.'
}

$helperArgs = @(
    '-std=c++23','-O2','-s','-DUNICODE','-D_UNICODE','-municode','-mwindows',
    'JoJo_HD_Helper.cpp','-o','JoJo HD Helper.exe',
    '-lgdi32','-luser32','-static-libgcc','-static-libstdc++'
)
& $gpp @helperArgs
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to compile helper.'
}

Remove-Item -LiteralPath $rc,$res -Force -ErrorAction SilentlyContinue
if ($generatedBg) {
    Remove-Item -LiteralPath $bg -Force -ErrorAction SilentlyContinue
}

Write-Host ''
Write-Host '=== BUILD COMPLETE ===' -ForegroundColor Green
Get-Item 'JoJo HD Launcher.exe','JoJo HD Helper.exe' | Select-Object Name,Length,LastWriteTime
Write-Host ''
Write-Host 'Custom launcher_bg.png / jojo_launcher.ico are optional local assets and are not required by the repository.'
