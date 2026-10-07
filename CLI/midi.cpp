// Midiクラス
#include "midi.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <vector>

#include "endian.h"
#include "text_encoding.h"

using std::cerr, std::flush;
using std::dec, std::hex;
using std::ifstream, std::ofstream, std::ios, std::istream;
using std::ostream, std::streamoff, std::streampos, std::streamsize, std::stringstream;
using std::list, std::stack, std::string, std::swap, std::vector;
using std::uint16_t, std::uint32_t, std::uint8_t;

/** @brief Standard MIDI Fileを読み込む */
int Midi::Load(                                                                 // Standard MIDI Fileを読み込む
    const string& file_path)                                                    ///< (i)UTF-8のSMFファイルパス
{
  // ファイルオープン
  ifstream ifs(file_path, ios::in | ios::binary);                               ///< ファイルストリーム
  if (!ifs) {
    cerr << "MIDIファイルを開けません\n";
    return -1;
  }
  ifs.seekg(0, ios::end);                                                       // MIDIファイルの終端へ移動
  const streampos file_end = ifs.tellg();                                       ///< MIDIファイルの終端位置
  if ((streampos(streamoff(-1)) == file_end) ||                                 // 終端位置を取得できない、または
      !ifs.seekg(0, ios::beg)) {                                                // MIDIファイルの先頭へ戻せないなら
    cerr << "MIDIファイルのサイズを取得できません\n";
    return -1;
  }
  format_ = 0;
  time_base_ = 0;
  time_type_ = TimeType::kRelative;
  track_list_.clear();

  // FileHeader
  FileHeader fh;                                                                ///< 読み込むMIDIファイルヘッダー
  ifs.read(reinterpret_cast<char*>(&fh), kFileHeaderSize);
  if (!ifs) {
    cerr << "MIDIファイルの File Header の読み込みに失敗しました。\n";
    return -1;
  }
  ReverseEndian(fh.key_word);                                                   // エンディアン反転
  if (fh.key_word != 'MThd') {                                                  // MIDIファイルヘッダ"MThd"であること
    cerr << "MIDIファイルの File Header チャンク'MThd'を認識できません。MIDIファイルではありません。\n";
    return -1;
  }
  ReverseEndian(fh.main_header_size);                                           // エンディアン反転
  cerr << "; Main Header Size " << fh.main_header_size << " byte.\n";
  if (kMainHeaderSize > fh.main_header_size || 0x1000 < fh.main_header_size) {  // ヘッダーサイズが範囲外なら
    cerr << "Main Headerのサイズ(" << fh.main_header_size << ")が異常です。\n";
    return -1;
  }
  // MainHeader
  MainHeader mh;                                                                ///< 読み込むMIDIメインヘッダー
  ifs.read(reinterpret_cast<char*>(&mh), kMainHeaderSize);
  if (!ifs) {
    cerr << "MIDIファイルの Main Header の読み込みに失敗しました。\n";
    return -1;
  }
  ReverseEndian(mh.format);                                                     // エンディアン反転
  ReverseEndian(mh.track_count);
  ReverseEndian(mh.time_base);
  cerr << "; midi format " << mh.format << "\n; track count " << mh.track_count
       << "\n; Time base   " << mh.time_base << '\n';
  format_ = mh.format;                                                          // フォーマット
  time_base_ = mh.time_base;                                                    // 分解能を

  const streamoff main_header_end =
      static_cast<streamoff>(kFileHeaderSize) +
      static_cast<streamoff>(fh.main_header_size);
  ifs.seekg(main_header_end, ios::beg);                                         // MainHeaderの終わりまで読み飛ばす
  if (!ifs) {
    cerr << "MIDIファイルの Main Header の後が途切れています。\n";
    return -1;
  }
  // Track
  for (uint16_t n = 0; n < mh.track_count; n++) {                               ///< トラック番号
    cerr << "; Track " << n << "\t";
    // TrackHeader
    TrackHeader th;                                                             ///< トラックヘッダー
    ifs.read(reinterpret_cast<char*>(&th), kTrackHeaderSize);
    if (!ifs) {
      cerr << "MIDIファイルの Track Header の読み込みに失敗しました。\n";
      return -1;
    }
    ReverseEndian(th.key_word);                                                 // エンディアン反転
    if (th.key_word != 'MTrk') {                                                // トラックヘッダ"MTrk"であること
      cerr << "MIDIファイルの Track Header チャンク'MTrk'を認識できません。トラックが壊れています。\n";
      return -1;
    }
    ReverseEndian(th.track_size);                                               // エンディアン反転
    cerr << "Track Data Size " << th.track_size << " byte.\n";
    const streampos track_begin = ifs.tellg();                                  ///< トラックデータの開始位置
    if ((streampos(streamoff(-1)) == track_begin) ||                            // 開始位置を取得できない、
        (file_end < track_begin) ||                                             // 開始位置がファイル終端を超えている、または
        (file_end - track_begin < static_cast<streamoff>(th.track_size))) {     // トラックサイズがファイルの残量を超えているなら
      cerr << "トラックサイズがMIDIファイルの残量を超えています\n";
      return -1;
    }
    // TrackData
    if (Decode(ifs, th.track_size)) {                                           // トラック内解析
      return -1;
    }
  }
  return 0;
}

/** @brief ストリームから1トラック分のMIDIイベントを解読する */
int Midi::Decode(                                                               // ストリームから1トラック分のMIDIイベントを解読する
    istream& is,                                                                ///< (i)入力ストリーム
    const uint32_t&
        track_size)                                                             ///< (i)トラックサイズ（トラックの終了位置判定に利用）
{
  list<Operate> ope_list;                                                       ///< 読み込んだイベントの一時リスト
  uint8_t status_backup = 0x00;                                                 ///< ステータスバックアップ
  const streampos track_begin = is.tellg();                                     ///< トラックデータの開始位置
  if (streampos(streamoff(-1)) == track_begin) {                                // トラックの開始位置を取得できなければ
    cerr << "トラックの開始位置を取得できません\n";
    return -1;
  }
  const streampos last_pos = track_begin + static_cast<streamoff>(track_size);  ///< トラックの終了位置
  const auto has_track_bytes = [&is, last_pos](streamoff byte_count) {          // トラック内に指定バイト数が残っているか調べる
    const streampos current_pos = is.tellg();                                   ///< 現在の読み込み位置
    return (streampos(streamoff(-1)) != current_pos) &&                         // 現在位置を取得でき、
           (current_pos <= last_pos) &&                                         // 現在位置がトラック終端以前で、
           (byte_count <= last_pos - current_pos);                              // 指定バイト数がトラック内に収まるか返す
  };
  while (last_pos > is.tellg()) {                                               // トラックの終了位置までループ
    Operate ope;                                                                ///< 読み込むMIDIイベント
    // デルタタイム取得
    if (!TimeDecode(is, last_pos, ope.time)) {                                  // 曲先頭からの時間を解析
      return -1;
    }
    // ステータス取得
    if (!has_track_bytes(1)) {                                                  // ステータス1バイトがトラック内に収まらなければ
      cerr << "MIDIイベントがトラック終端までに完結していません\n";
      return -1;
    }
    is.read(reinterpret_cast<char*>(&(ope.status)), 1);                         // ステータス取得
    if (!is) {
      cerr << "Status の読み込みに失敗しました。\n";
      break;
    }
    // ランニングステータスの処理
    uint8_t& status = reinterpret_cast<uint8_t&>(ope.status);                   ///< Alius
    if (0x80 & status) {                                                        // ステータス省略してなければ
      status_backup = status;                                                   // ステータスをバックアップ
    } else {                                                                    // ステータス省略されてたら
      status = status_backup;                                                   // 前回のステータスで補完
      is.seekg(-1, ios::cur);                                                   // 1バイト戻す
    }
    // ステータス別処理
    switch (0xF0 & status) {                                                    // ステータス処理
    // 2ByteStatus
    case 0xC0:                                                                  // プログラムチェンジ
    case 0xD0:                                                                  // チャンネルプレッシャー
      if (!has_track_bytes(1)) {                                                // データ1バイトがトラック内に収まらなければ
        cerr << "MIDIイベントがトラック終端までに完結していません\n";
        return -1;
      }
      is.read(reinterpret_cast<char*>(&(ope.status1)), 1);                      // ステータス1取得
      if (!is) break;
      ope_list.push_back(ope);                                                  // 制御をリストに追加
      break;
    // 3ByteStatus
    case 0x80:                                                                  // ノートオフ
    case 0x90:                                                                  // ノートオン
    case 0xA0:                                                                  // ポリフォニックキープレッシャー
    case 0xB0:                                                                  // コントロールチェンジ
    case 0xE0:                                                                  // ピッチベンド
      if (!has_track_bytes(2)) {                                                // データ2バイトがトラック内に収まらなければ
        cerr << "MIDIイベントがトラック終端までに完結していません\n";
        return -1;
      }
      is.read(reinterpret_cast<char*>(&(ope.status1)), 1);                      // ステータス1取得
      if (!is) break;
      is.read(reinterpret_cast<char*>(&(ope.status2)), 1);                      // ステータス2取得
      if (!is) break;
      ope_list.push_back(ope);                                                  // 制御をリストに追加
      break;
    // ControlStatus
    case 0xF0:                                                                  // SysEx
      if (0xFF == status) {                                                     // メタイベントなら
        if (!has_track_bytes(1)) {                                              // メタイベント種別がトラック内に収まらなければ
          cerr << "MIDIイベントがトラック終端までに完結していません\n";
          return -1;
        }
        is.read(reinterpret_cast<char*>(&(ope.status1)), 1);                    // ステータス1取得
        if (!is) break;
      }
      {
        uint32_t data_size;                                                     ///< MIDIイベントのデータサイズ
        if (!TimeDecode(is, last_pos, data_size)) {                             // メッセージ長を取得
          return -1;
        }
        if (kMaxEventDataSize < data_size) {                                    // データサイズが上限を超えていれば
          cerr << "MIDIイベントのデータサイズが上限を超えています\n";
          return -1;
        }
        const streampos data_begin = is.tellg();                                ///< イベントデータの開始位置
        if ((streampos(streamoff(-1)) == data_begin) ||                         // 開始位置を取得できない、
            (last_pos < data_begin) ||                                          // 開始位置がトラック終端を超えている、または
            (last_pos - data_begin < static_cast<streamoff>(data_size))) {      // データサイズがトラックの残量を超えていれば
          cerr << "MIDIイベントのデータサイズがトラックの残量を超えています\n";
          return -1;
        }
        ope.ex_data.resize(static_cast<string::size_type>(data_size));          // 検証済みのデータサイズを確保
        is.read(ope.ex_data.data(), static_cast<streamsize>(data_size));        // SysEx取得
        if (!is) break;
      }
      // テキスト表示
      switch (ope.status1) {                                                    // ステータス1がテキストなら
      case 0x01:                                                                // テキスト
      case 0x02:                                                                // 著作権表示
      case 0x03:                                                                // 曲名/トラック名
      case 0x04:                                                                // 楽器名
      case 0x05:                                                                // 歌詞
      case 0x08:                                                                // プログラム名(音色名)
      case 0x09:                                                                // デバイス名(音源名)
        if (!ope.ex_data.empty()) {                                             // 空じゃなければ
          if ('\0' == ope.ex_data[ope.ex_data.length() - 1]) {                  // NULL文字削除
            ope.ex_data = ope.ex_data.substr(0, ope.ex_data.length() - 1);
          }
        }
        if (!ope.ex_data.empty()) {                                             // 空じゃなければ
          const char* text_name[] = {
              // テキスト名
              "シーケンス番号",
              "テキスト",
              "著作権表示",
              "曲名/トラック名",
              "楽器名",
              "歌詞",
              "マーカー",
              "キューポイント",
              "プログラム名(音色名)",
              "デバイス名(音源名)",
          };                                                                    ///< メタイベント種別名の一覧
          cerr << text_name[ope.status1]                                        // テキスト名
               << ":\"" << EncodeMidiTextForConsole(ope.ex_data) << "\"\n";     // 元データを維持してUTF-8で表示
          ope_list.push_back(ope);                                              // 制御をリストに追加
        }
        break;

      case 0x51:                                                                // テンポイベント
        if (0xFF == status) {
          if (3 != ope.ex_data.size()) {                                        // データ長が異常なら
            cerr << "警告：テンポイベントのデータ長が異常であるため無視します。\n";
            break;
          }
          if (!(static_cast<uint8_t>(ope.ex_data[0]) |
                static_cast<uint8_t>(ope.ex_data[1]) |
                static_cast<uint8_t>(ope.ex_data[2]))) {                        // テンポ値が0なら
            cerr << "警告：テンポ値が異常であるため無視します。\n";
            break;
          }
        }
        ope_list.push_back(ope);                                                // 制御をリストに追加
        break;

      case 0x58:                                                                // 拍子イベント
        if (0xFF == status) {
          if (4 != ope.ex_data.size()) {                                        // データ長が異常なら
            cerr << "警告：拍子イベントのデータ長が異常であるため無視します。\n";
            break;
          }
          if (7 < static_cast<uint8_t>(ope.ex_data[1])) {                       // 分母指数が安全な範囲外なら
            cerr << "警告：拍子イベントの分母指数が異常であるため無視します。\n";
            break;
          }
          if (!static_cast<uint8_t>(ope.ex_data[0])) {                          // 分子が0なら
            cerr << "警告：拍子イベントの分子が異常であるため無視します。\n";
            break;
          }
        }
        ope_list.push_back(ope);                                                // 制御をリストに追加
        break;

      case 0x2F:                                                                // トラック終了の場合
        ope_list.push_back(ope);                                                // 制御をリストに追加
        track_list_.push_back(ope_list);                                        // 制御リストをトラックリストに追加
        ope_list.clear();                                                       // 制御リストをクリア
        break;

      default:                                                                  // 未知のメタイベント
        ope_list.push_back(ope);                                                // 制御をリストに追加
      }
      break;

    default:                                                                    // MIDIフォーマット異常
      cerr << "警告：MIDIフォーマットに異常があります。Status(" << hex << static_cast<uint16_t>(status)
           << ") の処理を続行します。\n";
      break;                                                                    // 処理は強行
    }
    if (!is) {                                                                  // 読み込みエラー抜け出し
      cerr << "読み込み中にエラーが発生しました。MIDIファイルが壊れている可能性があります。\n";
      break;
    }
  }
  // トラックリストに追加
  if (!ope_list.empty()) {                                                      // 制御リストに何か残っていたら
    track_list_.push_back(ope_list);                                            // 制御リストをトラックリストに追加
  }
  return (!is ? -1 : 0);                                                        // エラーなら-1
}

