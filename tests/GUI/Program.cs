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
    form.CreateControl();

    // 設定保存先にはテスト専用パスを渡し、実際のユーザー設定を書き換えない。
    Field<TextBox>(form, "_programPath").Text = "converter.exe";
    Field<TextBox>(form, "_midiPath").Text = "example.mid";
    Field<TextBox>(form, "_channels").Text = "CNOP";
    Field<ComboBox>(form, "_resolution").SelectedIndex = 5;
    Field<NumericUpDown>(form, "_trim").Value = 40;
    Field<ComboBox>(form, "_volumeMode").SelectedIndex = 1;
    Field<NumericUpDown>(form, "_volumeThreshold").Value = 120;
    Field<NumericUpDown>(form, "_pitchLimit").Value = 128;
    Field<NumericUpDown>(form, "_pitchThreshold").Value = 7;
    Field<CheckBox>(form, "_mergeDrums").Checked = false;
    Field<ComboBox>(form, "_drumMode").SelectedIndex = 0;
    Call(form, "SaveSettings", path);
    string saved = File.ReadAllText(path);
    Call(form, "_reset_Click", form, EventArgs.Empty);
    Field<TextBox>(form, "_programPath").Text = "changed.exe";
    Field<TextBox>(form, "_midiPath").Text = "changed.mid";
    Call(form, "LoadSettings", path);
    Call(form, "SaveSettings", path);
    Assert(File.ReadAllText(path) == saved, "All saved inputs must round-trip.");
    Assert(!Field<NumericUpDown>(form, "_volumeThreshold").Enabled, "Volume-mode enable state must restore.");
    Assert(Field<TextBox>(form, "_commandPreview").Text.Contains("-pt7"), "Restored values must update the command preview.");

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
    var busyDrag = Drag([midi]);
    Raise(input, "OnDragEnter", busyDrag);
    Assert(busyDrag.Effect == DragDropEffects.None, "Drops must be disabled during conversion.");
    Raise(input, "OnDragDrop", busyDrag);
    Assert(input.Text == smf, "Busy drops must not replace the path.");
    Call(form, "SetBusy", false);
    Console.WriteLine("GUI settings round-trip, failure handling, and wired drag/drop tests passed.");
    Console.WriteLine("Test artifacts: " + directory);
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
