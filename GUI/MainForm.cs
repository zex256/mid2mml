using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Text.Json;

namespace mid2mmlGUI;

/// <summary>MIDIからMML・NSFへの変換条件を受け取り、外部コマンドの実行結果を表示するフォーム。</summary>
public sealed partial class MainForm : Form
{
  private const string DefaultChannels = "ABCMNOabFXYZPQRSTUVWGHIJKL";
  private const string AllowedChannels = "ABCFGHIJKLMNOPQRSTUVWXYZab";
  private static readonly int[] Resolutions = [4, 8, 16, 32, 64, 128, 256];

  private readonly ToolTip _tips = new();
  private CancellationTokenSource? _cancelSource;
  private Process? _runningProcess;
  private bool _busy;
  private bool _closing;

  /// <summary>フォームを初期化し、入力欄の説明と実行コマンドの表示を設定する。</summary>
  public MainForm()
  {
    InitializeComponent();
    // デザイナーでは実行時の設定やファイル参照を行わない。
    if (LicenseManager.UsageMode == LicenseUsageMode.Designtime)
      return;

    // 初回の値と数値範囲はデザイナーで設定し、保存済みの値があれば復元する。
    ConfigureToolTips();
    LoadSettings();
    UpdatePreview();
  }

  /// <summary>変換中にフォームが閉じられたら、外部プロセスも終了させる。</summary>
  protected override void OnFormClosing(FormClosingEventArgs e)
  {
    _closing = true;
    if (_busy)
    {
      _cancelSource?.Cancel();
      StopRunningProcess();
    }
    base.OnFormClosing(e);
  }

