#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>
#include <list>
#include <string>
#include <utility>

using std::array, std::istream, std::list, std::move, std::streampos;
using std::ostream, std::string;
using std::uint16_t, std::uint32_t, std::uint8_t;

/** @brief Standard MIDI Fileの読み書きとイベント変換を行うクラス */
class Midi {
 protected:
  /** @brief Standard MIDI Fileのファイルヘッダー */
  struct FileHeader {
    uint32_t key_word;                                                          ///< "MThd"
    uint32_t main_header_size;                                                  ///< MainHeader size
  };

  /** @brief Standard MIDI Fileのメインヘッダー */
  struct MainHeader {
    uint16_t format;                                                            ///< midi format 0/1
    uint16_t track_count;                                                       ///< track count
    uint16_t time_base;                                                         ///< 分解能：4分音符とする数値(480が一般的)
  };

  /** @brief Standard MIDI Fileのトラックヘッダー */
  struct TrackHeader {
    uint32_t key_word;                                                          ///< "MTrk"
    uint32_t track_size;                                                        ///< track size
  };
  enum {
    kFileHeaderSize = sizeof(FileHeader),
    kMainHeaderSize = sizeof(MainHeader),
    kTrackHeaderSize = sizeof(TrackHeader),
  };
  static constexpr uint32_t kMaxEventDataSize = 16 * 1024 * 1024;               ///< MIDIイベントデータの最大サイズ

 public:
  // 制御クラス
  /** @brief MIDIイベントを時間順に保持する制御情報 */
  struct Operate {
    uint32_t time;                                                              ///< 直前の制御からの時間
    uint8_t status;                                                             ///< ステータス
    uint8_t status1;                                                            ///< ステータス1
    uint8_t status2;                                                            ///< ステータス2
    string ex_data;                                                             ///< システムエクスクルーシブメッセージ

    /**
     * @brief MIDIイベントを指定値で初期化する
     * @param time (i)直前のイベントからの時間
     * @param status (i)MIDIステータス
     * @param status1 (i)MIDI第1データバイト
     * @param status2 (i)MIDI第2データバイト
     * @param ex_data (i)システムエクスクルーシブのデータ
     */
    Operate(                                                                    // MIDIイベントを指定値で初期化する
        uint32_t time = 0,                                                      ///< (i)直前のイベントからの時間
        uint8_t status = 0x00,                                                  ///< (i)MIDIステータス
        uint8_t status1 = 0x00,                                                 ///< (i)MIDI第1データバイト
        uint8_t status2 = 0x00,                                                 ///< (i)MIDI第2データバイト
        string ex_data = {}) noexcept                                           ///< (i)システムエクスクルーシブのデータ
        : time(time),
          status(status),
          status1(status1),
          status2(status2),
          ex_data(move(ex_data)) {}

    /** @brief MIDIイベントを時間・チャンネル・音程順に比較する */
    bool operator<(                                                             // MIDIイベントを時間・チャンネル・音程順に比較する
        const Operate& other) const noexcept {                                  ///< (i)比較対象イベント
      // 絶対時間により判定するが同じ時間の場合はチャンネルで判定する
      const auto channel = status & 0x0f;                                       ///< 現在イベントのMIDIチャンネル
      const auto other_channel = other.status & 0x0f;                           ///< 比較対象イベントのMIDIチャンネル
      return time != other.time ? time < other.time : channel != other_channel
             ? channel < other_channel : (0x0f & status1) < (0x0f & other.status1); // それも同じなら低音順
    // return ( time < other.time );
    }
/* 非効率なのでやめた
    // オペレーター＝＝  (list::removeが使用する)
    bool operator == (
      const Operate &Ope) {
      // ステータス別に一致を判定する
      if ( Status != other.Status ) return FALSE;                               // ステータスが違う
      switch ( 0xF0 & Status ) {
      // 多バイト
      case 0xF0:                                                                // システムエクスクルーシブメッセージ
        if ( 0xFF == Status ) {                                                 // メタイベントなら
          if ( Status1 != other.Status1 ) return FALSE;                         // ステータス１が違う
        }
        if ( ExData != other.ExData ) return FALSE;                             // システムエクスクルーシブメッセージが違う
        break;
      // 3ByteStatus
      case 0x80:                                                                // ノートオフ
      case 0x90:                                                                // ノートオン
      case 0xA0:                                                                // ポリフォニックキープレッシャー
      case 0xB0:                                                                // コントロールチェンジ
      case 0xE0:                                                                // ピッチベンド
        if ( Status2 != other.Status2 ) return FALSE;                           // ステータス２が違う
        // ※breakせず続行
      // 2ByteStatus
      case 0xC0:                                                                // プログラムチェンジ
      case 0xD0:                                                                // チャンネルプレッシャー
        if ( Status1 != other.Status1 ) return FALSE;                           // ステータス１が違う
      //default:  // それ以外は異常なステータスだが、その場合ステータスのみの判定とする
      }
      return TRUE;                                                              // 一致と認める
    }
*/
  };
  using Track = list<Operate>;
  using TrackList = list<Track>;
  using KeyMap = array<uint8_t, 128>;

