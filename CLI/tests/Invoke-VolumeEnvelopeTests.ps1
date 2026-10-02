[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$outputRoot = Join-Path $PSScriptRoot 'output\volume-envelope'
$runDirectory = Join-Path $outputRoot ([guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# Program 1 (Bright Piano), velocity 127 の単音を含む format 0 MIDI。
[byte[]]$midiBytes = @(
    0x4D, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x60,
    0x4D, 0x54, 0x72, 0x6B, 0x00, 0x00, 0x00, 0x0F,
    0x00, 0xC0, 0x01,
    0x00, 0x90, 0x3C, 0x7F,
    0x60, 0x80, 0x3C, 0x00,
    0x00, 0xFF, 0x2F, 0x00
)

foreach ($case in @(
    @{ Channel = 'F'; Limit = 31; ExpectedVolume = 18 },
    @{ Channel = 'O'; Limit = 63; ExpectedVolume = 38 }
)) {
    $channel = $case.Channel
    $midiPath = Join-Path $runDirectory "$channel.mid"
    $mmlPath = Join-Path $runDirectory "$channel.mml"
    [System.IO.File]::WriteAllBytes($midiPath, $midiBytes)

    & $resolvedExecutable '-vm3' "-c:$channel" $midiPath | Out-Null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $mmlPath -PathType Leaf)) {
        throw "Channel $channel conversion failed."
    }

    $mml = [System.IO.File]::ReadAllText($mmlPath)
    $definitions = [regex]::Matches(
        $mml,
        "(?m)^\s*@v\d+\s*=\{([^}]*)\}.*// Ch\.$channel\s+Vol\.(\d+)"
    )
    if ($definitions.Count -eq 0) {
        throw "Channel $channel produced no volume envelope definitions."
    }

    foreach ($definition in $definitions) {
        $convertedVolume = [int]$definition.Groups[2].Value
        if ($convertedVolume -ne $case.ExpectedVolume) {
            throw "Channel $channel converted volume $convertedVolume differs from $($case.ExpectedVolume)."
        }

        $values = [regex]::Matches($definition.Groups[1].Value, '\d+')
        if ($values.Count -eq 0) {
            throw "Channel $channel has an empty volume envelope."
        }
        foreach ($value in $values) {
            if ([int]$value.Value -gt $case.Limit) {
                throw "Channel $channel envelope value $($value.Value) exceeds $($case.Limit)."
            }
        }
        $peak = ($values | ForEach-Object { [int]$_.Value } | Measure-Object -Maximum).Maximum
        if ($peak -ne $case.ExpectedVolume) {
            throw "Channel $channel envelope peak $peak differs from $($case.ExpectedVolume)."
        }
    }
}

Write-Host 'FDS and VRC6 saw volume envelope tests passed.'
