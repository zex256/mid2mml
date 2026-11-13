[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Executable)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = Join-Path $PSScriptRoot ('output\vrc7-op-control\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# MIDIの可変長整数として、イベント間のtick差を書き込む。
function Add-Delta($Bytes, [int]$Value) {
    $encoded = [System.Collections.Generic.List[byte]]::new()
    $encoded.Insert(0, [byte]($Value -band 127))
    while (($Value = $Value -shr 7) -gt 0) {
        $encoded.Insert(0, [byte](128 -bor ($Value -band 127)))
    }
    $Bytes.AddRange($encoded)
}

# MIDIチャンネルごとに、絶対時刻の音色変更・音符・ループを配置する。
function New-ToneTrack([int]$Channel, [int[]]$Programs, [int[]]$ProgramTimes,
    [int[]]$NoteTimes, [int]$LoopTime = -1) {
    $events = [System.Collections.Generic.List[object]]::new()
    for ($i = 0; $i -lt $Programs.Length; ++$i) {
        $events.Add(@{ Time = $ProgramTimes[$i]; Order = $events.Count;
            Data = [byte[]]@((0xC0 + $Channel), $Programs[$i]) })
    }
    if ($LoopTime -ge 0) {
        $events.Add(@{ Time = $LoopTime; Order = $events.Count; Data = [byte[]]@(0xFF, 6, 1, 0x4C) })
    }
    foreach ($time in $NoteTimes) {
        $events.Add(@{ Time = $time; Order = $events.Count; Data = [byte[]]@((0x90 + $Channel), 60, 100) })
        $events.Add(@{ Time = $time + 48; Order = $events.Count; Data = [byte[]]@((0x80 + $Channel), 60, 0) })
    }
    $events.Add(@{ Time = 480; Order = $events.Count; Data = [byte[]]@(0xFF, 0x2F, 0) })
    $bytes = [System.Collections.Generic.List[byte]]::new()
    $previous = 0
    foreach ($event in ($events | Sort-Object Time, Order)) {
        Add-Delta $bytes ($event.Time - $previous)
        $bytes.AddRange($event.Data)
        $previous = $event.Time
    }
    return ,$bytes.ToArray()
}

# 共通の変換設定で、共有OPの出力と警告を取得する。
function Convert-Fixture([string]$Name, [string]$Channels, [object[]]$Tracks, [int]$Lfo = 1) {
    $bytes = [System.Collections.Generic.List[byte]]::new()
    $bytes.AddRange([byte[]]@(0x4D,0x54,0x68,0x64,0,0,0,6,0,1,0,$Tracks.Count,0,96))
    foreach ($track in $Tracks) {
        $length = $track.Length
        $bytes.AddRange([byte[]]@(0x4D,0x54,0x72,0x6B,
            (($length -shr 24) -band 255), (($length -shr 16) -band 255),
            (($length -shr 8) -band 255), ($length -band 255)))
        $bytes.AddRange([byte[]]$track)
    }
    $path = Join-Path $runDirectory ($Name + '.mid')
    $stderr = Join-Path $runDirectory ($Name + '.stderr')
    [System.IO.File]::WriteAllBytes($path, $bytes.ToArray())
    & $resolvedExecutable "-c:$Channels" '-r32' '-t0' '-n0' "-l$Lfo" $path 2> $stderr | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "OP control conversion failed: $Name" }
    return @{
        Mml = [System.IO.File]::ReadAllText((Join-Path $runDirectory ($Name + '.mml')))
        Warning = [System.IO.File]::ReadAllText($stderr)
    }
}

function Get-ChannelText($Result, [char]$Channel) {
    return (@($Result.Mml -split '\r?\n' | Where-Object { $_ -cmatch "^$Channel\s" }) -join "`n")
}

function Assert-Instruments($Result, [char]$Channel, [string[]]$Expected) {
    $actual = @([regex]::Matches((Get-ChannelText $Result $Channel), '@@\d+') | ForEach-Object { $_.Value })
    if (($actual -join ',') -cne ($Expected -join ',')) {
        throw "Unexpected instrument sequence on ${Channel}: $($actual -join ',')"
    }
}