/** @brief ストリームからMIDI可変長値を解読する */
bool Midi::TimeDecode(                                                          // ストリームからMIDI可変長値を解読する
    istream& is,                                                               ///< (i)入力ストリーム
    streampos track_end,                                                       ///< (i)トラックデータの終端位置
    uint32_t& value) const                                                     ///< (o)解読した値
{
  value = 0;                                                                    // 解読値を初期化
  for (uint8_t byte_count = 0; byte_count < 4; ++byte_count) {                  // MIDI可変長値の上限4バイトまで読み込む
    const streampos current_pos = is.tellg();                                   ///< 現在の読み込み位置
    if ((streampos(streamoff(-1)) == current_pos) ||                            // 現在位置を取得できない、または
        (track_end <= current_pos)) {                                           // 現在位置がトラック終端に達していれば
      cerr << "MIDI可変長値がトラック終端までに完結していません\n";
      return false;
    }
    uint8_t data;                                                               ///< 読み込んだ可変長値の1バイト
    is.read(reinterpret_cast<char*>(&data), 1);                                 // 可変長値を1バイト読み込む
    if (!is) {                                                                  // 読み込みに失敗したら
      cerr << "MIDI可変長値の読み込みに失敗しました。MIDIファイルが壊れている可能性があります。\n";
      return false;
    }
    value <<= 7;                                                                // 既に解読した値を7ビット左へ移動
    value |= (data & 0x7F);                                                     // 今回の7ビットを解読値へ追加
    if (!(data & 0x80)) {                                                       // 継続ビットがなければ
      return true;
    }
  }
  cerr << "MIDI可変長値が4バイトを超えています\n";
  return false;                                                                 // 解析を中止するため残りは読み捨てず失敗を返す
}

/** @brief Standard MIDI Fileを書き出す */
int Midi::Save(                                                                 // Standard MIDI Fileを書き出す
    const string& file_path)                                                    ///< (i)UTF-8のSMFファイルパス
{
  // 相対時間に設定
  ChengeTimeType(TimeType::kRelative);                                          // 相対時間に変更
  // ファイルオープン
  ofstream ofs(file_path, ios::out | ios::binary);                              ///< ファイルストリーム
  if (!ofs) {
    cerr << "MIDIファイルの出力先を開けませんでした。\n";
    return -1;
  }
  // FileHeader
  FileHeader fh;                                                                ///< 出力するMIDIファイルヘッダー
  fh.key_word = ConvertEndian('MThd');                                          // ファイルヘッダチャンク識別を設定
  fh.main_header_size = ConvertEndian(static_cast<uint32_t>(kMainHeaderSize));  // MainHeaderサイズを設定
  ofs.write(reinterpret_cast<char*>(&fh), kFileHeaderSize);
  if (!ofs) {
    cerr << "MIDIファイルの File Header の出力に失敗しました。\n";
    return -1;
  }
  // MainHeader
  MainHeader mh;                                                                ///< 出力するMIDIメインヘッダー
  mh.format = ConvertEndian(format_);                                           // フォーマット１に設定
  uint16_t track_count = static_cast<uint16_t>(track_list_.size());             ///< トラック数
  mh.track_count = ConvertEndian(track_count);                                  // トラック数を設定
  mh.time_base = ConvertEndian(time_base_);                                     // 分解能を設定
  ofs.write(reinterpret_cast<char*>(&mh), kMainHeaderSize);
  if (!ofs) {
    cerr << "MIDIファイルの Main Header の出力に失敗しました。\n";
    return -1;
  }
  // Track
  for (auto& track : track_list_) {                                             // トラックリストループ
    // TrackData
    // 説明：トラックサイズを先に求める必要があるため、先にトラックデータを符号化しておく（ファイル出力は後で）
    stringstream ss;                                                            ///< 一時領域
    if (0 > Encode(ss, track)) {                                                // トラックデータ符号化
      cerr << "MIDIファイルの出力時にトラックのエンコードに失敗しました。\n";
      return -1;
    }
    int track_size = static_cast<int>(ss.tellp());                              ///< トラックサイズ取得
    if (0 > track_size) {                                                       // もしマイナス値ならトラックが空なので
      continue;                                                                 // 出力せず、次のトラックへ
    }
    // TrackHeader
    TrackHeader th;                                                             ///< トラックヘッダ
    th.key_word = ConvertEndian('MTrk');                                        // トラックヘッダチャンク識別を設定
    th.track_size = ConvertEndian(track_size);                                  // トラックサイズを設定
    ofs.write(reinterpret_cast<char*>(&th), kTrackHeaderSize);
    if (!ofs) {
      cerr << "MIDIファイルの Track Header の出力に失敗しました。\n";
      return -1;
    }
    // トラックデータをファイルに流し込む
    ofs << ss.str() << flush;                                                   // 一時領域をファイルに出力
    if (!ofs) {
      cerr << "MIDIファイルの Track Data の出力に失敗しました。\n";
      return -1;
    }
  }
  return (!ofs ? -1 : 0);                                                       // エラーなら-1
}

