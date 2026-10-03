# mid2mmlGUI

Windows用のMIDI → ppmck MML変換画面です。C# WinFormsで作成し、同じフォルダにある mid2mml.exe を外部プロセスとして呼び出します。

## 使い方

1. 「入力MIDI」の参照ボタンから .mid または .smf を選びます。
2. 必要に応じて変換オプションを変更します。「実行コマンド」は確認用です。
3. 「MMLへ変換」を押します。変換中の出力とエラーは下部に表示されます。
4. 正常終了後、生成した .mml をWindowsで関連付けられたアプリで開きます。

出力先は入力MIDIと同じフォルダです。同名の .mml と中間MIDIは上書きされます。関連付けアプリを起動できなくても、変換結果は残ります。オプションは起動時にCLIの既定値へ戻ります。

## 開発と発行

.NET 10 SDKと、同じソリューションの CLI プロジェクトでビルドした mid2mml.exe が必要です。
Visual Studioではルートの mid2mml.sln を開くと、CLIを先にビルドできます。

GUIプロジェクトを単独でビルドする場合は、先にCLIの対応する構成をビルドし、
GUIフォルダで以下を実行します。発行前にはCLIのRelease構成が必要です。

    dotnet build
    dotnet publish -c Release

発行先の publish フォルダには、mid2mmlGUI.exe と mid2mml.exe の2ファイルを配置します。GUIは自己完結型のwin-x64単一EXEで、利用者側に.NETのインストールは不要です。GUIのEXEは.NETランタイムを含むため、ファイルサイズは大きくなります。

CLIの場所が異なる場合は、発行時に MSBuild プロパティ Mid2mmlCliSource へそのEXEのフルパスを指定できます。開発時のGUI起動でも「変換プログラム」欄から任意の mid2mml.exe を選択できます。

## 画面の項目

画面から -c、-r、-vm、-vt、-pm、-pt、-n、-m、-d を指定できます。チャンネル割当順の既定値は `ABCMNOabFXYZPQRSTUVWGHIJKL`、音符分解能の初期値はCLI・GUIともに `32` です。その他の既定値と許容範囲はCLIの ReadMe.txt に合わせています。音量モード0・1では -vt の入力欄を無効にしますが、その値は保持してCLIへ渡します。

引数はシェルを通さず ProcessStartInfo.ArgumentList に追加します。入力MIDIのフォルダを作業フォルダとしてCLIを起動し、UTF-8の標準出力・標準エラーを順次表示します。中止ボタンを押すと変換プロセスを終了します。
