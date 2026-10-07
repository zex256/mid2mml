[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Executable)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = Join-Path $PSScriptRoot ('output\leading-silence\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# 4/4拍子・96 ticksで、1小節と1拍の無音の後に音符を置く。
[byte[]]$track = @(0,0xFF,0x58,4,4,2,24,8,0,0xC0,0,
    0x83,0x60,0x90,60,100,96,0x80,60,0,0,0xFF,0x2F,0)
[byte[]]$header = @(0x4D,0x54,0x68,0x64,0,0,0,6,0,0,0,1,0,96,0x4D,0x54,0x72,0x6B)
$midiPath = Join-Path $runDirectory 'silence.mid'
[System.IO.File]::WriteAllBytes($midiPath, [byte[]]($header + @(0,0,0,$track.Length) + $track))
$results = @{}
$hashes = @{}
foreach ($mode in @('default', '-t1', '-t0')) {
    $arguments = @('-c:A')
    if ($mode -ne 'default') { $arguments += $mode }
    & $resolvedExecutable @arguments $midiPath
    if ($LASTEXITCODE -ne 0) { throw "Silence conversion failed: $mode" }
    $results[$mode] = [System.IO.File]::ReadAllText((Join-Path $runDirectory 'silence.mml'))
    $hashes[$mode] = (Get-FileHash -LiteralPath (Join-Path $runDirectory 'silence_.mid')).Hash
}
if ($results['default'] -cne $results['-t1']) { throw 'Default differs from -t1.' }
if ($results['-t1'] -ceq $results['-t0']) { throw '-t0 must preserve leading silence.' }
if ($results['-t1'] -notmatch '(?m)^A rc\r?$' -or $results['-t1'] -match '(?m)^A l1') {
    throw 'Enabled trimming must retain the fractional-bar silence.'
}
if ($results['-t0'] -notmatch '(?m)^A l1' -or $results['-t0'] -notmatch '(?m)^A r\r?\nA r4c4\r?$') {
    throw 'Disabled trimming must retain one full bar and one beat of silence.'
}
if ($hashes['default'] -ne $hashes['-t1'] -or $hashes['default'] -ne $hashes['-t0']) {
    throw 'Silence trimming must not change the intermediate MIDI.'
}
foreach ($option in @('-t', '-t2', '-t-1', '-ttext', '-t256', '-t1junk')) {
    $output = & $resolvedExecutable $option $midiPath 2>&1 | Out-String
    if ($LASTEXITCODE -eq 0 -or -not $output.Contains('曲冒頭の無音区間を切詰は0または1で指定してください')) {
        throw "Invalid silence option was not rejected correctly: $option"
    }
}
Write-Host 'Leading silence tests passed (default, enabled, disabled, intermediate MIDI and invalid values).'