  /** @brief イベント時刻の保持形式 */
  enum class TimeType {
    kRelative,                                                                  ///< 相対時間
    kAbsolute,                                                                  ///< 絶対時間
  };

 private:
  // メンバ変数
  uint16_t format_{};                                                           ///< midi format 0/1
  uint16_t time_base_{};                                                        ///< 分解能
  TimeType time_type_{TimeType::kRelative};                                     ///< 時間タイプ FALSE:相対時間 TRUE:絶対時間
  TrackList track_list_{};                                                      ///< トラックデータ

 public:
  /** @brief MIDIの分解能を返す */
  [[nodiscard]] uint16_t time_base() const noexcept {                           // MIDIの分解能を返す
    return time_base_;
  }

  /** @brief MIDIトラック一覧を読み取り専用で返す */
  [[nodiscard]] const TrackList& tracks() const noexcept {                      // MIDIトラック一覧を読み取り専用で返す
    return track_list_;
  }

  /** @brief 空のMIDIデータを作成する */
  Midi() = default;                                                             // 空のMIDIデータを作成する

  /**
   * @brief 指定されたMIDIファイルを読み込むMIDIオブジェクトを作成する
   * @param file_path (i)入力MIDIファイルのパス
   */
  explicit Midi(                                                                // 指定されたMIDIファイルを読み込むMIDIオブジェクトを作成する
      const string& file_path) {                                                ///< (i)UTF-8のSMFファイルパス
    Load(file_path);                                                            // ロードする
  }

  /**
   * @brief Standard MIDI Fileを読み込む
   * @param file_path (i)入力MIDIファイルのパス
   * @return 成功時0、失敗時は負の値
   */
  int Load(                                                                     // Standard MIDI Fileを読み込む
      const string& file_path);                                                 ///< (i)UTF-8のSMFファイルパス

  /**
   * @brief Standard MIDI Fileを書き出す
   * @param file_path (i)出力MIDIファイルのパス
   * @return 成功時0、失敗時は負の値
   */
  int Save(                                                                     // Standard MIDI Fileを書き出す
      const string& file_path);                                                 ///< (i)UTF-8のSMFファイルパス

  /**
   * @brief TimeBaseを変更せず、全イベントの時間を指定倍率で伸縮する
   * @param ratio (i)時間の伸縮率
   */
  void TimeExpand(                                                              // TimeBaseを変更せず、全イベントの時間を指定倍率で伸縮する
      float ratio);                                                             ///< (i)伸縮率

  /**
   * @brief MIDIの分解能と全イベントの時間を変更する
   * @param new_time_base (i)変更後の分解能
   */
  void ChangeResolution(                                                        // MIDIの分解能と全イベントの時間を変更する
      uint16_t new_time_base);                                                  ///< (i)新しい分解能

  /**
   * @brief MIDIフォーマットを変更する
   * @param format (i)変更後のMIDIフォーマット番号
   */
  void ChengeFormat(                                                            // MIDIフォーマットを変更する
      uint16_t format) {                                                        ///< (i)変更後のMIDIフォーマット番号
    ChangeFormat(format);
  }