  /// <summary>入力欄と各変換オプションの説明をツールチップに設定する。</summary>
  private void ConfigureToolTips()
  {
    _tips.SetToolTip(_programPath,
        "変換に使用する mid2mml.exe を指定します。\n" +
        "ファイル名だけを指定した場合は、GUI実行ファイルと同じフォルダから探します。\n" +
        "別の場所にある場合は、フルパスを入力するか参照ボタンから選択してください。");
    _tips.SetToolTip(_midiPath,
        "変換元のMIDIファイル（.mid または .smf）を指定します。\n" +
        "パスを入力するか、参照ボタン、または1ファイルのドラッグ＆ドロップで選択してください。\n" +
        "変換後の .mml と中間MIDIは入力ファイルと同じフォルダに保存され、\n" +
        "同名ファイルがあれば上書きされます。");
    _tips.SetToolTip(_ppmckBin,
        "ppmckc.exe と nesasm.exe があるbinフォルダを指定します。\n" +
        "親フォルダの nes_include にある ppmck.asm とドライバーを使用します。\n" +
        "DPCMは入力MIDIのフォルダ、またはppmckの songs\\dmc から探します。");
    _tips.SetToolTip(_convertNsf,
        "選択中のMIDIと同名のMMLをNSFへ変換します。先にMMLを生成・保存してください。\n" +
        "成功時だけ同名のNSFを置き換え、関連付けられたプレーヤーで開きます。");
    _tips.SetToolTip(_channels,
        "MIDIメロディートラックから割当てるppmckのMMLチャンネルを順番に指定します。\n" +
        "指定可能チャンネルは ABCFGHIJKLMNOPQRSTUVWXYZab です。\n" +
        "DとEは、MIDIのパーカッション10チャンネルからの変換に使用するため指定できません。\n" +
        "同じチャンネルを重複して指定することはできません。\n" +
        "指定した文字数を超えるメロディートラックはMMLへ変換されません。");
    _tips.SetToolTip(_resolution,
        "変換する音符・休符の分解能を指定します、何分音符まで表現するかを指定します。\n" +
        "指定可能な値は 4、8、16、32、64、128、256 です。\n" +
        "大きい値ほど細かい分解能となり、繊細なタイミングを表現できる反面、\n" +
        "音符の付点やタイ2が増え、記述量が多く読み難い譜面となります。");
    _tips.SetToolTip(_trim,
        "同じMIDIトラックの音符が重複した場合、先の音符の末尾を切り詰めることにより、\n" +
        "重複せずに後の音符を同じトラックへ収める動作をします。\n" +
        "この切り詰める割合を0～100(%)で指定します。既定値は25です。\n" +
        "この割合で切り詰めてもなお重複する場合は、切り詰めるのではなく、\n" +
        "直後に新トラックを生成して、そちらに重複した後の音符を分離します。\n" +
        "0を指定すると音符切り詰めは行わず、重複した音符を全て新トラックへ分離します。\n" +
        "パーカッションチャンネルはこの設定の対象外です。\n" +
        "重複音符により新トラックが生成されたかどうかは、\n" +
        "中間MIDIファイルを出力していますので、そちらを確認して下さい。");
    _tips.SetToolTip(_mergeDrums,
        "MIDIのSysExにより（チャンネル10以外の）パーカッションに設定された\n" +
        "トラックを、チャンネル10へ統合するかを指定します。\n" +
        "指定可能な値は0/1です。0で統合せず、1で統合します。既定値は1です。");
    _tips.SetToolTip(_drumMode,
        "MIDIパーカッション（チャンネル10）をppmck MMLのDチャンネル（ノイズ音源）と\n" +
        "Eチャンネル（DPCM音源）のどちらへ割り当てるか指定します。\n" +
        "　1  全てノイズ音源\n" +
        "　2  全てDPCM音源\n" +
        "　3  ノート番号別の内部割当表によるノイズ音源とDPCM音源へ振り分け（既定値）\n" +
        "　　　これはシンバル系はノイズ音源、ドラム系やその他はDPCM音源に割り当てます。");
    _tips.SetToolTip(_volumeMode,
        "音量の変換方法を指定します。\n" +
        "　0  固定音量\n" +
        "　　　チャンネル先頭でvコマンドを出力し、以降の音量変化は出力しません。\n" +
        "　1  可変音量\n" +
        "　　　音量が変化するたびにv+またはv-コマンドを出力します。\n" +
        "　2  音色別音量\n" +
        "　　　音色と音量に対応する@v定義を使用し、チャンネル先頭と音色変更後に\n" +
        "　　　選択します。音色が同じ間の音量変化には追従しません。\n" +
        "　3  音色別音量＋可変音量（既定値）\n" +
        "　　　音色と音量に対応する@v定義を使用し、音量が変化するたびに\n" +
        "　　　対応する定義へ切り替えます。");
    _tips.SetToolTip(_volumeThreshold,
        "音量エンベロープ定義の数を間引き始める件数の閾値を指定します。\n" +
        "これは音量エンベロープ定義が増え過ぎるのを抑制する為の機能です。\n" +
        "（音量値そのものを制限するものではありません。）\n" +
        "指定可能な値は0～65535、既定値は60です。\n" +
        "0を指定すると最大限に間引く方向に働き、非常に大きい値を指定すると\n" +
        "実質的に間引きを無効にします。\n" +
        "登録数が閾値以上になったときに段階的に間引くため、最終的な登録数を\n" +
        "指定値以下に制限する上限ではありません。\n" +
        "この設定は、-vm2または-vm3で生成する@v定義にだけ影響します。");
    _tips.SetToolTip(_pitchLimit,
        "ピッチエンベロープを定義する最大数を指定します。\n" +
        "これはピッチエンベロープ定義が増え過ぎるのを数で制限する為の機能です。\n" +
        "指定可能な値は0～128、既定値は15です。\n" +
        "上限へ達した後も登録済みの同一定義は再利用しますが、新しい定義は登録せず、\n" +
        "その音符ではピッチエンベロープを無効にします。\n" +
        "0を指定した場合、ピッチエンベロープを定義しません。");
    _tips.SetToolTip(_pitchThreshold,
        "MIDI側の最低音程変化幅をセント単位で指定します。100セント＝1半音です。\n" +
        "RPNのベンド範囲を反映した偏差の最大値－最小値が閾値未満なら定義しません。\n" +
        "基準音程の0セントも含めるため、発音開始時から一定のベンドも判定対象です。\n" +
        "音源やノート番号によらず同じ基準で判定します。\n" +
        "これは微細な変化では定義しないことにより、定義数を抑制するための機能です。\n" +
        "（ピッチエンベロープ値の範囲を制限するものではありません。）\n" +
        "指定可能な値は0～65535セント、既定値は5セントです。\n" +
        "0を指定すると、変化量による登録除外を行いません。");
    // 出力先と上書きの説明は、入力欄のほかラベルと参照ボタンでも確認できるようにする。
    const string outputTip = "入力と同じフォルダに .mml と中間MIDIを保存します。同名ファイルは上書きされます。";
    _tips.SetToolTip(midiLabel, outputTip);
    _tips.SetToolTip(_browseMidi, outputTip);
  }

  /// <summary>各オプションを既定値に戻し、実行コマンドの表示を更新する。</summary>
  private void _reset_Click(object sender, EventArgs e)
  {
    _channels.Text = DefaultChannels;
    _resolution.SelectedIndex = Array.IndexOf(Resolutions, 32);
    _trim.Value = 25;
    _volumeMode.SelectedIndex = 3;
    _volumeThreshold.Value = 60;
    _pitchLimit.Value = 15;
    _pitchThreshold.Value = 5;
    _mergeDrums.Checked = true;
    _drumMode.SelectedIndex = 2;
    UpdatePreview();
  }

