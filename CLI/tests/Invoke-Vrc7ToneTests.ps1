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
    if ([regex]::Matches($lines[0], '@@0OP0').Count -ne 1) {
        throw "Unexpected OP command at channel header: $channel"
    }
    $body = ($lines | Select-Object -Skip 1) -join "`n"
    if ($body -notmatch '@@0OP1' -or $body -notmatch '@@0OP0' -or $body -notmatch '@@12') {
        throw "Missing OP command at program change or loop restoration: $channel"
    }

    $preset = Convert-ToneFixture "preset-$channel" "$channel" @(80, 38)
    $presetHeader = @($preset -split '\r?\n' | Where-Object { $_ -cmatch "^$channel\s" })[0]
    if ($presetHeader -notmatch '@@12' -or $presetHeader -match '@@0') {
        throw "LFO was mistaken for a custom tone at channel header: $channel"
    }
    $presetBody = (@($preset -split '\r?\n' | Where-Object { $_ -cmatch "^$channel\s" }) | Select-Object -Skip 1) -join "`n"
    if ($preset -notmatch '@OP0\s*=' -or $presetBody -notmatch '@@0OP0' -or $presetBody -notmatch '@@12') {
        throw "Preset/custom switching or preset loop restoration failed: $channel"
    }
}

# 同時に使用するVRC7チャンネルの初期OPだけを比較し、音色変更・ループの検証は上で維持する。
function Test-SharedInitialTone([string]$Name, [string]$Channels, [byte[]]$Programs,
    [string[]]$ExpectedOps, [int]$Warnings, [int]$Lfo = 1) {
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
    if ([regex]::Matches($warning, '警告: VRC7の初期OP設定').Count -ne $Warnings) {
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
        $custom = $Programs[$index] -ne 80
        if ($header -notmatch $(if ($custom) { '@@0' } else { '@@12' })) {
            throw "Instrument selection was removed: ${Name}/${channel}"
        }
        if ($custom -and $Lfo -eq 1 -and $header -notmatch 'MP\d+') {
            throw "OP suppression removed the channel LFO: ${Name}/${channel}"
        }
        if ($Lfo -eq 0 -and $mml -match 'MP\d+') { throw "Disabled LFO was emitted: $Name" }
    }
}

Test-SharedInitialTone 'same' 'GHIJKL' @(38,38,38,38,38,38) @('OP0','','','','','') 0
Test-SharedInitialTone 'changes' 'GHIJKL' @(38,38,39,39,38,80) @('OP0','','OP1','','OP0','') 2
Test-SharedInitialTone 'preset-first' 'GHI' @(80,38,38) @('','OP0','') 0
Test-SharedInitialTone 'preset-between' 'GHIJ' @(38,80,38,39) @('OP0','','','OP1') 1
Test-SharedInitialTone 'without-g' 'HK' @(38,38) @('OP0','') 0
Test-SharedInitialTone 'presets-only' 'GHIJKL' @(80,80,80,80,80,80) @('','','','','','') 0
Test-SharedInitialTone 'changes-no-lfo' 'GHIJKL' @(38,38,39,39,38,80) @('OP0','','OP1','','OP0','') 2 0

Write-Host 'VRC7 tone tests passed (shared initial OP, warnings, six channels, program changes and loops).'
