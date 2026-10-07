#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "midi.h"
#include "mml.h"
#include "text_encoding.h"

using std::array, std::cerr, std::errc, std::from_chars;
using std::ifstream, std::ios, std::make_unique, std::move;
using std::nullopt, std::optional, std::string, std::string_view;
using std::tolower, std::uint16_t, std::uint8_t;

extern const char kToolName[] = "mid2mml for ppmck Ver1.0α";                   ///< ツール名
extern const char kAuthor[] = "ZEX";                                            ///< 著作者名

/** @brief 打楽器音源割当モード */
enum class DrumMode : uint8_t { kNoise = 1, kDpcm = 2, kNoiseAndDpcm = 3 };

/** @brief コマンドラインから得られた変換設定 */
struct ConverterOptions {
  bool use_lfo{true};                                                           ///< LFO(MPコマンド)使用フラグ（既定値は使用）
  string midi_file;                                                             ///< 入力MIDIファイルのUTF-8パス
  string channels{"ABCMNOabFXYZPQRSTUVWGHIJKL"};                                ///< チャンネル割当順
  uint16_t resolution{8};                                                       ///< 音符分解能(TimeBase)8=32/4
  Mml::VolumeMode volume_mode{Mml::VolumeMode::kToneAndVariable};               ///< 音量モード
  uint16_t volume_definition_threshold{60};                                     ///< 音量定義数の間引き閾値
  uint16_t pitch_envelope_limit{15};                                            ///< ピッチ最大定義数
  uint16_t pitch_envelope_threshold{5};                                         ///< ピッチ最低変化量閾値（セント）
  double repeated_note_trim_ratio{0.25};                                        ///< 重複音符の切詰率
  bool merge_percussion_to_channel_9{true};                                     ///< 打楽器チャンネル統合
  DrumMode drum_mode{DrumMode::kNoiseAndDpcm};                                  ///< 打楽器音源割当モード

  /**
   * @brief コマンドライン引数を解析、保持する
   * @param argc (i)引数の個数
   * @param argv (i)UTF-8へ変換済みの引数配列
   * @param error_message (o)解析失敗時のエラーメッセージ格納先
   * @return 解析結果、失敗時は空のoptional
   */
  static optional<ConverterOptions> Parse(                                      // コマンドライン引数を解析、保持する
      int argc,                                                                 ///< (i)引数の個数
      char* argv[],                                                             ///< (i)UTF-8へ変換済みの引数配列
      string& error_message);                                                   ///< (o)解析失敗時のエラーメッセージ格納先

 private:
  /**
   * @brief 符号なし整数値を文字列から解析する
   * @param text (i)解析対象の文字列
   * @param value (o)解析結果の格納先
   * @return 解析成功時true、失敗時false
   */
  template <typename T>
  static bool ParseUnsigned(                                                    // 符号なし整数値を文字列から解析する
      string_view text,                                                         ///< (i)解析対象の文字列
      T& value) noexcept;                                                       ///< (o)解析結果の格納先
  /**
   * @brief 不正なコマンドラインオプションのエラー文を作成する
   * @param option (i)不正なオプション
   * @param reason (i)不正と判定した理由
   * @return 作成したエラーメッセージ
   */
  static string InvalidOption(                                                  // 不正なコマンドラインオプションのエラー文を作成する
      string_view option,                                                       ///< (i)不正なオプション
      string_view reason);                                                      ///< (i)不正と判定した理由
};

/**
 * @brief MIDIファイルを元にしてMMLファイルを生成する
 * @param options (i)コマンドライン引数の設定値
 * @return 成功時0、失敗時は1
 */
int Mid2Mml(                                                                    // MIDIファイルを元にしてMMLファイルを生成する
    const ConverterOptions& options);                                           ///< (i)変換設定

/**
 * @brief MIDIファイルを元にしてMMLファイルを生成するツールの開始
 * @param argc (i)引数の個数
 * @param argv (i)引数の配列
 * @return 成功時0、失敗時は1
 */