/** @brief 1トラックの制御リストをMIDI形式へ符号化する */
int Midi::Encode(                                                               // 1トラックの制御リストをMIDI形式へ符号化する
    ostream& os,                                                                ///< (o)出力ストリーム
    const list<Operate>& ope_list) const                                        ///< (i)制御リスト
{
  uint8_t status_backup(0x00);                                                  ///< ステータスバックアップ
  // 制御リストループ
  for (const auto& ope : ope_list) {                                            // 制御リストループ
    // 出力処理
    TimeEncode(os, ope.time);                                                   // 時間を符号化
    if (!os) return -1;
    const uint8_t& status = ope.status;                                         ///< ステータスの参照
    // ランニングステータスの処理
    if (status_backup != status) {                                              // ステータスが前回と違うなら
      status_backup = status;                                                   // ステータスをバックアップ
      os.write(reinterpret_cast<const char*>(&ope.status), 1);                  // ステータスを出力
      if (!os) return -1;
    }
    // ステータス別処理
    switch (0xF0 & status) {
    // 2ByteStatus
    case 0xC0:                                                                  // プログラムチェンジ
    case 0xD0:                                                                  // チャンネルプレッシャー
      os.write(reinterpret_cast<const char*>(&(ope.status1)), 1);               // ステータス1出力
      if (!os) return -1;
      break;
    // 3ByteStatus
    case 0x80:                                                                  // ノートオフ
    case 0x90:                                                                  // ノートオン
    case 0xA0:                                                                  // ポリフォニックキープレッシャー
    case 0xB0:                                                                  // コントロールチェンジ
    case 0xE0:                                                                  // ピッチベンド
      os.write(reinterpret_cast<const char*>(&(ope.status1)), 1);               // ステータス1出力
      if (!os) return -1;
      os.write(reinterpret_cast<const char*>(&(ope.status2)), 1);               // ステータス2出力
      if (!os) return -1;
      break;
    // ControlStatus
    case 0xF0:                                                                  // システムSysEx
      status_backup = 0x00;                                                     // ランニングステータスにしない
      if (0xFF == status) {                                                     // メタイベントなら
        os.write(reinterpret_cast<const char*>(&(ope.status1)), 1);             // ステータス1出力
        if (!os) return -1;
      }
      TimeEncode(os, ope.ex_data.size());                                       // メッセージ長を出力
      if (!os) return -1;
      os.write(ope.ex_data.c_str(), ope.ex_data.size());                        // システムSysEx出力
      if (!os) return -1;
    }
  }
  return 0;                                                                     // 正常
}

/** @brief 時間をデルタタイムへ符号化する */
int Midi::TimeEncode(                                                           // 時間をデルタタイムへ符号化する
    ostream& os,                                                                ///< (o)出力ストリーム
    uint32_t time) const                                                        ///< (i)時間
{
  // 下位ビットからスタックに積む
  stack<uint8_t> stk;                                                           ///< スタック
  bool final(true);                                                             ///< 最後に設定
  uint8_t data;                                                                 ///< 出力するデルタタイムの1バイト
  do {
    data = time & 0x7F;                                                         // 下７ビット取り出し
    time >>= 7;                                                                 // Time下７ビットを詰める
    if (final) {                                                                // 最後なら
      final = false;                                                            // 途中に設定
    } else {                                                                    // 途中なら
      data |= 0x80;                                                             // 続きありのビットを立てる
    }
    stk.push(data);                                                             // スタックに積む
  } while (time);                                                               // Time有効桁が無くなるまでループ
  // 逆順に出力
  while (!stk.empty()) {                                                        // 空になるまでループ
    data = stk.top();                                                           // スタック取得
    os.write(reinterpret_cast<char*>(&data), 1);                                // 出力
    if (!os) {                                                                  // エラーなら
      cerr << "デルタタイムの出力に失敗しました。\n";
      return -1;                                                                // -1を返す
    }
    stk.pop();                                                                  // スタックから抜く
  }
  return 0;
}

/**
 * @brief TimeBaseを変更せず、全イベントの時間を指定倍率で伸縮する
 * 小節と音符長の比率を補正するために使用する
 */
void Midi::TimeExpand(                                                          // TimeBaseを変更せず、全イベントの時間を指定倍率で伸縮する
    float ratio)                                                                ///< (i)伸縮率
{
  ChengeTimeType(TimeType::kRelative);                                          // 相対時間にしておく
  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    double sum(0);                                                              ///< 積算時間
    double ex_sum(0);                                                           ///< 時間伸縮後の積算時間
    // 制御リストループ
    for (auto& ope : track) {                                                   // 制御リストループ
      // 時間を伸縮
      // ※伸縮による時間のズレを解消するため、曲先頭からの時間を求め伸縮を行う
      sum += ope.time;                                                          // 積算時間に今回の時間を加算
      double now_ex_sum = sum * ratio;                                          ///< 今回の積算時間伸縮値を計算
      ope.time = static_cast<uint32_t>(now_ex_sum) - static_cast<uint32_t>(ex_sum); // 丸め処理した積算時間の差を新しい時間とする
      ex_sum = now_ex_sum;                                                      // 今回の積算時間伸縮値を保管
      // テンポ補正
      // ※時間を伸縮すると曲の速度も変わるため、テンポを補正する
      if ((0xFF == ope.status) && (0x51 == ope.status1)) {                      // テンポイベントなら
        uint32_t tempo = (static_cast<uint8_t>(ope.ex_data[0]) << 16) |         // テンポを取り出す
                      (static_cast<uint8_t>(ope.ex_data[1]) << 8) |
                       static_cast<uint8_t>(ope.ex_data[2]);
        tempo = static_cast<uint32_t>(1 / ratio * tempo);                       // テンポを伸縮
        ope.ex_data[0] = static_cast<char>(tempo >> 16);                        // テンポを格納
        ope.ex_data[1] = static_cast<char>(tempo >> 8);
        ope.ex_data[2] = static_cast<char>(tempo);
      }
    }
  }
}

/** @brief MIDIの分解能と全イベントの時間を変更する */
void Midi::ChangeResolution(                                                    // MIDIの分解能と全イベントの時間を変更する
    uint16_t new_time_base)                                                     ///< (i)新しい分解能
{
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  double ratio = static_cast<double>(new_time_base) / time_base_;               ///< 伸縮率を計算

  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    // 制御リストループ
    for (auto& ope : track) {                                                   // 制御リストループ
      // 時間を伸縮
      ope.precise_time = ope.time * ratio;                                      // ベンド用に丸め前の絶対tickを保存する
      ope.time = static_cast<uint32_t>(ope.precise_time + .5);                  // 0.5tickを加えて整数化し、音符の時刻を最も近いtickへ丸める
    }
  }
  time_base_ = new_time_base;                                                   // 新しい分解能に更新
}

/** @brief 全イベントの時間情報を相対時間または絶対時間へ変更する */
void Midi::ChangeTimeType(                                                      // 全イベントの時間情報を相対時間または絶対時間へ変更する
    TimeType new_time_type)                                                     ///< (i)新しい時間タイプ
{
  if (new_time_type == time_type_) return;                                      // 変更なければ帰れ
  // 絶対時間に変更する場合
  if (new_time_type == TimeType::kAbsolute) {
    // トラックリストループ
    for (auto& track : track_list_) {                                           // トラックリストループ
      uint32_t sum(0);                                                          ///< 絶対時間
      // 制御リストループ
      for (auto& ope : track) {                                                 // 制御リストループ
        sum += ope.time;                                                        // 絶対時間に相対時間を加算し
        ope.time = sum;                                                         // 絶対時間を格納する
      }
    }
  }
  // 相対時間に変更する場合
  else {
    // トラックリストループ
    for (auto& track : track_list_) {                                           // トラックリストループ
      uint32_t last(0);                                                         ///< 直前の時間
      // 制御リストループ
      for (auto& ope : track) {                                                 // 制御リストループ
        uint32_t now = ope.time;                                                ///< 今回絶対時間を退避
        ope.time -= last;                                                       // 相対時間は今回から前回を引いた時間を格納する
        last = now;                                                             // 次の前回絶対時間に今回絶対時間をセット
      }
    }
  }
  time_type_ = new_time_type;                                                   // 目的の時間タイプをセット
}

/**
 * @brief MIDIフォーマット0または1へ変換する
 * フォーマット0では全トラックを先頭トラックへ統合し、フォーマット1では
 * イベントをチャンネル別に分類する
 */
void Midi::ChangeFormat(                                                        // MIDIフォーマット0または1へ変換する
    uint16_t format)                                                            ///< (i)フォーマット
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  // フォーマット０に変更----
  if (0 == format) {
    // ２番トラック以降を先頭トラックにマージ
    auto first_track_it = track_list_.begin();                                  ///< 先頭トラックを指すイテレータ
    first_track_it->sort();
    auto track_it = first_track_it;                                             ///< それ以降のトラック
    for (++track_it; track_it != track_list_.end();) {                          // トラック削除により手前に詰められるためイテレータを進めない
      track_it->sort();
      first_track_it->merge(*track_it);                                         // 先頭トラックにマージ
      track_it = track_list_.erase(track_it);                                   // 不要になったトラックを破棄
    }
    format_ = 0;                                                                // フォーマット０にする
  }
  // フォーマット１に変更----
  else if (1 == format) {
    // トラックにある全ての制御をチャンネル毎に分類し移し変える
    list<Operate> ch_list[17];                                                  ///< チャンネルリスト
    // トラックリストループ
    for (auto& track : track_list_) {                                           // トラックリストループ
      // 一番多いチャンネルを、このトラックのチャンネルとする
      int this_ch(0);                                                           ///< 一番多いチャンネル
      if (format_ == 1) {                                                       // フォーマットが１だったら
        uint32_t ch_cnt[16] = {0};                                              ///< チャンネルカウンタ
        // 制御リストループ
        for (const auto& ope : track) {                                         // 制御リストループ
          if (0xF0 != (0xF0 & ope.status)) {                                    // システムSysEx以外なら
            ++ch_cnt[(0xF & ope.status)];                                       // チャンネルを数える
          }
        }
        // 一番多いチャンネルを検索
        uint32_t ch_max(0);                                                     ///< チャンネル数最大値
        for (int ch = 0; ch < 16; ++ch) {                                       ///< チャンネル番号
          if (ch_max < ch_cnt[ch]) {                                            // 最大値より多ければ
            ch_max = ch_cnt[ch];                                                // 最大値更新
            this_ch = ch + 1;                                                   // 一番多いチャンネル更新
          }
        }
      }
      // 制御をチャンネルに分類する
      for (auto ope_it = track.begin(); !track.empty(); ope_it = track.begin()) { // 制御リストを先頭から処理し、削除後は先頭へ戻す
        int ch;                                                                 ///< チャンネル番号
        uint8_t sta = 0xF0 & ope_it->status;                                    ///< イベント種別
        if (0xF0 == sta) {                                                      // システムSysExなら
          if ((0xFF == ope_it->status) &&                                       // メタイベントで
              ((0x06 == ope_it->status1) ||                                     // マーカー　または
               (0x51 == ope_it->status1) ||                                     // テンポ設定　または
               (0x58 == ope_it->status1))) {                                    // 拍子の設定　なら
            ch = 0;                                                             // Conductor Trackに分類
          } else {                                                              // それ以外なら
            ch = this_ch;                                                       // このトラックのチャンネルに分類
          }
        } else {                                                                // システムSysEx以外は
          ch = (0xF & ope_it->status) + 1;                                      // ステータスの下４ビットがチャンネル
        }
        ch_list[ch].splice(ch_list[ch].end(), track, ope_it);                   // トラックリストからチャンネルリストに制御を移す
      }
    }
    // トラックリストをクリア
    track_list_.clear();
    // 全チャンネルをトラックに戻す
    for (int ch = 0; ch < 17; ++ch) {                                           ///< チャンネル番号
      if (!ch_list[ch].empty()) {                                               // このチャンネルが空じゃなければ
        ch_list[ch].sort();                                                     // ソートをかける
        track_list_.push_back(ch_list[ch]);                                     // トラックにチャンネルを追加し
        // (メモリ効率悪っ)
        ch_list[ch].clear();                                                    // このチャンネルをクリア
      }
    }
    format_ = 1;                                                                // フォーマット１にする
  }
  // トラックエンド付け替え
  AppendAllTrackEnd(DeleteAllTrackEnd());                                       // トラックエンド付け替え
}

