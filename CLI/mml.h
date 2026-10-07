#pragma once
#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iosfwd>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ToneDef.h"
#include "midi.h"
#include "text_encoding.h"

using std::array, std::from_chars, std::map, std::move;
using std::optional, std::ostream, std::ostringstream, std::size_t;
using std::string, std::to_string;
using std::uint16_t, std::uint32_t, std::uint8_t, std::vector;

/** @brief MIDIイベントをMMLへ変換して出力するクラス */
class Mml {
 public:                                                                        // メンバ変数
  /** @brief 音量の出力方法 */
  enum class VolumeMode : uint8_t {
    kConstant,                                                                  ///< 固定音量モード
    kVariable,                                                                  ///< 可変音量モード
    kToneBased,                                                                 ///< 音色音量モード
    kToneAndVariable,                                                           ///< 音色可変音量モード
  };
  /** @brief 音量モードのビット表現を取得する */
  friend constexpr uint8_t operator&(                                           // 音量モードのビット表現を取得する
      VolumeMode left,                                                          ///< (i)左側の音量モード
      VolumeMode right) noexcept {                                              ///< (i)右側の音量モード
    return static_cast<uint8_t>(left) & static_cast<uint8_t>(right);
  }

 private:
  string ch_str_;                                                               ///< チャンネル文字列
  string file_name_;                                                            ///< ファイル名
  string title_;                                                                ///< タイトル
  string composer_;                                                             ///< 作曲者名
  string maker_;                                                                ///< 原著作者名
  string programmer_;                                                           ///< 打ち込み者名
  optional<TextEncoding> midi_text_encoding_;                                   ///< MIDI由来テキストの文字コード
  uint32_t first_tempo_;                                                        ///< 最初のテンポ
  // 音質管理
  /** @brief 音色・音量定義を管理するクラス */
  class Tone {
   public:
    bool use_lfo_{true};                                                        ///< LFO定義・コマンドを使用するか
    VolumeMode volume_mode_;                                                    ///< 音量モード
    // 音色
    // ABabチャンネル MNチャンネル(音色番号+128)
    uint16_t serial_no_abab_mn_;                                                ///< ABabMNチャンネル管理番号
    map<uint16_t, string> color_cmd_abab_mn_;                                   ///< ABabMNチャンネルコマンド
    map<uint16_t, string> color_def_abab_mn_;                                   ///< ABabMNチャンネル定義
    // Dチャンネル
    uint16_t serial_no_d_;                                                      ///< Dチャンネル管理番号
    map<uint16_t, string> color_cmd_d_;                                         ///< Dチャンネルコマンド
    map<uint16_t, string> note_cmd_d_;                                          ///< Dチャンネル音符コマンド
    // Eチャンネル
    uint16_t serial_no_e_;                                                      ///< Eチャンネル管理番号
    map<uint16_t, string> note_cmd_e_;                                          ///< Eチャンネル音符コマンド
    map<uint16_t, string> color_def_e_;                                         ///< Eチャンネル定義
    // Fチャンネル
    uint16_t serial_no_f_;                                                      ///< Fチャンネル管理番号
    map<uint16_t, string> color_cmd_f_;                                         ///< Fチャンネルコマンド
    map<uint16_t, string> color_def_f_;                                         ///< Fチャンネル定義
    // GHIJKLチャンネル
    uint16_t serial_no_ghijkl_;                                                 ///< GHIJKLチャンネル管理番号
    map<uint16_t, string> color_cmd_ghijkl_;                                    ///< GHIJKLチャンネルコマンド
    map<uint16_t, string> color_def_ghijkl_;                                    ///< GHIJKLチャンネル定義
    // PQRSTUVWチャンネル
    uint16_t serial_no_pqrstuvw_;                                               ///< PQRSTUVWチャンネル管理番号
    map<uint16_t, string> color_cmd_pqrstuvw_;                                  ///< PQRSTUVWチャンネルコマンド
    map<uint16_t, string> color_def_pqrstuvw_;                                  ///< PQRSTUVWチャンネル定義
    // XYZチャンネル
    uint16_t serial_no_xyz_;                                                    ///< XYZチャンネル管理番号
    map<uint16_t, string> color_cmd_xyz_;                                       ///< XYZチャンネルコマンド

