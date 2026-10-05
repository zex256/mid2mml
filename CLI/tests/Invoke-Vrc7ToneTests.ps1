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

Write-Host 'VRC7 tone tests passed (six channels, headers, program changes and loops).'