int main(                                                                       // MIDIファイルを元にしてMMLファイルを生成するツールの開始
    int argc,                                                                   ///< (i)引数の個数
    char* argv[]) {                                                             ///< (i)UTF-8の引数配列
#ifdef _WIN32                                                                   // Windows
  const ConsoleOutputCodePageGuard console_output_code_page;                    ///< コンソール出力コードページの復元管理
#endif
  cerr << kToolName << ' ' << kAuthor << '\n';                                  // プログラム名と著作者名を表示

  // コマンドライン引数の取り込み
  string error_message;                                                         ///< コマンドライン引数
  const auto options = ConverterOptions::Parse(argc, argv, error_message);      // コマンドライン引数の設定値
  if (!options) {                                                               // オプション解析に失敗したら
    cerr << error_message << '\n';                                              // オプション解析エラーを表示
    return 1;
  }

  // MIDIファイルを元にしてMMLファイルを生成
  return Mid2Mml(*options);
}

/**
 * @brief コマンドライン引数を解析、保持する
 * @param argc (i)引数の個数
 * @param argv (i)UTF-8へ変換済みの引数配列
 * @param error_message (o)解析失敗時のエラーメッセージ格納先
 * @return 解析結果、失敗時は空のoptional
 */
optional<ConverterOptions> ConverterOptions::Parse(                             // コマンドライン引数を解析、保持する
    int argc,                                                                   ///< (i)引数の個数
    char* argv[],                                                               ///< (i)UTF-8へ変換済みの引数配列
    string& error_message) {                                                    ///< (o)解析失敗時のエラーメッセージ格納先
  ConverterOptions options;                                                     ///< 解析中の変換設定

  for (int i = 1; i < argc; ++i) {                                              ///< コマンドライン引数ループ
    const string_view arg{argv[i]};                                             ///< 解析対象の引数
    if (!arg.empty() && arg.front() != '-') {                                   // 空でなくオプション接頭辞がなければ
      if (!options.midi_file.empty()) {                                         // MIDIファイルが既に指定されていれば
        error_message = "MIDIファイルを複数指定できません: " + string(arg);
        return nullopt;
      }
      options.midi_file = argv[i];                                              // 入力MIDIファイルのUTF-8パスを設定
      continue;
    }

    if (arg.size() < 2) {                                                       // オプション名がないなら
      error_message = InvalidOption(arg, "オプション名が指定されていません");
      return nullopt;
    }

    switch (tolower(static_cast<unsigned char>(arg[1]))) {
    case 'c':                                                                   // チャンネル割当順 -c:<チャンネル文字列>
      if (arg.size() < 3 || arg[2] != ':') {                                    // チャンネルオプションの形式が不正なら
        error_message = InvalidOption(arg, "チャンネル指定は-c:<チャンネル文字列>の形式で指定してください");
        return nullopt;
      }
      {
        constexpr string_view kValidChannels{"ABCFGHIJKLMNOPQRSTUVWXYZab"};     ///< 指定可能なメロディーチャンネル
        const string_view channels = arg.substr(3);                             ///< 指定されたメロディーチャンネル
        if (channels.empty()) {                                                 // チャンネルが指定されていなければ
          error_message = InvalidOption(arg, "チャンネルが指定されていません");
          return nullopt;
        }
        for (const char channel : channels) {                                   // 指定されたチャンネルを検査
          if (string_view::npos == kValidChannels.find(channel)) {              // 未対応のチャンネルなら
            error_message = InvalidOption(arg, "未対応のチャンネルが指定されています");
            return nullopt;
          }
          if (channels.find(channel) != channels.rfind(channel)) {              // 重複したチャンネルなら
            error_message = InvalidOption(arg, "同じチャンネルが重複しています");
            return nullopt;
          }
        }
      }
      options.channels = arg.substr(3);                                         // チャンネル割当順を設定
      break;

    case 'r':                                                                   // 音符分解能 -r4 8 16 32 64 128 256
      {
        uint16_t musical_resolution{};                                          ///< 指定された音楽的分解能
        if (!ParseUnsigned(arg.substr(2), musical_resolution)) {                // 分解能が数値でなければ
          error_message = InvalidOption(arg, "分解能は整数で指定してください");
          return nullopt;
        }
        switch (musical_resolution) {
        case 4:                                                                 // 対応するMIDI分解能
        case 8:
        case 16:
        case 32:
        case 64:
        case 128:
        case 256:
          break;

        default:                                                                // 非対応のMIDI分解能
          error_message = InvalidOption(arg, "対応していない分解能です");
          return nullopt;
        }
        options.resolution = static_cast<uint16_t>(musical_resolution / 4);     // MML用分解能をTimeBaseで設定
      }
      break;

    case 'v':                                                                   // 音量設定
      if (arg.size() < 4) {                                                     // 音量オプションの値が不足なら
        error_message = InvalidOption(arg, "音量オプションの値が指定されていません");
        return nullopt;
      }
      // 0:固定音量、1:可変音量、2:音色別音量、3:音色別音量＋可変音量
      if (tolower(static_cast<unsigned char>(arg[2])) == 'm') {                 // 音量モード -vm0～3
        uint8_t value{};                                                        ///< 音量モード番号
        if (!ParseUnsigned(arg.substr(3), value)) {                             // 音量モード値が整数でなければ
          error_message = InvalidOption(arg, "音量モードは整数で指定してください");
          return nullopt;
        }
        if (3 < value) {                                                        // 音量モード値が範囲外なら
          error_message = InvalidOption(arg, "音量モードは0～3で指定してください");
          return nullopt;
        }
        options.volume_mode = static_cast<Mml::VolumeMode>(value);              // 音量モードを設定
      } else if (tolower(static_cast<unsigned char>(arg[2])) == 't') {          // 音量定義数の間引き閾値 -vt0～65535
        // 音量定義数の間引き閾値を検証
        if (!ParseUnsigned(arg.substr(3), options.volume_definition_threshold)) {
          error_message = InvalidOption(arg, "音量定義閾値は0～65535の整数で指定してください");
          return nullopt;
        }
      } else {
        error_message = InvalidOption(arg, "音量オプションは-vmまたは-vtで指定してください");
        return nullopt;
      }
      break;

    case 'p':                                                                   // ピッチ設定
      if (arg.size() < 4) {                                                     // ピッチオプションの値が不足なら
        error_message = InvalidOption(arg, "ピッチオプションの値が指定されていません");
        return nullopt;
      }
      if (tolower(static_cast<unsigned char>(arg[2])) == 'm') {                 // ピッチ最大定義数 -pm0～128
        if (!ParseUnsigned(arg.substr(3), options.pitch_envelope_limit)) {      // ピッチ最大定義数が整数でなければ
          error_message = InvalidOption(arg, "ピッチエンベロープ登録数は整数で指定してください");
          return nullopt;
        }
        if (128 < options.pitch_envelope_limit) {                               // ピッチ最大定義数が範囲外なら
          error_message = InvalidOption(arg, "ピッチエンベロープ登録数は0～128で指定してください");
          return nullopt;
        }
      } else if (tolower(static_cast<unsigned char>(arg[2])) == 't') {          // ピッチ最低変化量閾値 -pt0～65535（セント）
        if (!ParseUnsigned(arg.substr(3), options.pitch_envelope_threshold)) {  // ピッチ最低変化量閾値が不正なら
          error_message = InvalidOption(arg, "ピッチエンベロープ閾値は0～65535の整数で指定してください");
          return nullopt;
        }
      } else {
        error_message = InvalidOption(arg, "ピッチオプションは-pmまたは-ptで指定してください");
        return nullopt;
      }
      break;

    case 'n':                                                                   // 重複音符の切詰率(%) -n0～100
      {
        uint16_t percentage{};                                                  ///< 入力されたパーセント値
        if (!ParseUnsigned(arg.substr(2), percentage)) {                        // パーセント値が整数でなければ
          error_message = InvalidOption(arg, "重複音符の切り詰め割合は整数で指定してください");
          return nullopt;
        }
        if (100 < percentage) {                                                 // パーセント値が範囲外なら
          error_message = InvalidOption(arg, "重複音符の切り詰め割合は0～100で指定してください");
          return nullopt;
        }
        options.repeated_note_trim_ratio = percentage / 100.0;                  // 重複音符を切り詰める割合を設定
      }
      break;

    case 'm':                                                                   // 打楽器チャンネル統合 -m0, -m1
      {
        uint16_t value{};                                                       ///< 打楽器チャンネル統合フラグ
        if (!ParseUnsigned(arg.substr(2), value)) {                             // 打楽器チャンネル統合値が不正なら
          error_message = InvalidOption(arg, "パーカッション統合値は0～65535の整数で指定してください");
          return nullopt;
        }
        options.merge_percussion_to_channel_9 = value != 0;                     // 打楽器チャンネル統合を設定
      }
      break;

    case 'd':                                                                   // 打楽器音源割当モード -d1:ノイズ -d2:DPCM -d3:両方
      {
        uint8_t value{};                                                        ///< 打楽器音源割当モード番号
        if (!ParseUnsigned(arg.substr(2), value)) {                             // 打楽器音源割当モード値が整数でなければ
          error_message = InvalidOption(arg, "ドラム音源モードは整数で指定してください");
          return nullopt;
        }
        if ((1 > value) || (3 < value)) {                                       // 打楽器音源割当モード値が範囲外なら
          error_message = InvalidOption(arg, "ドラム音源モードは1～3で指定してください");
          return nullopt;
        }
        options.drum_mode = static_cast<DrumMode>(value);                       // 打楽器音源割当モードを設定
      }
      break;

    case 'l':                                                                   // LFO(MPコマンド)使用 -l0:不使用 -l1:使用
      {
        uint8_t value{};                                                        ///< LFO(MPコマンド)使用フラグ
        if (!ParseUnsigned(arg.substr(2), value) || value > 1) {                // 0または1でなければ
          error_message = InvalidOption(arg, "LFO(MPコマンド)使用は0または1で指定してください");
          return nullopt;
        }
        options.use_lfo = value != 0;                                           // LFO(MPコマンド)使用を設定
      }
      break;

    default:                                                                    // 未知のオプション
      error_message = InvalidOption(arg, "未対応のオプションです");
      return nullopt;
    }
  }

  if (options.midi_file.empty()) {                                              // MIDIファイルが指定されていなければ
    error_message = "MIDIファイルを指定してください";
    return nullopt;
  }

  ifstream midi_file(options.midi_file, ios::in | ios::binary);                 ///< 入力MIDIファイルの確認用ストリーム
  if (!midi_file) {                                                             // MIDIファイルを開けなければ
    error_message = "MIDIファイルが見つかりません: " + options.midi_file;
    return nullopt;
  }

  return options;
}