    // LFO
    uint16_t serial_no_lfo_;                                                    ///< LFO管理番号
    map<uint16_t, string> lfo_cmd_;                                             ///< LFOコマンド
    map<uint16_t, string> lfo_def_;                                             ///< LFO定義

    // 音量(エンヴェロープ) FOチャンネル(音色番号+128) Dチャンネル(音色番号+256)
    uint8_t volume_def_mask_;                                                   ///< 音量マスク（間引き）

    /** @brief 音量定義1件の管理情報 */
    struct VolumeInfo {
      uint16_t serial_no{};                                                     ///< 管理番号
      uint8_t ch{};                                                             ///< チャンネル
      string define;                                                            ///< 定義
    };
    map<uint16_t, map<uint16_t, VolumeInfo>> volume_def_;                       ///< 定義
    optional<TextEncoding> midi_text_encoding_;                                 ///< MIDI由来テキストの文字コード
    // 音色定義
    array<string, 128> tone_def_vct_noise_;                                     ///< 音色定義ノイズ
    array<string, 128> tone_def_vct_square_;                                    ///< 音色定義矩形波
    array<string, 128> tone_def_vct_vrc6_;                                      ///< 音色定義VRC6矩形波
    array<string, 128> tone_def_vct_vrc7_;                                      ///< 音色定義VRC7
    array<string, 128> tone_def_vct_n106_;                                      ///< 音色定義N106
    array<string, 128> tone_def_vct_fds_;                                       ///< 音色定義FDS
    // 音色別音量エンヴェロープ定義
    array<string, 128> volume_def_vct_noise_;                                   ///< 音色別音量エンヴェロープ定義ノイズ
    array<string, 128> volume_def_vct_common_;                                  ///< 音色別音量エンヴェロープ定義共通
    array<string, 128> volume_def_vct_common32_;                                ///< FDS用音量エンヴェロープ定義32段階
    array<string, 128> volume_def_vct_common64_;                                ///< VRC6鋸波用音量エンヴェロープ定義64段階
    array<string, 128> volume_def_vct_vrc7_pre_;                                ///< 音色別音量エンヴェロープ定義VRC7

    /** @brief 音色定義を初期化する */
    Tone()                                                                      // 音色定義を初期化する
        : volume_mode_(VolumeMode::kToneAndVariable),
          serial_no_abab_mn_(),
          color_cmd_abab_mn_(),
          color_def_abab_mn_(),
          serial_no_d_(),
          color_cmd_d_(),
          note_cmd_d_(),
          serial_no_e_(),
          note_cmd_e_(),
          color_def_e_(),
          serial_no_f_(),
          color_cmd_f_(),
          color_def_f_(),
          serial_no_ghijkl_(),
          color_cmd_ghijkl_(),
          color_def_ghijkl_(),
          serial_no_pqrstuvw_(),
          color_cmd_pqrstuvw_(),
          color_def_pqrstuvw_(),
          serial_no_xyz_(),
          color_cmd_xyz_(),
          serial_no_lfo_(),
          lfo_cmd_(),
          lfo_def_(),
          volume_def_mask_(),
          volume_def_(),
          midi_text_encoding_(TextEncoding::kAscii),
          tone_def_vct_noise_(kToneDefNoise),
          tone_def_vct_square_(kToneDefSquare),
          tone_def_vct_vrc6_(kToneDefVrc6),
          tone_def_vct_vrc7_(kToneDefVrc7User),
          tone_def_vct_n106_(kToneDefN106),
          tone_def_vct_fds_(kToneDefFds),
          volume_def_vct_noise_(kVolumeDefNoise),
          volume_def_vct_common_(kVolumeDefCommon),
          volume_def_vct_common32_(),
          volume_def_vct_common64_(),
          volume_def_vct_vrc7_pre_(kVolumeDefVrc7) {
      // 音色別音量エンヴェロープ定義共通を元にFDS用、VRC6鋸波用を作成
      for (size_t i = 0; i < volume_def_vct_common_.size(); ++i) {              // 音量定義配列ループ
        const string& source = volume_def_vct_common_[i];                       ///< 変換元の音量定義
        string& common32 = volume_def_vct_common32_[i];                         ///< 32段階音量定義の変換先
        string& common64 = volume_def_vct_common64_[i];                         ///< 64段階音量定義の変換先
        for (size_t j = 0; j < source.size();) {                                // 音量定義文字列ループ
          if ((source[j] >= '0') && (source[j] <= '9')) {                       // 数値の先頭なら（isdigitは意図的に不使用）
            int value = 0;                                                      ///< 読み取った音量値
            const auto result = from_chars(                                     ///< 数値解析結果
                source.data() + j, source.data() + source.size(), value);
            j = static_cast<size_t>(result.ptr - source.data());
            common32 += to_string(static_cast<int>(value * 2.13));              // 32段階音量へ変換
            common64 += to_string(static_cast<int>(value * 4.21));              // 64段階音量へ変換
          } else {                                                              // 数値以外なら
            common32 += source[j];                                              // 区切り文字や'|'をコピー
            common64 += source[j];                                              // 区切り文字や'|'をコピー
            ++j;
          }
        }
      }
    }