  /// <summary>MIDIファイルを選択して入力欄に反映する。</summary>
  private void _browseMidi_Click(object sender, EventArgs e)
  {
    using var dialog = new OpenFileDialog
    {
      Title = "MIDIファイルを選択",
      Filter = "MIDIファイル (*.mid;*.smf)|*.mid;*.smf|すべてのファイル (*.*)|*.*",
      InitialDirectory = ExistingDirectory(_midiPath.Text),
      CheckFileExists = true
    };
    if (dialog.ShowDialog(this) == DialogResult.OK)
      _midiPath.Text = dialog.FileName;
  }

  /// <summary>変換プログラムを選択して入力欄に反映する。</summary>
  private void _browseProgram_Click(object sender, EventArgs e)
  {
    using var dialog = new OpenFileDialog
    {
      Title = "mid2mml.exe を選択",
      Filter = "実行ファイル (*.exe)|*.exe|すべてのファイル (*.*)|*.*",
      InitialDirectory = ExistingDirectory(ResolveProgramPath(_programPath.Text)),
      CheckFileExists = true
    };
    if (dialog.ShowDialog(this) == DialogResult.OK)
      _programPath.Text = dialog.FileName;
  }

  /// <summary>ppmckのbinフォルダを選択して入力欄に反映する。</summary>
  private void _browsePpmckBin_Click(object sender, EventArgs e)
  {
    using var dialog = new FolderBrowserDialog
    {
      Description = "ppmckc.exe と nesasm.exe のあるbinフォルダを選択してください。",
      UseDescriptionForTitle = true,
      SelectedPath = _ppmckBin.Text.Trim().Trim('"')
    };
    if (dialog.ShowDialog(this) == DialogResult.OK)
      _ppmckBin.Text = dialog.SelectedPath;
  }

  /// <summary>ppmckのbin変更を実行予定のコマンド表示に反映する。</summary>
  private void _ppmckBin_TextChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>実行中の変換に中止を要求する。</summary>
  private void _cancel_Click(object sender, EventArgs e)
  {
    SetStatus("中止中…");
    _cancelSource?.Cancel();
  }

  /// <summary>変換プログラムの変更を実行コマンドの表示に反映する。</summary>
  private void _programPath_TextChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>入力MIDIの変更を実行コマンドの表示に反映する。</summary>
  private void _midiPath_TextChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>入力MIDI欄へ既存の.mid/.smfを1つドラッグした場合だけコピーを許可する。</summary>
  private void _midiPath_DragEnter(object sender, DragEventArgs e)
  {
    e.Effect = !_busy && (e.AllowedEffect & DragDropEffects.Copy) != 0
      && DroppedMidiPath(e.Data) is not null ? DragDropEffects.Copy : DragDropEffects.None;
  }

  /// <summary>ドロップされたMIDIを再検査し、入力欄へ反映する。</summary>
  private void _midiPath_DragDrop(object sender, DragEventArgs e)
  {
    if (!_busy && DroppedMidiPath(e.Data) is { } path)
      _midiPath.Text = path;
  }

  /// <summary>複数ファイル、フォルダ、未対応拡張子は受け付けず、有効なMIDIパスを返す。</summary>
  private static string? DroppedMidiPath(IDataObject? data)
  {
    if (data?.GetData(DataFormats.FileDrop) is not string[] { Length: 1 } paths)
      return null;
    string extension = Path.GetExtension(paths[0]);
    return File.Exists(paths[0]) &&
      (extension.Equals(".mid", StringComparison.OrdinalIgnoreCase) ||
       extension.Equals(".smf", StringComparison.OrdinalIgnoreCase)) ? paths[0] : null;
  }

  /// <summary>EXE配置先の書き込み権限に依存しない、ユーザー別の設定保存先。</summary>
  private static string SettingsPath => Path.Combine(
    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
    "mid2mmlGUI", "settings.json");