/** @brief 全トラックの終端イベントを削除し、最後のイベント時刻を返す */
uint32_t Midi::DeleteAllTrackEnd(void)                                          // 全トラックの終端イベントを削除し、最後のイベント時刻を返す
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  // トラックリストループ
  uint32_t last_time(0);                                                        ///< 最後の時間
  for (auto& track : track_list_) {                                             // トラックリストループ
    // 制御リストループ
    for (auto ope_it = track.begin(); ope_it != track.end();) {                 // 制御リストループし、ループ内で進める
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      if (last_time < ope.time) {                                               // 最後の時間なら
        last_time = ope.time;                                                   // 更新する
      }
      if ((0xFF == ope.status) && (0x2F == ope.status1)) {                      // トラックエンドなら
        ope_it = track.erase(ope_it);                                           // 削除
      } else {                                                                  // それ以外なら
        ++ope_it;
      }
    }
  }
  return (last_time);                                                           // トラック最後の時間
}

/** @brief 全トラックにトラック終端イベントを追加する */
void Midi::AppendAllTrackEnd(                                                   // 全トラックにトラック終端イベントを追加する
    const uint32_t& last_time)                                                  ///< (i)トラックエンド時間
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  Operate track_end(last_time, 0xFF, 0x2F);                                     ///< トラックエンドを作成
  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    track.push_back(track_end);                                                 // トラックエンドを追加
  }
}

/** @brief 指定された削除モードに対応するSysExイベントを削除する */
void Midi::DeleteSysEx(                                                         // 指定された削除モードに対応するSysExイベントを削除する
    uint16_t mode)                                                              ///< (i)SysEx削除モード
{
  if (0 == mode) return;                                                        // モード０なら削除しない
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  string title;                                                                 ///< タイトル
  // トラックリストループ
  for (auto track_it  = track_list_.begin();
            track_it != track_list_.end(); ++track_it) {                        // トラックリストループ
    // 制御リストループ
    for (auto ope_it  = track_it->begin();
              ope_it != track_it->end();) {                                     // 制御リストを先頭から処理し、ループ内で進める
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      if (0xF0 != sta) {                                                        // SysEx以外
        ++ope_it;                                                               // 制御リストのイテレータを進める
        continue;                                                               // 次の制御へ
      } else if (0xFF == ope.status) {                                          // メタイベントなら
        switch (ope.status1) {                                                  // ステータス１が
        case 0x03:                                                              // 曲名/トラック名
          if (title.empty()) {                                                  // タイトル未取得なら
            title = ope.ex_data;                                                // タイトルを取る
            ++ope_it;
            break;                                                              // このタイトル情報は残す
          }
          [[fallthrough]];                                                      // タイトル取得済みなら続行

        case 0x01:                                                              // テキスト
        case 0x02:                                                              // 著作権表示
        case 0x04:                                                              // 楽器名
        case 0x05:                                                              // 歌詞
        case 0x08:                                                              // プログラム名(音色名)
        case 0x09:                                                              // デバイス名(音源名)
        case 0x0A:                                                              // 未使用のメタイベント
        case 0x0B:                                                              // 未使用のメタイベント
        case 0x0C:                                                              // 未使用のメタイベント
        case 0x0D:                                                              // 未使用のメタイベント
        case 0x0E:                                                              // 未使用のメタイベント
        case 0x0F:                                                              // 未使用のメタイベント
        case 0x10:                                                              // 未使用のメタイベント
        case 0x11:                                                              // 未使用のメタイベント
        case 0x12:                                                              // 未使用のメタイベント
        case 0x14:                                                              // 未使用のメタイベント
        case 0x15:                                                              // 未使用のメタイベント
        case 0x18:                                                              // 未使用のメタイベント
        case 0x19:                                                              // 未使用のメタイベント
        case 0x1A:                                                              // 未使用のメタイベント
        case 0x1B:                                                              // 未使用のメタイベント
        case 0x1C:                                                              // 未使用のメタイベント
        case 0x1D:                                                              // 未使用のメタイベント
        case 0x1E:                                                              // 未使用のメタイベント
        case 0x1F:                                                              // 未使用のメタイベント
          if (0x1 & mode)                                                       // モード１なら
            ope_it = track_it->erase(ope_it);                                   // 制御を削除
          else                                                                  // それ以外なら
            ++ope_it;                                                           // 制御リストのイテレータを進める
          break;

        case 0x7F:                                                              // シーケンサ特定メタイベント
          if (0x4 & mode)                                                       // モード４なら
            ope_it = track_it->erase(ope_it);                                   // 制御を削除
          else                                                                  // それ以外なら
            ++ope_it;                                                           // 制御リストのイテレータを進める
          break;

        case 0x06:                                                              // マーカー
        case 0x58:                                                              // 拍子
        case 0x59:                                                              // 調
          if (0x2 & mode)                                                       // モード２なら
            ope_it = track_it->erase(ope_it);                                   // 制御を削除
          else {                                                                // それ以外なら
            if (track_it != track_list_.begin())                                // 先頭トラック以外
              ope_it = track_it->erase(ope_it);                                 // 制御を削除
            else ++ope_it;                                                      // 制御リストのイテレータを進める
          }
          break;

        case 0x51:                                                              // テンポ設定
          if (track_it != track_list_.begin())                                  // 先頭トラック以外
            ope_it = track_it->erase(ope_it);                                   // 制御を削除
          else ++ope_it;                                                        // 制御リストのイテレータを進める
          break;

        case 0x2F:                                                              // トラックエンド
          ++ope_it;                                                             // 制御リストのイテレータを進める
          break;

        default:                                                                // それ以外
          if (0x8 & mode)                                                       // モード８なら
            ope_it = track_it->erase(ope_it);                                   // 制御を削除
          else                                                                  // それ以外なら
            ++ope_it;                                                           // 制御リストのイテレータを進める
          break;
        }
      } else {                                                                  // メタイベント以外のSysExなら
        if (0x4 & mode)                                                         // モード４なら
          ope_it = track_it->erase(ope_it);                                     // 制御を削除
        else                                                                    // それ以外なら
          ++ope_it;                                                             // 制御リストのイテレータを進める
      }
    }
  }
}

/**
 * @brief 音符またはSysExが無いチャンネルの不要な制御を削除する
 * 音符が別トラックに存在する場合は、同じチャンネルの制御を保持する
 */
void Midi::DeleteUnusedOperate(void)                                            // 音符またはSysExが無いチャンネルの不要な制御を削除する
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  // チャンネルの音符数をカウント
  int ch_cnt[16] = {0};                                                         ///< チャンネルの音符数
  // トラックリストループ
  for (const auto& track : track_list_) {                                       // トラックリストループ
    // 制御リストループ
    for (const auto& ope : track) {                                             // 制御リストループ
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      if ((0x90 == sta) || (0x80 == sta)) {                                     // ノートオンまたはノートオフならば
        ++ch_cnt[0xF & ope.status];                                             // そのチャンネルの音符カウント
      }
    }
  }
  // 未使用チャンネルを対象とするトラックを削除
  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    // 制御リストループ
    for (auto ope_it  = track.begin();
              ope_it != track.end();) {                                         // 制御リストを先頭から処理し、ループ内で進める
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      if ((0x90 == sta) || (0x80 == sta) || (0xF0 == sta)) {                    // ノートオン、ノートオフ、またはSysExなら
        ++ope_it;                                                               // 制御リストのイテレータを進める
      } else if (ch_cnt[0xF & ope.status]) {                                    // それ以外なら
        // そのチャンネルに音符があれば
        ++ope_it;                                                               // 制御リストのイテレータを進める
      } else {                                                                  // そのチャンネルに音符が無ければ
        ope_it = track.erase(ope_it);                                           // その制御を削除
      }
    }
  }
}

/** @brief イベントを持たないトラックを削除する */
void Midi::DeleteEmptyTrack(void)                                               // イベントを持たないトラックを削除する
{
  // トラックリストループ
  for (auto track_it  = track_list_.begin();
            track_it != track_list_.end();) {                                   // トラックを先頭から処理し、ループ内で進める
    if (track_it->empty()) {                                                    // 空トラックなら
      track_it = track_list_.erase(track_it);                                   // このトラックを削除
    } else {
      auto ope_it = track_it->begin();                                          // イテレータに制御リストの先頭をセット
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      if ((0xFF == ope.status) && (0x2F == ope.status1)) {                      // 先頭がトラックエンドなら空トラックなので
        track_it = track_list_.erase(track_it);                                 // このトラックを削除
      } else {                                                                  // それ以外なら
        ++track_it;                                                             // トラックイテレータを進める
      }
    }
  }
}