    /**
     * @brief 音色コマンドと定義を登録する
     * @param ch (i)対象チャンネル
     * @param prg_no (i)MIDIプログラム番号
     * @param volume (i)MIDIベロシティ
     */
    void Set(                                                                   // 音色コマンドと定義を登録する
        const uint8_t& ch,                                                      ///< (i)チャンネル
        const uint8_t& prg_no,                                                  ///< (i)プログラム番号
        uint8_t volume = 0);                                                    ///< (i)音量

    /**
     * @brief MIDIベロシティをMML音量へ変換する
     * @param ch (i)対象チャンネル
     * @param vel (i)MIDIベロシティ
     * @return MML音量
     */
    uint8_t VolumeConvert(                                                      // MIDIベロシティをMML音量へ変換する
        const uint8_t& ch,                                                      ///< (i)チャンネル
        const uint8_t& vel) const noexcept;                                     ///< (i)ベロシティ

    /**
     * @brief 音量定義を音色へ割り当てる
     * @param vol_def_threshold (i)音量定義を間引く登録数の閾値
     */
    void AssignVolume(                                                          // 音量定義を音色へ割り当てる
        uint16_t vol_def_threshold = 64);                                       ///< (i)音量登録数間引き閾値

    /**
     * @brief 音量定義の値を指定割合で調整する
     * @param ratio (i)調整割合
     * @param define (i)調整対象の音量定義
     * @return 調整後の音量定義
     */
    string AdjustVolume(                                                        // 音量定義の値を指定割合で調整する
        const float& ratio,                                                     ///< (i)割合
        const string& define) const;                                            ///< (i)定義

    /**
     * @brief 登録済みの音質コマンドを取得する
     * @param ch (i)対象チャンネル
     * @param prg_no (i)MIDIプログラム番号
     * @return 音色コマンド
     */
    string Get(                                                                 // 登録済みの音質コマンドを取得する
        const uint8_t& ch,                                                      ///< (i)チャンネル
        const uint8_t& prg_no) const;                                           ///< (i)プログラム番号

    /**
     * @brief 登録済みの音符コマンドを取得する
     * @param ch (i)対象チャンネル
     * @param prg_no (i)MIDIプログラム番号
     * @return 音符コマンド
     */
    string GetNote(                                                             // 登録済みの音符コマンドを取得する
        const uint8_t& ch,                                                      ///< (i)チャンネル
        const uint8_t& prg_no) const;                                           ///< (i)プログラム番号

