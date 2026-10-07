using System.Reflection;
using System.Text.Json.Nodes;
using mid2mmlGUI;

/// <summary>実際のフォームでドロップ受付と設定保存・復元を確認する、STA検証プログラム。</summary>
internal static class Program
{
  private static readonly BindingFlags PrivateInstance = BindingFlags.Instance | BindingFlags.NonPublic;

  [STAThread]
  private static void Main()
  {
    ApplicationConfiguration.Initialize();
    string directory = Path.Combine(Path.GetTempPath(), "mid2mml-gui-test-" + Guid.NewGuid().ToString("N"));
    Directory.CreateDirectory(directory);
    string path = Path.Combine(directory, "settings.json");
    using var form = new MainForm();
    // 透明な検証フォームでハンドルと実際のレイアウトを作り、利用者の画面には表示しない。
    form.ShowInTaskbar = false;
    form.Opacity = 0;
    form.Show();
    Application.DoEvents();

    // 設定保存先にはテスト専用パスを渡し、実際のユーザー設定を書き換えない。
    Field<TextBox>(form, "_programPath").Text = "converter.exe";
    Field<TextBox>(form, "_midiPath").Text = "example.mid";
    Field<TextBox>(form, "_ppmckBin").Text = @"C:\ppmck\bin";
    Field<TextBox>(form, "_channels").Text = "CNOP";
    Field<ComboBox>(form, "_resolution").SelectedIndex = 5;
    Field<NumericUpDown>(form, "_trim").Value = 40;
    Field<ComboBox>(form, "_volumeMode").SelectedIndex = 1;
    Field<NumericUpDown>(form, "_volumeThreshold").Value = 120;
    Field<NumericUpDown>(form, "_pitchLimit").Value = 128;
    Field<NumericUpDown>(form, "_pitchThreshold").Value = 7;
    Field<CheckBox>(form, "_mergeDrums").Checked = false;
    Field<CheckBox>(form, "_useLfo").Checked = false;
    Assert(Field<TextBox>(form, "_commandPreview").Text.Contains("-l0"), "LFO event must update the preview.");
    Field<ComboBox>(form, "_drumMode").SelectedIndex = 0;
    Call(form, "SaveSettings", path);
    string saved = File.ReadAllText(path);
    Call(form, "_reset_Click", form, EventArgs.Empty);
    Assert(Field<CheckBox>(form, "_useLfo").Checked, "Reset must enable LFO.");
    Field<TextBox>(form, "_programPath").Text = "changed.exe";
    Field<TextBox>(form, "_midiPath").Text = "changed.mid";
    Field<TextBox>(form, "_ppmckBin").Text = @"C:\changed\bin";
    Call(form, "LoadSettings", path);
    Call(form, "SaveSettings", path);
    Assert(File.ReadAllText(path) == saved, "All saved inputs must round-trip.");
    Assert(!Field<NumericUpDown>(form, "_volumeThreshold").Enabled, "Volume-mode enable state must restore.");
    Assert(Field<TextBox>(form, "_commandPreview").Text.Contains("-pt7"), "Restored values must update the command preview.");
    Assert(Field<TextBox>(form, "_commandPreview").Text.Contains(@"C:\ppmck\bin\ppmckc.exe"), "NSF command preview must restore.");
    var oldSettings = JsonNode.Parse(saved)!;
    oldSettings.AsObject().Remove("PpmckBin");
    oldSettings.AsObject().Remove("UseLfo");
    File.WriteAllText(path, oldSettings.ToJsonString());
    Call(form, "LoadSettings", path);
    Assert(Field<TextBox>(form, "_ppmckBin").Text == @"D:\mck\bin", "Old settings must use the designer-compatible bin default.");
    Assert(Field<CheckBox>(form, "_useLfo").Checked, "Old settings must default to LFO enabled.");

    // 不正JSON、範囲外値、非整数値、未対応版、必須キー欠落では画面を部分更新しない。
    foreach (string bad in new[] { "{broken", "{}", Change(saved, "PitchLimit", 129),
        Change(saved, "Trim", 1.5m), Change(saved, "Version", 99) })
    {
      File.WriteAllText(path, bad);
      Call(form, "LoadSettings", path);
      Assert(Field<NumericUpDown>(form, "_pitchLimit").Value == 128, "Invalid settings must not partially restore.");
    }
    Call(form, "LoadSettings", Path.Combine(directory, "missing.json"));
    // 保存先がフォルダなら保存は失敗するが、例外を外へ出さずログへ通知する。
    Call(form, "SaveSettings", directory);
    Assert(Field<RichTextBox>(form, "_log").Text.Contains("設定を保存できませんでした"), "Save failures must be visible.");

    // デザイナーで登録した実イベントを発火し、有効・無効なドロップを確認する。
    string midi = Path.Combine(directory, "test.MID");
    string smf = Path.Combine(directory, "test.smf");
    string text = Path.Combine(directory, "test.txt");
    File.WriteAllBytes(midi, []);
    File.WriteAllBytes(smf, []);
    File.WriteAllText(text, "not midi");
    var input = Field<TextBox>(form, "_midiPath");
    Assert(input.AllowDrop, "Input must allow drops.");
    foreach (string[] files in new[] { new[] { midi }, new[] { smf } })
    {
      var drag = Drag(files);
      Raise(input, "OnDragEnter", drag);
      Assert(drag.Effect == DragDropEffects.Copy, "MIDI file should be accepted.");
      Raise(input, "OnDragDrop", drag);
      Assert(input.Text == files[0], "Dropped MIDI path must appear in the input.");
    }
    foreach (string[] files in new[] { new[] { midi, smf }, new[] { text },
        new[] { directory }, new[] { Path.Combine(directory, "missing.mid") } })
    {
      var drag = Drag(files);
      Raise(input, "OnDragEnter", drag);
      Assert(drag.Effect == DragDropEffects.None, "Unsupported drops must be rejected.");
      string previous = input.Text;
      Raise(input, "OnDragDrop", drag);
      Assert(input.Text == previous, "Rejected drops must not change the input.");
    }
    Call(form, "SetBusy", true);
    Assert(!Field<Button>(form, "_convertNsf").Enabled, "NSF conversion must be disabled while busy.");
    var busyDrag = Drag([midi]);
    Raise(input, "OnDragEnter", busyDrag);
    Assert(busyDrag.Effect == DragDropEffects.None, "Drops must be disabled during conversion.");
    Raise(input, "OnDragDrop", busyDrag);
    Assert(input.Text == smf, "Busy drops must not replace the path.");
    Call(form, "SetBusy", false);
    Assert(Field<Button>(form, "_convertNsf").Enabled, "NSF conversion must be re-enabled after conversion.");
    VerifyLayout(form);
    using (var bitmap = new Bitmap(form.Width, form.Height))
    {
      form.DrawToBitmap(bitmap, new Rectangle(Point.Empty, bitmap.Size));
      bitmap.Save(Path.Combine(directory, "layout-minimum.png"));
    }
    try { VerifyNsfBuild(form, directory); }
    catch
    {
      Console.WriteLine(Field<RichTextBox>(form, "_log").Text);
      throw;
    }
    Console.WriteLine("GUI settings round-trip, failure handling, and wired drag/drop tests passed.");
    Console.WriteLine("Test artifacts: " + directory);
  }

