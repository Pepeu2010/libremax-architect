param(
    [string]$BuildDirectory = 'build-install/bin',
    [string]$OutputDirectory = 'dist/windows',
    [string]$ToolPrefix = '',
    [string]$Python = 'python',
    [ValidatePattern('^$|^[0-9]+\.[0-9]+\.[0-9]+$')][string]$Version = ''
)
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (!$ToolPrefix) { $ToolPrefix = Join-Path $env:USERPROFILE '.codex\tmp\lmx-tools\msys64\ucrt64' }
$ToolPrefix = (Resolve-Path -LiteralPath $ToolPrefix).Path
$env:PATH = (Join-Path $ToolPrefix 'bin') + ';' + $env:PATH
$build = (Resolve-Path -LiteralPath (Join-Path $sourceRoot $BuildDirectory)).Path
$config = Get-Content -LiteralPath (Join-Path $build 'CPackConfig.cmake') -Raw
if ($config -notmatch 'set\(CPACK_PACKAGE_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"\)') { throw 'Build package version missing.' }
$builtVersion = $Matches[1]
if ($Version -and $Version -ne $builtVersion) { throw 'Installer version differs from the compiled application.' }
$Version = $builtVersion
$output = [IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
$stage = Join-Path $output ('payload-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage -Force | Out-Null
& cmake --install $build --prefix $stage
if ($LASTEXITCODE) { throw 'Failed to install application resources.' }
$binaryDirectory = Join-Path $stage 'bin'
if (!(Test-Path -LiteralPath (Join-Path $stage 'share/libremax/runtime/blender/blender.exe')) -or
    !(Test-Path -LiteralPath (Join-Path $stage 'share/libremax/runtime/blender/libremax-runtime.json'))) {
    throw 'Installer must include the verified Blender runtime; configure LMX_BLENDER_RUNTIME_DIR.'
}
$app = Join-Path $binaryDirectory 'libremax-architect.exe'
& (Join-Path $ToolPrefix 'bin\windeployqt.exe') --release --no-translations --no-plugins --no-opengl-sw --dir $binaryDirectory $app
if ($LASTEXITCODE) { throw 'Qt deployment failed.' }
foreach ($plugin in @('platforms/qwindows.dll', 'platforms/qoffscreen.dll', 'sqldrivers/qsqlite.dll', 'imageformats/qjpeg.dll', 'imageformats/qico.dll')) {
    $source = Join-Path $ToolPrefix ('share/qt6/plugins/' + $plugin)
    if (!(Test-Path -LiteralPath $source)) { throw ('Missing Qt plugin: ' + $plugin) }
    $destination = Join-Path $binaryDirectory ('plugins/' + $plugin)
    New-Item -ItemType Directory -Path (Split-Path $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
}
"[Paths]`nPrefix=.`nPlugins=plugins`n" | Set-Content -LiteralPath (Join-Path $binaryDirectory 'qt.conf') -Encoding ascii
& $Python (Join-Path $PSScriptRoot 'collect-windows-runtime.py') --prefix $ToolPrefix --stage $stage
if ($LASTEXITCODE) { throw 'Native runtime dependency collection failed.' }
$installer = Join-Path $output "LibreMax-Architect-$Version-Windows-x64-Setup.exe"
& (Join-Path $ToolPrefix 'bin/makensis.exe') /V2 "/DAPP_VERSION=$Version" "/DPAYLOAD=$stage" "/DOUTPUT=$installer" (Join-Path $sourceRoot 'INSTALADOR/windows/installer.nsi')
if ($LASTEXITCODE) { throw 'Windows installer creation failed.' }
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $([IO.Path]::GetFileName($installer))" | Set-Content -LiteralPath (Join-Path $output 'SHA256SUMS-Windows.txt') -Encoding ascii
Write-Output ('WINDOWS_PAYLOAD=' + $stage)
Get-Item -LiteralPath $installer | Select-Object FullName, Length