    /**
     * @brief 登録済みの音量コマンドを取得する
     * @param ch (i)対象チャンネル
     * @param prg_no (i)MIDIプログラム番号
     * @param volume (i)MML音量
     * @return 音量コマンド
     */
    string GetVolume(                                                           // 登録済みの音量コマンドを取得する
        const uint8_t& ch,                                                      ///< (i)チャンネル
        const uint8_t& prg_no,                                                  ///< (i)プログラム番号
        const uint8_t& volume) const;                                           ///< (i)音量

    /**
     * @brief 音色定義をストリームへ出力する
     * @param os (o)出力ストリーム
     * @param def_map (i)音色定義マップ
     */
    void PutToneDef(                                                            // 音色定義をストリームへ出力する
        ostream& os,                                                            ///< (o)出力ストリーム
        const map<uint16_t, string>& def_map) const;                            ///< (i)音質定義マップ

    /**
     * @brief 音量定義をストリームへ出力する
     * @param os (o)出力ストリーム
     * @param def_map (i)音量定義マップ
     */
    void PutVolumeDef(                                                          // 音量定義をストリームへ出力する
        ostream& os,                                                            ///< (o)出力ストリーム
        const map<uint16_t, map<uint16_t, VolumeInfo>>& def_map) const;         ///< (i)音量定義マップ

    /** @brief MIDI由来テキストに合わせる文字コードを設定する */
    void SetMidiTextEncoding(                                                   // MIDI由来テキストに合わせる文字コードを設定する
        const optional<TextEncoding>& encoding) noexcept {                      ///< (i)MIDI由来テキストの文字コード
      midi_text_encoding_ = encoding;
    }

    /** @brief UTF-8の音色由来文字列をMML出力用へ変換する */
    string EncodeSourceText(                                                    // UTF-8の音色由来文字列をMML出力用へ変換する
        const string& text) const {                                             ///< (i)UTF-8の音色由来文字列
      const auto encoded = EncodeUtf8ForMml(text, midi_text_encoding_);         ///< 変換後の文字列
      return encoded ? *encoded : "/* text encoding conversion failed */";
    }
   protected:
    /**
     * @brief 未登録のパーカッション音色を補完する
     * @param prg_no (i)MIDIドラム番号
     * @return 補完後のドラム番号
     */
    uint16_t ParMapChg(                                                         // 未登録のパーカッション音色を補完する
        const uint16_t& prg_no) const noexcept;                                 ///< (i)プログラム番号

    /** @brief 定義の最終行をタブ幅4で0始まり80桁まで進め、コメントを追加する */
    void CommentEdit(                                                           // 定義ストリームへ桁揃えしたコメントを追加する
        ostringstream& def,                                                     ///< (io)コメント追加前の定義ストリーム
        const uint8_t& ch,                                                      ///< (i)チャンネル
        const uint16_t& prg_no,                                                 ///< (i)プログラム番号または打楽器番号
        optional<uint16_t> volume = std::nullopt) const                         ///< (i)音量定義の場合の音量値
    {
      const string text = def.str();                                            ///< コメント追加前の定義文字列
      const size_t newline = text.find_last_of('\n');                           ///< 最後の改行位置
      const size_t start = newline == string::npos ? 0 : newline + 1;           ///< 最終行の開始位置
      size_t column = 0;                                                        ///< 最終行の0始まり表示桁数
      for (size_t i = start; i < text.size(); ++i) {                            // 数値・記号・固定ASCIIパスの定義部分を数える
        column += text[i] == '\t' ? 4 - column % 4 : 1;                         // タブは次の4桁境界まで進める
      }
      const size_t tabs = column >= 80 ? 1 : (80 - column + 3) / 4;             ///< 80桁までのタブ数、長い定義には1個
      def << string(tabs, '\t') << "// Ch." << static_cast<char>(ch);           // タブだけで字下げしてコメントを開始する
      if (volume.has_value()) {                                                 // 音量定義では音量と従来のプログラム番号を記載する
        def << " Vol." << std::left << std::setfill(' ') << std::setw(2) << *volume
            << "\tPrgNo." << std::setw(3) << prg_no;
      } else {                                                                  // 音色定義では打楽器番号または1始まりの音色番号を記載する
        def << "\t\t\tPrgNo." << std::left << std::setfill(' ') << std::setw(3)
            << (ch == 'D' || ch == 'E' ? prg_no : prg_no + 1);
      }
      def << '\t' << (ch == 'D' || ch == 'E' ? kDrumName[prg_no] : kToneName[prg_no]) << '\n'; // 音色名または打楽器名と改行を追加する
    }
  } tone_;                                                                      ///< 音質管理クラス実体
  //--------------------------------------------------------------
  // ピッチ管理
  /** @brief ピッチエンベロープ定義を管理するクラス */
  class Pitch {
   public:
    uint16_t register_max_;                                                     ///< 最大登録数
    uint16_t register_threshold_;                                               ///< 登録閾値（MIDI音程偏差の変化幅下限、セント）
    vector<string> envelope_def_;                                               ///< 定義

