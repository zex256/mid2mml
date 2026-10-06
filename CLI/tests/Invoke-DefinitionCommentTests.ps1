[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Executable)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = Join-Path $PSScriptRoot ('output\definition-comments\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

# 音色・LFO・音量・複数行FDS定義を生成し、短い定義と長い定義を検査する。
$track = [System.Collections.Generic.List[byte]]::new()
foreach ($program in @(0,1,7,8,38,80,127)) {
    $track.AddRange([byte[]]@(0,0xC0,$program,0,0x90,60,100,96,0x80,60,0))
}
$track.AddRange([byte[]]@(0,0xFF,0x2F,0))
[byte[]]$header = @(0x4D,0x54,0x68,0x64,0,0,0,6,0,0,0,1,0,96,0x4D,0x54,0x72,0x6B)
$shortCount = 0
$longCount = 0
$definitionKinds = [System.Collections.Generic.HashSet[string]]::new()
foreach ($channel in 'ABabMNFGOP'.ToCharArray()) {
    $midiPath = Join-Path $runDirectory "$channel.mid"
    [System.IO.File]::WriteAllBytes($midiPath, [byte[]]($header + @(0,0,0,$track.Count) + $track.ToArray()))
    & $resolvedExecutable "-c:$channel" $midiPath | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Conversion failed: $channel" }
    $mmlPath = [System.IO.Path]::ChangeExtension($midiPath, '.mml')
    foreach ($line in [System.IO.File]::ReadAllLines($mmlPath)) {
        if ($line -match '^(@[A-Za-z]*)\d+') { [void]$definitionKinds.Add($Matches[1]) }
        $comment = $line.IndexOf('// Ch.')
        if ($comment -lt 0) { continue }
        $programField = [regex]::Match($line.Substring($comment), 'PrgNo\.([^\t]+)')
        if (-not $programField.Success -or $programField.Groups[1].Value -cne ([int]$programField.Groups[1].Value).ToString().PadRight(3)) {
            throw "Program number is not left-aligned in three columns: $line"
        }
        $volumeField = [regex]::Match($line.Substring($comment), ' Vol\.([^\t]+)')
        if ($volumeField.Success -and $volumeField.Groups[1].Value -cne ([int]$volumeField.Groups[1].Value).ToString().PadRight(2)) {
            throw "Volume is not left-aligned in two columns: $line"
        }
        $prefix = $line.Substring(0, $comment)
        $code = $prefix.TrimEnd([char[]]@(' ', "`t"))
        $padding = $prefix.Substring($code.Length)
        $width = 0
        foreach ($character in $code.ToCharArray()) {
            $width += if ($character -eq "`t") { 4 - $width % 4 } else { 1 }
        }
        if ($width -ge 80) {
            if ($padding -cne "`t") { throw "Long definition must have exactly one tab: $line" }
            $longCount++
        } else {
            $expected = ''
            while ($width -lt 80) {
                $expected += "`t"
                $width = ([math]::Floor($width / 4) + 1) * 4
            }
            if ($padding -cne $expected) { throw "Comment is not aligned to zero-based column 80 with tabs only: $line" }
            $shortCount++
        }
    }
}

# DPCM定義には既存の打楽器MIDIを使用する。
$dpcmPath = Join-Path $runDirectory 'dpcm.mid'
Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) 'DPCM.mid') -Destination $dpcmPath
& $resolvedExecutable $dpcmPath | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'DPCM conversion failed.' }
foreach ($line in [System.IO.File]::ReadAllLines([System.IO.Path]::ChangeExtension($dpcmPath, '.mml'))) {
    if (-not $line.StartsWith('@DPCM')) { continue }
    $column = 0
    foreach ($character in $line.Substring(0, $line.IndexOf('//')).ToCharArray()) {
        $column += if ($character -eq "`t") { 4 - $column % 4 } else { 1 }
    }
    if ($column -ne 80) { throw 'DPCM comment is not at zero-based column 80.' }
    [void]$definitionKinds.Add('@DPCM')
}
foreach ($kind in @('@','@FM','@OP','@N','@MP','@v','@DPCM')) {
    if (-not $definitionKinds.Contains($kind)) { throw "Definition kind not tested: $kind" }
}
if ($shortCount -eq 0 -or $longCount -eq 0) { throw 'Short and long definition coverage is required.' }
Write-Host "Definition comment tests passed (column 80, four-column tabs, $shortCount short / $longCount long definitions)."
