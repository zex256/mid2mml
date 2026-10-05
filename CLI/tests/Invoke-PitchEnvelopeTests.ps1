[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$outputRoot = Join-Path $PSScriptRoot 'output\pitch-envelope'
$runDirectory = Join-Path $outputRoot ([guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# A4の発音中に、既定の範囲で中央→最大→中央→最小→中央とベンドする。
[byte[]]$midiBytes = @(
    0x4D, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x60,
    0x4D, 0x54, 0x72, 0x6B, 0x00, 0x00, 0x00, 0x1C,
    0x00, 0x90, 0x45, 0x7F,
    0x30, 0xE0, 0x7F, 0x7F,
    0x30, 0xE0, 0x00, 0x40,
    0x30, 0xE0, 0x00, 0x00,
    0x30, 0xE0, 0x00, 0x40,
    0x60, 0x80, 0x45, 0x00,
    0x00, 0xFF, 0x2F, 0x00
)

# 出力オクターブの補正を維持し、タイマ値の量子化後も中央へ戻ることを確認する。
foreach ($case in @(
    @{ Channel = 'C'; Maximum = 14; Minimum = -16; Octave = 5 },
    @{ Channel = 'A'; Maximum = 28; Minimum = -31; Octave = 4 }
)) {
    $channel = $case.Channel
    $midiPath = Join-Path $runDirectory "$channel.mid"
    $mmlPath = Join-Path $runDirectory "$channel.mml"
    [System.IO.File]::WriteAllBytes($midiPath, $midiBytes)
    & $resolvedExecutable "-c:$channel" '-pt0' '-pm128' $midiPath | Out-Null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $mmlPath -PathType Leaf)) {
        throw "Channel $channel conversion failed."
    }

    $mml = [System.IO.File]::ReadAllText($mmlPath)
    $definition = [regex]::Match($mml, '(?m)^\s*@EP\d+\s*=\{([^}]*)\}')
    if (-not $definition.Success) {
        throw "Channel $channel produced no pitch envelope."
    }
    $position = 0
    $positions = @(foreach ($value in [regex]::Matches($definition.Groups[1].Value, '-?\d+')) {
        $position += [int]$value.Value
        $position
    })
    $maximum = ($positions | Measure-Object -Maximum).Maximum
    $minimum = ($positions | Measure-Object -Minimum).Minimum
    if ($maximum -ne $case.Maximum -or $minimum -ne $case.Minimum -or $position -ne 0) {
        throw "Channel $channel pitch envelope mismatch: maximum=$maximum, minimum=$minimum, final=$position."
    }
    if ($mml -notmatch "(?m)^$channel\s+.*\bo$($case.Octave)\b" -or
        $mml -notmatch "(?m)^$channel\s+EP\d+a") {
        throw "Channel $channel output octave correction was not preserved."
    }
}

# MIDIイベントの時刻を自由に指定し、時間精度や大きなベンドも検証する。
function Write-PitchMidi {
    param([string]$Path, [object[]]$Events)
    $track = [System.Collections.Generic.List[byte]]::new()
    foreach ($event in $Events) {
        $delta = [uint32]$event.Delta
        $vlq = [System.Collections.Generic.List[byte]]::new()
        $vlq.Insert(0, [byte]($delta -band 127))
        while (($delta = $delta -shr 7) -gt 0) {
            $vlq.Insert(0, [byte](128 -bor ($delta -band 127)))
        }
        $track.AddRange($vlq)
        $track.AddRange([byte[]]$event.Data)
    }
    $bytes = [System.Collections.Generic.List[byte]]::new()
    $bytes.AddRange([byte[]]@(
        0x4D,0x54,0x68,0x64,0,0,0,6,0,0,0,1,1,0xE0,
        0x4D,0x54,0x72,0x6B
    ))
    foreach ($shift in @(24,16,8,0)) {
        $bytes.Add([byte](($track.Count -shr $shift) -band 255))
    }
    $bytes.AddRange($track)
    [System.IO.File]::WriteAllBytes($Path, $bytes.ToArray())
}

function Convert-PitchMidi {
    param([string]$Name, [string]$Channel, [object[]]$Events)
    $path = Join-Path $runDirectory "$Name.mid"
    Write-PitchMidi -Path $path -Events $Events
    $messages = & $resolvedExecutable "-c:$Channel" '-pt0' '-pm128' $path 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { throw "Conversion failed: $Name`n$messages" }
    $mml = [System.IO.File]::ReadAllText([System.IO.Path]::ChangeExtension($path, '.mml'))
    $match = [regex]::Match($mml, '(?m)^\s*@EP\d+\s*=\{([^}]*)\}')
    $values = @([regex]::Matches($match.Groups[1].Value, '-?\d+') | ForEach-Object { [int]$_.Value })
    $position = 0
    $positions = @(foreach ($value in $values) {
        if ($value -lt -127 -or $value -gt 126) { throw "Invalid EP value: $Name" }
        $position += $value
        $position
    })
    return @{ Mml = $mml; Values = $values; Positions = $positions; Messages = $messages }
}

function Assert-PitchPosition {
    param($Result, [int]$Frame, [int]$Expected, [string]$Name)
    $positions = $Result.Positions
    $actual = if ($positions.Count -eq 0) { 0 } else { $positions[[math]::Min($Frame, $positions.Count - 1)] }
    if ($actual -ne $Expected) { throw "$Name frame $Frame expected $Expected, got $actual." }
}

# ドライバのAの基準値と物理的な周波数比で、全26チャンネル・3オクターブを検証する。
foreach ($channel in 'ABCabMNOGHIJKLFPQRSTUVWXYZ'.ToCharArray()) {
    foreach ($key in @(57,69,81)) {
        $octave = [int][math]::Floor($key / 12) - 1
        $direct = 'FGHIJKLPQRSTUVW'.Contains($channel)
        $scale = if ('PQRSTUVW'.Contains($channel)) { 128 } else { 1 }
        $base = switch ($channel) {
            'C' { 0x3F9 -shr ($octave - 1) }
            { 'ABab'.Contains($_) } { 0x3F9 -shr ($octave - 2) }
            { 'MN'.Contains($_) } { 0x7F2 -shr ($octave - 1) }
            'O' { 0x914 -shr ($octave - 1) }
            { 'XYZ'.Contains($_) } { 0x7F2 -shr $octave }
            'F' { 0x101C -shr (6 - $octave) }
            { 'GHIJKL'.Contains($_) } { 0x121 }
            { 'PQRSTUVW'.Contains($_) } { 0x3C6B0 -shr (7 - $octave) }
        }
        $bias = if ($direct -or 'XYZ'.Contains($channel)) { 0 } else { 1 }
        $expected = @(foreach ($semitones in @(2,-2)) {
            $ratio = [math]::Pow(2, $semitones / 12.0)
            $offset = if ($direct) { $base * ($ratio - 1) } else { ($base + $bias) * (1 - 1/$ratio) }
            [int][math]::Round($offset / $scale, [MidpointRounding]::AwayFromZero)
        })
        $name = "source-$([int]$channel)-$key"
        $result = Convert-PitchMidi $name "$channel" @(
            @{ Delta=0; Data=@(0x90,$key,127) },
            @{ Delta=240; Data=@(0xE0,127,127) },
            @{ Delta=240; Data=@(0xE0,0,64) },
            @{ Delta=240; Data=@(0xE0,0,0) },
            @{ Delta=240; Data=@(0xE0,0,64) },
            @{ Delta=480; Data=@(0x80,$key,0) },
            @{ Delta=0; Data=@(0xFF,0x2F,0) }
        )
        Assert-PitchPosition $result 20 $expected[0] $name
        Assert-PitchPosition $result 35 0 $name
        Assert-PitchPosition $result 50 $expected[1] $name
        Assert-PitchPosition $result 65 0 $name
    }
}

# 12半音範囲の中間位置を指数計算し、全音源種別で確認する。
foreach ($channel in 'ACMOGFPX'.ToCharArray()) {
    $result = Convert-PitchMidi "midpoint-$channel" "$channel" @(
        @{ Delta=0; Data=@(0xB0,101,0) }, @{ Delta=0; Data=@(0xB0,100,0) },
        @{ Delta=0; Data=@(0xB0,6,12) }, @{ Delta=0; Data=@(0x90,69,127) },
        @{ Delta=240; Data=@(0xE0,0,96) }, @{ Delta=240; Data=@(0xE0,0,64) },
        @{ Delta=480; Data=@(0x80,69,0) }, @{ Delta=0; Data=@(0xFF,0x2F,0) }
    )
    $ratio = [math]::Pow(2, (12.0 * 4096 / 8191) / 12)
    $base = switch ($channel) { 'A' {254} 'C' {127} 'M' {254} 'O' {290} 'G' {289} 'F' {1031} 'P' {30934} 'X' {127} }
    $offset = if ('GFP'.Contains($channel)) { $base * ($ratio-1) } else {
        ($base + $(if ($channel -eq 'X') {0} else {1})) * (1-1/$ratio)
    }
    $scale = if ($channel -eq 'P') {128} else {1}
    Assert-PitchPosition $result 20 ([int][math]::Round($offset/$scale, [MidpointRounding]::AwayFromZero)) "midpoint-$channel"
    Assert-PitchPosition $result 35 0 "midpoint-$channel"
}

# 分解能32でも、20ms後のベンドをフレーム1へ保持する。
$result = Convert-PitchMidi 'timing' 'A' @(
    @{ Delta=0; Data=@(0x90,69,127) }, @{ Delta=20; Data=@(0xE0,127,127) },
    @{ Delta=460; Data=@(0xE0,0,64) }, @{ Delta=480; Data=@(0x80,69,0) },
    @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
Assert-PitchPosition $result 0 0 'timing'
Assert-PitchPosition $result 1 28 'timing'

# 同一フレームでは最後のベンドが優先される。
$result = Convert-PitchMidi 'same-frame' 'A' @(
    @{ Delta=0; Data=@(0x90,69,127) }, @{ Delta=240; Data=@(0xE0,127,127) },
    @{ Delta=1; Data=@(0xE0,0,64) }, @{ Delta=719; Data=@(0x80,69,0) },
    @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
if ($result.Values.Count -ne 0) { throw 'Same-frame final bend was not applied.' }

# 大きな差分の分割中に新しい目標が来ても、未反映の差分を失わない。
$result = Convert-PitchMidi 'spill' 'A' @(
    @{ Delta=0; Data=@(0xB0,101,0) }, @{ Delta=0; Data=@(0xB0,100,0) },
    @{ Delta=0; Data=@(0xB0,6,12) }, @{ Delta=0; Data=@(0x90,33,127) },
    @{ Delta=240; Data=@(0xE0,127,127) }, @{ Delta=16; Data=@(0xE0,0,96) },
    @{ Delta=464; Data=@(0xE0,0,64) }, @{ Delta=720; Data=@(0x80,33,0) },
    @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
$expected = [int][math]::Round(2035*(1-1/[math]::Pow(2,4096.0/8191)), [MidpointRounding]::AwayFromZero)
Assert-PitchPosition $result 25 $expected 'spill'
Assert-PitchPosition $result 55 0 'spill'

# 旧511フレーム制限を超えた位置のベンドも保持する。
$result = Convert-PitchMidi 'long-note' 'A' @(
    @{ Delta=0; Data=@(0x90,69,127) }, @{ Delta=8640; Data=@(0xE0,127,127) },
    @{ Delta=960; Data=@(0x80,69,0) }, @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
Assert-PitchPosition $result 539 0 'long-note'
Assert-PitchPosition $result 540 28 'long-note'

# RPNのセント指定を反映し、未選択のデータエントリーやNRPNは無視する。
foreach ($case in @(
    @{ Name='cents'; Controls=@(@(101,0),@(100,0),@(6,2),@(38,50)); Range=2.5 },
    @{ Name='unselected'; Controls=@(@(6,12)); Range=2.0 },
    @{ Name='nrpn'; Controls=@(@(101,0),@(100,0),@(99,0),@(6,12)); Range=2.0 }
)) {
    $events = @(foreach ($cc in $case.Controls) { @{ Delta=0; Data=@(0xB0,$cc[0],$cc[1]) } })
    $events += @(
        @{ Delta=0; Data=@(0x90,69,127) }, @{ Delta=240; Data=@(0xE0,127,127) },
        @{ Delta=240; Data=@(0xE0,0,64) }, @{ Delta=480; Data=@(0x80,69,0) },
        @{ Delta=0; Data=@(0xFF,0x2F,0) }
    )
    $result = Convert-PitchMidi $case.Name 'A' $events
    $expected = [int][math]::Round(255*(1-1/[math]::Pow(2,$case.Range/12)), [MidpointRounding]::AwayFromZero)
    Assert-PitchPosition $result 15 $expected $case.Name
}

# VRC7がブロックをまたぐベンドでも、F-numberを9bit内に制限して警告する。
$result = Convert-PitchMidi 'vrc7-limit' 'G' @(
    @{ Delta=0; Data=@(0xB0,101,0) }, @{ Delta=0; Data=@(0xB0,100,0) },
    @{ Delta=0; Data=@(0xB0,6,12) }, @{ Delta=0; Data=@(0x90,71,127) },
    @{ Delta=240; Data=@(0xE0,127,127) }, @{ Delta=240; Data=@(0x80,71,0) },
    @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
Assert-PitchPosition $result 20 (511-325) 'vrc7-limit'
if ($result.Messages -notmatch 'レジスタ範囲') { throw 'Missing VRC7 range warning.' }

# 発音中のRPN変更とコントローラリセットを、次のベンドを待たずに反映する。
$result = Convert-PitchMidi 'range-change' 'A' @(
    @{ Delta=0; Data=@(0xE0,0,96) }, @{ Delta=0; Data=@(0x90,69,127) },
    @{ Delta=240; Data=@(0xB0,101,0) }, @{ Delta=0; Data=@(0xB0,100,0) },
    @{ Delta=0; Data=@(0xB0,6,12) }, @{ Delta=240; Data=@(0xB0,121,0) },
    @{ Delta=240; Data=@(0xE0,127,127) }, @{ Delta=480; Data=@(0x80,69,0) },
    @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
Assert-PitchPosition $result 0 14 'range-change'
Assert-PitchPosition $result 15 75 'range-change'
Assert-PitchPosition $result 30 0 'range-change'
Assert-PitchPosition $result 50 128 'range-change'

# 大きいN106ベンドでも18bitレジスタを越えず、SA7単位で安全に制限する。
$result = Convert-PitchMidi 'n106-limit' 'P' @(
    @{ Delta=0; Data=@(0xB0,101,0) }, @{ Delta=0; Data=@(0xB0,100,0) },
    @{ Delta=0; Data=@(0xB0,6,127) }, @{ Delta=0; Data=@(0x90,69,127) },
    @{ Delta=240; Data=@(0xE0,127,127) }, @{ Delta=960; Data=@(0xE0,0,0) },
    @{ Delta=960; Data=@(0xE0,0,64) }, @{ Delta=480; Data=@(0x80,69,0) },
    @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
Assert-PitchPosition $result 40 ([int][math]::Floor((262143-30934)/128)) 'n106-limit'
Assert-PitchPosition $result 100 (-[int][math]::Floor(30934/128)) 'n106-limit'
Assert-PitchPosition $result 140 0 'n106-limit'
if ($result.Messages -notmatch 'レジスタ範囲') { throw 'Missing N106 range warning.' }

# ppmck定義表の上限を超えるベンドは、無言で欠落させず警告する。
$result = Convert-PitchMidi 'length-limit' 'A' @(
    @{ Delta=0; Data=@(0x90,69,127) }, @{ Delta=18000; Data=@(0xE0,127,127) },
    @{ Delta=1200; Data=@(0x80,69,0) }, @{ Delta=0; Data=@(0xFF,0x2F,0) }
)
if ($result.Messages -notmatch '長さ上限') { throw 'Missing EP length warning.' }

# 閾値はRPNを反映したセント数で判定し、音源・ノート番号のレジスタ幅に依存しない。
foreach ($channel in 'ABCFGHIJKLMNOPQRSTUVWXYZab'.ToCharArray()) {
    foreach ($key in @(45, 69, 81)) {
        foreach ($cents in @(4, 5, 6)) {
            $name = "cent-threshold-$channel-$key-$cents"
            $path = Join-Path $runDirectory "$name.mid"
            Write-PitchMidi $path @(
                @{ Delta=0; Data=@(0xB0,101,0) }, @{ Delta=0; Data=@(0xB0,100,0) },
                @{ Delta=0; Data=@(0xB0,6,0) }, @{ Delta=0; Data=@(0xB0,38,$cents) },
                @{ Delta=0; Data=@(0x90,$key,100) }, @{ Delta=240; Data=@(0xE0,127,127) },
                @{ Delta=240; Data=@(0xE0,0,64) }, @{ Delta=480; Data=@(0x80,$key,0) },
                @{ Delta=0; Data=@(0xFF,0x2F,0) }
            )
            $registered = @{}
            foreach ($threshold in @(0, 5)) {
                & $resolvedExecutable "-c:$channel" "-pt$threshold" '-pm128' $path | Out-Null
                if ($LASTEXITCODE -ne 0) { throw "Cent threshold conversion failed: $name" }
                $text = [System.IO.File]::ReadAllText([System.IO.Path]::ChangeExtension($path, '.mml'))
                $registered[$threshold] = $text -match '(?m)^\s*@EP\d+\s*='
            }
            # 整数化後に変化が消えた音符は、閾値0でも登録されない。
            $expected = $cents -ge 5 -and $registered[0]
            if ($registered[5] -ne $expected) { throw "Cent threshold mismatch: $name" }
        }
    }
}

# 正負の偏差の幅、一定の初期ベンド、発音中のRPN変更もセント単位で判定する。
foreach ($case in @(
    @{ Name='signed-span'; Cents=4; Expected=$true; Events=@(
        @{ Delta=0; Data=@(0xE0,127,127) }, @{ Delta=0; Data=@(0x90,69,100) },
        @{ Delta=240; Data=@(0xE0,0,0) }, @{ Delta=240; Data=@(0xE0,0,64) }) },
    @{ Name='constant-bend'; Cents=4; Expected=$false; Events=@(
        @{ Delta=0; Data=@(0xE0,127,127) }, @{ Delta=0; Data=@(0x90,69,100) }) },
    @{ Name='constant-bend-at-threshold'; Cents=5; Expected=$true; Events=@(
        @{ Delta=0; Data=@(0xE0,127,127) }, @{ Delta=0; Data=@(0x90,69,100) }) },
    @{ Name='range-change-cents'; Cents=4; Expected=$true; Events=@(
        @{ Delta=0; Data=@(0xE0,127,127) }, @{ Delta=0; Data=@(0x90,69,100) },
        @{ Delta=240; Data=@(0xB0,38,6) }) }
)) {
    $path = Join-Path $runDirectory "$($case.Name).mid"
    $events = @(
        @{ Delta=0; Data=@(0xB0,101,0) }, @{ Delta=0; Data=@(0xB0,100,0) },
        @{ Delta=0; Data=@(0xB0,6,0) }, @{ Delta=0; Data=@(0xB0,38,$case.Cents) }
    ) + $case.Events + @(
        @{ Delta=960; Data=@(0x80,69,0) }, @{ Delta=0; Data=@(0xFF,0x2F,0) }
    )
    Write-PitchMidi $path $events
    & $resolvedExecutable '-c:A' '-pt5' '-pm128' $path | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Cent span conversion failed: $($case.Name)" }
    $text = [System.IO.File]::ReadAllText([System.IO.Path]::ChangeExtension($path, '.mml'))
    if (($text -match '(?m)^\s*@EP\d+\s*=') -ne $case.Expected) {
        throw "Cent span mismatch: $($case.Name)"
    }
}

Write-Host 'All sound-source pitch envelope tests passed.'