function Assert-Ops($Result, [char]$Channel, [string[]]$Expected) {
    $actual = @([regex]::Matches((Get-ChannelText $Result $Channel), 'OP\d+') | ForEach-Object { $_.Value })
    if (($actual -join ',') -cne ($Expected -join ',')) {
        throw "Unexpected OP sequence on ${Channel}: $($actual -join ',')"
    }
}

# 同じMIDI音色のままでも、共有OPの変更・保持・復帰に合わせて音色を切り替える。
$owner = New-ToneTrack 0 @(38,39,80,38) @(0,96,192,288) @(0,96,192,288)
$bass1 = New-ToneTrack 1 @(38) @(0) @(0,96,192,288)
$bass2 = New-ToneTrack 2 @(39) @(0) @(0,96,192,288)
foreach ($lfo in @(1,0)) {
    $result = Convert-Fixture "changes-$lfo" 'GHI' @($owner,$bass1,$bass2) $lfo
    Assert-Ops $result 'G' @('OP0','OP1','OP0')
    Assert-Ops $result 'H' @()
    Assert-Ops $result 'I' @()
    Assert-Instruments $result 'H' @('@@0','@@14','@@0')
    Assert-Instruments $result 'I' @('@@3','@@0','@@3')
    if ([regex]::Matches($result.Warning, '共有OPと一致しない').Count -ne 3) {
        throw 'Preset substitution warning count differs.'
    }
    if ($lfo -eq 0 -and $result.Mml -match 'MP\d+') { throw 'Disabled MP was emitted.' }
    if ($lfo -eq 1) {
        foreach ($channel in 'GHI'.ToCharArray()) {
            if ((Get-ChannelText $result $channel) -notmatch '@@\d+(?:OP\d+)?MP\d+') {
                throw "MP missing on $channel"
            }
        }
    }
}

# OP制御担当はG固定ではなく、逆順の割当でも最初の使用チャンネルになる。
$result = Convert-Fixture 'reverse' 'LKJ' @($owner,$bass1,$bass2)
Assert-Ops $result 'L' @('OP0','OP1','OP0')
Assert-Ops $result 'K' @()
Assert-Ops $result 'J' @()
Assert-Instruments $result 'K' @('@@0','@@14','@@0')
Assert-Instruments $result 'J' @('@@3','@@0','@@3')

# 初期設定にOPがなければ、曲中で最も早くOPを使うチャンネルを担当にする。
$late = New-ToneTrack 0 @(80,38) @(0,192) @(0,192,288)
$early = New-ToneTrack 1 @(80,39) @(0,96) @(0,96,192)
$result = Convert-Fixture 'late-first-op' 'GH' @($late,$early)
Assert-Ops $result 'G' @()
Assert-Ops $result 'H' @('OP1')
Assert-Instruments $result 'G' @('@@12','@@14')

# 同じtickの複数プログラム変更では、最後の音色を採用する。
$sameTick = New-ToneTrack 0 @(38,39) @(0,0) @(0,96)
$other = New-ToneTrack 1 @(39) @(0) @(0,96)
$different = New-ToneTrack 2 @(38) @(0) @(0,96)
$result = Convert-Fixture 'same-tick' 'GHI' @($sameTick,$other,$different)
Assert-Ops $result 'G' @('OP0')
Assert-Ops $result 'H' @()
Assert-Instruments $result 'H' @('@@0')
Assert-Instruments $result 'I' @('@@14')

# ループ復帰でも共有OPは担当だけに出力し、他チャンネルは音色選択だけを戻す。
$loopOwner = New-ToneTrack 0 @(38,39) @(0,96) @(0,96) 0
$loopFollower = New-ToneTrack 1 @(38) @(0) @(0,96) 0
$result = Convert-Fixture 'loop' 'GH' @($loopOwner,$loopFollower)
Assert-Ops $result 'G' @('OP0','OP1','OP0')
Assert-Ops $result 'H' @()
Assert-Instruments $result 'H' @('@@0','@@14','@@0')

Write-Host 'VRC7 OP control tests passed (history, preset fallback, fixed MIDI program, same tick, reverse order, late owner, loops and LFO).'