  /**
   * @brief イベント時刻の保持形式を変更する
   * @param new_time_type (i)変更後の時刻形式
   */
  void ChengeTimeType(                                                          // イベント時刻の保持形式を変更する
      TimeType new_time_type) {                                                 ///< (i)変更後の時刻形式
    ChangeTimeType(new_time_type);
  }

  /**
   * @brief 指定された削除モードに対応するSysExイベントを削除する
   * @param mode (i)削除対象を指定するビットマスク
   */
  void DeleteSysEx(                                                             // 指定された削除モードに対応するSysExイベントを削除する
      uint16_t mode = 0x3);                                                     ///< (i)SysEx削除モード

  /** @brief 音符またはSysExが無いチャンネルの不要な制御を削除する */
  void DeleteUnusedOperate(void);                                               // 音符またはSysExが無いチャンネルの不要な制御を削除する

  /** @brief イベントを持たないトラックを削除する */
  void DeleteEmptyTrack(void);                                                  // イベントを持たないトラックを削除する

  /** @brief ノートオフの表現方法を変更する */
  void ChengeNoteEndType(                                                       // ノートオフの表現方法を変更する
      uint16_t new_note_end = 0x90) {                                           ///< (i)新しいノートオフステータス
    ChangeNoteEndType(new_note_end);
  }

  /** @brief パーカッションチャンネルをMIDI 9チャンネルへ統一する */
  void UnifyPercussionCh9(void);                                                // パーカッションチャンネルをMIDI 9チャンネルへ統一する

  /**
   * @brief パーカッションチャンネルをノイズ音源用とDPCM音源用に分離する
   * @param note_map (i)音符番号ごとの音源割り当て表
   */
  void DividePercussion(                                                        // パーカッションチャンネルをノイズ音源用とDPCM音源用に分離する
      const KeyMap* note_map = nullptr);                                        ///< (i)割り当て表 0:ノイズ 1:DPCM

  /**
   * @brief パーカッションチャンネルの重複音符を削除する
   * @param priority (i)音符番号ごとの優先度表
   */
  void DeleteCh9RepeatNote(                                                     // パーカッションチャンネルの重複音符を削除する
      const KeyMap* priority = nullptr);                                        ///< (i)優先度表

  /** @brief 音符をトラック終端まで延長する */
  void ExtendNote(                                                              // 音符をトラック終端まで延長する
      uint16_t ch_mask = 0x0200);                                               ///< (i)チャンネルマスク(９ｃｈ)

  /**
   * @brief 重複する音符を別トラックへ分離する
   * @param tail_ratio_max (i)音符末尾の重複を許容する最大割合
   * @param ch_mask (i)処理対象とするチャンネルのビットマスク
   */
  void DivideRepeatNote(                                                        // 重複する音符を別トラックへ分離する
      float tail_ratio_max = .25,                                               ///< (i)尻尾割合最大
      uint16_t ch_mask = 0xFDFF);                                               ///< (i)チャンネルマスク(９ｃｈ以外)

  /** @brief 先頭トラックのテンポ・拍子・マーカーを全トラックへコピーする */
  void CopyTempoAllTrack(void);                                                 // 先頭トラックのテンポ・拍子・マーカーを全トラックへコピーする

  /** @brief 曲先頭の空白を小節単位で切り詰める */
  void BrankTrim() { BlankTrim(); }                                             // 曲先頭の空白を小節単位で切り詰める

  /**
   * @brief MIDIフォーマット0または1へ変換する
   * @param format (i)変換後のMIDIフォーマット番号
   */
  void ChangeFormat(                                                            // MIDIフォーマット0または1へ変換する
      uint16_t format);                                                         ///< (i)変換後のMIDIフォーマット番号

  /**
   * @brief 全イベントの時間情報を相対時間または絶対時間へ変更する
   * @param new_time_type (i)変換後の時刻形式
   */
  void ChangeTimeType(                                                          // 全イベントの時間情報を相対時間または絶対時間へ変更する
      TimeType new_time_type);                                                  ///< (i)変換後の時刻形式

