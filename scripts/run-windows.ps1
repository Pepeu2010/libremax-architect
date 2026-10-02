param([string]$Project = '', [string]$BuildDirectory = '')
$ErrorActionPreference = 'Stop'
$toolBin = Join-Path $env:USERPROFILE '.codex\tmp\lmx-tools\msys64\ucrt64\bin'
if (!$BuildDirectory) {
    $BuildDirectory = if (Test-Path -LiteralPath (Join-Path $PSScriptRoot '..\build-modern\libremax-architect.exe')) { 'build-modern' } else { 'build' }
}
$executable = Join-Path $PSScriptRoot ('..\' + $BuildDirectory + '\libremax-architect.exe')
if (!(Test-Path -LiteralPath $executable)) { throw 'Compile first; see docs/BUILDING.md.' }
if (Test-Path -LiteralPath $toolBin) { $env:PATH = $toolBin + ';' + $env:PATH }
if ($Project) { & $executable $Project } else { & $executable }
