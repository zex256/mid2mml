[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'

function Add-UInt32BigEndian {
    param(
        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.List[byte]]$Bytes,
        [Parameter(Mandatory = $true)]
        [int]$Value
    )

    $Bytes.Add([byte](($Value -shr 24) -band 0xFF))
    $Bytes.Add([byte](($Value -shr 16) -band 0xFF))
    $Bytes.Add([byte](($Value -shr 8) -band 0xFF))
    $Bytes.Add([byte]($Value -band 0xFF))
}

function New-EncodingTestMidi {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [byte[]]$Title
    )

    if ($Title.Length -gt 127) {
        throw 'The encoding test title is too long for the one-byte MIDI length.'
    }

    # MIDI由来テキストと音符を含む最小構成のトラックを作成
    $track = [System.Collections.Generic.List[byte]]::new()
    [byte[]](0x00, 0xFF, 0x03, $Title.Length) | ForEach-Object { $track.Add($_) }
    $Title | ForEach-Object { $track.Add($_) }
    [byte[]](
        0x00, 0xC0, 0x00,
        0x00, 0x90, 0x3C, 0x64,
        0x60, 0x80, 0x3C, 0x00,
        0x00, 0xFF, 0x2F, 0x00
    ) | ForEach-Object { $track.Add($_) }

    # MIDIヘッダとトラックチャンクを結合
    $midi = [System.Collections.Generic.List[byte]]::new()
    [byte[]](0x4D, 0x54, 0x68, 0x64) | ForEach-Object { $midi.Add($_) }
    Add-UInt32BigEndian -Bytes $midi -Value 6
    [byte[]](0x00, 0x00, 0x00, 0x01, 0x00, 0x60) | ForEach-Object { $midi.Add($_) }
    [byte[]](0x4D, 0x54, 0x72, 0x6B) | ForEach-Object { $midi.Add($_) }
    Add-UInt32BigEndian -Bytes $midi -Value $track.Count
    $track | ForEach-Object { $midi.Add($_) }
    [System.IO.File]::WriteAllBytes($Path, $midi.ToArray())
}

function Test-ByteSequence {
    param(
        [Parameter(Mandatory = $true)]
        [byte[]]$Bytes,
        [Parameter(Mandatory = $true)]
        [byte[]]$Sequence
    )

    if ($Sequence.Length -eq 0) {
        return $true
    }
    for ($i = 0; $i -le $Bytes.Length - $Sequence.Length; ++$i) {
        $matched = $true
        for ($j = 0; $j -lt $Sequence.Length; ++$j) {
            if ($Bytes[$i + $j] -ne $Sequence[$j]) {
                $matched = $false
                break
            }
        }
        if ($matched) {
            return $true
        }
    }
    return $false
}

$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$outputDirectory = Join-Path $PSScriptRoot 'output\encoding'
Remove-Item -LiteralPath $outputDirectory -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$utf8 = [System.Text.UTF8Encoding]::new($false, $true)
$shiftJis = [System.Text.Encoding]::GetEncoding(932)
$toolName = 'mid2mml for ppmck Ver1.0α'
$cases = @(
    [pscustomobject]@{
        Name = 'ascii'
        Title = [System.Text.Encoding]::ASCII.GetBytes('ASCII title')
        OutputEncoding = $utf8
    },
    [pscustomobject]@{
        Name = 'utf8'
        Title = $utf8.GetBytes('UTF-8曲名')
        OutputEncoding = $utf8
    },
    [pscustomobject]@{
        Name = 'ambiguous-shift-jis'
        Title = [byte[]](0xC2, 0xA2)
        OutputEncoding = $shiftJis
    },
    [pscustomobject]@{
        Name = 'shift-jis'
        Title = $shiftJis.GetBytes('Shift-JIS曲名')
        OutputEncoding = $shiftJis
    },
    [pscustomobject]@{
        Name = 'shift-jis-extension'
        Title = $shiftJis.GetBytes('ﾘｰﾄﾞ①髙')
        OutputEncoding = $shiftJis
    },
    [pscustomobject]@{
        Name = 'utf8-only'
        Title = $utf8.GetBytes('曲名😀')
        OutputEncoding = $utf8
    },
    [pscustomobject]@{
        Name = 'invalid'
        Title = [byte[]](0x81)
        OutputEncoding = $utf8
        DisplayTitle = '[文字コードを判別できないため、表示を省略しました。]'
    }
)