  /**
   * @brief ノートオフの表現を0x80または0x90ベロシティ0へ切り替える
   * @param new_note_end (i)変換後のノートオフステータス
   */
  void ChangeNoteEndType(                                                       // ノートオフの表現を0x80または0x90ベロシティ0へ切り替える
      uint16_t new_note_end = 0x90);                                            ///< (i)変換後のノートオフステータス

  /** @brief 曲先頭の空白を拍子に基づく小節単位で切り詰める */
  void BlankTrim();                                                             // 曲先頭の空白を拍子に基づく小節単位で切り詰める

  /** @brief 全MIDIイベントをデバッグ用に表示する */
  void PrintAllOperate(void) const;                                             // 全MIDIイベントをデバッグ用に表示する

  /** @brief MIDIイベントの時刻情報を検査し、異常を表示する */
  void CheckAllOperate(void) const;                                             // MIDIイベントの時刻情報を検査し、異常を表示する

  /**
   * @brief 指定したステータスのMIDIイベントを検索して表示する
   * @param status (i)検索対象のMIDIステータス
   * @param status1 (i)検索対象の第1データバイト
   * @param status2 (i)検索対象の第2データバイト
   */
  void FindViewOperate(                                                         // 指定したステータスのMIDIイベントを検索して表示する
      const uint8_t& status,                                                    ///< (i)検索する制御
      const uint8_t& status1 = 0x00,                                            ///< (i)検索する制御ステータス１
      const uint8_t& status2 = 0x00) const;                                     ///< (i)検索する制御ステータス２

 protected:                                                                     // 内部関数
  /**
   * @brief ストリームから1トラック分のMIDIイベントを解読する
   * @param is (i)入力ストリーム
   * @param track_size (i)トラックデータのサイズ
   * @return 成功時0、失敗時は負の値
   */
  int Decode(                                                                   // ストリームから1トラック分のMIDIイベントを解読する
      istream& is,                                                              ///< (i)入力ストリーム
      const uint32_t& track_size);                                              ///< (i)トラックサイズ（トラックの終了位置判定に利用）

  /**
   * @brief ストリームからMIDI可変長値を解読する
   * @param is (i)入力ストリーム
   * @param track_end (i)トラックデータの終端位置
   * @param value (o)解読した値
   * @return 成功時true、失敗時false
   */
  bool TimeDecode(                                                              // ストリームからMIDI可変長値を解読する
      istream& is,                                                              ///< (i)入力ストリーム
      streampos track_end,                                                      ///< (i)トラックデータの終端位置
      uint32_t& value) const;                                                   ///< (o)解読した値

  /**
   * @brief 1トラックの制御リストをMIDI形式へ符号化する
   * @param os (o)出力ストリーム
   * @param ope_list (i)符号化するイベントリスト
   * @return 成功時0、失敗時は負の値
   */
  int Encode(                                                                   // 1トラックの制御リストをMIDI形式へ符号化する
      ostream& os,                                                              ///< (o)出力ストリーム
      const Track& ope_list) const;                                             ///< (i)制御リスト

  /**
   * @brief 時間をデルタタイムへ符号化する
   * @param os (o)出力ストリーム
   * @param time (i)符号化する時間
   * @return 成功時0、失敗時は負の値
   */
  int TimeEncode(                                                               // 時間をデルタタイムへ符号化する
      ostream& os,                                                              ///< (o)出力ストリーム
      uint32_t time) const;                                                     ///< (i)時間

  /** @brief 全トラックの終端イベントを削除し、最後のイベント時刻を返す */
  uint32_t DeleteAllTrackEnd(void);                                             // 全トラックの終端イベントを削除し、最後のイベント時刻を返す

  /**
   * @brief 全トラックにトラック終端イベントを追加する
   * @param last_time (i)トラック終端の時刻
   */
  void AppendAllTrackEnd(                                                       // 全トラックにトラック終端イベントを追加する
      const uint32_t& last_time);                                               ///< (i)トラックエンド時間
};