    /** @brief ピッチエンベロープ管理情報を初期化する */
    Pitch()                                                                     // ピッチエンベロープ管理情報を初期化する
        : register_max_(128),
          register_threshold_(5),
          envelope_def_() {}

    /**
     * @brief ピッチエンベロープ定義を登録し、EPコマンド文字列を返す
     * @param envelope (io)登録対象のピッチエンベロープ
     * @param midi_span_cents (i)RPN範囲を反映したMIDI音程偏差の変化幅（セント）
     * @param ch (i)対象チャンネル
     * @return EPコマンド文字列
     */
    string Regist(                                                              // ピッチエンベロープ定義を登録し、EPコマンド文字列を返す
        vector<char>& envelope,                                                 ///< (io)ピッチエンヴェロープ
        double midi_span_cents,                                                 ///< (i)MIDI側の音程偏差の最大値－最小値（セント）
        uint8_t ch = '?');                                                      ///< (i)チャンネル

    /**
     * @brief ピッチエンベロープ定義をストリームへ出力する
     * @param os (o)出力ストリーム
     */
    void PutDef(                                                                // ピッチエンベロープ定義をストリームへ出力する
        ostream& os) const;                                                     ///< (o)出力ストリーム
  } pitch_;

  //--------------------------------------------------------------
  // 音符情報
  /** @brief MMLへ出力する音符または制御情報 */
  struct NoteInfo {
    string str;                                                                 ///< 音符その他(c,c+,d,d+,e,f,f+,g,g+,a,a+,b,n,r,w,&,@...)
    uint32_t len;                                                               ///< 長さ
    uint8_t oct;                                                                ///< 音程(-1～10)
    uint8_t vol;                                                                ///< 音量(0～15,0～63)

    /** @brief 音符情報を初期化する */
    NoteInfo(                                                                   // 音符情報を初期化する
        string str_value = "",                                                  ///< (i)音符・休符・制御コマンドの文字列
        uint32_t len_value = 0,                                                 ///< (i)音符または制御情報の長さ
        uint8_t oct_value = 0,                                                  ///< (i)音程または音色番号
        uint8_t vol_value = 0) noexcept                                         ///< (i)音量
        : str(move(str_value)),
          len(len_value),
          oct(oct_value),
          vol(vol_value) {}
  };

  // チャンネル情報構造体
  /** @brief MMLチャンネルごとの変換情報 */
  struct ChInfo {
    uint8_t default_len;                                                        ///< デフォルトレングス
    uint8_t first_octave;                                                       ///< 最初のオクターブ
    uint8_t first_volume;                                                       ///< 最初の音量
    uint8_t first_prg_no;                                                       ///< 最初のプログラム番号
    vector<NoteInfo> note_vector;                                               ///< 音符ベクタ

    /** @brief チャンネル情報を初期化する */
    ChInfo()                                                                    // チャンネル情報を初期化する
        : default_len(),
          first_octave(255),
          first_volume(255),
          first_prg_no(0),
          note_vector() {}
  };
  map<uint8_t, ChInfo> ch_map_;                                                 ///< チャンネルマップ
  uint16_t time_base_;                                                          ///< 分解能
 public:
  // メンバ関数
  /** @brief 空のMML変換結果を作成する */
  Mml()                                                                         // 空のMML変換結果を作成する
      : ch_str_(),
        file_name_(),
        title_(),
        composer_(),
        maker_(),
        programmer_(),
        midi_text_encoding_(TextEncoding::kAscii),
        first_tempo_(),
        tone_(),
        pitch_(),
        ch_map_(),
        time_base_(64) {}

