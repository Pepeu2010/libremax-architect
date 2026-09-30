param([string]$Project = '')
$ErrorActionPreference = 'Stop'
$toolBin = Join-Path $env:USERPROFILE '.codex\tmp\lmx-tools\msys64\ucrt64\bin'
$executable = Join-Path $PSScriptRoot '..\build\libremax-architect.exe'
if (!(Test-Path -LiteralPath $executable)) { throw 'Compile first; see docs/BUILDING.md.' }
if (Test-Path -LiteralPath $toolBin) { $env:PATH = $toolBin + ';' + $env:PATH }
if ($Project) { & $executable $Project } else { & $executable }