  /// <summary>最小サイズでも追加した入力欄・ボタンが親の領域内に収まることを確認する。</summary>
  private static void VerifyLayout(MainForm form)
  {
    form.Size = form.MinimumSize;
    form.PerformLayout();
    var inputGrid = Field<TableLayoutPanel>(form, "inputGrid");
    var lfoLabel = Field<Label>(form, "lfoLabel");
    Assert(lfoLabel.Text == "LFO(MPコマンド)使用 -l", "LFO label must explain the MP command.");
    Assert(TextRenderer.MeasureText(lfoLabel.Text, lfoLabel.Font).Width <= lfoLabel.ClientSize.Width,
      "LFO label must fit at the minimum form size.");
    Assert(inputGrid.GetRow(Field<TextBox>(form, "_midiPath")) == 0, "MIDI input must be on the first row.");
    Assert(inputGrid.GetRow(Field<TextBox>(form, "_programPath")) == 1, "Converter input must be on the second row.");
    Assert(Field<Label>(form, "midiLabel").Text == "入力MIDIファイル", "MIDI label must use the updated wording.");
    Assert(Field<TextBox>(form, "_midiPath").TabIndex < Field<TextBox>(form, "_programPath").TabIndex,
      "Tab order must follow the visual row order.");
    foreach (string name in new[] { "_ppmckBin", "_browsePpmckBin", "_convertNsf", "_useLfo", "lfoLabel" })
    {
      var control = Field<Control>(form, name);
      Assert(control.Parent!.ClientRectangle.Contains(control.Bounds), "Control must not be clipped: " + name);
    }
  }

