[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'

$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$outputDirectory = Join-Path $PSScriptRoot 'output\malformed-midi'

Remove-Item -LiteralPath $outputDirectory -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$cases = @(
    @{
        Name = 'short-tempo'
        MetaType = 0x51
        EventData = [byte[]]@(0x07, 0xA1)
        ShouldRemain = $false
    },
    @{
        Name = 'long-tempo'
        MetaType = 0x51
        EventData = [byte[]]@(0x07, 0xA1, 0x20, 0x00)
        ShouldRemain = $false
    },
    @{
        Name = 'zero-tempo'
        MetaType = 0x51
        EventData = [byte[]]@(0x00, 0x00, 0x00)
        ShouldRemain = $false
    },
    @{
        Name = 'short-time-signature'
        MetaType = 0x58
        EventData = [byte[]]@(0x04, 0x02, 0x18)
        ShouldRemain = $false
    },
    @{
        Name = 'long-time-signature'
        MetaType = 0x58
        EventData = [byte[]]@(0x04, 0x02, 0x18, 0x08, 0x00)
        ShouldRemain = $false
    },
    @{
        Name = 'zero-numerator-time-signature'
        MetaType = 0x58
        EventData = [byte[]]@(0x00, 0x02, 0x18, 0x08)
        ShouldRemain = $false
    },
    @{
        Name = 'unsafe-denominator-time-signature'
        MetaType = 0x58
        EventData = [byte[]]@(0x04, 0x08, 0x18, 0x08)
        ShouldRemain = $false
    },
    @{
        Name = 'maximum-denominator-time-signature'
        MetaType = 0x58
        EventData = [byte[]]@(0x04, 0x07, 0x18, 0x08)
        ShouldRemain = $true
    }
)

foreach ($case in $cases) {
    $trackData = [byte[]](@(
        0x00, 0xFF, $case.MetaType, $case.EventData.Length
    ) + $case.EventData + @(
        0x00, 0xC0, 0x00,
        0x00, 0x90, 0x3C, 0x40,
        0x60, 0x80, 0x3C, 0x00,
        0x00, 0xFF, 0x2F, 0x00
    ))
    $midiBytes = [byte[]](@(
        0x4D, 0x54, 0x68, 0x64,
        0x00, 0x00, 0x00, 0x06,
        0x00, 0x00,
        0x00, 0x01,
        0x00, 0x60,
        0x4D, 0x54, 0x72, 0x6B,
        0x00, 0x00, 0x00, $trackData.Length
    ) + $trackData)
    $midiPath = Join-Path $outputDirectory "$($case.Name).mid"
    [System.IO.File]::WriteAllBytes($midiPath, $midiBytes)

    Push-Location $outputDirectory
    try {
        & $resolvedExecutable ".\$($case.Name).mid"
        if ($LASTEXITCODE -ne 0) {
            throw "Malformed MIDI test conversion failed: $($case.Name)"
        }
    }
    finally {
        Pop-Location
    }

    $intermediatePath = Join-Path $outputDirectory "$($case.Name)_.mid"
    $intermediateBytes = [System.IO.File]::ReadAllBytes($intermediatePath)
    $eventRemained = $false
    for ($i = 0; $i -le $intermediateBytes.Length - 2; ++$i) {
        if (($intermediateBytes[$i] -eq 0xFF) -and
            ($intermediateBytes[$i + 1] -eq $case.MetaType)) {
            $eventRemained = $true
            break
        }
    }
    if ($case.ShouldRemain -ne $eventRemained) {
        throw "Unexpected meta-event result in intermediate MIDI: $($case.Name)"
    }
}

$fatalCases = @(
    @{
        Name = 'missing-status'
        TrackData = [byte[]]@(0x00)
    },
    @{
        Name = 'incomplete-channel-event'
        TrackData = [byte[]]@(0x00, 0x90, 0x3C)
    },
    @{
        Name = 'unterminated-delta-time'
        TrackData = [byte[]]@(0x80)
    },
    @{
        Name = 'overlong-delta-time'
        TrackData = [byte[]]@(0x81, 0x80, 0x80, 0x80, 0x00)
    },
    @{
        Name = 'overlong-event-length'
        TrackData = [byte[]]@(0x00, 0xFF, 0x01, 0x81, 0x80, 0x80, 0x80, 0x00)
    },
    @{
        Name = 'oversized-event-data'
        TrackData = [byte[]]@(0x00, 0xFF, 0x01, 0x88, 0x80, 0x80, 0x01)
    },
    @{
        Name = 'event-exceeds-track'
        TrackData = [byte[]]@(0x00, 0xFF, 0x01, 0x05, 0x41)
    },
    @{
        Name = 'track-exceeds-file'
        TrackData = [byte[]]@(0x00, 0xFF, 0x2F, 0x00)
        DeclaredTrackSize = 0x20
    }
)

foreach ($case in $fatalCases) {
    $trackSize = if ($null -ne $case.DeclaredTrackSize) {
        $case.DeclaredTrackSize
    }
    else {
        $case.TrackData.Length
    }
    $midiBytes = [byte[]](@(
        0x4D, 0x54, 0x68, 0x64,
        0x00, 0x00, 0x00, 0x06,
        0x00, 0x00,
        0x00, 0x01,
        0x00, 0x60,
        0x4D, 0x54, 0x72, 0x6B,
        (($trackSize -shr 24) -band 0xFF),
        (($trackSize -shr 16) -band 0xFF),
        (($trackSize -shr 8) -band 0xFF),
        ($trackSize -band 0xFF)
    ) + $case.TrackData)
    $midiPath = Join-Path $outputDirectory "$($case.Name).mid"
    [System.IO.File]::WriteAllBytes($midiPath, $midiBytes)

    Push-Location $outputDirectory
    try {
        & $resolvedExecutable ".\$($case.Name).mid"
        if ($LASTEXITCODE -eq 0) {
            throw "Malformed MIDI was unexpectedly accepted: $($case.Name)"
        }
    }
    finally {
        Pop-Location
    }
}

Write-Host 'Malformed MIDI tests passed.'