  /**
   * @brief MIDIデータからMML変換結果を作成する
   * @param midi (i)変換元のMIDIデータ
   */
  explicit Mml(                                                                 // MIDIデータからMML変換結果を作成する
      const Midi& midi)                                                         ///< (i)Midiクラス
      : ch_str_(),
        file_name_(),
        title_(),
        composer_(),
        maker_(),
        programmer_(),
        midi_text_encoding_(TextEncoding::kAscii),
        first_tempo_(),
        tone_(),
        pitch_(),
        ch_map_(),
        time_base_(64) {
    Load(midi);                                                                 // ロードを呼ぶ
  }

  /**
   * @brief MIDIデータと対象チャンネルからMML変換結果を作成する
   * @param midi (i)変換元のMIDIデータ
   * @param ch_str (i)MMLへ出力するチャンネル文字列
   */
  Mml(                                                                          // MIDIデータと対象チャンネルからMML変換結果を作成する
      const Midi& midi,                                                         ///< (i)Midiクラス
      string ch_str)                                                            ///< (i)チャンネル文字列(※D,Eチャンネルは9chからの変換固定のため含てはならない)
      : ch_str_(),
        file_name_(),
        title_(),
        composer_(),
        maker_(),
        programmer_(),
        midi_text_encoding_(TextEncoding::kAscii),
        first_tempo_(),
        tone_(),
        pitch_(),
        ch_map_(),
        time_base_(64) {
    Load(midi, move(ch_str));                                                   // ロードを呼ぶ
  }

  /**
   * @brief MIDIクラスから必要な情報を読み込み、MML用中間情報を作成する
   * @param midi (i)変換元のMIDIデータ
   * @param ch_str (i)変換対象のチャンネル文字列
   */
  void Load(                                                                    // MIDIクラスから必要な情報を読み込み、MML用中間情報を作成する
      const Midi& midi,                                                         ///< (i)Midiクラス
      string ch_str = "ABCMNOabFXYZPQRSTUVWGHIJKL");                            ///< (i)チャンネル文字列(※D,Eチャンネルは9chからの変換固定のため含てはならない)

  /**
   * @brief MMLファイルへ保存する
   * @param file_path (i)出力ファイルのパス
   * @param one_line_bar (i)1行に出力する小節数
   * @return 成功時0、失敗時は負の値
   */
  int Save(                                                                     // MMLファイルへ保存する
      const string& file_path,                                                  ///< (i)UTF-8のファイルパス
      uint8_t one_line_bar = 1) const;                                          ///< (i)一行に出力する小節数

  /**
   * @brief 出力ファイル名の基底名を設定する
   * @param file_name (i)出力ファイル名の基底名
   */
  void SetFileName(                                                             // 出力ファイル名の基底名を設定する
      string file_name) noexcept                                                ///< (i)ファイル名
  {
    file_name_ = move(file_name);
  }

  /**
   * @brief 音量出力モードを設定する
   * @param volume_mode (i)音量出力モード
   */
  void SetVolMode(                                                              // 音量出力モードを設定する
      VolumeMode volume_mode) noexcept                                          ///< (i)音量モード
  {
    tone_.volume_mode_ = volume_mode;                                           // 音量モードを変更する
  }

  /** @brief LFO定義・コマンドの使用を設定する */
  void SetLfoEnabled(bool enabled) noexcept {                                   // LFO定義・コマンドの使用を設定する
    tone_.use_lfo_ = enabled;                                                   // LFO(MPコマンド)使用フラグを保存する
  }

