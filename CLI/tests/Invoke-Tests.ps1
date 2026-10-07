[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'

$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$regressionScript = Join-Path $PSScriptRoot 'Invoke-Regression.ps1'
$encodingScript = Join-Path $PSScriptRoot 'Invoke-EncodingTests.ps1'
$malformedMidiScript = Join-Path $PSScriptRoot 'Invoke-MalformedMidiTests.ps1'
$volumeEnvelopeScript = Join-Path $PSScriptRoot 'Invoke-VolumeEnvelopeTests.ps1'
$pitchEnvelopeScript = Join-Path $PSScriptRoot 'Invoke-PitchEnvelopeTests.ps1'
$vrc7ToneScript = Join-Path $PSScriptRoot 'Invoke-Vrc7ToneTests.ps1'
$fdsToneScript = Join-Path $PSScriptRoot 'Invoke-FdsToneTests.ps1'
$definitionCommentScript = Join-Path $PSScriptRoot 'Invoke-DefinitionCommentTests.ps1'

# 既定値と-c/-r明示指定が同じ結果になることを確認する。
& $regressionScript -Executable $resolvedExecutable -SkipGoldenComparison
$defaultMml = Join-Path $PSScriptRoot 'output\DPCM\DPCM.mml'
$defaultMmlHash = (Get-FileHash -LiteralPath $defaultMml -Algorithm SHA256).Hash
& $regressionScript -Executable $resolvedExecutable -ConverterArguments @('-c:ABCMNOabFXYZPQRSTUVWGHIJKL', '-r32') -SkipGoldenComparison
$explicitMmlHash = (Get-FileHash -LiteralPath $defaultMml -Algorithm SHA256).Hash
if ($defaultMmlHash -ne $explicitMmlHash) {
    throw 'Default channel order or resolution differs from the explicit -c/-r order.'
}

# 従来のゴールデンデータは旧チャンネル順を明示して比較する。
& $regressionScript -Executable $resolvedExecutable -ConverterArguments @('-c:GHIJKLABCPQRSTUVWabMNOXYZ', '-r128')
& $regressionScript -Executable $resolvedExecutable -ConverterArguments @(
    '-r128',
    '-vm3',
    '-vt60',
    '-pm15',
    '-pt5',
    '-n25',
    '-m1',
    '-d3',
    '-c:GHIJKLABCPQRSTUVWabMNOXYZ'
)

$smokeOptions = @(
    @('-d1'),
    @('-d2'),
    @('-m0'),
    @('-d1', '-m0'),
    @('-vm0', '-vt65535', '-pm128', '-pt65535', '-n100', '-m65535', '-d3', '-c:F')
)

foreach ($arguments in $smokeOptions) {
    & $regressionScript -Executable $resolvedExecutable -ConverterArguments $arguments -SkipGoldenComparison
}

$invalidArguments = @(
    @(),
    @('-r3'),
    @('-r5'),
    @('-r512'),
    @('-vm'),
    @('-c'),
    @('-dtext'),
    @('missing-input.mid')
)

foreach ($arguments in $invalidArguments) {
    & $resolvedExecutable @arguments
    if ($LASTEXITCODE -eq 0) {
        throw "Invalid command line was unexpectedly accepted: $($arguments -join ' ')"
    }
}

$invalidOptions = @(
    @{ Arguments = @('-');       Reason = 'オプション名が指定されていません' },
    @{ Arguments = @('-c');      Reason = 'チャンネル指定は-c:<チャンネル文字列>の形式で指定してください' },
    @{ Arguments = @('-c:');     Reason = 'チャンネルが指定されていません' },
    @{ Arguments = @('-c:D');    Reason = '未対応のチャンネルが指定されています' },
    @{ Arguments = @('-c:AA');   Reason = '同じチャンネルが重複しています' },
    @{ Arguments = @('-rtext');  Reason = '分解能は整数で指定してください' },
    @{ Arguments = @('-r5');     Reason = '対応していない分解能です' },
    @{ Arguments = @('-vm');     Reason = '音量オプションの値が指定されていません' },
    @{ Arguments = @('-vmtext'); Reason = '音量モードは整数で指定してください' },
    @{ Arguments = @('-vm4');    Reason = '音量モードは0～3で指定してください' },
    @{ Arguments = @('-vttext'); Reason = '音量定義閾値は0～65535の整数で指定してください' },
    @{ Arguments = @('-vx1');    Reason = '音量オプションは-vmまたは-vtで指定してください' },
    @{ Arguments = @('-pm');     Reason = 'ピッチオプションの値が指定されていません' },
    @{ Arguments = @('-pmtext'); Reason = 'ピッチエンベロープ登録数は整数で指定してください' },
    @{ Arguments = @('-pm129');  Reason = 'ピッチエンベロープ登録数は0～128で指定してください' },
    @{ Arguments = @('-pttext'); Reason = 'ピッチエンベロープ閾値は0～65535の整数で指定してください' },
    @{ Arguments = @('-px1');    Reason = 'ピッチオプションは-pmまたは-ptで指定してください' },
    @{ Arguments = @('-ntext');  Reason = '重複音符の切り詰め割合は整数で指定してください' },
    @{ Arguments = @('-n101');   Reason = '重複音符の切り詰め割合は0～100で指定してください' },
    @{ Arguments = @('-mtext');  Reason = 'パーカッション統合値は0～65535の整数で指定してください' },
    @{ Arguments = @('-dtext');  Reason = 'ドラム音源モードは整数で指定してください' },
    @{ Arguments = @('-d0');     Reason = 'ドラム音源モードは1～3で指定してください' },
    @{ Arguments = @('-z1');     Reason = '未対応のオプションです' }
)

foreach ($testCase in $invalidOptions) {
    $arguments = $testCase.Arguments
    $output = & $resolvedExecutable @arguments 2>&1 | Out-String
    if ($LASTEXITCODE -eq 0) {
        throw "Invalid option was unexpectedly accepted: $($arguments -join ' ')"
    }
    if ($output -notmatch [regex]::Escape('コマンドラインオプションが不正です')) {
        throw "Invalid option was rejected for the wrong reason: $($arguments -join ' ')"
    }
    if ($output -notmatch [regex]::Escape($testCase.Reason)) {
        throw "Invalid option reason was not reported: $($arguments -join ' ')"
    }
}

$output = & $resolvedExecutable 'first.mid' 'second.mid' 2>&1 | Out-String
if ($LASTEXITCODE -eq 0) {
    throw 'Multiple MIDI files were unexpectedly accepted.'
}
if ($output -notmatch [regex]::Escape('MIDIファイルを複数指定できません')) {
    throw 'Multiple MIDI files were rejected for the wrong reason.'
}

& $encodingScript -Executable $resolvedExecutable
& $malformedMidiScript -Executable $resolvedExecutable
& $volumeEnvelopeScript -Executable $resolvedExecutable
& $pitchEnvelopeScript -Executable $resolvedExecutable
& $vrc7ToneScript -Executable $resolvedExecutable
& $fdsToneScript -Executable $resolvedExecutable
& $definitionCommentScript -Executable $resolvedExecutable
& (Join-Path $PSScriptRoot 'Invoke-LfoTests.ps1') -Executable $resolvedExecutable

Write-Host 'All converter option tests passed.'
