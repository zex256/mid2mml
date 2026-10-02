using System.Drawing;
using System.Windows.Forms;

namespace mid2mmlGUI
{
  partial class MainForm
  {
        private void InitializeComponent()
        {
            root = new TableLayoutPanel();
            heading = new Label();
            _inputGroup = new GroupBox();
            inputGrid = new TableLayoutPanel();
            programLabel = new Label();
            _programPath = new TextBox();
            _browseProgram = new Button();
            midiLabel = new Label();
            _midiPath = new TextBox();
            _browseMidi = new Button();
            _optionsGroup = new GroupBox();
            optionColumns = new TableLayoutPanel();
            noteGrid = new TableLayoutPanel();
            channelsLabel = new Label();
            _channels = new TextBox();
            resolutionLabel = new Label();
            _resolution = new InitialSelectionComboBox();
            trimLabel = new Label();
            _trim = new NumericUpDown();
            mergeLabel = new Label();
            _mergeDrums = new CheckBox();
            drumLabel = new Label();
            _drumMode = new InitialSelectionComboBox();
            soundGrid = new TableLayoutPanel();
            volumeModeLabel = new Label();
            _volumeMode = new InitialSelectionComboBox();
            _volumeThresholdLabel = new Label();
            _volumeThreshold = new NumericUpDown();
            pitchLimitLabel = new Label();
            _pitchLimit = new NumericUpDown();
            pitchThresholdLabel = new Label();
            _pitchThreshold = new NumericUpDown();
            commandGroup = new GroupBox();
            _commandPreview = new TextBox();
            actions = new TableLayoutPanel();
            _status = new Label();
            _reset = new Button();
            _cancel = new Button();
            _convert = new Button();
            logGroup = new GroupBox();
            _log = new RichTextBox();
            root.SuspendLayout();
            _inputGroup.SuspendLayout();
            inputGrid.SuspendLayout();
            _optionsGroup.SuspendLayout();
            optionColumns.SuspendLayout();
            noteGrid.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)_trim).BeginInit();
            soundGrid.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)_volumeThreshold).BeginInit();
            ((System.ComponentModel.ISupportInitialize)_pitchLimit).BeginInit();
            ((System.ComponentModel.ISupportInitialize)_pitchThreshold).BeginInit();
            commandGroup.SuspendLayout();
            actions.SuspendLayout();
            logGroup.SuspendLayout();
            SuspendLayout();
            // 
            // root
            // 
            root.ColumnCount = 1;
            root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            root.Controls.Add(heading, 0, 0);
            root.Controls.Add(_inputGroup, 0, 1);
            root.Controls.Add(_optionsGroup, 0, 2);
            root.Controls.Add(commandGroup, 0, 3);
            root.Controls.Add(actions, 0, 4);
            root.Controls.Add(logGroup, 0, 5);
            root.Dock = DockStyle.Fill;
            root.Location = new Point(0, 0);
            root.Name = "root";
            root.Padding = new Padding(12);
            root.RowCount = 6;
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 48F));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 101F));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 217F));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 75F));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 48F));
            root.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            root.Size = new Size(1004, 741);
            root.TabIndex = 0;
            // 
            // heading
            // 
            heading.Dock = DockStyle.Fill;
            heading.Font = new Font("Yu Gothic UI", 17F, FontStyle.Bold);
            heading.ForeColor = Color.FromArgb(34, 61, 87);
            heading.Location = new Point(15, 12);
            heading.Name = "heading";
            heading.Size = new Size(974, 48);
            heading.TabIndex = 0;
            heading.Text = "MIDI → MML (ppmck)";
            heading.TextAlign = ContentAlignment.MiddleLeft;
            // 
            // _inputGroup
            // 
            _inputGroup.Controls.Add(inputGrid);
            _inputGroup.Dock = DockStyle.Fill;
            _inputGroup.Location = new Point(15, 63);
            _inputGroup.Name = "_inputGroup";
            _inputGroup.Padding = new Padding(8, 12, 8, 7);
            _inputGroup.Size = new Size(974, 95);
            _inputGroup.TabIndex = 1;
            _inputGroup.TabStop = false;
            _inputGroup.Text = "入力と変換プログラム";
            // 
            // inputGrid
            // 
            inputGrid.ColumnCount = 3;
            inputGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130F));
            inputGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            inputGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 105F));
            inputGrid.Controls.Add(programLabel, 0, 0);
            inputGrid.Controls.Add(_programPath, 1, 0);
            inputGrid.Controls.Add(_browseProgram, 2, 0);
            inputGrid.Controls.Add(midiLabel, 0, 1);
            inputGrid.Controls.Add(_midiPath, 1, 1);
            inputGrid.Controls.Add(_browseMidi, 2, 1);
            inputGrid.Dock = DockStyle.Fill;
            inputGrid.Location = new Point(8, 28);
            inputGrid.Name = "inputGrid";
            inputGrid.Padding = new Padding(5, 0, 5, 0);
            inputGrid.RowCount = 2;
            inputGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 30F));
            inputGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 30F));
            inputGrid.Size = new Size(958, 60);
            inputGrid.TabIndex = 0;
            // 
            // programLabel
            // 
            programLabel.Dock = DockStyle.Fill;
            programLabel.Location = new Point(8, 0);
            programLabel.Name = "programLabel";
            programLabel.Size = new Size(124, 30);
            programLabel.TabIndex = 0;
            programLabel.Text = "変換プログラム";
            programLabel.TextAlign = ContentAlignment.MiddleLeft;
            // 
            // _programPath
            // 
            _programPath.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _programPath.Location = new Point(138, 3);
            _programPath.Name = "_programPath";
            _programPath.Size = new Size(707, 23);
            _programPath.TabIndex = 1;
            _programPath.Text = "mid2mml.exe";
            _programPath.TextChanged += _programPath_TextChanged;
            // 
            // _browseProgram
            // 
            _browseProgram.Dock = DockStyle.Fill;
            _browseProgram.Location = new Point(851, 3);
            _browseProgram.Name = "_browseProgram";
            _browseProgram.Size = new Size(99, 24);
            _browseProgram.TabIndex = 2;
            _browseProgram.Text = "参照…";
            _browseProgram.Click += _browseProgram_Click;
            // 
            // midiLabel
            // 
            midiLabel.Dock = DockStyle.Fill;
            midiLabel.Location = new Point(8, 30);
            midiLabel.Name = "midiLabel";
            midiLabel.Size = new Size(124, 30);
            midiLabel.TabIndex = 3;
            midiLabel.Text = "入力MIDI";
            midiLabel.TextAlign = ContentAlignment.MiddleLeft;
            // 
            // _midiPath
            // 
            _midiPath.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _midiPath.Location = new Point(138, 33);
            _midiPath.Name = "_midiPath";
            _midiPath.PlaceholderText = "MIDIファイルを選択してください";
            _midiPath.Size = new Size(707, 23);
            _midiPath.TabIndex = 4;
            _midiPath.TextChanged += _midiPath_TextChanged;
            // 
            // _browseMidi
            // 
            _browseMidi.Dock = DockStyle.Fill;
            _browseMidi.Location = new Point(851, 33);
            _browseMidi.Name = "_browseMidi";
            _browseMidi.Size = new Size(99, 24);
            _browseMidi.TabIndex = 5;
            _browseMidi.Text = "参照…";
            _browseMidi.Click += _browseMidi_Click;
            // 
            // _optionsGroup
            // 
            _optionsGroup.Controls.Add(optionColumns);
            _optionsGroup.Dock = DockStyle.Fill;
            _optionsGroup.Location = new Point(15, 164);
            _optionsGroup.Name = "_optionsGroup";
            _optionsGroup.Padding = new Padding(8, 12, 8, 7);
            _optionsGroup.Size = new Size(974, 211);
            _optionsGroup.TabIndex = 2;
            _optionsGroup.TabStop = false;
            _optionsGroup.Text = "変換オプション";
            // 
            // optionColumns
            // 
            optionColumns.ColumnCount = 2;
            optionColumns.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            optionColumns.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            optionColumns.Controls.Add(noteGrid, 0, 0);
            optionColumns.Controls.Add(soundGrid, 1, 0);
            optionColumns.Dock = DockStyle.Fill;
            optionColumns.Location = new Point(8, 28);
            optionColumns.Name = "optionColumns";
            optionColumns.RowCount = 1;
            optionColumns.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            optionColumns.Size = new Size(958, 176);
            optionColumns.TabIndex = 0;
            // 
            // noteGrid
            // 
            noteGrid.ColumnCount = 3;
            noteGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 44F));
            noteGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            noteGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 6F));
            noteGrid.Controls.Add(channelsLabel, 0, 0);
            noteGrid.Controls.Add(_channels, 1, 0);
            noteGrid.Controls.Add(resolutionLabel, 0, 1);
            noteGrid.Controls.Add(_resolution, 1, 1);
            noteGrid.Controls.Add(trimLabel, 0, 2);
            noteGrid.Controls.Add(_trim, 1, 2);
            noteGrid.Controls.Add(mergeLabel, 0, 3);
            noteGrid.Controls.Add(_mergeDrums, 1, 3);
            noteGrid.Controls.Add(drumLabel, 0, 4);
            noteGrid.Controls.Add(_drumMode, 1, 4);
            noteGrid.Dock = DockStyle.Fill;
            noteGrid.Location = new Point(3, 3);
            noteGrid.Name = "noteGrid";
            noteGrid.Padding = new Padding(4, 0, 5, 0);
            noteGrid.RowCount = 5;
            noteGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            noteGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            noteGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            noteGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            noteGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            noteGrid.Size = new Size(473, 170);
            noteGrid.TabIndex = 0;
            // 
            // channelsLabel
            // 
            channelsLabel.Dock = DockStyle.Fill;
            channelsLabel.Location = new Point(7, 0);
            channelsLabel.Name = "channelsLabel";
            channelsLabel.Size = new Size(198, 34);
            channelsLabel.TabIndex = 0;
            channelsLabel.Text = "チャンネル割当順 -c:";
            channelsLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _channels
            // 
            _channels.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _channels.Location = new Point(211, 5);
            _channels.Name = "_channels";
            _channels.Size = new Size(226, 23);
            _channels.TabIndex = 1;
            _channels.Text = "ABCMNOabFXYZPQRSTUVWGHIJKL";
            _channels.TextChanged += _channels_TextChanged;
            // 
            // resolutionLabel
            // 
            resolutionLabel.Dock = DockStyle.Fill;
            resolutionLabel.Location = new Point(7, 34);
            resolutionLabel.Name = "resolutionLabel";
            resolutionLabel.Size = new Size(198, 34);
            resolutionLabel.TabIndex = 2;
            resolutionLabel.Text = "音符分解能 -r";
            resolutionLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _resolution
            // 
            _resolution.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _resolution.DropDownStyle = ComboBoxStyle.DropDownList;
            _resolution.InitialSelectedIndex = 3;
            _resolution.Items.AddRange(new object[] { "4", "8", "16", "32", "64", "128", "256" });
            _resolution.Location = new Point(211, 39);
            _resolution.Name = "_resolution";
            _resolution.Size = new Size(226, 23);
            _resolution.TabIndex = 3;
            _resolution.SelectedIndexChanged += _resolution_SelectedIndexChanged;
            // 
            // trimLabel
            // 
            trimLabel.Dock = DockStyle.Fill;
            trimLabel.Location = new Point(7, 68);
            trimLabel.Name = "trimLabel";
            trimLabel.Size = new Size(198, 34);
            trimLabel.TabIndex = 4;
            trimLabel.Text = "重複音符の切詰率(%) -n";
            trimLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _trim
            // 
            _trim.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _trim.Location = new Point(211, 73);
            _trim.Name = "_trim";
            _trim.Size = new Size(226, 23);
            _trim.TabIndex = 5;
            _trim.Value = new decimal(new int[] { 25, 0, 0, 0 });
            _trim.ValueChanged += _trim_ValueChanged;
            // 
            // mergeLabel
            // 
            mergeLabel.Dock = DockStyle.Fill;
            mergeLabel.Location = new Point(7, 102);
            mergeLabel.Name = "mergeLabel";
            mergeLabel.Size = new Size(198, 34);
            mergeLabel.TabIndex = 6;
            mergeLabel.Text = "打楽器チャンネル統合 -m";
            mergeLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _mergeDrums
            // 
            _mergeDrums.Checked = true;
            _mergeDrums.CheckState = CheckState.Checked;
            _mergeDrums.Dock = DockStyle.Fill;
            _mergeDrums.Location = new Point(211, 105);
            _mergeDrums.Name = "_mergeDrums";
            _mergeDrums.Size = new Size(226, 28);
            _mergeDrums.TabIndex = 7;
            _mergeDrums.Text = "チャンネル10へ統合";
            _mergeDrums.CheckedChanged += _mergeDrums_CheckedChanged;
            // 
            // drumLabel
            // 
            drumLabel.Dock = DockStyle.Fill;
            drumLabel.Location = new Point(7, 136);
            drumLabel.Name = "drumLabel";
            drumLabel.Size = new Size(198, 34);
            drumLabel.TabIndex = 8;
            drumLabel.Text = "打楽器音源割当モード -d";
            drumLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _drumMode
            // 
            _drumMode.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _drumMode.DropDownStyle = ComboBoxStyle.DropDownList;
            _drumMode.InitialSelectedIndex = 2;
            _drumMode.Items.AddRange(new object[] { "1：すべてノイズ", "2：すべてDPCM", "3：ノート別に振り分け" });
            _drumMode.Location = new Point(211, 141);
            _drumMode.Name = "_drumMode";
            _drumMode.Size = new Size(226, 23);
            _drumMode.TabIndex = 9;
            _drumMode.SelectedIndexChanged += _drumMode_SelectedIndexChanged;
            // 
            // soundGrid
            // 
            soundGrid.ColumnCount = 3;
            soundGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 44F));
            soundGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            soundGrid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 6F));
            soundGrid.Controls.Add(volumeModeLabel, 0, 0);
            soundGrid.Controls.Add(_volumeMode, 1, 0);
            soundGrid.Controls.Add(_volumeThresholdLabel, 0, 1);
            soundGrid.Controls.Add(_volumeThreshold, 1, 1);
            soundGrid.Controls.Add(pitchLimitLabel, 0, 2);
            soundGrid.Controls.Add(_pitchLimit, 1, 2);
            soundGrid.Controls.Add(pitchThresholdLabel, 0, 3);
            soundGrid.Controls.Add(_pitchThreshold, 1, 3);
            soundGrid.Dock = DockStyle.Fill;
            soundGrid.Location = new Point(482, 3);
            soundGrid.Name = "soundGrid";
            soundGrid.Padding = new Padding(4, 0, 5, 0);
            soundGrid.RowCount = 5;
            soundGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            soundGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            soundGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            soundGrid.RowStyles.Add(new RowStyle(SizeType.Absolute, 34F));
            soundGrid.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            soundGrid.Size = new Size(473, 170);
            soundGrid.TabIndex = 1;
            // 
            // volumeModeLabel
            // 
            volumeModeLabel.Dock = DockStyle.Fill;
            volumeModeLabel.Location = new Point(7, 0);
            volumeModeLabel.Name = "volumeModeLabel";
            volumeModeLabel.Size = new Size(198, 34);
            volumeModeLabel.TabIndex = 0;
            volumeModeLabel.Text = "音量モード -vm";
            volumeModeLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _volumeMode
            // 
            _volumeMode.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _volumeMode.DropDownStyle = ComboBoxStyle.DropDownList;
            _volumeMode.InitialSelectedIndex = 3;
            _volumeMode.Items.AddRange(new object[] { "0：固定音量", "1：可変音量", "2：音色別音量", "3：音色別＋可変音量" });
            _volumeMode.Location = new Point(211, 5);
            _volumeMode.Name = "_volumeMode";
            _volumeMode.Size = new Size(226, 23);
            _volumeMode.TabIndex = 1;
            _volumeMode.SelectedIndexChanged += _volumeMode_SelectedIndexChanged;
            // 
            // _volumeThresholdLabel
            // 
            _volumeThresholdLabel.Dock = DockStyle.Fill;
            _volumeThresholdLabel.Location = new Point(7, 34);
            _volumeThresholdLabel.Name = "_volumeThresholdLabel";
            _volumeThresholdLabel.Size = new Size(198, 34);
            _volumeThresholdLabel.TabIndex = 2;
            _volumeThresholdLabel.Text = "音量定義数の間引き閾値 -vt";
            _volumeThresholdLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _volumeThreshold
            // 
            _volumeThreshold.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _volumeThreshold.Location = new Point(211, 39);
            _volumeThreshold.Maximum = new decimal(new int[] { 65535, 0, 0, 0 });
            _volumeThreshold.Name = "_volumeThreshold";
            _volumeThreshold.Size = new Size(226, 23);
            _volumeThreshold.TabIndex = 3;
            _volumeThreshold.Value = new decimal(new int[] { 60, 0, 0, 0 });
            _volumeThreshold.ValueChanged += _volumeThreshold_ValueChanged;
            // 
            // pitchLimitLabel
            // 
            pitchLimitLabel.Dock = DockStyle.Fill;
            pitchLimitLabel.Location = new Point(7, 68);
            pitchLimitLabel.Name = "pitchLimitLabel";
            pitchLimitLabel.Size = new Size(198, 34);
            pitchLimitLabel.TabIndex = 4;
            pitchLimitLabel.Text = "ピッチ最大定義数 -pm";
            pitchLimitLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _pitchLimit
            // 
            _pitchLimit.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _pitchLimit.Location = new Point(211, 73);
            _pitchLimit.Maximum = new decimal(new int[] { 128, 0, 0, 0 });
            _pitchLimit.Name = "_pitchLimit";
            _pitchLimit.Size = new Size(226, 23);
            _pitchLimit.TabIndex = 5;
            _pitchLimit.Value = new decimal(new int[] { 15, 0, 0, 0 });
            _pitchLimit.ValueChanged += _pitchLimit_ValueChanged;
            // 
            // pitchThresholdLabel
            // 
            pitchThresholdLabel.Dock = DockStyle.Fill;
            pitchThresholdLabel.Location = new Point(7, 102);
            pitchThresholdLabel.Name = "pitchThresholdLabel";
            pitchThresholdLabel.Size = new Size(198, 34);
            pitchThresholdLabel.TabIndex = 6;
            pitchThresholdLabel.Text = "ピッチ最低変化量閾値 -pt";
            pitchThresholdLabel.TextAlign = ContentAlignment.MiddleRight;
            // 
            // _pitchThreshold
            // 
            _pitchThreshold.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            _pitchThreshold.Location = new Point(211, 107);
            _pitchThreshold.Maximum = new decimal(new int[] { 65535, 0, 0, 0 });
            _pitchThreshold.Name = "_pitchThreshold";
            _pitchThreshold.Size = new Size(226, 23);
            _pitchThreshold.TabIndex = 7;
            _pitchThreshold.Value = new decimal(new int[] { 5, 0, 0, 0 });
            _pitchThreshold.ValueChanged += _pitchThreshold_ValueChanged;
            // 
            // commandGroup
            // 
            commandGroup.Controls.Add(_commandPreview);
            commandGroup.Dock = DockStyle.Fill;
            commandGroup.Location = new Point(15, 381);
            commandGroup.Name = "commandGroup";
            commandGroup.Padding = new Padding(8, 12, 8, 7);
            commandGroup.Size = new Size(974, 69);
            commandGroup.TabIndex = 3;
            commandGroup.TabStop = false;
            commandGroup.Text = "実行コマンド表示（確認用）";
            // 
            // _commandPreview
            // 
            _commandPreview.Dock = DockStyle.Fill;
            _commandPreview.Location = new Point(8, 28);
            _commandPreview.Multiline = true;
            _commandPreview.Name = "_commandPreview";
            _commandPreview.ReadOnly = true;
            _commandPreview.ScrollBars = ScrollBars.Vertical;
            _commandPreview.Size = new Size(958, 34);
            _commandPreview.TabIndex = 0;
            // 
            // actions
            // 
            actions.ColumnCount = 4;
            actions.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            actions.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130F));
            actions.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 95F));
            actions.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 145F));
            actions.Controls.Add(_status, 0, 0);
            actions.Controls.Add(_reset, 1, 0);
            actions.Controls.Add(_cancel, 2, 0);
            actions.Controls.Add(_convert, 3, 0);
            actions.Dock = DockStyle.Fill;
            actions.Location = new Point(15, 456);
            actions.Name = "actions";
            actions.Padding = new Padding(5);
            actions.RowCount = 1;
            actions.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            actions.Size = new Size(974, 42);
            actions.TabIndex = 4;
            // 
            // _status
            // 
            _status.Dock = DockStyle.Fill;
            _status.Location = new Point(8, 5);
            _status.Name = "_status";
            _status.Size = new Size(588, 32);
            _status.TabIndex = 0;
            _status.Text = "MIDIファイルを選択してください";
            _status.TextAlign = ContentAlignment.MiddleLeft;
            // 
            // _reset
            // 
            _reset.Dock = DockStyle.Fill;
            _reset.Location = new Point(602, 8);
            _reset.Name = "_reset";
            _reset.Size = new Size(124, 26);
            _reset.TabIndex = 1;
            _reset.Text = "既定値に戻す";
            _reset.Click += _reset_Click;
            // 
            // _cancel
            // 
            _cancel.Dock = DockStyle.Fill;
            _cancel.Enabled = false;
            _cancel.Location = new Point(732, 8);
            _cancel.Name = "_cancel";
            _cancel.Size = new Size(89, 26);
            _cancel.TabIndex = 2;
            _cancel.Text = "中止";
            _cancel.Click += _cancel_Click;
            // 
            // _convert
            // 
            _convert.BackColor = Color.FromArgb(37, 139, 90);
            _convert.Dock = DockStyle.Fill;
            _convert.FlatStyle = FlatStyle.Flat;
            _convert.ForeColor = Color.White;
            _convert.Location = new Point(827, 8);
            _convert.Name = "_convert";
            _convert.Size = new Size(139, 26);
            _convert.TabIndex = 3;
            _convert.Text = "MMLへ変換";
            _convert.UseVisualStyleBackColor = false;
            _convert.Click += _convert_Click;
            // 
            // logGroup
            // 
            logGroup.Controls.Add(_log);
            logGroup.Dock = DockStyle.Fill;
            logGroup.Location = new Point(15, 504);
            logGroup.Name = "logGroup";
            logGroup.Padding = new Padding(8, 12, 8, 7);
            logGroup.Size = new Size(974, 222);
            logGroup.TabIndex = 5;
            logGroup.TabStop = false;
            logGroup.Text = "実行結果";
            // 
            // _log
            // 
            _log.BackColor = Color.White;
            _log.DetectUrls = false;
            _log.Dock = DockStyle.Fill;
            _log.Font = new Font("Consolas", 9F);
            _log.Location = new Point(8, 28);
            _log.Name = "_log";
            _log.ReadOnly = true;
            _log.Size = new Size(958, 187);
            _log.TabIndex = 0;
            _log.Text = "";
            _log.WordWrap = false;
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(96F, 96F);
            AutoScaleMode = AutoScaleMode.Dpi;
            ClientSize = new Size(1004, 741);
            Controls.Add(root);
            Font = new Font("Yu Gothic UI", 9F);
            MinimumSize = new Size(860, 710);
            Name = "MainForm";
            StartPosition = FormStartPosition.CenterScreen;
            Text = "mid2mmlGUI";
            root.ResumeLayout(false);
            _inputGroup.ResumeLayout(false);
            inputGrid.ResumeLayout(false);
            inputGrid.PerformLayout();
            _optionsGroup.ResumeLayout(false);
            optionColumns.ResumeLayout(false);
            noteGrid.ResumeLayout(false);
            noteGrid.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)_trim).EndInit();
            soundGrid.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)_volumeThreshold).EndInit();
            ((System.ComponentModel.ISupportInitialize)_pitchLimit).EndInit();
            ((System.ComponentModel.ISupportInitialize)_pitchThreshold).EndInit();
            commandGroup.ResumeLayout(false);
            commandGroup.PerformLayout();
            actions.ResumeLayout(false);
            logGroup.ResumeLayout(false);
            ResumeLayout(false);
        }

        private TableLayoutPanel root;
    private Label heading;
    private GroupBox _inputGroup;
    private TableLayoutPanel inputGrid;
    private Label programLabel;
    private TextBox _programPath;
    private Button _browseProgram;
    private Label midiLabel;
    private TextBox _midiPath;
    private Button _browseMidi;
    private GroupBox _optionsGroup;
    private TableLayoutPanel optionColumns;
    private TableLayoutPanel noteGrid;
    private Label channelsLabel;
    private TextBox _channels;
    private Label resolutionLabel;
    private InitialSelectionComboBox _resolution;
    private Label trimLabel;
    private NumericUpDown _trim;
    private Label mergeLabel;
    private CheckBox _mergeDrums;
    private Label drumLabel;
    private InitialSelectionComboBox _drumMode;
    private TableLayoutPanel soundGrid;
    private Label volumeModeLabel;
    private InitialSelectionComboBox _volumeMode;
    private Label _volumeThresholdLabel;
    private NumericUpDown _volumeThreshold;
    private Label pitchLimitLabel;
    private NumericUpDown _pitchLimit;
    private Label pitchThresholdLabel;
    private NumericUpDown _pitchThreshold;
    private GroupBox commandGroup;
    private TextBox _commandPreview;
    private TableLayoutPanel actions;
    private Label _status;
    private Button _reset;
    private Button _cancel;
    private Button _convert;
    private GroupBox logGroup;
    private RichTextBox _log;
  }
}
