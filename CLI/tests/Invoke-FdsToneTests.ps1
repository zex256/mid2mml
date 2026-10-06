[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Executable)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = Join-Path $PSScriptRoot ('output\fds-tone\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# Bright Pianoの定義済み波形とCelestaの未定義時の固定波形を確認する。
[byte[]]$track = @(0,0xC0,1,0,0x90,60,100,96,0x80,60,0,
    0,0xC0,8,0,0x90,60,100,96,0x80,60,0,0,0xFF,0x2F,0)
[byte[]]$header = @(0x4D,0x54,0x68,0x64,0,0,0,6,0,0,0,1,0,96,0x4D,0x54,0x72,0x6B)
$midiPath = Join-Path $runDirectory 'fds.mid'
[System.IO.File]::WriteAllBytes($midiPath, [byte[]]($header + @(0,0,0,$track.Length) + $track))
& $resolvedExecutable '-c:F' $midiPath | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'FDS conversion failed.' }
$mml = [System.IO.File]::ReadAllText((Join-Path $runDirectory 'fds.mml'))
$customWave = ' 1, 0, 1, 2, 5,11,15,19,21,20,18,17,18,21,26,33,' +
    '38,45,48,51,49,48,47,48,49,52,58,61,63,61,59,58,' +
    '58,59,61,63,61,56,52,48,44,46,47,44,39,32,25,24,' +
    '25,27,25,20,15,13,14,16,14,10, 6, 3, 2, 4, 5, 4'
# 2桁幅の16値ずつを4行に出力し、コメントを閉じ括弧の後に置く。
$fallbackWave = ((@('63') * 32 + @(' 0') * 32) -join ',')
foreach ($fixture in @(
    @{ Serial = 0; Wave = $customWave; Program = 2; Name = 'Bright Piano' },
    @{ Serial = 1; Wave = $fallbackWave; Program = 9; Name = 'Celesta' }
)) {
    $wave = $fixture.Wave
    $expected = '@FM' + $fixture.Serial + "`t={" + $wave.Substring(0, 48) + "`n" +
        "`t`t  " + $wave.Substring(48, 48) + "`n" +
        "`t`t  " + $wave.Substring(96, 48) + "`n" +
        "`t`t  " + $wave.Substring(144) + "}`t// Ch.F`t`t`tPrgNo." +
        $fixture.Program + "`t" + $fixture.Name + "`n"
    if (-not ($mml -replace "`r", '').Contains($expected)) {
        throw "FDS waveform values, line breaks, indentation or comment are incorrect: $($fixture.Name)"
    }
}
Write-Host 'FDS tone tests passed (defined waveform, trailing comment, fallback waveform and tab indentation).'
