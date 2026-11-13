[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = Join-Path $PSScriptRoot ('output\vrc7-tone\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# 単音・音色変更・ループ位置を含むMIDIで、チャンネルごとの音色出力を確認する。
function Convert-ToneFixture([string]$Name, [string]$Channel, [byte[]]$Programs) {
    $track = [System.Collections.Generic.List[byte]]::new()
    foreach ($program in $Programs) {
        $track.AddRange([byte[]]@(0, 0xC0, $program))
        if ($program -eq $Programs[0]) {
            # 最初の音色をループ復帰時に再設定させる。
            $track.AddRange([byte[]]@(0, 0xFF, 0x06, 1, 0x4C))
        }
        $track.AddRange([byte[]]@(0, 0x90, 60, 100, 96, 0x80, 60, 0))
    }
    $track.AddRange([byte[]]@(0, 0xFF, 0x2F, 0))
    [byte[]]$header = @(0x4D,0x54,0x68,0x64,0,0,0,6,0,0,0,1,0,96,0x4D,0x54,0x72,0x6B)
    [byte[]]$length = @(0, 0, 0, $track.Count)
    $midiPath = Join-Path $runDirectory ($Name + '.mid')
    [System.IO.File]::WriteAllBytes($midiPath, [byte[]]($header + $length + $track.ToArray()))
    & $resolvedExecutable "-c:$Channel" $midiPath | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Conversion failed: $Name" }
    return [System.IO.File]::ReadAllText((Join-Path $runDirectory ($Name + '.mml')))
}

foreach ($channel in 'GHIJKL'.ToCharArray()) {
    # Synth Bass 1/2はユーザー音色があり、Lead 1はプリセット音色を使用する。
    $mml = Convert-ToneFixture "custom-$channel" "$channel" @(38, 39, 80)
    $definitions = [regex]::Matches($mml, '(?m)^@OP\d+\s*=')
    if ($definitions.Count -ne 2) { throw "Missing custom tone definitions: $channel" }
    if ($mml -notmatch [regex]::Escape('{$11,$03,$50,$06,$A4,$D4,$44,$41}')) {
        throw "Missing Synth Bass 1 registers: $channel"
    }
    $lines = @($mml -split '\r?\n' | Where-Object { $_ -cmatch "^$channel\s" })
    if ($lines.Count -lt 2 -or $lines[0] -notmatch '@@0') {
        throw "Missing custom instrument selection: $channel"
    }
    if ([regex]::Matches($lines[0], '@@0OP0MP\d+').Count -ne 1) {
        throw "Unexpected OP command at channel header: $channel"
    }
    $body = ($lines | Select-Object -Skip 1) -join "`n"
    if ($body -notmatch '@@0OP1MP\d+' -or $body -notmatch '@@0OP0MP\d+' -or $body -notmatch '@@12MP\d+') {
        throw "Missing OP command at program change or loop restoration: $channel"
    }

    $preset = Convert-ToneFixture "preset-$channel" "$channel" @(80, 38)
    $presetHeader = @($preset -split '\r?\n' | Where-Object { $_ -cmatch "^$channel\s" })[0]
    if ($presetHeader -notmatch '@@12MP\d+' -or $presetHeader -match '@@0') {
        throw "LFO was mistaken for a custom tone at channel header: $channel"
    }
    $presetBody = (@($preset -split '\r?\n' | Where-Object { $_ -cmatch "^$channel\s" }) | Select-Object -Skip 1) -join "`n"
    if ($preset -notmatch '@OP0\s*=' -or $presetBody -notmatch '@@0OP0MP\d+' -or $presetBody -notmatch '@@12MP\d+') {
        throw "Preset/custom switching or preset loop restoration failed: $channel"
    }
}

# 共有OPは担当だけに出力し、他チャンネルは一致時だけユーザー音色を選ぶ。
function Test-SharedInitialTone([string]$Name, [string]$Channels, [byte[]]$Programs,
    [string[]]$ExpectedOps, [int]$Warnings, [int]$Lfo = 1, [string[]]$ExpectedInstruments = @()) {
    $bytes = [System.Collections.Generic.List[byte]]::new()
    $bytes.AddRange([byte[]]@(0x4D,0x54,0x68,0x64,0,0,0,6,0,1,0,$Programs.Length,0,96))
    for ($index = 0; $index -lt $Programs.Length; ++$index) {
        [byte[]]$track = @(0,(0xC0 + $index),$Programs[$index],
            0,(0x90 + $index),60,100,96,(0x80 + $index),60,0,0,0xFF,0x2F,0)
        $bytes.AddRange([byte[]]@(0x4D,0x54,0x72,0x6B,0,0,0,$track.Length))
        $bytes.AddRange($track)
    }
    $midiPath = Join-Path $runDirectory ($Name + '.mid')
    $warningPath = Join-Path $runDirectory ($Name + '.stderr')
    [System.IO.File]::WriteAllBytes($midiPath, $bytes.ToArray())
    & $resolvedExecutable "-c:$Channels" "-l$Lfo" $midiPath 2> $warningPath | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Shared OP conversion failed: $Name" }
    $mml = [System.IO.File]::ReadAllText((Join-Path $runDirectory ($Name + '.mml')))
    $warning = [System.IO.File]::ReadAllText($warningPath)
    if ([regex]::Matches($warning, '警告: VRC7のCh\.').Count -ne $Warnings) {
        throw "Unexpected shared OP warning count: $Name"
    }
    if ($Warnings -gt 0 -and $warning -notmatch '変換を続行します') {
        throw "Warning must explain that conversion continues: $Name"
    }
    for ($index = 0; $index -lt $Channels.Length; ++$index) {
        $channel = $Channels[$index]
        $header = @($mml -split '\r?\n' | Where-Object { $_ -cmatch "^$channel\s" })[0]
        $ops = [regex]::Matches($header, 'OP\d+')
        $expected = $ExpectedOps[$index]
        if (($expected -eq '' -and $ops.Count -ne 0) -or
            ($expected -ne '' -and ($ops.Count -ne 1 -or $ops[0].Value -ne $expected))) {
            throw "Unexpected OP at header ${Name}/${channel}: $header"
        }
        $instrument = if ($ExpectedInstruments.Count) {
            $ExpectedInstruments[$index]
        } elseif ($Programs[$index] -eq 80) { '@@12' } else { '@@0' }
        if ($header -notmatch ([regex]::Escape($instrument) + '(?!\d)')) {
            throw "Instrument selection was removed: ${Name}/${channel}"
        }
        if ($Lfo -eq 1 -and $header -notmatch 'MP\d+') {
            throw "Missing channel LFO: ${Name}/${channel}"
        }
        if ($Lfo -eq 0 -and $mml -match 'MP\d+') { throw "Disabled LFO was emitted: $Name" }
    }
}

Test-SharedInitialTone 'same' 'GHIJKL' @(38,38,38,38,38,38) @('OP0','','','','','') 0
Test-SharedInitialTone 'changes' 'GHIJKL' @(38,38,39,39,38,80) @('OP0','','','','','') 2 -ExpectedInstruments @('@@0','@@0','@@3','@@3','@@0','@@12')
Test-SharedInitialTone 'preset-first' 'GHI' @(80,38,38) @('','OP0','') 0
Test-SharedInitialTone 'preset-between' 'GHIJ' @(38,80,38,39) @('OP0','','','') 1 -ExpectedInstruments @('@@0','@@12','@@0','@@3')
Test-SharedInitialTone 'without-g' 'HK' @(38,38) @('OP0','') 0
Test-SharedInitialTone 'presets-only' 'GHIJKL' @(80,80,80,80,80,80) @('','','','','','') 0
Test-SharedInitialTone 'changes-no-lfo' 'GHIJKL' @(38,38,39,39,38,80) @('OP0','','','','','') 2 0 @('@@0','@@0','@@3','@@3','@@0','@@12')
Test-SharedInitialTone 'presets-only-no-lfo' 'GHIJKL' @(80,80,80,80,80,80) @('','','','','','') 0 0

Write-Host 'VRC7 tone tests passed (shared initial OP, warnings, preset/custom LFO, six channels, program changes and loops).'