/** @brief ノートオフの表現を0x80または0x90ベロシティ0へ切り替える */
void Midi::ChangeNoteEndType(                                                   // ノートオフの表現を0x80または0x90ベロシティ0へ切り替える
    uint16_t new_note_end)                                                      ///< (i)新しいノートエンド表現 0x80 / 0x90
{
  // ノートエンド表現 0x80 に変更する場合
  if (0x80 == new_note_end) {
      // トラックリストループ
    for (auto& track : track_list_) {                                           // トラックリストループ
      // 制御リストループ
      for (auto& ope : track) {                                                 // 制御リストループ
        uint8_t sta = 0xF0 & ope.status;                                        ///< イベント抽出
        if ((0x90 == sta) && (0x00 == ope.status2)) {                           // ノートエンド 0x90ベロシティ０なら
          ope.status = 0x80 | (0xF & ope.status);                               // ノートエンド 0x80に変更
        }
      }
    }
  }
  // ノートエンド表現 0x90ベロシティ０ に変更する場合
  else {
      // トラックリストループ
    for (auto& track : track_list_) {                                           // トラックリストループ
      // 制御リストループ
      for (auto& ope : track) {                                                 // 制御リストループ
        uint8_t sta = 0xF0 & ope.status;                                        ///< イベント抽出
        if (0x80 == sta) {                                                      // ノートエンド 0x80なら
          ope.status = 0x90 | (0xF & ope.status);                               // ノートエンド 0x90に変更
          ope.status2 = 0x00;                                                   // ベロシティ０に変更
        }
      }
    }
  }
}

/**
 * @brief パーカッションチャンネルをMIDI 9チャンネルへ統一する
 * SysExで指定されたパーカッションチャンネル情報を利用する
 */
void Midi::UnifyPercussionCh9(void)                                             // パーカッションチャンネルをMIDI 9チャンネルへ統一する
{
  // パーカッションチャンネルを検出する
  int per_ch[16] = {0};                                                         ///< パーカッションチャンネル
  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    // 制御リストループ
    for (auto& ope : track) {                                                   // 制御リストループ
      if ((0xF0 == (0xF0 & ope.status)) &&                                      // SysExでかつ
          (0xFF != ope.status) &&                                               // メタイベント以外でかつ
          (9 <= ope.ex_data.size()) &&                                          // 9Byte以上かつ
          ("\x41\x10\x42\x12\x40" ==
           ope.ex_data.substr(0, 5)) &&                                         // 先頭 5Byteがパーカッション指定なら
          (0x10 == (0xF0 & ope.ex_data[5])) &&                                  // 上位ビットも確認
          (0x15 == ope.ex_data[6])) {                                           // ここも確認
        int no = 0xF & ope.ex_data[5];                                          ///< パートを抽出
        if (0 == no)                                                            // パートが０なら
          no = 9;                                                               // チャンネルは９だよ
        else if (9 >= no)                                                       // パート９以下なら
          --no;                                                                 // チャンネルはパートの１つ下
        per_ch[no] = ope.ex_data[8];                                            // パーカッションチャンネルのフラグOn/Off
      }
    }
  }
  // パーカッションチャンネルを9chに統一する
  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    // 制御リストループ
    for (auto& ope : track) {                                                   // 制御リストループ
      if ((0xF0 != (ope.status & 0xF0)) && per_ch[0xF & ope.status]) {          // SysEx以外で、かつパーカッションチャンネルなら
        ope.status = 9 | (0xF0 & ope.status);                                   // チャンネルを９にする
      }
    }
  }
}

/**
 * @brief パーカッションチャンネルをノイズ音源用とDPCM音源用に分離する
 * @param note_map (i)音符番号ごとの音源割り当て0はノイズ、1はDPCM
 */
void Midi::DividePercussion(                                                    // パーカッションチャンネルをノイズ音源用とDPCM音源用に分離する
    const KeyMap* note_map)                                                     ///< (i)割り当て表 0:ノイズ 1:DPCM
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  static const KeyMap kDefaultMap =                                             ///< デフォルトの音源割り当て表
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
       0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  const KeyMap& effective_note_map = note_map ? *note_map : kDefaultMap;        ///< 音符番号ごとの音源割り当て表
  // トラックリストループ
  for (auto track_it  = track_list_.begin();
            track_it != track_list_.end(); ++track_it) {                        // トラックリストループ
    list<Operate> dpcm_track;                                                   ///< DPCM用トラック
    // 制御リストループ
    for (auto ope_it  = track_it->begin();
              ope_it != track_it->end();) {                                     // 制御リストループし、ループ内で進める
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      uint8_t ch = 0xF & ope.status;                                            ///< チャンネル抽出
      if (9 != ch) {                                                            // パーカッションチャンネル以外は
        ++ope_it;                                                               // 制御イテレータを進める
        continue;                                                               // 対象外
      }
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      if (0xB0 <= sta) {                                                        // 音符／ポリフォニックキープレッシャー以外
        ++ope_it;                                                               // 制御イテレータを進める
        continue;                                                               // そのまま
      }
      if (!effective_note_map[ope.status1]) {                                   // このキーがノイズ向けなら
        ++ope_it;                                                               // 制御イテレータを進める
        continue;                                                               // そのまま
      }
      // DPCM用の音符をDPCM用トラックに移す
      auto it_work = ope_it;                                                    ///< splice用のイテレータにセット
      ++ope_it;                                                                 // 制御イテレータを進める
      dpcm_track.splice(dpcm_track.end(), *track_it, it_work);                  // この制御をDPCM用トラックに移す
    }
    // DPCM用トラックに１つでも音符が入ってたらトラックリストに挿入
    if (!dpcm_track.empty()) {                                                  // DPCM用トラックが空じゃなければ
      // 制御リストループ
      for (const auto& ope : *track_it) {                                       // 制御リストループ
        uint8_t sta = 0xF0 & ope.status;                                        ///< イベント抽出
        if (0xB0 <= sta) {                                                      // コントロールチェンジ・プログラムチェンジ・チャンネルプレッシャー・ピッチベンド・SysExなら
          dpcm_track.push_back(ope);                                            // DPCM用トラックに追加
        }
      }
      dpcm_track.sort();                                                        // DPCM用トラックにソートをかける
      ++track_it;                                                               // トラックイテレータを進める
      track_it = track_list_.insert(track_it, dpcm_track);                      // トラックリストにDPCM用トラックを挿入
      dpcm_track.clear();                                                       // DPCM用トラックをクリア
    }
  }
}

/**
 * @brief パーカッションチャンネルの重複音符を削除する
 * @param priority (i)音符番号ごとの優先度値が大きい音符を残す
 */