foreach ($case in $cases) {
    # 日本語ファイル名もMIDIテキストの文字コードに合わせて出力されることを確認
    $stem = "日本語-$($case.Name)"
    $midiPath = Join-Path $outputDirectory "$stem.mid"
    $mmlPath = Join-Path $outputDirectory "$stem.mml"
    $intermediatePath = Join-Path $outputDirectory "$stem`_.mid"
    New-EncodingTestMidi -Path $midiPath -Title $case.Title

    $originalCodePage = [Console]::OutputEncoding.CodePage
    & $resolvedExecutable $midiPath
    if ($LASTEXITCODE -ne 0) {
        throw "Encoding test conversion failed: $($case.Name)"
    }
    if ([Console]::OutputEncoding.CodePage -ne $originalCodePage) {
        throw "Console output code page was not restored: $($case.Name)"
    }

    # GUIと同じUTF-8指定で標準出力・標準エラーを受け取り、その場の表示を検証
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $resolvedExecutable
    $startInfo.Arguments = '"' + $midiPath + '"'
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.StandardOutputEncoding = $utf8
    $startInfo.StandardErrorEncoding = $utf8
    $process = [System.Diagnostics.Process]::Start($startInfo)
    try {
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        if ($process.ExitCode -ne 0) {
            throw "Captured encoding test conversion failed: $($case.Name)"
        }
        if ($case.Name -eq 'invalid') {
            $expectedTitle = $case.DisplayTitle
        } else {
            $expectedTitle = $case.OutputEncoding.GetString($case.Title)
        }
        if (-not $stderr.Contains('曲名/トラック名:"' + $expectedTitle + '"')) {
            throw "MIDI title was not displayed correctly as UTF-8: $($case.Name)"
        }
        if ($stderr.IndexOf($expectedTitle) -gt $stderr.IndexOf('done.')) {
            throw "MIDI title display was deferred until conversion completed."
        }
    }
    finally {
        $process.Dispose()
    }

    $mmlBytes = [System.IO.File]::ReadAllBytes($mmlPath)
    $intermediateBytes = [System.IO.File]::ReadAllBytes($intermediatePath)
    if (-not (Test-ByteSequence -Bytes $mmlBytes -Sequence $case.Title)) {
        throw "MIDI text bytes changed in MML output: $($case.Name)"
    }
    if (-not (Test-ByteSequence -Bytes $intermediateBytes -Sequence $case.Title)) {
        throw "MIDI text bytes changed in intermediate MIDI output: $($case.Name)"
    }

    $expectedToolName = $case.OutputEncoding.GetBytes($toolName)
    $expectedFileName = $case.OutputEncoding.GetBytes($stem)
    if (-not (Test-ByteSequence -Bytes $mmlBytes -Sequence $expectedToolName)) {
        throw "Tool name encoding is incorrect: $($case.Name)"
    }
    if (-not (Test-ByteSequence -Bytes $mmlBytes -Sequence $expectedFileName)) {
        throw "MIDI file name encoding is incorrect: $($case.Name)"
    }

    if ($case.Name -eq 'shift-jis') {
        if (Test-ByteSequence -Bytes $mmlBytes -Sequence $utf8.GetBytes($toolName)) {
            throw 'UTF-8 tool name remained in Shift-JIS MML output.'
        }
        if (Test-ByteSequence -Bytes $mmlBytes -Sequence $utf8.GetBytes($stem)) {
            throw 'UTF-8 MIDI file name remained in Shift-JIS MML output.'
        }
    }
}

Write-Host 'All text encoding tests passed.'
