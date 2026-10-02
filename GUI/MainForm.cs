using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Text;

namespace mid2mmlGUI;

/// <summary>MIDIからMMLへの変換条件を受け取り、外部コマンドの実行結果を表示するフォーム。</summary>
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

    // 入力値と数値範囲はデザイナーで設定し、実行時は説明とプレビューだけを整える。
    ConfigureToolTips();
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
        "ファイルのパスを入力するか、参照ボタンから選択してください。\n" +
        "変換後の .mml と中間MIDIは入力ファイルと同じフォルダに保存され、\n" +
        "同名ファイルがあれば上書きされます。");
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
        "ピッチエンベロープを定義する条件として最低限の変化量を指定します。\n" +
        "ピッチエンベロープの最大値と最小値の差がこの閾値未満なら定義しません。\n" +
        "これは微細な変化では定義しないことにより、定義数を抑制するための機能です。\n" +
        "（ピッチエンベロープ値の範囲を制限するものではありません。）\n" +
        "指定可能な値は0～65535、既定値は5です。\n" +
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
    _commandPreview.Text = QuoteForDisplay(program) + " " + string.Join(" ", arguments);
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
    AppendLog("実行: " + _commandPreview.Text);
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
        AppendLog($"変換失敗（終了コード {process.ExitCode}）");
        SetStatus("変換失敗");
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
      AppendLog("プロセスエラー: " + ex.Message);
      SetStatus("起動または実行に失敗");
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