void Midi::DeleteCh9RepeatNote(                                                 // パーカッションチャンネルの重複音符を削除する
    const KeyMap* priority)                                                     ///< (i)優先度表 0～127 大きいほど優先
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  static const KeyMap kDefaultPriority =                                        ///< デフォルトの音符優先度表
      {0,                                                                       //   0:
       1,                                                                       //   1:
       2,                                                                       //   2:
       3,                                                                       //   3:
       4,                                                                       //   4:
       5,                                                                       //   5:
       6,                                                                       //   6:
       7,                                                                       //   7:
       8,                                                                       //   8:
       9,                                                                       //   9:
       10,                                                                      //  10:
       11,                                                                      //  11:
       12,                                                                      //  12:
       13,                                                                      //  13:
       14,                                                                      //  14:
       15,                                                                      //  15:
       16,                                                                      //  16:
       17,                                                                      //  17:
       18,                                                                      //  18:
       19,                                                                      //  19:
       20,                                                                      //  20:
       21,                                                                      //  21:
       22,                                                                      //  22:
       23,                                                                      //  23:
       68,                                                                      //  24:
       69,                                                                      //  25:Snare Roll
       70,                                                                      //  26:Finger Snap
       71,                                                                      //  27:High Q
       72,                                                                      //  28:Slap
       73,                                                                      //  29:Scratch Push
       74,                                                                      //  30:Scratch Pull
       75,                                                                      //  31:Sticks
       76,                                                                      //  32:Square Click
       77,                                                                      //  33:Metronome Click
       78,                                                                      //  34:Metronome Bell
       103,                                                                     //  35:Bass Drum 2
       104,                                                                     //  36:Bass Drum 1
       106,                                                                     //  37:Side Stick
       108,                                                                     //  38:Snare Drum 1
       111,                                                                     //  39:Hand Clap
       109,                                                                     //  40:Snare Drum 2
       112,                                                                     //  41:Low Tom 2
       113,                                                                     //  42:Closed Hi-hat
       114,                                                                     //  43:Low Tom 1
       115,                                                                     //  44:Pedal Hi-hat
       116,                                                                     //  45:Mid Tom 2
       117,                                                                     //  46:Open Hi-hat
       119,                                                                     //  47:Mid Tom 1
       120,                                                                     //  48:High Tom 2
       121,                                                                     //  49:Crash Cymbal 1
       122,                                                                     //  50:High Tom 1
       123,                                                                     //  51:Ride Cymbal 1
       102,                                                                     //  52:Chinese Cymbal
       124,                                                                     //  53:Ride Bell
       125,                                                                     //  54:Tambourine
       126,                                                                     //  55:Splash Cymbal
       110,                                                                     //  56:Cowbell
       127,                                                                     //  57:Crash Cymbal 2
       105,                                                                     //  58:Vibra Slap
       107,                                                                     //  59:Ride Cymbal 2
       101,                                                                     //  60:High Bongo
       100,                                                                     //  61:Low Bongo
       99,                                                                      //  62:Mute High Conga
       98,                                                                      //  63:Open High Conga
       97,                                                                      //  64:Low Conga
       96,                                                                      //  65:High Timbale
       95,                                                                      //  66:Low Timbale
       94,                                                                      //  67:High Agogo
       93,                                                                      //  68:Low Agogo
       92,                                                                      //  69:Cabasa
       91,                                                                      //  60:Maracas
       90,                                                                      //  71:Short Whistle
       89,                                                                      //  72:Long Whistle
       88,                                                                      //  73:Short Guiro
       87,                                                                      //  74:Long Guiro
       86,                                                                      //  75:Claves
       85,                                                                      //  76:High Wood Block
       84,                                                                      //  77:Low Wood Block
       83,                                                                      //  78:Mute Cuica
       82,                                                                      //  79:Open Cuica
       81,                                                                      //  70:Mute Triangle
       80,                                                                      //  81:Open Triangle
       118,                                                                     //  82:Shaker
       79,                                                                      //  83:Jingle Bell
       67,                                                                      //  84:Bell Tree
       66,                                                                      //  85:Castanets
       65,                                                                      //  86:Mute Surdo
       64,                                                                      //  87:Open Surdo
       63,                                                                      //  88:Low Whistle
       62,                                                                      //  89:Mute Cuica
       61,                                                                      //  90:Open Cuica
       60,                                                                      //  91:Mute Triangle
       59,                                                                      //  92:Open Triangle
       58,                                                                      //  93:Short Guiro
       57,                                                                      //  94:Long Guiro
       56,                                                                      //  95:Cabasa Up
       55,                                                                      //  96:Cabasa Down
       54,                                                                      //  97:Claves
       53,                                                                      //  98:High Wood Block
       52,                                                                      //  99:Low Wood Block
       51,                                                                      // 100:
       50,                                                                      // 101:
       49,                                                                      // 102:
       48,                                                                      // 103:
       47,                                                                      // 104:
       46,                                                                      // 105:
       45,                                                                      // 106:
       44,                                                                      // 107:
       43,                                                                      // 108:
       42,                                                                      // 109:
       41,                                                                      // 110:
       40,                                                                      // 111:
       39,                                                                      // 112:
       38,                                                                      // 113:
       37,                                                                      // 114:
       36,                                                                      // 115:
       35,                                                                      // 116:
       34,                                                                      // 117:
       33,                                                                      // 118:
       32,                                                                      // 119:
       31,                                                                      // 120:
       30,                                                                      // 121:
       29,                                                                      // 122:
       28,                                                                      // 123:
       27,                                                                      // 124:
       26,                                                                      // 125:
       25,                                                                      // 126:
       24};                                                                     // 127:
  const KeyMap& effective_priority = priority ? *priority : kDefaultPriority;   ///< 音符番号ごとの優先度表
  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    enum {
      kIdle,                                                                    // 空状態
      kSearchSubjectNoteOff,                                                    // 対象音符オフ探索状態
      kSearchRepeatNote,                                                        // 重複音符探索状態
      kSearchDeleteNoteOff,                                                     // 削除音符オフ探索状態
    } condition(kIdle);                                                         // 状態
    auto subject_note_on = track.end();                                         ///< 対象音符ノートオンの位置
    auto subject_note_off = track.end();                                        ///< 対象音符ノートオフの位置
    uint32_t
        subject_tail_time;                                                      // 対象音符の尻尾時間（これ以降に音符が重複したら尻尾を切り落とす）
    uint8_t delete_note_key;                                                    ///< 削除音符のキー
    // 制御リストループ
    for (auto ope_it  = track.begin();
              ope_it != track.end();) {                                         // 制御リストを先頭から処理し、ループ内で進める
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      uint8_t ch = 0xF & ope.status;                                            ///< チャンネル抽出
      if (9 != ch) {                                                            // チャンネルが９以外
        ++ope_it;                                                               // 制御イテレータを進める
        continue;                                                               // 対象外
      }
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      switch (condition) {                                                      // 状態
      case kIdle:                                                               // 空状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン（対象音符を見つけた）
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            condition = kSearchSubjectNoteOff;                                  // 対象音符オフ探索状態に移行する
            subject_note_on = ope_it;                                           // 対象音符ノートオンの位置を記録
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          cerr << "空状態でノートオンが無いノートオフがありました\n";
          break;
        }
        ++ope_it;                                                               // 制御イテレータを進める
        break;

      case kSearchSubjectNoteOff:                                               // 対象音符オフ探索状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          if (ope.status1 == subject_note_on->status1) {                        // 対象音符のキーなら
            // 尻尾時間を求める
            uint32_t length = ope.time - subject_note_on->time;                 // 対象音符の長さを求める
            float tail_ratio = 1.F - (time_base_ / 480000.F * length);          // 尻尾割合を求める(４分音符なら48%)
            if (.05F > tail_ratio) {                                            // 5%以下なら
              tail_ratio = .05F;                                                // 5%とする
            }
            subject_tail_time = ope.time - static_cast<uint32_t>(tail_ratio * length); // 尻尾時間＝対象音符オフ時間－尻尾割合×対象音符の長さ
            subject_note_off = ope_it;                                          // 対象音符ノートオフの位置を記録
            ope_it = subject_note_on;                                           // 対象音符ノートオンの位置に戻る
            condition = kSearchRepeatNote;                                      // 重複音符探索状態
          }
          break;
        }
        ++ope_it;                                                               // 制御イテレータを進める
        break;

      case kSearchRepeatNote:                                                   // 重複音符探索状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン（重複音符を見つけた）
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            if (subject_tail_time < ope.time) {                                 // 尻尾に重複したら 尻尾切り落とし
              subject_note_off->time = ope.time;                                // 対象音符の時間を重複音符の時間に切り詰める
              track.splice(ope_it, track, subject_note_off);                    // 対象音符ノートオフを重複音符の直前に移動
              // ここでは制御イテレータは進めない
              condition = kIdle;                                                // 空状態に移行する
              cerr << "." << flush;
            } else {                                                            // 完全重複
              if (effective_priority[subject_note_on->status1] > effective_priority[ope.status1]) { // 対象音符の優先度が重複音符の優先度より高ければ
                delete_note_key = ope.status1;                                  // 削除音符のキーは重複音符のキーを記録
              } else {                                                          // 重複音符の優先度が高ければ
                delete_note_key = subject_note_on->status1;                     // 削除音符のキーは対象音符のキーを記録
                swap(ope_it, subject_note_on);                                  // 対象の音符に戻る、この音符を対象の音符ノートオンとする（戻り先）
              }
              ope_it = track.erase(ope_it);                                     // この制御を削除
              condition = kSearchDeleteNoteOff;                                 // 削除音符オフ探索状態に移行する
            }
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          if (ope.status1 == subject_note_on->status1) {                        // 対象音符のキーなら
            condition = kIdle;                                                  // 空状態に移行する
          } else {                                                              // 対象音符以外のキーなら
          cerr << "重複音符探索状態でノートオンが無いノートオフがありました\n";
          }
          [[fallthrough]];                                                      // ↓続行

        default:                                                                // その他
          ++ope_it;                                                             // 制御イテレータを進める
        }
        break;

      case kSearchDeleteNoteOff:                                                // 削除音符オフ探索状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            ++ope_it;                                                           // 制御イテレータを進める
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          if (delete_note_key == ope.status1) {                                 // 削除音符のキーなら
            track.erase(ope_it);                                                // この制御を削除
            ope_it = subject_note_on;                                           // 対象音符ノートオンの位置に戻る
            condition = kSearchSubjectNoteOff;                                  // 対象音符オフ探索状態に移行する
          }
          ++ope_it;                                                             // 制御イテレータを進める
          break;

        case 0xA0:                                                              // ポリフォニックキープレッシャー
          if (delete_note_key == ope.status1) {                                 // 重複音符のキーなら
            ope_it = track.erase(ope_it);                                       // この制御を削除
          } else {
            ++ope_it;                                                           // 制御イテレータを進める
          }
          break;

        default:                                                                // その他のMIDIイベント
          ++ope_it;                                                             // 制御イテレータを進める
        }
        break;
      }
    }
  }
}

/**
 * @brief 音符をトラック終端まで延長する
 * @param ch_mask (i)処理対象とするチャンネルのビットマスク
 */
void Midi::ExtendNote(                                                          // 音符をトラック終端まで延長する
    uint16_t ch_mask)                                                           ///< (i)チャンネルマスク
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  uint32_t note_max_len = 4 * time_base_;                                       ///< 音符の最大長（全音符）
  // トラックリストループ
  for (auto& track : track_list_) {                                             // トラックリストループ
    uint32_t max_extend_time(0);                                                ///< 最大延長時間
    uint32_t note_on_time(0);                                                   ///< ノートオン時間
    auto note_off = track.end();                                                ///< ノートオフの位置
    // 制御リストループ
    for (auto ope_it  = track.begin();
              ope_it != track.end(); ++ope_it) {                                // 制御リストループ
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      if (note_off != track.end()) {                                            // ノートオフがあるなら
        if (max_extend_time > ope.time) {                                       // この時間が最大延長時間未満なら
          note_off->time = ope.time;                                            // 前の音符のノートエンドはこの時間とする
        } else {
          note_off->time = max_extend_time;                                     // 前の音符のノートエンドは最大延長時間とする
        }
        note_off = track.end();                                                 // ノートオフ無しに設定
      }
      uint8_t ch = 0xF & ope.status;                                            ///< チャンネル抽出
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      switch (sta) {                                                            // イベント判定
      case 0x90:                                                                // ノートオン
        if (0 < ope.status2) {                                                  // ベロシティが０以外なら
          if (!((1 << ch) & ch_mask)) {                                         // このチャンネルが対象じゃなければ
            continue;                                                           // 対象外
          }
          note_on_time = ope.time;                                              // ノートオン時間を取得
          break;
        }
        [[fallthrough]];                                                        // ベロシティが０ならノートオフなので↓続行

      case 0x80:                                                                // ノートオフ
        if (!((1 << ch) & ch_mask)) {                                           // このチャンネルが対象じゃなければ
          continue;                                                             // 対象外
        }
        if ((ope.time - note_on_time) < note_max_len) {                         // 音符の長さが音符の最大長未満なら
          note_off = ope_it;                                                    // ノートオフの位置を記録
          max_extend_time = note_on_time + note_max_len;                        // 最大延長時間を計算
        }
        break;
      }
    }
  }
}

/**
 * @brief 重複する音符を別トラックへ分離する
 * 重複後半の音符は指定割合まで切り詰め、制御イベントを複製する
 */
