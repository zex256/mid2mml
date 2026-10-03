using System.ComponentModel;
using System.Windows.Forms;

namespace mid2mmlGUI;

/// <summary>初期選択位置をWinFormsデザイナーのプロパティとして保存できるコンボボックス。</summary>
public class InitialSelectionComboBox : ComboBox
{
  private int _initialSelectedIndex = -1;

  /// <summary>項目一覧から起動時・デザイン時に選択する位置。-1は未選択。</summary>
  [Category("Behavior")]
  [DefaultValue(-1)]
  [Description("項目一覧から最初に選択する位置。-1 は未選択。")]
  public int InitialSelectedIndex
  {
    get => _initialSelectedIndex;
    set
    {
      if (value < -1)
        throw new ArgumentOutOfRangeException(nameof(value));

      _initialSelectedIndex = value;
      // 項目が登録済みなら即時反映し、デザイナー上の表示も更新する。
      if (value == -1 || value < Items.Count)
        SelectedIndex = value;
    }
  }

  /// <summary>デザイナーが初期選択値より後に項目を登録した場合も選択を反映する。</summary>
  protected override void OnCreateControl()
  {
    base.OnCreateControl();
    if (SelectedIndex == -1 && _initialSelectedIndex >= 0 && _initialSelectedIndex < Items.Count)
      SelectedIndex = _initialSelectedIndex;
  }
}
