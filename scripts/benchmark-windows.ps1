param(
    [string]$Application = "$env:LOCALAPPDATA/Programs/LibreMax Architect/bin/libremax-architect.exe",
    [string]$OutputDirectory = "$env:USERPROFILE/Documents/LibreMax-Benchmark"
)
$ErrorActionPreference = 'Stop'
$binary = (Resolve-Path -LiteralPath $Application).Path
$destination = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $destination -Force | Out-Null
$cpu = Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors
$graphics = Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion
$memoryBytes = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory
$hardware = [ordered]@{ cpu = $cpu; graphics = $graphics; ramGiB = [math]::Round($memoryBytes / 1GB, 2); os = (Get-CimInstance Win32_OperatingSystem).Caption }
$arguments = @('--apartment-tools-smoke', ('"' + $destination + '"'))
$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = $binary
$start.Arguments = $arguments -join ' '
$start.UseShellExecute = $false
$start.CreateNoWindow = $true
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
$process = [Diagnostics.Process]::Start($start)
$stdout = $process.StandardOutput.ReadToEndAsync()
$stderr = $process.StandardError.ReadToEndAsync()
$peak = 0L
while (!$process.HasExited) {
    $identifiers = [System.Collections.Generic.HashSet[int]]::new()
    [void]$identifiers.Add($process.Id)
    $running = Get-CimInstance Win32_Process | Select-Object ProcessId, ParentProcessId
    do {
        $changed = $false
        foreach ($item in $running) {
            if ($identifiers.Contains([int]$item.ParentProcessId) -and $identifiers.Add([int]$item.ProcessId)) { $changed = $true }
        }
    } while ($changed)
    $working = 0L
    foreach ($identifier in $identifiers) {
        $child = Get-Process -Id $identifier -ErrorAction SilentlyContinue
        if ($child) { $working += $child.WorkingSet64 }
    }
    $peak = [math]::Max($peak, $working)
    if (!$process.WaitForExit(1000)) { $process.Refresh() }
}
$process.WaitForExit()
[IO.File]::WriteAllText((Join-Path $destination 'acceptance.stdout.txt'), $stdout.GetAwaiter().GetResult())
[IO.File]::WriteAllText((Join-Path $destination 'acceptance.stderr.txt'), $stderr.GetAwaiter().GetResult())
$isTarget = (($cpu.Name -join ' ') -match 'i3-6006U') -and (($graphics.Name -join ' ') -match 'HD Graphics 520') -and $memoryBytes -ge 7.5GB
$result = [ordered]@{ hardware = $hardware; targetNotebook = $isTarget; acceptanceExitCode = $process.ExitCode; peakCombinedWorkingSetMiB = [math]::Round($peak / 1MB, 1); acceptanceReport = 'report.json'; comparisonWithVDMaxCompleted = $false }
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $destination 'hardware.json') -Encoding utf8
if ($process.ExitCode -ne 0) { throw 'O teste encontrou uma falha. Os relatórios foram preservados na pasta escolhida.' }
Write-Output ('Teste concluído. Relatórios: ' + $destination)
Write-Output ('Notebook mínimo identificado: ' + $isTarget + '. Confira os tempos de edição no report.json e a memória no hardware.json.')
