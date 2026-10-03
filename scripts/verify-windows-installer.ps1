param(
    [Parameter(Mandatory)][string]$Installer,
    [string]$EvidenceDirectory = 'build-install/installation-evidence',
    [switch]$Gui,
    [string]$Blender = ''
)
$ErrorActionPreference = 'Stop'
$Installer = (Resolve-Path -LiteralPath $Installer).Path
if ([IO.Path]::GetFileName($Installer) -notmatch '^LibreMax-Architect-([0-9]+\.[0-9]+\.[0-9]+)-Windows-x64-Setup\.exe$') {
    throw 'Installer filename must include its application version.'
}
$expectedVersion = $Matches[1]
$evidence = [IO.Path]::GetFullPath($EvidenceDirectory)
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
$installation = Join-Path $evidence ('installed-' + [guid]::NewGuid().ToString('N'))
$desktopLink = Join-Path ([Environment]::GetFolderPath('Desktop')) 'LibreMax Architect.lnk'
$menu = Join-Path ([Environment]::GetFolderPath('Programs')) 'LibreMax Architect'
$registry = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect'
if ((Test-Path -LiteralPath $registry) -or (Test-Path -LiteralPath $desktopLink) -or (Test-Path -LiteralPath $menu)) {
    throw 'Existing installation or shortcuts found. Verification preserves them; use a clean test account.'
}
# Installation and uninstallation target a newly allocated directory inside this evidence root.
if (!$installation.StartsWith($evidence.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Unsafe installation verification target.'
}
$savedPath = $env:PATH
$savedPlatform = $env:QT_QPA_PLATFORM
$savedPlugins = $env:QT_PLUGIN_PATH
$installed = $false
function Invoke-App([string[]]$Arguments, [string]$Name) {
    $quoted = foreach ($argument in $Arguments) { '"' + $argument + '"' }
    $process = Start-Process -FilePath (Join-Path $installation 'bin/libremax-architect.exe') -ArgumentList ($quoted -join ' ') -PassThru -WindowStyle Hidden -RedirectStandardOutput (Join-Path $evidence "$Name.stdout.txt") -RedirectStandardError (Join-Path $evidence "$Name.stderr.txt")
    if (!$process.WaitForExit(180000)) {
        $process.Kill()
        throw ('Installed application check timed out: ' + $Name)
    }
    $process.WaitForExit()
    $process.Refresh()
    if ($process.ExitCode -ne 0) {
        Get-Content -LiteralPath (Join-Path $evidence "$Name.stderr.txt")
        throw ('Installed application check failed: ' + $Name + ' exit ' + $process.ExitCode)
    }
    Get-Content -LiteralPath (Join-Path $evidence "$Name.stdout.txt")
}
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = ''
    $env:QT_QPA_PLATFORM = 'offscreen'
    $setup = Start-Process -FilePath $Installer -ArgumentList "/S /D=$installation" -Wait -PassThru -WindowStyle Hidden
    if ($setup.ExitCode -ne 0) { throw 'Silent installer failed.' }
    $installed = $true
    $shortcut = (New-Object -ComObject WScript.Shell).CreateShortcut($desktopLink)
    if ($shortcut.TargetPath -ne (Join-Path $installation 'bin\libremax-architect.exe')) {
        throw 'Desktop shortcut target differs from installed application.'
    }
    if (!(Test-Path -LiteralPath (Join-Path $menu 'LibreMax Architect.lnk'))) { throw 'Start menu shortcut missing.' }
    if ((Get-ItemProperty -LiteralPath $registry).DisplayVersion -notlike "$expectedVersion *") { throw 'Installed version registration failed.' }
    Invoke-App -Arguments @('--installation-smoke', (Join-Path $evidence 'runtime')) -Name 'runtime'
    if ($Gui) {
        $env:QT_QPA_PLATFORM = 'windows'
        Invoke-App -Arguments @('--ui-smoke', (Join-Path $evidence 'expanded-library')) -Name 'expanded-library'
        Invoke-App -Arguments @('--modern-smoke', (Join-Path $evidence 'modern')) -Name 'modern'
        Invoke-App -Arguments @('--experience-smoke', (Join-Path $evidence 'experience')) -Name 'experience'
        Invoke-App -Arguments @('--recovery-smoke') -Name 'recovery'
    }
    if ($Blender) {
        Invoke-App -Arguments @('--render-smoke', (Join-Path $evidence 'render'), '--render-project', (Join-Path $installation 'share/libremax/examples/apartamento-moderno.lmx'), '--render-size', '320x180', '--render-samples', '8', '--blender', $Blender) -Name 'render'
    }
    Copy-Item -LiteralPath (Join-Path $evidence 'runtime/portable-project.lmx') -Destination (Join-Path $installation 'preserve-me.lmx')
} finally {
    try {
        if ($installed) {
            $uninstaller = Join-Path $installation 'Desinstalar.exe'
            if (Test-Path -LiteralPath $uninstaller) {
                $uninstall = Start-Process -FilePath $uninstaller -ArgumentList "/S _?=$installation" -Wait -PassThru -WindowStyle Hidden
                if ($uninstall.ExitCode -ne 0) { throw 'Uninstaller failed.' }
            }
        }
    } finally {
        $env:PATH = $savedPath
        $env:QT_QPA_PLATFORM = $savedPlatform
        $env:QT_PLUGIN_PATH = $savedPlugins
    }
}
if ((Test-Path -LiteralPath (Join-Path $installation 'bin/libremax-architect.exe')) -or
    (Test-Path -LiteralPath $desktopLink) -or (Test-Path -LiteralPath $menu) -or (Test-Path -LiteralPath $registry)) {
    throw 'Application files or installed registration remained after uninstall.'
}
if (!(Test-Path -LiteralPath (Join-Path $installation 'preserve-me.lmx'))) { throw 'Uninstaller removed a project.' }
Write-Output 'WINDOWS_INSTALLER_PASS: install, clean PATH runtime, shortcuts, uninstall and preserved project'