  /**
   * @brief ピッチエンベロープの登録条件を設定する
   * @param regist_max (i)登録する定義の最大数
   * @param regist_threshold (i)登録するMIDI音程変化幅の閾値（セント）
   */
  void SetPitchEnvelopeOptions(                                                 // ピッチエンベロープの登録条件を設定する
      uint16_t regist_max,                                                      ///< (i)登録する定義の最大数
      uint16_t regist_threshold) noexcept {                                     ///< (i)登録するMIDI音程変化幅の閾値（セント）
    pitch_.register_max_ = regist_max;
    pitch_.register_threshold_ = regist_threshold;
  }

  /**
   * @brief 音量定義を割り当てる
   * @param vol_def_threshold (i)音量定義を間引く登録数の閾値
   */
  void AssignVolume(                                                            // 音量定義を割り当てる
      uint16_t vol_def_threshold = 64)                                          ///< (i)音量登録数間引き閾値
  {
    tone_.AssignVolume(vol_def_threshold);                                      // 音量割当て
  }

  /** @brief 複数のLコマンドから最後のループ位置を決定する */
  void LoopPointConclusion();                                                   // 複数のLコマンドから最後のループ位置を決定する
 protected:
  //--------------------------------------------------------------
  /**
   * @brief MIDIキーから音符の周波数レジスタ値を計算する
   * @param key_no (i)MIDIキー番号
   * @param ch (i)MMLチャンネル
   * @return ppmck音階表と出力オクターブに対応する周波数レジスタ値
   */
  uint32_t NoteFrequencyCalc(                                                   // MIDIキーから音符の周波数レジスタ値を計算する
      const uint8_t& key_no,                                                    ///< (i)MIDIキー番号
      const uint8_t& ch) const noexcept;                                        ///< (i)チャンネル

  /**
   * @brief 半音単位のベンドからEPの累積変化量を計算する
   * @param key_no (i)MIDIキー番号
   * @param ch (i)MMLチャンネル
   * @param pitch_bend_sensitivity (i)ピッチベンド感度
   * @param pitch_bend (i)ピッチベンド値
   * @return 正数で高音となるEPの累積変化量（N106はSA7単位）
   */
  int PitchOffsetCalc(                                                          // 半音単位のベンドからEPの累積変化量を計算する
      const uint8_t& key_no,                                                    ///< (i)MIDIキー番号
      const uint8_t& ch,                                                        ///< (i)チャンネル
      double pitch_bend_sensitivity,                                            ///< (i)ピッチベンド範囲（半音＋セント）
      const short& pitch_bend) const noexcept;                                  ///< (i)ピッチベンド(範囲-8192～0～8191)

  /**
   * @brief フレームごとの目標累積値から、実際に反映できた差分をEPへ出力する
   * @param targets (i)フレーム番号と目標累積値
   * @param frames (i)発音期間のフレーム数
   * @return ppmckの1フレームの値域に収めたEP
   */
  vector<char> MakePitchEnvelope(                                               // 目標累積値からEPのフレーム差分を作成する
      const map<uint32_t, int>& targets,                                        ///< (i)フレーム番号と目標累積値
      uint32_t frames) const;                                                   ///< (i)発音期間のフレーム数
  //--------------------------------------------------------------
  /**
   * @brief 音符長の使用回数を集計する
   * @param use_len (io)音符長ごとの使用回数
   * @param note_len (i)集計対象の音符長
   * @param whole_note (i)全音符の長さ
   */
  void UseLenCnt(                                                               // 音符長の使用回数を集計する
      map<uint8_t, uint32_t>& use_len,                                          ///< (io)音符長の数
      const uint32_t& note_len,                                                 ///< (i)音符の長さ
      const uint16_t& whole_note) const;                                        ///< (i)全音符の長さ

  /**
   * @brief 使用する音源の宣言をストリームへ出力する
   * @param os (o)出力ストリーム
   * @param chk_ch (i)確認するチャンネル文字列
   * @param def (i)出力する音源宣言
   */
  void PutChUseDef(                                                             // 使用する音源の宣言をストリームへ出力する
      ostream& os,                                                              ///< (o)出力ストリーム
      const string& chk_ch,                                                     ///< (i)対象チャンネル文字列
      const string& def) const;                                                 ///< (i)音源使用宣言
};
