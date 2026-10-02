namespace mid2mmlGUI;

/// <summary>WinFormsアプリケーションの起動処理をまとめる。</summary>
internal static class Program
{
  /// <summary>Windows Formsの設定を初期化し、メインフォームを表示する。</summary>
  [STAThread]
  private static void Main()
  {
    ApplicationConfiguration.Initialize();
    Application.Run(new MainForm());
  }
}