  /// <summary>保存データ全体を検証してから画面へ復元する。不正な場合は既定値を維持する。</summary>
  private void LoadSettings(string? path = null)
  {
    try
    {
      path ??= SettingsPath;
      if (!File.Exists(path))
        return;
      var settings = JsonSerializer.Deserialize<ConversionSettings>(File.ReadAllText(path));
      if (settings is null || settings.Version != 1 || settings.ProgramPath is null ||
          settings.MidiPath is null || settings.PpmckBin is null || settings.Channels is null ||
          ValidateChannels(settings.Channels) is not null ||
          !Resolutions.Contains(settings.Resolution) ||
          settings.Trim < _trim.Minimum || settings.Trim > _trim.Maximum ||
          settings.VolumeMode < 0 || settings.VolumeMode >= _volumeMode.Items.Count ||
          settings.VolumeThreshold < _volumeThreshold.Minimum || settings.VolumeThreshold > _volumeThreshold.Maximum ||
          settings.PitchLimit < _pitchLimit.Minimum || settings.PitchLimit > _pitchLimit.Maximum ||
          settings.PitchThreshold < _pitchThreshold.Minimum || settings.PitchThreshold > _pitchThreshold.Maximum ||
          decimal.Truncate(settings.Trim) != settings.Trim ||
          decimal.Truncate(settings.VolumeThreshold) != settings.VolumeThreshold ||
          decimal.Truncate(settings.PitchLimit) != settings.PitchLimit ||
          decimal.Truncate(settings.PitchThreshold) != settings.PitchThreshold ||
          settings.DrumMode < 0 || settings.DrumMode >= _drumMode.Items.Count)
        throw new JsonException("保存された設定値が範囲外、または未対応の形式です。");

      // パスが現在存在しなくても復元し、変換時の既存チェックで利用可否を確認する。
      _programPath.Text = settings.ProgramPath;
      _midiPath.Text = settings.MidiPath;
      _ppmckBin.Text = settings.PpmckBin;
      _channels.Text = settings.Channels;
      _resolution.SelectedIndex = Array.IndexOf(Resolutions, settings.Resolution);
      _trim.Value = settings.Trim;
      _volumeMode.SelectedIndex = settings.VolumeMode;
      _volumeThreshold.Value = settings.VolumeThreshold;
      _pitchLimit.Value = settings.PitchLimit;
      _pitchThreshold.Value = settings.PitchThreshold;
      _mergeDrums.Checked = settings.MergeDrums;
      _drumMode.SelectedIndex = settings.DrumMode;
    }
    catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException)
    {
      AppendLog("設定を読み込めませんでした。初期値を使用します: " + ex.Message);
    }
  }

  /// <summary>変換開始時の画面をJSONへ保存し、一時ファイルから置き換えて破損を防ぐ。</summary>
  private void SaveSettings(string? path = null)
  {
    try
    {
      var settings = new ConversionSettings
      {
        ProgramPath = _programPath.Text,
        MidiPath = _midiPath.Text,
        PpmckBin = _ppmckBin.Text,
        Channels = _channels.Text,
        Resolution = _resolution.SelectedIndex < 0 ? 32 : Resolutions[_resolution.SelectedIndex],
        Trim = _trim.Value,
        VolumeMode = _volumeMode.SelectedIndex,
        VolumeThreshold = _volumeThreshold.Value,
        PitchLimit = _pitchLimit.Value,
        PitchThreshold = _pitchThreshold.Value,
        MergeDrums = _mergeDrums.Checked,
        DrumMode = _drumMode.SelectedIndex
      };
      path ??= SettingsPath;
      Directory.CreateDirectory(Path.GetDirectoryName(path)!);
      string temporary = path + ".tmp";
      File.WriteAllText(temporary, JsonSerializer.Serialize(settings,
        new JsonSerializerOptions { WriteIndented = true }));
      File.Move(temporary, path, overwrite: true);
    }
    catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
    {
      // 設定保存が失敗しても変換は続行し、失敗したことをログへ残す。
      AppendLog("設定を保存できませんでした: " + ex.Message);
    }
  }

  /// <summary>チャンネル指定を検証し、結果と実行コマンドの表示を更新する。</summary>
  private void _channels_TextChanged(object sender, EventArgs e)
  {
    string? channelError = ValidateChannels(_channels.Text);
    _tips.SetToolTip(_channels, channelError ?? "使用可能: A～C、F～Z、a、b。D・Eは打楽器用です。重複は指定できません。");
    UpdatePreview();
  }

  /// <summary>分解能の変更を実行コマンドの表示に反映する。</summary>
  private void _resolution_SelectedIndexChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>重複音符の切詰範囲の変更を実行コマンドの表示に反映する。</summary>
  private void _trim_ValueChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>打楽器統合の切り替えを実行コマンドの表示に反映する。</summary>
  private void _mergeDrums_CheckedChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>打楽器の音源選択を実行コマンドの表示に反映する。</summary>
  private void _drumMode_SelectedIndexChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>音量モードに応じて閾値欄の可否を切り替え、実行コマンドを更新する。</summary>
  private void _volumeMode_SelectedIndexChanged(object sender, EventArgs e)
  {
    bool toneVolume = _volumeMode.SelectedIndex >= 2;
    _volumeThreshold.Enabled = toneVolume;
    _volumeThresholdLabel.Enabled = toneVolume;
    UpdatePreview();
  }

  /// <summary>音量定義の閾値変更を実行コマンドの表示に反映する。</summary>
  private void _volumeThreshold_ValueChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>ピッチ登録数の変更を実行コマンドの表示に反映する。</summary>
  private void _pitchLimit_ValueChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>ピッチ変化量の変更を実行コマンドの表示に反映する。</summary>
  private void _pitchThreshold_ValueChanged(object sender, EventArgs e)
  {
    UpdatePreview();
  }

  /// <summary>現在の入力とオプションから、実行予定のコマンド文字列を表示する。</summary>
  private void UpdatePreview()
  {
    string midi = string.IsNullOrWhiteSpace(_midiPath.Text) ? "<入力MIDI>" : _midiPath.Text.Trim();
    string program = string.IsNullOrWhiteSpace(_programPath.Text)
      ? "<mid2mml.exe>" : ResolveProgramPath(_programPath.Text);
    string[] arguments = BuildArguments(midi);
    arguments[^1] = QuoteForDisplay(arguments[^1]);
    string bin = _ppmckBin.Text.Trim().Trim('"');
    string mml = Path.ChangeExtension(midi, ".mml");
    _commandPreview.Text = QuoteForDisplay(program) + " " + string.Join(" ", arguments)
      + Environment.NewLine + QuoteForDisplay(Path.Combine(bin, "ppmckc.exe"))
      + " -i " + QuoteForDisplay(mml) + " song.h"
      + Environment.NewLine + QuoteForDisplay(Path.Combine(bin, "nesasm.exe"))
      + " -s -raw ppmck.asm";
  }

  /// <summary>画面のオプションを、変換プログラムへ渡す引数の配列に変換する。</summary>
  /// <param name="midi">入力MIDIファイルのパス。</param>
  /// <returns>オプションと入力ファイル名を順番に並べた配列。</returns>
  private string[] BuildArguments(string midi)
  {
    int resolution = _resolution.SelectedIndex < 0 ? 32 : Resolutions[_resolution.SelectedIndex];
    return [
      "-c:" + _channels.Text,
      "-r" + resolution,
      "-vm" + Math.Max(0, _volumeMode.SelectedIndex),
      "-vt" + _volumeThreshold.Value,
      "-pm" + _pitchLimit.Value,
      "-pt" + _pitchThreshold.Value,
      "-n" + _trim.Value,
      _mergeDrums.Checked ? "-m1" : "-m0",
      "-d" + Math.Max(1, _drumMode.SelectedIndex + 1),
      midi
    ];
  }

  /// <summary>入力を検証して変換を実行し、結果を表示してMMLを開く。</summary>
  private async void _convert_Click(object sender, EventArgs e)
  {
    if (_busy)
      return;

    // 不正な指定や存在しないファイルでは、外部プロセスを起動しない。
    string? channelError = ValidateChannels(_channels.Text);
    string midi = _midiPath.Text.Trim().Trim('"');
    string program = ResolveProgramPath(_programPath.Text);
    if (channelError is not null || !File.Exists(midi) || !File.Exists(program))
    {
      string message = channelError
        ?? (!File.Exists(midi) ? "入力MIDIファイルを選択してください。" : "変換プログラムが見つかりません。");
      AppendLog(message);
      SetStatus("入力を確認してください");
      return;
    }

    // 入力ファイルのフォルダを作業場所にし、標準出力と標準エラーを受け取る。
    midi = Path.GetFullPath(midi);
    program = Path.GetFullPath(program);
    string mml = Path.ChangeExtension(midi, ".mml");
    var startInfo = new ProcessStartInfo(program)
    {
      WorkingDirectory = Path.GetDirectoryName(midi)!,
      UseShellExecute = false,
      CreateNoWindow = true,
      RedirectStandardOutput = true,
      RedirectStandardError = true,
      StandardOutputEncoding = Encoding.UTF8,
      StandardErrorEncoding = Encoding.UTF8
    };
    foreach (string argument in BuildArguments(midi))
      startInfo.ArgumentList.Add(argument);

    // 実行中は設定変更を禁止し、中止要求に使うトークンとプロセスを保持する。
    _log.Clear();
    SaveSettings();
    AppendLog("実行: " + QuoteForDisplay(program) + " "
      + string.Join(" ", startInfo.ArgumentList.Select(QuoteForDisplay)));
    _cancelSource = new CancellationTokenSource();
    SetBusy(true);
    SetStatus("変換中…");
    using var process = new Process { StartInfo = startInfo };
    _runningProcess = process;
    try
    {
      if (!process.Start())
        throw new InvalidOperationException("変換プログラムを起動できませんでした。");

      // 両方のストリームを並行して読み、出力バッファが詰まるのを防ぐ。
      Task stdout = ShowOutputAsync(process.StandardOutput);
      Task stderr = ShowOutputAsync(process.StandardError);
      try
      {
        await process.WaitForExitAsync(_cancelSource.Token);
      }
      catch (OperationCanceledException)
      {
        // 中止時もプロセス終了と残りの出力を待ってから結果を確定する。
        StopRunningProcess();
        await process.WaitForExitAsync();
        await Task.WhenAll(stdout, stderr);
        AppendLog("変換を中止しました。");
        SetStatus("中止しました");
        return;
      }

      // 終了後に残った出力を表示し、終了コードと生成ファイルを確認する。
      await Task.WhenAll(stdout, stderr);
      if (_cancelSource.IsCancellationRequested)
      {
        AppendLog("変換を中止しました。");
        SetStatus("中止しました");
        return;
      }
      if (process.ExitCode != 0)
      {
        AppendLog($"変換に失敗しました（終了コード {process.ExitCode}）。");
        SetStatus("変換に失敗しました");
        return;
      }
      if (!File.Exists(mml))
      {
        AppendLog("変換プログラムは正常終了しましたが、MMLが見つかりません: " + mml);
        SetStatus("MMLが見つかりません");
        return;
      }

      // 変換済みのMMLは、Windowsで関連付けられたアプリに渡す。
      AppendLog("変換成功。MMLと中間MIDIを入力と同じフォルダに保存しました。");
      try
      {
        using var viewer = Process.Start(new ProcessStartInfo(mml) { UseShellExecute = true });
        AppendLog("関連付けアプリでMMLを開きました: " + mml);
        SetStatus("変換完了・MMLを開きました");
      }
      catch (Exception ex) when (ex is Win32Exception or InvalidOperationException)
      {
        AppendLog("MMLを関連付けアプリで開けませんでした: " + ex.Message);
        SetStatus("変換完了・MMLを開けません");
      }
    }
    catch (Exception ex) when (ex is Win32Exception or IOException or InvalidOperationException or UnauthorizedAccessException)
    {
      AppendLog("プロセスでエラーが発生しました: " + ex.Message);
      SetStatus("起動または実行に失敗しました");
    }
    finally
    {
      // 成否や中止に関係なく、実行状態と操作可能な部品を元に戻す。
      _runningProcess = null;
      _cancelSource.Dispose();
      _cancelSource = null;
      if (!_closing && !IsDisposed)
        SetBusy(false);
    }
  }

  /// <summary>既存のMMLをNSFへ変換し、成功したファイルを関連付けプレーヤーで開く。</summary>
  private async void _convertNsf_Click(object sender, EventArgs e)
  {
    if (_busy)
      return;

    // NSF変換はMMLの保存済み内容を使い、MIDIからの再変換は行わない。
    _log.Clear();
    _cancelSource = new CancellationTokenSource();
    SetBusy(true);
    SetStatus("NSFへ変換中…");
    try
    {
      string midi = Path.GetFullPath(_midiPath.Text.Trim().Trim('"'));
      string bin = Path.GetFullPath(_ppmckBin.Text.Trim().Trim('"'));
      ValidateNsfInputs(midi, bin);
      SaveSettings();
      string nsf = await BuildNsfAsync(midi, bin, _cancelSource.Token);
      AppendLog("NSFを生成しました: " + nsf);

      // 生成とプレーヤー起動の成否を分け、関連付けがなくてもNSFは残す。
      try
      {
        using var player = Process.Start(new ProcessStartInfo(nsf) { UseShellExecute = true });
        AppendLog("関連付けプレーヤーでNSFを開きました: " + nsf);
        SetStatus("NSF変換完了・プレーヤーを起動しました");
      }
      catch (Exception ex) when (ex is Win32Exception or InvalidOperationException)
      {
        AppendLog("NSFは生成しましたが、関連付けプレーヤーを起動できませんでした: " + ex.Message);
        SetStatus("NSF変換完了・プレーヤーを起動できませんでした");
      }
    }
    catch (OperationCanceledException)
    {
      AppendLog("NSF変換を中止しました。既存のNSFは変更していません。");
      SetStatus("NSF変換を中止しました");
    }
    catch (Exception ex) when (ex is Win32Exception or IOException or InvalidOperationException
        or UnauthorizedAccessException or ArgumentException or NotSupportedException)
    {
      AppendLog("NSF変換に失敗しました: " + ex.Message);
      SetStatus("NSF変換に失敗しました");
    }
    finally
    {
      // 失敗・中止・フォーム終了時も、実行状態を確実に解除する。
      _runningProcess = null;
      _cancelSource.Dispose();
      _cancelSource = null;
      if (!_closing && !IsDisposed)
        SetBusy(false);
    }
  }

  /// <summary>MIDIに対応するMMLと、ppmckの実行ファイル・アセンブリを確認する。</summary>
  private static void ValidateNsfInputs(string midi, string bin)
  {
    if (!File.Exists(midi))
      throw new FileNotFoundException("入力MIDIファイルを選択してください。", midi);
    string mml = Path.ChangeExtension(midi, ".mml");
    if (!File.Exists(mml))
      throw new FileNotFoundException("MMLが見つかりません。先にMMLへ変換して保存してください: " + mml);
    foreach (string name in new[] { "ppmckc.exe", "nesasm.exe" })
    {
      if (!File.Exists(Path.Combine(bin, name)))
        throw new FileNotFoundException("ppmckのbinに実行ファイルが見つかりません: " + name);
    }
    string assembly = Path.GetFullPath(Path.Combine(bin, "..", "nes_include", "ppmck.asm"));
    if (!File.Exists(assembly))
      throw new FileNotFoundException("ppmck.asmが見つかりません: " + assembly);
  }

  /// <summary>専用の作業フォルダでコンパイル・アセンブルし、有効なNSFだけを出力先へ配置する。</summary>
  /// <param name="midi">名前と出力先を決める入力MIDIの絶対パス。</param>
  /// <param name="bin">ppmckc.exeとnesasm.exeのあるフォルダの絶対パス。</param>
  /// <param name="token">外部コマンドの停止と、次の工程への進行を中止するトークン。</param>
  /// <returns>生成したNSFの絶対パス。</returns>
  private async Task<string> BuildNsfAsync(string midi, string bin, CancellationToken token)
  {
    ValidateNsfInputs(midi, bin);
    token.ThrowIfCancellationRequested();
    string mml = Path.ChangeExtension(midi, ".mml");
    string nsf = Path.ChangeExtension(midi, ".nsf");
    string inputDirectory = Path.GetDirectoryName(midi)!;
    string baseDirectory = Path.GetFullPath(Path.Combine(bin, ".."));
    string includeDirectory = Path.Combine(baseDirectory, "nes_include");
    string work = Directory.CreateTempSubdirectory("mid2mml-nsf-").FullName;
    string stagedNsf = Path.Combine(inputDirectory, ".mid2mml-" + Guid.NewGuid().ToString("N") + ".nsf.tmp");
    try
    {
      // バッチと同じ探索環境を、この2プロセスだけへ渡す。利用者の環境変数は変更しない。
      var environment = new Dictionary<string, string>
      {
        ["PPMCK_BASEDIR"] = baseDirectory,
        ["NES_INCLUDE"] = includeDirectory,
        ["DMC_INCLUDE"] = string.Join(";", inputDirectory, Path.Combine(baseDirectory, "songs"),
          Path.Combine(inputDirectory, "dmc"), Path.Combine(baseDirectory, "songs", "dmc"))
      };
      File.Copy(Path.Combine(includeDirectory, "ppmck.asm"), Path.Combine(work, "ppmck.asm"));

      // 出力ヘッダー名も明示し、MMLの隣の既存.hやeffect.hを上書きしない。
      await RunNsfCommandAsync(Path.Combine(bin, "ppmckc.exe"), ["-i", mml, "song.h"], work, environment, token);
      if (!File.Exists(Path.Combine(work, "effect.h")) || !File.Exists(Path.Combine(work, "song.h")))
        throw new IOException("ppmckcは終了しましたが、effect.hまたはsong.hが生成されていません。");

      // nesasmはエラー時に終了コード0を返す版もあるため、出力内容も検査する。
      await RunNsfCommandAsync(Path.Combine(bin, "nesasm.exe"), ["-s", "-raw", "ppmck.asm"], work, environment, token);
      string nes = Path.Combine(work, "ppmck.nes");
      using (var stream = File.OpenRead(nes))
      {
        byte[] header = new byte[8];
        if (stream.Length <= 128 || stream.Read(header) != header.Length ||
            !header.AsSpan(0, 5).SequenceEqual("NESM\x1A"u8) || header[5] != 1 || header[6] == 0)
          throw new IOException("ppmck.nesが有効なNSF形式ではないため、既存のNSFは変更しません。");
      }
      token.ThrowIfCancellationRequested();

      // 出力先と同じボリュームで最終置換する。コピー失敗・中止では既存NSFを残す。
      File.Copy(nes, stagedNsf);
      token.ThrowIfCancellationRequested();
      File.Move(stagedNsf, nsf, overwrite: true);
      return nsf;
    }
    finally
    {
      // この呼び出しで作ったファイルだけを片付け、入力や既存の中間生成物には触れない。
      try
      {
        if (File.Exists(stagedNsf))
          File.Delete(stagedNsf);
        Directory.Delete(work, recursive: true);
      }
      catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
      {
        AppendLog("一時ファイルを削除できませんでした: " + work + " / " + ex.Message);
      }
    }
  }

  /// <summary>NSF生成コマンドを起動し、両出力を表示しながら終了・中止を待つ。</summary>
  private async Task RunNsfCommandAsync(string executable, string[] arguments, string work,
      Dictionary<string, string> environment, CancellationToken token)
  {
    token.ThrowIfCancellationRequested();
    // 既存のppmckツールはShift-JISで出力するため、MIDI変換CLIとは別にデコードする。
    Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
    var startInfo = new ProcessStartInfo(executable)
    {
      WorkingDirectory = work,
      UseShellExecute = false,
      CreateNoWindow = true,
      RedirectStandardOutput = true,
      RedirectStandardError = true,
      StandardOutputEncoding = Encoding.GetEncoding(932),
      StandardErrorEncoding = Encoding.GetEncoding(932)
    };
    foreach (string argument in arguments)
      startInfo.ArgumentList.Add(argument);
    foreach (var variable in environment)
      startInfo.Environment[variable.Key] = variable.Value;
    AppendLog("実行: " + QuoteForDisplay(executable) + " " + string.Join(" ", arguments.Select(QuoteForDisplay)));
    using var process = new Process { StartInfo = startInfo };
    _runningProcess = process;
    try
    {
      if (!process.Start())
        throw new InvalidOperationException("コマンドを起動できませんでした: " + executable);
      Task stdout = ShowOutputAsync(process.StandardOutput);
      Task stderr = ShowOutputAsync(process.StandardError);
      try
      {
        await process.WaitForExitAsync(token);
      }
      catch (OperationCanceledException)
      {
        StopRunningProcess();
        await process.WaitForExitAsync();
        await Task.WhenAll(stdout, stderr);
        throw;
      }
      await Task.WhenAll(stdout, stderr);
      token.ThrowIfCancellationRequested();
      if (process.ExitCode != 0)
        throw new InvalidOperationException(Path.GetFileName(executable) +
          $"が失敗しました（終了コード {process.ExitCode}）。");
    }
    finally
    {
      _runningProcess = null;
    }
  }

  /// <summary>外部プロセスの出力を一行ずつ読み、ログ欄へ表示する。</summary>
  /// <param name="reader">標準出力または標準エラーの読み取り元。</param>
  private async Task ShowOutputAsync(StreamReader reader)
  {
    while (await reader.ReadLineAsync() is { } line)
    {
      if (!_closing && !IsDisposed)
        AppendLog(line);
    }
  }

  /// <summary>まだ動いている変換プロセスとその子プロセスを停止する。</summary>
  private void StopRunningProcess()
  {
    try
    {
      if (_runningProcess is { HasExited: false })
        _runningProcess.Kill(entireProcessTree: true);
    }
    catch (InvalidOperationException) { }
    catch (Win32Exception) { }
  }

  /// <summary>変換中かどうかに応じて入力欄と操作ボタンの可否を切り替える。</summary>
  private void SetBusy(bool busy)
  {
    _busy = busy;
    _inputGroup.Enabled = !busy;
    _optionsGroup.Enabled = !busy;
    _reset.Enabled = !busy;
    _convert.Enabled = !busy;
    _convertNsf.Enabled = !busy;
    _convert.Text = busy ? "変換中…" : "MMLへ変換";
    _cancel.Enabled = busy;
  }

  /// <summary>フォームが有効な間だけログ末尾にメッセージを追加する。</summary>
  private void AppendLog(string message)
  {
    if (_closing || IsDisposed)
      return;
    _log.AppendText(message + Environment.NewLine);
    _log.SelectionStart = _log.TextLength;
    _log.ScrollToCaret();
  }

  /// <summary>フォームが有効な間だけ状態表示を更新する。</summary>
  private void SetStatus(string message)
  {
    if (!_closing && !IsDisposed)
      _status.Text = message;
  }

  /// <summary>チャンネルの空指定、不正文字、重複を検査する。</summary>
  /// <param name="channels">画面で指定されたチャンネル順。</param>
  /// <returns>検査エラーの説明。正しい場合は <see langword="null"/>。</returns>
  private static string? ValidateChannels(string channels)
  {
    if (channels.Length == 0)
      return "チャンネルの割り当て順を入力してください。";
    var used = new HashSet<char>();
    foreach (char channel in channels)
    {
      if (!AllowedChannels.Contains(channel))
        return "チャンネルには A～C、F～Z、a、b のみ指定できます。";
      if (!used.Add(channel))
        return $"チャンネル「{channel}」が重複しています。";
    }
    return null;
  }

  /// <summary>指定ファイルの親フォルダを返し、利用できなければドキュメントフォルダを返す。</summary>
  /// <param name="file">入力欄に表示されているファイルパス。</param>
  /// <returns>ファイル選択ダイアログの開始フォルダ。</returns>
  private static string ExistingDirectory(string file)
  {
    string? directory = Path.GetDirectoryName(file.Trim().Trim('"'));
    return directory is not null && Directory.Exists(directory)
      ? directory : Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
  }

  /// <summary>相対指定された変換プログラムをGUI実行ファイルのフォルダから探す。</summary>
  private static string ResolveProgramPath(string program)
    => Path.Combine(AppContext.BaseDirectory, program.Trim().Trim('"'));

  /// <summary>実行コマンドの表示用に文字列を引用符で囲む。</summary>
  private static string QuoteForDisplay(string text) => "\"" + text + "\"";

}