void Midi::DivideRepeatNote(                                                    // 重複する音符を別トラックへ分離する
    float tail_ratio_max,                                                       ///< (i)尻尾割合最大
    uint16_t ch_mask)                                                           ///< (i)チャンネルマスク
{
  // 絶対時間に設定
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  // トラックリストループ
  for (auto track_it  = track_list_.begin();
            track_it != track_list_.end();) {                                   // トラックリストループし、ループ内で進める
    enum {
      kIdle,                                                                    // 空状態
      kSearchSubjectNoteOff,                                                    // 対象音符オフ探索状態
      kSearchRepeatNote,                                                        // 重複音符探索状態
      kSearchRepeatNoteOff,                                                     // 重複音符オフ探索状態
    } condition(kIdle);                                                         // 状態
    auto subject_note_on = track_it->end();                                     ///< 対象音符ノートオンの位置
    auto subject_note_off = track_it->end();                                    ///< 対象音符ノートオフの位置
    uint32_t subject_tail_time;                                                 ///< 対象音符の尻尾時間（これ以降に音符が重複したら尻尾を切り落とす）
    list<Operate> repeat_track;                                                 ///< 重複音符用トラック
    uint8_t repeat_key;                                                         ///< 重複音符のキー
    // 制御リストループ
    for (auto ope_it  = track_it->begin();
              ope_it != track_it->end(); ++ope_it) {                            // 制御リストループ
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      uint8_t ch = 0xF & ope.status;                                            ///< チャンネル抽出
      if (!((1 << ch) & ch_mask)) {                                             // このチャンネルが対象じゃなければ
        continue;                                                               // 対象外
      }
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      switch (condition) {                                                      // 状態
      case kIdle:                                                               // 空状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン（対象音符を見つけた）
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            condition = kSearchSubjectNoteOff;                                  // 対象音符オフ探索状態に移行する
            subject_note_on = ope_it;                                           // 対象音符ノートオンの位置を記録
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          cerr << "空状態でノートオンが無いノートオフがありました\n";
          break;
        }
        break;

      case kSearchSubjectNoteOff:                                               // 対象音符オフ探索状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          if (ope.status1 == subject_note_on->status1) {                        // 対象音符のキーなら
            // 尻尾時間を求める
            uint32_t length = ope.time - subject_note_on->time;                 // 対象音符の長さを求める
            float tail_ratio = static_cast<float>(length) / time_base_;         // 尻尾割合を求める(４分音符なら48%を切り落とす対象とする)
            if (tail_ratio_max < tail_ratio) {                                  // 尻尾割合最長を超えたら
              tail_ratio = tail_ratio_max;                                      // 尻尾割合最長とする
            }
            subject_tail_time = ope.time - static_cast<uint32_t>(tail_ratio * length); // 尻尾時間＝対象音符オフ時間－尻尾割合×対象音符の長さ
            subject_note_off = ope_it;                                          // 対象音符ノートオフの位置を記録
            ope_it = subject_note_on;                                           // 対象音符ノートオンの位置に戻る
            condition = kSearchRepeatNote;                                      // 重複音符探索状態
          }
          break;
        }
        break;

      case kSearchRepeatNote:                                                   // 重複音符探索状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン（重複音符を見つけた）
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            if (subject_tail_time < ope.time) {                                 // 尻尾に重複したら 尻尾切り落とし
              subject_note_off->time = ope.time;                                // 対象音符の時間を重複音符の時間に切り詰める
              track_it->splice(ope_it, *track_it, subject_note_off);            // 対象音符ノートオフを重複音符の直前に移動
              --ope_it;                                                         // 必然的に１つ進んでしまうため戻す
              condition = kIdle;                                                // 空状態に移行する
              cerr << "." << flush;
            } else {                                                            // 完全重複
              repeat_key = ope.status1;                                         // 重複音符のキーを記録
              auto it_work = ope_it;                                            // splice用のイテレータにセット
              --ope_it;                                                         // イテレータを１つ前に退避
              repeat_track.splice(repeat_track.end(), *track_it, it_work);      // この制御を重複音符用トラックに移す
              condition = kSearchRepeatNoteOff;                                 // 重複音符オフ探索状態に移行する
            }
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          if (ope.status1 == subject_note_on->status1) {                        // 対象音符のキーなら
            condition = kIdle;                                                  // 空状態に移行する
          } else {                                                              // 対象音符以外のキーなら
          cerr << "重複音符探索状態でノートオンが無いノートオフがありました\n";
          }
          break;
        }
        break;

      case kSearchRepeatNoteOff:                                                // 重複音符オフ探索状態
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン
          if (0 < ope.status2) {                                                // ベロシティが０以外なら
            break;
          }
          [[fallthrough]];                                                      // ベロシティが０ならノートオフなので↓続行

        case 0x80:                                                              // ノートオフ
          if (repeat_key == ope.status1) {                                      // 重複音符のキーなら
            if (ope_it == subject_note_off) {                                   // このノートオフが対象音符のノートオフなら
              break;                                                            // 対象外とする
            }
            repeat_track.splice(repeat_track.end(), *track_it, ope_it);         // この制御を重複音符用トラックに移す
            ope_it = subject_note_on;                                           // 対象音符ノートオンの位置に戻る
            condition = kSearchRepeatNote;                                      // 重複音符探索状態に移行する
          }
          break;

        case 0xA0:                                                              // ポリフォニックキープレッシャー
          if (repeat_key == ope.status1) {                                      // 重複音符のキーなら
            auto it_work = ope_it;                                              // splice用のイテレータにセット
            --ope_it;                                                           // イテレータを１つ前に退避
            repeat_track.splice(repeat_track.end(), *track_it, it_work);        // この制御を重複音符用トラックに移す
          }
          break;
        }
        break;
      }
    }
    // 重複トラックに１つでも音符が入ってたらトラックリストに挿入
    if (!repeat_track.empty()) {                                                // 重複トラックが空じゃなければ
      cerr << "重複音符がありましたトラック分割します。\n";
      // 制御リストループ
      for (auto ope_it  = track_it->begin();
                ope_it != track_it->end(); ++ope_it) {                          // 制御リストループ
        Operate& ope = *ope_it;                                                 ///< 現在のMIDIイベント
        uint8_t sta = 0xF0 & ope.status;                                        ///< イベント抽出
        if (0xB0 <= sta) {                                                      // コントロールチェンジ・プログラムチェンジ・チャンネルプレッシャー・ピッチベンド・SysExなら
          repeat_track.push_back(ope);                                          // 重複トラックに追加
        }
      }
      repeat_track.sort();                                                      // 重複トラックにソートをかける
      ++track_it;                                                               // トラックイテレータを進める
      track_it = track_list_.insert(track_it, repeat_track);                    // トラックリストに重複トラックを挿入
      repeat_track.clear();                                                     // 重複トラックをクリア
    } else {                                                                    // 空なら
      ++track_it;                                                               // トラックイテレータを進める
    }
  }
}

/**
 * @brief 先頭トラックのテンポ・拍子・マーカーを全トラックへコピーする
 * コピー位置が音符の途中の場合は、音符を分断して再開する
 */
void Midi::CopyTempoAllTrack(void)                                              // 先頭トラックのテンポ・拍子・マーカーを全トラックへコピーする
{
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  // 先頭トラックからテンポを探す
  auto first_track_it = track_list_.begin();                                    ///< 先頭トラックを指すイテレータ
  for (const auto& f_ope : *first_track_it) {                                   // 制御リストループ
    if (!((0xFF == f_ope.status) &&
          ((0x51 == f_ope.status1) ||                                           // （テンポまたは
           (0x58 == f_ope.status1) ||                                           // 拍子または
           (0x06 == f_ope.status1)))) {                                         // マーカー）以外は
      continue;                                                                 // 対象外
    }
    auto track_it = first_track_it;                                             ///< それ以降のトラック
    // トラックリストループ
    for (++track_it; track_it != track_list_.end(); ++track_it) {               // トラックリストループ
      uint8_t note[128] = {0};                                                  ///< 音符状態
      uint8_t ch(0);                                                            ///< このトラックの対象チャンネル
      // 制御リストループ
      for (auto ope_it  = track_it->begin();
                ope_it != track_it->end(); ++ope_it) {                          // 制御リストループ
        Operate& ope = *ope_it;                                                 ///< 現在のMIDIイベント
        uint8_t sta = 0xF0 & ope.status;                                        ///< イベント抽出
        // チャンネル抽出
        if (0xF0 != sta) {                                                      // SysEx以外なら
          ch = 0xF & ope.status;                                                // このトラックのチャンネル抽出
        }
        // テンポ／拍子挿入位置確認
        if ((ope.time > f_ope.time) ||                                          // 挿入位置以降または
            ((ope.time == f_ope.time) &&                                        // 挿入位置で
             !((0x80 == sta) ||                                                 // （ノートオフまたは
               ((0x90 == sta) &&                                                // 　ノートオンで
                (0x00 == ope.status2))))) {                                     // 　ベロシティ０）以外なら
          // ノートオフ挿入
          for (int key = 0; key < 128; ++key) {                                 ///< 確認中のMIDIキー番号
            if (note[key]) {                                                    // このキーが音符の途中なら
              Operate ope_note(f_ope.time, 0x90 | ch, key, 0x00);               // ノートオフを作成
              ope_it = track_it->insert(ope_it, ope_note);                      // ノートオフを挿入
              ++ope_it;                                                         // 制御リストのイテレータを進める
            }
          }
          // テンポ／拍子／マーカー挿入
          ope_it = track_it->insert(ope_it, f_ope);                             // テンポ／拍子／マーカーを挿入
          ++ope_it;                                                             // 制御リストのイテレータを進める
          // ノートオン挿入
          for (int key = 0; key < 128; ++key) {                                 ///< 確認中のMIDIキー番号
            if (note[key]) {                                                    // このキーが音符の途中なら
              Operate ope_note(f_ope.time, 0x90 | ch, key, note[key]);          // ノートオンを作成
              ope_it = track_it->insert(ope_it, ope_note);                      // ノートオンを挿入
              ++ope_it;                                                         // 制御リストのイテレータを進める
            }
          }
          break;                                                                // 次のトラックへ
        }
        // 音符状態確認
        switch (sta) {                                                          // イベント判定
        case 0x90:                                                              // ノートオン
          if (0x00 != ope.status2) {                                            // ベロシティ０でなければ
            note[ope.status1] = ope.status2;                                    // ノートオン状態
            break;
          }
          [[fallthrough]];                                                      // ベロシティ０ならノートオフなので続行

        case 0x80:                                                              // ノートオフ
          note[ope.status1] = 0;                                                // ノートオフ状態
          break;
        }
      }
    }
  }
}