/**
 * @brief 符号なし整数値を文字列から解析する
 * @param text (i)解析対象の文字列
 * @param value (o)解析結果の格納先
 * @return 解析成功時true、失敗時false
 */
template <typename T>
bool ConverterOptions::ParseUnsigned(                                           // 符号なし整数値を文字列から解析する
    string_view text,                                                           ///< (i)解析対象の文字列
    T& value) noexcept {                                                        ///< (o)解析結果の格納先
  const auto result = from_chars(text.data(), text.data() + text.size(), value); ///< 文字列解析結果
  return result.ec == errc{} && result.ptr == text.data() + text.size();
}

/**
 * @brief 不正なコマンドラインオプションのエラー文を作成する
 * @param option (i)不正なオプション
 * @param reason (i)不正と判定した理由
 * @return 作成したエラーメッセージ
 */
string ConverterOptions::InvalidOption(                                         // 不正なコマンドラインオプションのエラー文を作成する
    string_view option,                                                         ///< (i)不正なオプション
    string_view reason) {                                                       ///< (i)不正と判定した理由
  return "コマンドラインオプションが不正です: " + string(option) + "（" + string(reason) + "）";
}

/**
 * @brief MIDIファイルを元にしてMMLファイルを生成する
 * @param options (i)コマンドライン引数の設定値
 * @return 成功時0、失敗時は1
 */
