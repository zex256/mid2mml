[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Executable)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = Join-Path $PSScriptRoot ('output\lfo\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# 音色変更とループ復帰でLFOが再出力されるMIDIを作成する。
$track = [System.Collections.Generic.List[byte]]::new()
$track.AddRange([byte[]]@(0, 0xFF, 6, 1, 0x4C))
foreach ($program in @(38, 80, 39)) {
    $track.AddRange([byte[]]@(0, 0xC0, $program, 0, 0xE0, 0, 64,
        0, 0x90, 60, 100, 24, 0xE0, 0, 96, 72, 0x80, 60, 0))
}
$track.AddRange([byte[]]@(0, 0xFF, 0x2F, 0))
[byte[]]$header = @(0x4D,0x54,0x68,0x64,0,0,0,6,0,0,0,1,0,96,0x4D,0x54,0x72,0x6B)
[byte[]]$length = @(0,0,0,$track.Count)
$midiPath = Join-Path $runDirectory 'lfo.mid'
[System.IO.File]::WriteAllBytes($midiPath, [byte[]]($header + $length + $track.ToArray()))

foreach ($channel in 'ABCabFGHIJKLMNOPQRSTUVWXYZ'.ToCharArray()) {
    $results = @{}
    foreach ($mode in @('default', '-l1', '-l0')) {
        $arguments = @("-c:$channel")
        if ($mode -ne 'default') { $arguments += $mode }
        & $resolvedExecutable @arguments $midiPath
        if ($LASTEXITCODE -ne 0) { throw "LFO conversion failed: $channel $mode" }
        $results[$mode] = [System.IO.File]::ReadAllText((Join-Path $runDirectory 'lfo.mml'))
    }
    if ($results['default'] -cne $results['-l1']) {
        throw "Default LFO differs from -l1: $channel"
    }
    if ($results['-l1'] -notmatch '(?m)^@MP\d+' -or $results['-l1'] -notmatch 'MP\d+') {
        throw "Enabled LFO is missing: $channel"
    }
    if ($results['-l0'] -match 'MP\d+|Lfo未登録') {
        throw "Disabled LFO still emits a definition, command or warning: $channel"
    }
    if ($results['-l0'] -notmatch '(?m)^@EP\d+') {
        throw "Disabling LFO removed the pitch-bend envelope: $channel"
    }
    $withoutLfo = $results['-l1'] -replace '(?m)^@MP[^\r\n]*\r?\n', '' -replace 'MP\d+', ''
    if ($withoutLfo -cne $results['-l0']) {
        throw "Disabling LFO changed other MML output: $channel"
    }
}

foreach ($option in @('-l', '-l2', '-l-1', '-ltext', '-l256', '-l1junk')) {
    $output = & $resolvedExecutable $option $midiPath 2>&1 | Out-String
    if ($LASTEXITCODE -eq 0 -or -not $output.Contains('LFO(MPコマンド)使用は0または1で指定してください')) {
        throw "Invalid LFO option was not rejected correctly: $option"
    }
}
Write-Host 'LFO tests passed (default, enabled, disabled, program changes, loops and invalid values).'
