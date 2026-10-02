[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [string[]]$ConverterArguments = @(),
    [switch]$SkipGoldenComparison
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$fixture = Join-Path $projectRoot 'DPCM.mid'
$goldenDirectory = Join-Path $PSScriptRoot 'golden'
$outputDirectory = Join-Path $PSScriptRoot 'output\DPCM'

if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "Converter executable was not found: $Executable"
}
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path

if (-not (Test-Path -LiteralPath $fixture -PathType Leaf)) {
    throw "Regression fixture was not found: $fixture"
}

Remove-Item -LiteralPath $outputDirectory -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
Copy-Item -LiteralPath $fixture -Destination (Join-Path $outputDirectory 'DPCM.mid')

Push-Location $outputDirectory
try {
    & $resolvedExecutable @ConverterArguments '.\DPCM.mid'
    if ($LASTEXITCODE -ne 0) {
        throw "Converter exited with code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

$expectedFiles = @('DPCM.mml', 'DPCM_.mid')
foreach ($name in $expectedFiles) {
    $actual = Join-Path $outputDirectory $name

    if (-not (Test-Path -LiteralPath $actual -PathType Leaf)) {
        throw "Converter did not create: $name"
    }

    if ($SkipGoldenComparison) {
        continue
    }

    $expected = Join-Path $goldenDirectory $name

    if (-not (Test-Path -LiteralPath $expected -PathType Leaf)) {
        throw "Golden file was not found: $expected"
    }

    if ([System.IO.Path]::GetExtension($name) -eq '.mml') {
        $actualText = [System.IO.File]::ReadAllText($actual).Replace("`r`n", "`n")
        $expectedText = [System.IO.File]::ReadAllText($expected).Replace("`r`n", "`n")
        if ($actualText -cne $expectedText) {
            throw "Regression mismatch for $name"
        }
    }
    else {
        $actualHash = (Get-FileHash -LiteralPath $actual -Algorithm SHA256).Hash
        $expectedHash = (Get-FileHash -LiteralPath $expected -Algorithm SHA256).Hash
        if ($actualHash -ne $expectedHash) {
            throw "Regression mismatch for $name"
        }
    }
}

if ($SkipGoldenComparison) {
    Write-Host 'DPCM conversion smoke test passed.'
}
else {
    Write-Host 'DPCM regression passed.'
}
