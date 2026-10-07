namespace mid2mmlGUI;

/// <summary>変換時に保存する入力パスと各オプション。閾値の単位はセント。</summary>
internal sealed record ConversionSettings
{
  public int Version { get; init; } = 1;
  public required string ProgramPath { get; init; }
  public required string MidiPath { get; init; }
  public string PpmckBin { get; init; } = @"D:\mck\bin";
  public required string Channels { get; init; }
  public required int Resolution { get; init; }
  public required decimal Trim { get; init; }
  public required int VolumeMode { get; init; }
  public required decimal VolumeThreshold { get; init; }
  public required decimal PitchLimit { get; init; }
  public required decimal PitchThreshold { get; init; }
  public required bool MergeDrums { get; init; }
  public required int DrumMode { get; init; }
  public bool UseLfo { get; init; } = true;
  public bool TrimLeadingSilence { get; init; } = true;
}
