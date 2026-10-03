# mid2mml

MIDIをppmck向けMMLへ変換するCLIと、Windows用WinForms GUIをまとめたソリューションです。

- [CLI](CLI/ReadMe.txt): 変換プログラム、DPCMデータ、回帰テスト
- [GUI](GUI/README.md): 変換オプションを設定する画面

Visual Studioで `mid2mml.sln` を開き、`Debug|Win32` または `Release|Win32` をビルドしてください。GUIはCLIのビルド後に実行ファイルを自身の出力フォルダへコピーします。CLIはWin32、GUIのRelease発行は自己完結型のwin-x64です。

GUIを配布用の単一ファイルとして発行するときは、CLIのReleaseビルド後に次を実行します。

```powershell
dotnet publish .\GUI\mid2mmlGUI.csproj -c Release
```

出力と使用方法は各フォルダの説明を参照してください。