  /// <summary>実際のppmckでNSFを生成し、失敗・中止時の既存ファイル保護も確認する。</summary>
  private static void VerifyNsfBuild(MainForm form, string directory)
  {
    const string bin = @"D:\mck\bin";
    Assert(File.Exists(Path.Combine(bin, "ppmckc.exe")), "Real ppmck tools are required for NSF integration tests.");
    string inputDirectory = Path.Combine(directory, "日本語 with spaces");
    Directory.CreateDirectory(inputDirectory);
    string midi = Path.Combine(inputDirectory, "NSF test.mid");
    string mml = Path.ChangeExtension(midi, ".mml");
    string nsf = Path.ChangeExtension(midi, ".nsf");
    File.WriteAllBytes(midi, []);
    const string score = "#TITLE NSF test\nA t120 o4 l4 v8 cdef\n";
    File.WriteAllText(mml, score);
    File.WriteAllText(nsf, "old nsf");
    // 関連付け再生と利用者設定の保存は呼ばず、変換工程のみを実行する。
    string generated = Pump(Build(form, midi, bin, CancellationToken.None));
    Assert(generated == nsf, "NSF must use the MIDI basename and input folder.");
    byte[] bytes = File.ReadAllBytes(nsf);
    Assert(bytes.Length > 128 && bytes.AsSpan(0, 5).SequenceEqual("NESM\x1A"u8), "Real tools must generate an NSF header.");
    Assert(!File.Exists(Path.Combine(inputDirectory, "effect.h")) && !File.Exists(Path.Combine(inputDirectory, "song.h")),
      "Intermediate headers must not pollute the input folder.");

    // DPCMはMMLの隣にあるdmcフォルダからも取得できることを確認する。
    Directory.CreateDirectory(Path.Combine(inputDirectory, "dmc"));
    File.WriteAllBytes(Path.Combine(inputDirectory, "dmc", "test.dmc"), new byte[17]);
    File.WriteAllText(mml, "@DPCM0={\"dmc/test.dmc\",15}\nA t120 o4 l4 v8 c\nE l4 n0\n");
    Pump(Build(form, midi, bin, CancellationToken.None));
    bytes = File.ReadAllBytes(nsf);

    // コンパイル失敗と事前の中止では、前回成功したNSFをそのまま残す。
    File.WriteAllText(mml, "A this_is_invalid_MML\n");
    ExpectFailure(Build(form, midi, bin, CancellationToken.None));
    Assert(File.ReadAllBytes(nsf).SequenceEqual(bytes), "Compiler failure must preserve the previous NSF.");
    File.WriteAllText(mml, score);
    using var cancelled = new CancellationTokenSource();
    cancelled.Cancel();
    ExpectFailure(Build(form, midi, bin, cancelled.Token));
    Assert(File.ReadAllBytes(nsf).SequenceEqual(bytes), "Cancellation must preserve the previous NSF.");
    ExpectFailure(Build(form, midi, Path.Combine(directory, "missing bin"), CancellationToken.None));
    Assert(File.ReadAllBytes(nsf).SequenceEqual(bytes), "Missing tools must preserve the previous NSF.");

    // 壊れたアセンブリを使い、nesasmの終了コードだけを信じず生成物も検査する。
    string brokenRoot = Path.Combine(directory, "broken-ppmck");
    string brokenBin = Path.Combine(brokenRoot, "bin");
    string brokenIncludes = Path.Combine(brokenRoot, "nes_include");
    Directory.CreateDirectory(brokenBin);
    Directory.CreateDirectory(brokenIncludes);
    foreach (string name in new[] { "ppmckc.exe", "nesasm.exe" })
      File.Copy(Path.Combine(bin, name), Path.Combine(brokenBin, name));
    File.WriteAllText(Path.Combine(brokenIncludes, "ppmck.asm"), "\t.invalid_directive\n");
    ExpectFailure(Build(form, midi, brokenBin, CancellationToken.None));
    Assert(File.ReadAllBytes(nsf).SequenceEqual(bytes), "Assembly failure must preserve the previous NSF.");

    // 長時間コマンドを動かしてから中止し、プロセスと両ストリームの終了を待てることを確認する。
    using var runningCancellation = new CancellationTokenSource();
    runningCancellation.CancelAfter(200);
    string ping = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "ping.exe");
    var running = (Task)typeof(MainForm).GetMethod("RunNsfCommandAsync", PrivateInstance)!
      .Invoke(form, [ping, new[] { "-n", "30", "127.0.0.1" }, directory,
        new Dictionary<string, string>(), runningCancellation.Token])!;
    try
    {
      Wait(running);
      throw new InvalidOperationException("The running command must be cancelled.");
    }
    catch (OperationCanceledException) { }
    Assert(typeof(MainForm).GetField("_runningProcess", PrivateInstance)!.GetValue(form) is null,
      "The cancelled process must be released.");
    Console.WriteLine("Real ppmck NSF generation, DPCM lookup, spaces/Japanese paths, failure and cancellation tests passed.");
  }

  /// <summary>実フォームの非公開NSF工程を呼び出す。</summary>
  private static Task<string> Build(MainForm form, string midi, string bin, CancellationToken token) =>
    (Task<string>)typeof(MainForm).GetMethod("BuildNsfAsync", PrivateInstance)!.Invoke(form, [midi, bin, token])!;

  /// <summary>STAのメッセージを処理しながら非同期のGUI工程を待つ。</summary>
  private static string Pump(Task<string> task)
  {
    Wait(task);
    return task.GetAwaiter().GetResult();
  }

  /// <summary>GUIのメッセージを処理しながら工程の終了を待つ。</summary>
  private static void Wait(Task task)
  {
    while (!task.IsCompleted)
    {
      Application.DoEvents();
      Thread.Sleep(5);
    }
    task.GetAwaiter().GetResult();
  }

  /// <summary>検証用の失敗が呼び出し側へ通知されることを確認する。</summary>
  private static void ExpectFailure(Task<string> task)
  {
    try { Pump(task); }
    catch (Exception ex) when (ex is IOException or InvalidOperationException or OperationCanceledException) { return; }
    throw new InvalidOperationException("NSF build was expected to fail.");
  }

  private static T Field<T>(MainForm form, string name) =>
    (T)typeof(MainForm).GetField(name, PrivateInstance)!.GetValue(form)!;

  private static void Call(MainForm form, string name, params object?[] arguments) =>
    typeof(MainForm).GetMethod(name, PrivateInstance)!.Invoke(form, arguments);

  private static void Raise(Control control, string name, DragEventArgs arguments) =>
    typeof(Control).GetMethod(name, PrivateInstance)!.Invoke(control, [arguments]);

  private static DragEventArgs Drag(string[] paths)
  {
    var data = new DataObject();
    data.SetData(DataFormats.FileDrop, paths);
    return new DragEventArgs(data, 0, 0, 0, DragDropEffects.Copy, DragDropEffects.None);
  }

  private static string Change(string saved, string name, JsonNode value)
  {
    var node = JsonNode.Parse(saved)!;
    node[name] = value;
    return node.ToJsonString();
  }

  private static void Assert(bool condition, string message)
  {
    if (!condition)
      throw new InvalidOperationException(message);
  }
}
