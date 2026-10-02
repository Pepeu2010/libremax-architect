param([ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')][string]$Version = '0.6.0-dev')
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Push-Location $sourceRoot
try {
    git rev-parse --verify HEAD | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Commit the source before generating a source archive.' }
    New-Item -ItemType Directory -Path dist -Force | Out-Null
    $archives = @(
        @{ Name = 'source.tar.gz'; Paths = @(); Prefix = "LibreMax-Architect-$Version/" },
        @{ Name = 'starter-library.tar.gz'; Paths = @('starter-library'); Prefix = '' },
        @{ Name = 'starter-materials.tar.gz'; Paths = @('starter-materials'); Prefix = '' },
        @{ Name = 'starter-models.tar.gz'; Paths = @('starter-models'); Prefix = '' },
        @{ Name = 'sample-projects.tar.gz'; Paths = @('examples'); Prefix = '' }
    )
    foreach ($archive in $archives) {
        $arguments = @('archive', '--format=tar.gz', "--output=dist/$($archive.Name)")
        if ($archive.Prefix) { $arguments += "--prefix=$($archive.Prefix)" }
        $arguments += @('HEAD') + $archive.Paths
        & git @arguments
        if ($LASTEXITCODE -ne 0) { throw "Failed to archive $($archive.Name)." }
    }
    $checksums = foreach ($archive in $archives) {
        $hash = (Get-FileHash -LiteralPath "dist/$($archive.Name)" -Algorithm SHA256).Hash.ToLowerInvariant()
        "$hash  $($archive.Name)"
    }
    $checksums | Set-Content -LiteralPath dist/SHA256SUMS -Encoding ascii
    Write-Output 'Archives contain committed HEAD only; Linux executables are not included.'
    Get-Content -LiteralPath dist/SHA256SUMS
} finally {
    Pop-Location
}