int Mid2Mml(                                                                    // MIDIファイルを元にしてMMLファイルを生成する
    const ConverterOptions& options) {                                          ///< (i)変換設定
  // 入力MIDIファイルパスから各出力パスとファイル名を生成
  const auto separator_position = options.midi_file.find_last_of("/\\");        ///< 最後のパス区切り位置
  const auto file_name_position = separator_position == string::npos
      ? 0 : separator_position + 1;                                             ///< ファイル名部分の開始位置
  const auto found_extension_position = options.midi_file.find_last_of('.');    ///< 最後のピリオド位置
  const bool has_extension = found_extension_position != string::npos &&
      found_extension_position > file_name_position;                            ///< ファイル名に拡張子があるか
  const auto extension_position = has_extension
      ? found_extension_position : options.midi_file.size();                    ///< 拡張子の開始位置
  string midi_file_name = options.midi_file.substr(
      file_name_position, extension_position - file_name_position);             ///< 拡張子を除いたMIDIファイル名
  string intermediate_midi_file = options.midi_file;                            ///< 中間MIDIファイルパス
  if (has_extension) {                                                          // 入力ファイルに拡張子があれば
    intermediate_midi_file.insert(extension_position, "_");                     // 拡張子の前に識別文字を追加
  } else {
    intermediate_midi_file += "_.mid";                                          // 既定の拡張子を付加
  }
  const string mml_file = options.midi_file.substr(0, extension_position) + ".mml"; // MMLファイルパス

  Midi midi;                                                                    ///< MIDIデータ
  if (midi.Load(options.midi_file) != 0) {                                      // MIDIファイルを読み込む
    return 1;
  }

  if (options.merge_percussion_to_channel_9) {                                  // パーカッションを9chに統一する設定なら
    midi.UnifyPercussionCh9();                                                  // パーカッションを9chに統一
  }

  midi.DeleteSysEx(0xC);                                                        // 不要なSysExを削除
  midi.ChangeFormat(1);                                                         // フォーマット1へ変換
  midi.DeleteUnusedOperate();                                                   // 未使用の制御情報を削除
  midi.DeleteEmptyTrack();                                                      // 空のトラックを削除
  midi.ChangeNoteEndType();                                                     // ノートエンド表現を変更

  switch (options.drum_mode) {                                                  // ドラム音源モードに応じてトラックを分割
  case DrumMode::kNoise:                                                        // ノイズ音源へ分割
    {
      const array<uint8_t, 128> all_noise_map{};                                ///< 全音符をノイズへ割り当てる表
      midi.DividePercussion(&all_noise_map);                                    // パーカッションをノイズへ割り当て
    }
    break;

  case DrumMode::kDpcm:                                                         // DPCM音源へ分割
    {
      array<uint8_t, 128> all_dpcm_map{};                                       ///< 全音符をDPCMへ割り当てる表
      all_dpcm_map.fill(1);                                                     // 全音符をDPCMへ割り当て
      midi.DividePercussion(&all_dpcm_map);                                     // パーカッションをDPCMへ割り当て
    }
    break;

  case DrumMode::kNoiseAndDpcm:                                                 // ノイズとDPCMへ分割
  default:                                                                      // 既定のドラム音源設定
    midi.DividePercussion();                                                    // パーカッションをデフォルト設定で割り当て
    break;
  }

  midi.DeleteCh9RepeatNote();                                                   // パーカッションの重複音符を削除
  midi.ExtendNote();                                                            // 音符を延長
  midi.DivideRepeatNote(static_cast<float>(options.repeated_note_trim_ratio));  // 重複音符をトラック分割
  midi.CopyTempoAllTrack();                                                     // テンポ・拍子・マーカーを全トラックへコピー

  if (midi.Save(intermediate_midi_file) != 0) {                                 // 中間MIDIファイルを出力
    return 1;
  }

  midi.BlankTrim();                                                             // 曲先頭の空白を切り詰める
  midi.ChangeResolution(options.resolution);                                    // MML用の分解能へ変更
  midi.ChangeTimeType(Midi::TimeType::kAbsolute);                               // 絶対時間へ変更

  auto mml = make_unique<Mml>();                                                ///< MML変換結果
  mml->SetLfoEnabled(options.use_lfo);                                          // MIDI読み込み前にLFO登録の可否を設定
  mml->SetPitchEnvelopeOptions(options.pitch_envelope_limit, options.pitch_envelope_threshold); // ピッチエンベロープの登録条件を設定
  mml->Load(midi, options.channels);                                            // MIDIからMML変換情報を読み込む
  mml->LoopPointConclusion();                                                   // ループ位置を決定
  mml->SetVolMode(options.volume_mode);                                         // 音量出力モードを設定
  mml->AssignVolume(options.volume_definition_threshold);                       // 音量定義を割り当て
  mml->SetFileName(move(midi_file_name));                                       // MML出力ファイル名をUTF-8で設定

  if (mml->Save(mml_file) != 0) {                                               // MMLファイルを出力
    return 1;
  }

  cerr << "done.\n";                                                            // 処理完了を表示
  return 0;
}