/** @brief 曲先頭の空白を拍子に基づく小節単位で切り詰める */
void Midi::BlankTrim(void)                                                      // 曲先頭の空白を拍子に基づく小節単位で切り詰める
{
  ChengeTimeType(TimeType::kAbsolute);                                          // 絶対時間に変更
  // 最初の音符位置を調べる
  uint32_t first_note_time(0x7FFFFFFF);                                         ///< 最初の音符位置（初期値最大値）
  for (const auto& track : track_list_) {                                       // トラックリストループ
    for (const auto& ope : track) {                                             // 制御リストループ
      if (0x90 == (0xF0 & ope.status)) {                                        // 音符が見つかったら
        if (first_note_time > ope.time) {                                       // より早い位置の音符ならば
          first_note_time = ope.time;                                           // 最初の音符位置を更新する
        }
        break;                                                                  // 次のトラックへ
      }
    }
    if (0 == first_note_time)                                                   // 最初の音符位置が曲先頭なら
      return;                                                                   // 切詰処理無用
  }
  // 最初の音符までに在る最後の拍子の情報（位置と小節の長さ）を調べる
  uint32_t rhythm_time(0);                                                      ///< 拍子の時間（初期値先頭）
  uint32_t bar_len(4 * time_base_);                                             ///< 小節の長さ（初期値4/4拍子）
  {
    auto track_it = track_list_.begin();                                        // トラックリストの先頭を指すイテレータ
    for (const auto& ope : *track_it) {                                         // 制御リストループ
      if (first_note_time < ope.time) {                                         // 最初の音符の位置を超えたら
        break;                                                                  // ループを抜ける
      }
      if ((0xFF == ope.status) && (0x58 == ope.status1)) {                      // 拍子が見つかったら
        rhythm_time = ope.time;                                                 // 拍子の時間を更新
        bar_len = static_cast<uint32_t>((time_base_ << 2) * ope.ex_data[0] /
            static_cast<double>(1 << ope.ex_data[1]));                          // 小節の長さ＝ 全音符×拍子の分子／拍子の分母
      }
    }
  }
  // 切詰る時間を求める
  // 例）  ↓曲先頭↓拍子の時間      　      ↓最初の音符の時間
  //       ┠4/4 ─╂5/4拍子 ─╂─────╂■────╂────
  //       │      └小節の長さ┘          │
  //       └────切詰る時間──────┘
  uint32_t trim_time = ((first_note_time - rhythm_time) / bar_len) * bar_len + rhythm_time; // 最初の音符までに切り詰める時間を計算
  // 切り詰め処理
  for (auto& track : track_list_) {                                             // トラックリストループ
    for (auto& ope : track) {                                                   // 制御リストループ
      ope.time = (ope.time < trim_time) ? 0 : ope.time - trim_time;             // 空白にある制御の時間を０、それ以外を切り詰める
    }
  }
  // 空白に複数のテンポ・拍子が在った場合、曲先頭に溜るので重複してるテンポ・拍子を削除する（最後の１つは残す）
  for (auto& track : track_list_) {                                             // トラックリストループ
    int tempo_cnt(0);                                                           ///< テンポの数
    int rhythm_cnt(0);                                                          ///< 拍子の数
    // 曲先頭にあるテンポ・拍子の数をカウント
    for (const auto& ope : track) {                                             // 制御リストループ
      if (0 < ope.time)                                                         // 時間が０以上の制御が現れたら
        break;                                                                  // カウント完了
      if (0xFF != ope.status)                                                   // SysExでなければ
        continue;                                                               // 次の制御へ
      switch (ope.status1) {
      case 0x51:                                                                // テンポなら
        ++tempo_cnt;                                                            // テンポの数をカウント
        break;

      case 0x58:                                                                // 拍子が見つかったら
        ++rhythm_cnt;                                                           // 拍子の数をカウント
        break;
      }
    }
    // 重複しているテンポ・拍子を削除する（最後の１つは残す）
    for (auto ope_it  = track.begin();
              ope_it != track.end();) {                                         // 制御リストループし、ループ内で進める
      if ((2 > tempo_cnt) && (2 > rhythm_cnt))                                  // テンポも拍子も重複してないなら
        break;                                                                  // ループを抜ける
      Operate& ope = *ope_it;                                                   ///< 現在のMIDIイベント
      if (0 < ope.time)                                                         // 時間が０以上の制御が現れたら
        break;                                                                  // ループを抜ける
      if (0xFF != ope.status) {                                                 // SysExでなければ
        ++ope_it;                                                               // 制御イテレータを進めて
        continue;                                                               // 次へ
      }
      switch (ope.status1) {
      case 0x51:                                                                // テンポなら
        if (1 < tempo_cnt) {                                                    // テンポが重複しているなら
          ope_it = track.erase(ope_it);                                         // この制御を削除
        } else {                                                                // 重複していないなら
          ++ope_it;                                                             // 制御イテレータを進めて
        }
        --tempo_cnt;                                                            // テンポの数を更新
        break;

      case 0x58:                                                                // 拍子が見つかったら
        if (1 < rhythm_cnt) {                                                   // 拍子が重複しているなら
          ope_it = track.erase(ope_it);                                         // この制御を削除
        } else {                                                                // 重複していないなら
          ++ope_it;                                                             // 制御イテレータを進めて
        }
        --rhythm_cnt;                                                           // 拍子の数を更新
        break;

      default:                                                                  // それ以外
        ++ope_it;                                                               // 制御イテレータを進めて
      }
    }
  }
}

/** @brief 全MIDIイベントをデバッグ用に表示する */
void Midi::PrintAllOperate(void) const                                          // 全MIDIイベントをデバッグ用に表示する
{
  // トラックリストループ
  int track_no(0);                                                              ///< トラック番号
  for (const auto& track : track_list_) {                                       // トラックリストループ
    cerr << "Track " << dec << track_no << ":";
    // 制御リストループ
    for (const auto& ope : track) {                                             // 制御リストループ
      cerr << "(" << hex << ope.time << "," << static_cast<uint16_t>(ope.status) << ","
           << static_cast<uint16_t>(ope.status1) << "," << static_cast<uint16_t>(ope.status2); // 表示
      if ((0xFF == ope.status) && (0x01 <= ope.status1) && (0x1F >= ope.status1)) { // SysExのテキストなら
        cerr << ",\"" << EncodeMidiTextForConsole(ope.ex_data) << "\"";         // 元データを維持してUTF-8で表示
      }
      cerr << ") " << flush;
    }
    cerr << '\n';                                                               // 改行
    ++track_no;                                                                 // トラック番号を進める
  }
  cerr << dec;                                                                  // １０進に戻す
}

/** @brief MIDIイベントの時刻情報を検査し、異常を表示する */
void Midi::CheckAllOperate(void) const                                          // MIDIイベントの時刻情報を検査し、異常を表示する
{
  // トラックリストループ
  if (time_type_ == TimeType::kAbsolute) {                                      // 絶対時間
    cerr << "絶対時間\n";
    int track_no(0);                                                            ///< トラック番号
    for (const auto& track : track_list_) {                                     // トラックリストループ
      int ope_no(0);                                                            ///< イベント番号
      uint32_t lasttime(0);                                                     ///< 直前のイベント時刻
      // 制御リストループ
      for (const auto& ope : track) {                                           // 制御リストループ
        if (ope.time < lasttime) {
          cerr << "TrackNo." << track_no << " OpeNo." << ope_no << "に異常なTime("
               << ope.time << ")前のTime(" << lasttime << ")\n";
        }
        lasttime = ope.time;
        ++ope_no;                                                               // イベント番号を進める
      }
      ++track_no;                                                               // トラック番号を進める
    }
  } else {                                                                      // 相対時間
    cerr << "相対時間\n";
    int track_no(0);                                                            ///< トラック番号
    for (const auto& track : track_list_) {                                     // トラックリストループ
      int ope_no(0);                                                            ///< イベント番号
      // 制御リストループ
      for (const auto& ope : track) {                                           // 制御リストループ
        if (ope.time > 0xaaaaaaa) {
          cerr << "TrackNo." << track_no << " OpeNo." << ope_no << "に異常なTime("
               << ope.time << ")\n";
        }
        ++ope_no;                                                               // イベント番号を進める
      }
      ++track_no;                                                               // トラック番号を進める
    }
  }
}

/**
 * @brief 指定したステータスのMIDIイベントを検索して表示する
 * @param status (i)検索対象のMIDIステータス
 * @param status1 (i)検索対象の第1データバイト
 * @param status2 (i)検索対象の第2データバイト
 */
void Midi::FindViewOperate(                                                     // 指定したステータスのMIDIイベントを検索して表示する
    const uint8_t& status,                                                      ///< (i)検索する制御ステータス
    const uint8_t& status1,                                                     ///< (i)検索する制御ステータス１
    const uint8_t& status2) const                                               ///< (i)検索する制御ステータス２
{
  cerr << "デバッグ（制御検索）:" << status << ":" << status1 << ":" << status2
       << '\n';
  // トラックリストループ
  int track_no(0);                                                              ///< トラック番号
  for (const auto& track : track_list_) {                                       // トラックリストループ
    int ope_no(0);                                                              ///< イベント番号
    // 制御リストループ
    for (const auto& ope : track) {                                             // 制御リストループ
      if ((ope.status == status) && (ope.status1 == status1)) {                 // 一致
        cerr << "TrackNo." << track_no << " OpeNo." << ope_no                   // 表示
             << "  Time:" << ope.time << "  Status:" << hex
             << ope.status << ":" << ope.status1 << ":" << ope.status2
             << dec << '\n';
      }
      ++ope_no;                                                                 // イベント番号を進める
    }
    ++track_no;                                                                 // トラック番号を進める
  }
}
