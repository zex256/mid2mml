// MMLクラス
#include "mml.h"

#include <math.h>                                                               // 乗数計算に使用

#include <algorithm>                                                            // replaceとかで使用
#include <cstdlib>                                                              // 環境変数取得用
#include <fstream>                                                              // ファイル
#include <iostream>

using std::cerr, std::getenv, std::istringstream, std::ios;
using std::map, std::ofstream, std::ostream, std::ostringstream;
using std::pow, std::size_t, std::string;
using std::uint16_t, std::uint32_t, std::uint8_t, std::vector;

namespace {

constexpr double kNesMasterClock = 21477272.7272;                               // ファミコンのマスタークロック
constexpr double kNesSystemClock = kNesMasterClock / 12.0;                      // ファミコンのシステムクロック

}                                                                               // namespace

/**
 * @brief MIDIクラスから必要な情報を読み込み、MML用中間情報を作成する
 * 音符・休符の採譜、チャンネル割り当て、音色情報の登録を行う
 */
void Mml::Load(                                                                 // MIDIクラスから必要な情報を読み込み、MML用中間情報を作成する
    const Midi& midi,                                                           ///< (i)Midiクラス
    string ch_str)                                                              ///< (i)チャンネル文字列(※D,Eチャンネルは9chからの変換固定のため含てはならない)
{
#ifdef _MSC_VER                                                                 // MSVC
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
  const char* username = getenv(
#ifdef _WIN32                                                                   // Windows
      "USERNAME"
#else                                                                           // Linux・MacOS
      "USER"
#endif
  );                                                                            ///< 環境変数から取得したユーザー名
#ifdef _MSC_VER                                                                 // MSVC
#pragma warning(pop)
#endif
  programmer_ = username == nullptr ? "" : username;                            // 打ち込み者名を設定
  ch_str_ = ch_str;                                                             // チャンネル文字列を保存
  time_base_ = midi.time_base();                                                // Midiクラスからタイムベースを取得する
  const uint16_t whole_note = time_base_ << 2;                                  ///< 全音符の長さ
  string per_ch_str("DE");                                                      ///< パーカッションチャンネル文字列
  const string note_char[] = {
    "c", "c+", "d", "d+", "e", "f", "f+", "g", "g+", "a", "a+", "b"};           ///< 音符名の対応表
  // トラックリストループ
  const Midi::TrackList& tracks = midi.tracks();                                ///< MIDIトラック一覧
  TextEncodingDetector text_encoding_detector;                                  ///< MIDIテキストの文字コード判別器
  for (const auto& track : tracks) {                                            // MIDIトラックループ
    for (const auto& ope : track) {                                             // MIDIイベントループ
      if ((ope.status == 0xFF) && (ope.status1 >= 0x01) && (ope.status1 <= 0x1F)) { // テキスト系メタイベントなら
        text_encoding_detector.Add(ope.ex_data);                                // MIDI由来の生バイト列を判別対象へ追加
      }
    }
  }
  midi_text_encoding_ = text_encoding_detector.Detect();                        // MIDIテキストの文字コードを記録
  tone_.SetMidiTextEncoding(midi_text_encoding_);                               // 音色由来文字列の出力文字コードを設定
  bool first_track = true;                                                      ///< 先頭トラック判定
  for (const auto& track : tracks) {                                            // トラックリストループ
    ChInfo channel;                                                             ///< チャンネル情報作成
    map<uint8_t, uint32_t> use_len;                                             ///< 音符長ごとの使用回数キー0は全音符、1以降は分音符
    auto note_on = track.end();                                                 ///< ノートオンの位置
    auto note_off = track.end();                                                ///< ノートオフの位置
    const auto get_note_length = [&track](uint32_t current_time, auto note_it) noexcept {
      return current_time - (note_it == track.end() ? uint32_t{0} : note_it->time);
    };                                                                          ///< 音符または休符の長さを計算するラムダ関数
    uint32_t tempo(120);                                                        ///< テンポ
    uint8_t mml_ch(0);                                                          ///< MMLでのチャンネル
    uint8_t prg_no(0);                                                          ///< プログラム(音色)番号
    uint8_t prev_prg_no(255);                                                   ///< １つ前の音符のプログラム(音色)番号
    // ↓ピッチエンヴェロープ用
    uint8_t rpn_lsb(0);                                                         ///< RPN LSB(コントロールチェンジ100)
    uint8_t rpn_msb(0);                                                         ///< RPN MSB(コントロールチェンジ101)
    uint8_t pitch_bend_sensitivity(2);                                          ///< ピッチベンドセンシティヴィティ
    short pitch_bend(0);                                                        ///< ピッチベンド(範囲-8192～0～8191)
    uint32_t prev_pitch_frequency(0);                                           ///< 直前のピッチ周波数（レジスタ値）
    int inside_note(0);                                                         ///< 音符中 0:音符外 1:音符内
    vector<char> pitch_envelope;                                                ///< ピッチエンべロープ
    string prev_e_pcommand("EPOF");                                             ///< 直前のEPコマンド
    // ↑ピッチエンヴェロープ用
    // 制御リストループ
    for (auto ope_it  = track.begin();
              ope_it != track.end(); ++ope_it) {                                // 制御リストループ
      const Midi::Operate& ope = *ope_it;                                       ///< 現在のMIDIイベント
      uint8_t sta = 0xF0 & ope.status;                                          ///< イベント抽出
      switch (sta) {
      case 0xF0:                                                                // SysEx
        if (0xFF == ope.status) {                                               // メタイベント
          switch (ope.status1) {                                                // テキスト種類別処理
          case 0x03:                                                            // タイトル
            if (first_track && title_.empty()) {                                // 先頭トラックでタイトルが空なら
              title_ = ope.ex_data;                                             // タイトルを取得
              replace(title_.begin(), title_.end(), '\0', ' ');                 // ヌル文字をスペースにする
            }
            break;

          case 0x02:                                                            // 著作権表示
            {
              string::size_type idx1 = ope.ex_data.find("(C)");                 ///< "(C)"を検索結果
              string::size_type idx2 = ope.ex_data.find("Copyright");           ///< "Copyright"を検索結果
              if ((idx1 == string::npos) && (idx2 == string::npos)) {           // 含まれていなければ
                if (composer_.empty()) {                                        // 作曲者名が空なら
                  composer_ = ope.ex_data;                                      // 作曲者名を取得
                  replace(composer_.begin(), composer_.end(), '\0', ' ');       // ヌル文字をスペースにする
                }
              } else {
                if (maker_.empty()) {                                           // 原著作者名が空なら
                  maker_ = ope.ex_data;                                         // 原著作者名を取得
                  replace(maker_.begin(), maker_.end(), '\0', ' ');             // ヌル文字をスペースにする
                }
              }
            }
            break;

          case 0x2F:                                                            // トラックエンド
            {
              /// 音符または休符の長さを求める
              uint32_t note_len = get_note_length(ope.time, note_off);          // 音符または休符の長さ
              if (note_len) {                                                   // 音符の長さが０以外なら
                UseLenCnt(use_len, note_len, whole_note);                       // 音符長を数える
                /// MMLへ出力する音符・休符・制御情報
                NoteInfo note((mml_ch != 'E') ? "r" : "w", note_len);           ///< この休符情報を作成
                channel.note_vector.push_back(note);                            // 音符ベクタに音符情報を登録
              }
            }
            break;

          case 0x51:                                                            // テンポ設定
            {
              /// 音符または休符の長さを求める
              uint32_t note_len = get_note_length(ope.time, note_off);          // 音符または休符の長さ
              if (note_len) {                                                   // 音符の長さが０以外なら
                UseLenCnt(use_len, note_len, whole_note);                       // 音符長を数える
                /// MMLへ出力する音符・休符・制御情報
                NoteInfo note((mml_ch != 'E') ? "r" : "w", note_len);           ///< この休符情報を作成
                channel.note_vector.push_back(note);                            // 音符ベクタに音符情報を登録
                note_off = ope_it;                                              // ノートオフの位置を記録
              }
              tempo = 60000000 / ((static_cast<uint8_t>(ope.ex_data[0]) << 16) |
                                  (static_cast<uint8_t>(ope.ex_data[1]) << 8) |
                                   static_cast<uint8_t>(ope.ex_data[2]));       // テンポを取り出す
              ostringstream cmd;                                                ///< コマンド文字列
              cmd << "t" << tempo;                                              // コマンド文字列を編集
              NoteInfo note(cmd.str(), tempo);                                  ///< この音符情報を作成
              channel.note_vector.push_back(note);                              // 音符ベクタに音符情報を登録
              if (!first_tempo_) {                                              // 最初のテンポが未設定なら
                first_tempo_ = tempo;                                           // 最初のテンポとして設定
              }
            }
            break;

          case 0x58:                                                            // 拍子
            {
              /// 音符または休符の長さを求める
              uint32_t note_len = get_note_length(ope.time, note_off);          // 音符または休符の長さ
              if (note_len) {                                                   // 音符の長さが０以外なら
                UseLenCnt(use_len, note_len, whole_note);                       // 音符長を数える
                /// MMLへ出力する音符・休符・制御情報
                NoteInfo note((mml_ch != 'E') ? "r" : "w", note_len);           ///< この休符情報を作成
                channel.note_vector.push_back(note);                            // 音符ベクタに音符情報を登録
                note_off = ope_it;                                              // ノートオフの位置を記録
              }
              uint8_t num = ope.ex_data[0];                                     ///< 分子を取り出す
              uint8_t denom = 1 << ope.ex_data[1];                              ///< 分母を取り出す
              ostringstream cmd;                                                ///< コマンド文字列
              cmd << "!" << static_cast<uint16_t>(num) << "/" << static_cast<uint16_t>(denom); // コマンド文字列を編集(拍子の内部表現を'!'とする)
              NoteInfo note(cmd.str(), 0, num, denom);                          ///< この音符情報を作成
              channel.note_vector.push_back(note);                              // 音符ベクタに音符情報を登録
            }
            break;

          case 0x06:                                                            // マーカー
            {
              /// 音符または休符の長さを求める
              uint32_t note_len = get_note_length(ope.time, note_off);          // 音符または休符の長さ
              if (note_len) {                                                   // 音符の長さが０以外なら
                UseLenCnt(use_len, note_len, whole_note);                       // 音符長を数える
                /// MMLへ出力する音符・休符・制御情報
                NoteInfo note((mml_ch != 'E') ? "r" : "w", note_len);           ///< この休符情報を作成
                channel.note_vector.push_back(note);                            // 音符ベクタに音符情報を登録
                note_off = ope_it;                                              // ノートオフの位置を記録
              }
              NoteInfo note("L");                                               ///< この音符情報を作成
              channel.note_vector.push_back(note);                              // 音符ベクタに音符情報を登録
            }
            break;
          }
          if ((0x01 <= ope.status1) && (0x1F >= ope.status1)) {                 // テキストなら
            ostringstream cmd;                                                  ///< コマンド文字列
            cmd << "// " << ope.ex_data;                                        // コマンド文字列を編集
            /// MMLへ出力する音符・休符・制御情報
            NoteInfo note(cmd.str());                                           ///< キーを登録し、音色コマンドを取得した音符情報
            channel.note_vector.push_back(note);                                // 音符ベクタに音符情報を登録
          }
        }
        continue;                                                               // 次の制御へ
      }
      uint8_t ch = 0xF & ope.status;                                            ///< このトラックのMIDIチャンネル抽出
      if (0x9 != ch) {                                                          // MIDIチャンネルが9ch以外で
        if (!mml_ch) {                                                          // チャンネルが決まってなく
          if (!ch_str.empty()) {                                                // チャンネル文字列が残っていたら
            mml_ch = ch_str[0];                                                 // チャンネル文字列の先頭１文字を取り出しMMLのチャンネルとする
            ch_str = ch_str.erase(0, 1);                                        // チャンネル文字列の先頭１文字を切り詰める
          } else {                                                              // チャンネル文字列が残ってなければ
            break;                                                              // 次のトラックへ
          }
        }
        switch (sta) {                                                          // イベント別処理
        case 0xC0:                                                              // プログラムチェンジ
          prg_no = ope.status1;                                                 // プログラム(音色)番号を記録
          break;

        case 0xB0:                                                              // コントロールチェンジ
          switch (ope.status1) {                                                // コントローラナンバーで分岐
          case 100:                                                             // RPN LSB
            rpn_lsb = ope.status2;                                              // RPN LSBを設定
            break;

          case 101:                                                             // RPN MSB
            rpn_msb = ope.status2;                                              // RPN MSBを設定
            break;

          case 6:                                                               // データエントリーMSB
            if ((0 == rpn_lsb) &&(0 == rpn_msb)) {                              // RPN LSB MSBが共に０なら
              pitch_bend_sensitivity = ope.status2;                             // ピッチベンドセンシティヴィティを設定
            }
            break;
          }
          break;

        case 0xE0:                                                              // ピッチベンド
          pitch_bend = ((static_cast<int>(ope.status2) << 7) | ope.status1) - 8192; // ピッチベンドを取り出す(範囲-8192～0～8191)
          if (inside_note) {                                                    // 音符内なら
            /// ピッチエンベロープ内のフレーム位置
            uint16_t frame = static_cast<uint16_t>(                             // 音符先頭からのフレーム　＝　整数化（
                 3600. / tempo * (ope.time - note_on->time) / time_base_);      // 3600フレーム（１分間のフレーム数）／テンポ＊音符先頭からここまでの時間／分解能　）
            pitch_envelope.resize(frame + 1);                                   // 音符先頭からのフレーム数だけピッチエンべロープの要素を確保
            /// ピッチを反映した周波数レジスタ値
            uint32_t pitch_frequency = PitchFrequencyCalc(                      // ピッチ周波数（レジスタ値）計算
                note_on->status1, mml_ch, pitch_bend_sensitivity, pitch_bend);  // (i)音符のキー番号 (i)チャンネル (i)ピッチベンドセンシティヴィティ (i)ピッチベンド
            SetPitchEnvelope(pitch_envelope, frame, prev_pitch_frequency - pitch_frequency); // フレーム位置に直前との差分を設定
            prev_pitch_frequency = pitch_frequency;                             // 直前のピッチ周波数を更新
          }
          break;

        case 0x90:                                                              // ノートオン
          if (0x00 != ope.status2) {                                            // ベロシティ０でなければ
            inside_note = 1;                                                    // 音符内を設定
            note_on = ope_it;                                                   // ノートオンの位置を記録
            /// 音符または休符の長さを求める
            uint32_t note_len = get_note_length(ope.time, note_off);            // 音符または休符の長さ
            // ピッチエンヴェロープの処理
            pitch_envelope.clear();                                             // ピッチエンヴェロープをクリア
            prev_pitch_frequency = NoteFrequencyCalc(note_on->status1, mml_ch); // 音符の周波数（レジスタ値）を計算し直前の周波数とする
            if (pitch_bend) {                                                   // ピッチベンドが０じゃなければ
              /// ピッチを反映した周波数レジスタ値
              uint32_t pitch_frequency =                                        ///< ピッチ周波数
                  PitchFrequencyCalc(note_on->status1, mml_ch,                  // ピッチ周波数（レジスタ値）計算 (i)音符のキー番号 (i)チャンネル
                      pitch_bend_sensitivity, pitch_bend);                      // (i)ピッチベンドセンシティヴィティ (i)ピッチベンド
              SetPitchEnvelope(pitch_envelope, 0, prev_pitch_frequency - pitch_frequency); // ピッチエンベロープに周波数差を設定
              prev_pitch_frequency = pitch_frequency;                           // 直前のピッチ周波数を更新
            }
            // 休符の処理
            if (!note_len) {                                                    // 音符の長さが０なら
              continue;                                                         // 次の制御へ
            }
            UseLenCnt(use_len, note_len, whole_note);                           // 音符長を数える
            NoteInfo note("r", note_len);                                       ///< この休符情報を作成
            channel.note_vector.push_back(note);                                // 音符ベクタに音符情報を登録
            break;
          }
          [[fallthrough]];                                                      // ベロシティ０ならノートオフなので続行

        case 0x80:                                                              // ノートオフ
          if (ope.status1 != note_on->status1) {                                // ノートオンとキーが一致しているなら
            cerr << "音符が重なっています、正しく変換できません。\n";
            break;
          }
          {
            inside_note = 0;                                                    // 音符外を設定
            note_off = ope_it;                                                  // ノートオフの位置を記録
            /// 音符または休符の長さを求める
            uint32_t note_len = get_note_length(ope.time, note_on);             // 音符または休符の長さ
            if (!note_len) {                                                    // 音符の長さが０なら
              continue;                                                         // 次の制御へ
            }
            UseLenCnt(use_len, note_len, whole_note);                           // 音符長を数える
            // 音色
            tone_.Set(mml_ch, prg_no, note_on->status2);                        // プログラム番号を登録
            if (prev_prg_no != prg_no) {                                        // １つ前の音符のプログラム（音色）番号と違うなら
              prev_prg_no = prg_no;                                             // １つ前の音符のプログラム（音色）番号を更新
              /// MMLへ出力する音符・休符・制御情報
              NoteInfo note("$", 0, prg_no);                                    ///< プログラム番号を保持する音符情報'$'で音符情報を作成
              channel.note_vector.push_back(note);                              // 音符ベクタに音符情報を登録
            }
            // ピッチ
            /// 登録したピッチエンベロープのコマンド
            string command = pitch_.Regist(pitch_envelope, mml_ch);             // ピッチエンヴェロープを登録し、EPコマンド文字列を受け取る
            if ((!command.empty()) && (prev_e_pcommand != command)) {           // コマンドが空じゃなく、かつ 直前のEPコマンドと違うなら
              prev_e_pcommand = command;                                        // 直前のEPコマンドを更新
              NoteInfo note(command);                                           ///< EPコマンドの音符情報を作る
              channel.note_vector.push_back(note);                              // 音符ベクタに音符情報を登録
            }
            // 音符
            /// MMLへ出力する音符・休符・制御情報
            NoteInfo note(
                note_char[ope.status1 % 12],                                    // 音符情報を作成
                note_len, ope.status1 / 12 - 1,
                tone_.VolumeConvert(mml_ch, note_on->status2));                 // MIDI音量をMML音量に変換
            if (('C' == mml_ch) || ('P' <= mml_ch) && ('W' >= mml_ch)) {        // C,P～Wチャンネルなら
              ++note.oct;                                                       // １オクターブ上げる
            }
            if (static_cast<signed char>(note.oct) < 0) {                       // オクターブがマイナスなら
              note.oct = 0;                                                     // 0オクターブに調整
            }
            channel.note_vector.push_back(note);                                // 音符ベクタに音符情報を登録
            // 最初の値収集
            if (255 == channel.first_octave) {                                  // 最初のオクターブが未設定なら
              channel.first_octave = note.oct;                                  // 最初のオクターブを設定
              channel.first_volume = note.vol;                                  // 最初の音量を設定
              channel.first_prg_no = prg_no;                                    // 最初のプログラム番号を設定
            }
          }
          break;
        }
      } else {                                                                  // MIDIチャンネルが9chなら
        if (!mml_ch) {                                                          // チャンネルが決まってなく
          if (!per_ch_str.empty()) {                                            // パーカッションチャンネル文字列が残っていたら
            mml_ch = per_ch_str[0];                                             // パーカッションチャンネル文字列の先頭１文字を取り出しMMLのチャンネルとする
            per_ch_str = per_ch_str.erase(0, 1);                                // パーカッションチャンネル文字列の先頭１文字を切り詰める
          } else {                                                              // パーカッションチャンネル文字列が残ってなければ
            break;                                                              // 次のトラックへ
          }
        }
        switch (sta) {                                                          // イベント別処理
        case 0x90:                                                              // ノートオン
          if (0x00 != ope.status2) {                                            // ベロシティ０でなければ
            note_on = ope_it;                                                   // ノートオンの位置を記録
            /// 音符または休符の長さを求める
            uint32_t note_len = get_note_length(ope.time, note_off);            // 音符または休符の長さ
            if (!note_len) {                                                    // 音符の長さが０なら
              continue;                                                         // 次の制御へ
            }
            UseLenCnt(use_len, note_len, whole_note);                           // 音符長を数える
            /// MMLへ出力する音符・休符・制御情報
            NoteInfo note((mml_ch != 'E') ? "r" : "w", note_len);               ///< この休符情報を作成
            channel.note_vector.push_back(note);                                // 音符ベクタに音符情報を登録
            break;
          }
          [[fallthrough]];                                                      // ベロシティ０ならノートオフなので続行

        case 0x80:                                                              // ノートオフ
          if (ope.status1 != note_on->status1) {                                // ノートオンとキーが一致しているなら
            cerr << "音符が重なっています、正しく変換できません。\n";
            break;
          }
          {
            note_off = ope_it;                                                  // ノートオフの位置を記録
            /// 音符または休符の長さを求める
            uint32_t note_len = get_note_length(ope.time, note_on);             // 音符または休符の長さ
            if (!note_len) {                                                    // 音符の長さが０なら
              continue;                                                         // 次の制御へ
            }
            UseLenCnt(use_len, note_len, whole_note);                           // 音符長を数える
            tone_.Set(mml_ch, ope.status1, note_on->status2);                   // キーを登録する
            if (prev_prg_no != ope.status1) {                                   // １つ前の音符のキー（音色）番号と違うなら
              prev_prg_no = ope.status1;                                        // １つ前の音符のキー（音色）番号を更新
              if ('D' == mml_ch) {                                              // ノイズチャンネルなら
                /// MMLへ出力する音符・休符・制御情報
                NoteInfo note("$", 0, ope.status1);                             ///< プログラム番号を保持する音符情報'$'で音符情報を作成
                channel.note_vector.push_back(note);                            // 音符ベクタに音符情報を登録
              }
            }
            /// MMLへ出力する音符・休符・制御情報
            NoteInfo note(
                tone_.GetNote(mml_ch, ope.status1),                             // キーを登録し音質コマンド取得し、この音符情報を作成
                note_len, ope.status1 / 12 - 1,
                tone_.VolumeConvert(mml_ch, note_on->status2));                 // MIDI音量をMML音量に変換
            channel.note_vector.push_back(note);                                // 音符ベクタに音符情報を登録
            if (255 == channel.first_octave) {                                  // 最初のオクターブが未設定なら
              channel.first_octave = note.oct;                                  // 最初のオクターブを設定
              channel.first_volume = note.vol;                                  // 最初の音量を設定
              channel.first_prg_no = ope.status1;                               // 最初のプログラム番号を設定
            }
          }
          break;
        }
      }
    }
    if (mml_ch) {                                                               // チャンネルが決まっていたなら
      // 一番多く使われているレングスをデフォルトレングスとする
      uint32_t max_cnt(0);                                                      ///< 最大使用数をカウント
      for (const auto& [Length, Count] : use_len) {                             // 使用レングスループ
        if (max_cnt < Count) {                                                  // より多く使われていたら
          max_cnt = Count;                                                      // 最大使用数を更新
          channel.default_len = 1 << Length;                                    // そのレングス(分音符)を更新
        }
      }
      ch_map_[mml_ch] = channel;                                                // チャンネルマップにチャンネル情報を追加
    }
    first_track = false;                                                        // 先頭トラック判定を解除
    if (ch_str.empty() && per_ch_str.empty()) {                                 // チャンネル文字列が空なら
      break;                                                                    // これ以降のトラックを処理しない
    }
  }
}

/** @brief MIDIキーから音符の周波数レジスタ値を計算する */
uint32_t Mml::NoteFrequencyCalc(                                                // MIDIキーから音符の周波数レジスタ値を計算する
    const uint8_t& key_no,                                                      ///< (i)MIDIキー番号
    const uint8_t& ch,                                                          ///< (i)チャンネル
    uint8_t base_key_no) const noexcept                                         ///< (i)基準MIDIキー番号(VRC7用)
{
  double freq = 440 * pow(2, (key_no - 69) / 12.);                              ///< 音声周波素
  int note_frequency;                                                           ///< 音符の周波数（レジスタ値）
  switch (ch) {                                                                 // チャンネル分岐
  case 'C':                                                                     // 2A03 三角波
    freq /= 2;                                                                  // 音声周波数を半分に（1オクターブ低い）
    // 周波数（レジスタ値）＝ 整数化（システムクロック／音符の周波数）を下位5ビットカット
    note_frequency = static_cast<int>(kNesSystemClock / freq) >> 5;
    break;

  case 'O':                                                                     // VRC6 鋸波
    // 周波数（レジスタ値）＝ 整数化（システムクロック／音符の周波数）／14
    note_frequency = static_cast<int>(kNesSystemClock / freq) / 14;
    break;

  case 'X':                                                                     // FME7
  case 'Y':                                                                     // FME7
  case 'Z':                                                                     // FME7
    // 周波数（レジスタ値）＝ 整数化（システムクロック／２／音符の周波数）を下位4ビットカット
    note_frequency = static_cast<int>(kNesSystemClock / 2 / freq) >> 4;
    break;

  case 'F':                                                                     // FDS
    // 周波数（レジスタ値）＝ 整数化（音符の周波数／（システムクロック／65536／64））
    note_frequency = static_cast<int>(freq / (kNesSystemClock / 65536 / 64));
    break;

  case 'P':                                                                     // N106
  case 'Q':                                                                     // N106
  case 'R':                                                                     // N106
  case 'S':                                                                     // N106
  case 'T':                                                                     // N106
  case 'U':                                                                     // N106
  case 'V':                                                                     // N106
  case 'W':                                                                     // N106
    freq /= 2;                                                                  // 音声周波数を半分に（16サンプルでは1オクターブ低い）
    // 周波数（レジスタ値）＝ 整数化（音声周波数(Hz) × $40000 × 45 × 有効チャンネル数 × サンプル数 ÷ マスタークロック）
    note_frequency = static_cast<int>(freq * (0x40000 * 45 * 8 * 16 / kNesMasterClock)) >> 7; // SA6用の設定
    break;

  case 'G':                                                                     // VRC7
  case 'H':                                                                     // VRC7
  case 'I':                                                                     // VRC7
  case 'J':                                                                     // VRC7
  case 'K':                                                                     // VRC7
  case 'L':                                                                     // VRC7
  // 周波数（レジスタ値）＝ 整数化（音声周波数(Hz) × 2＾(18－オクターブ) ÷ (システムクロック÷72)）
  //    NoteFrequency = static_cast<int>( Freq * pow ( 2.,
  //        (18 - 1 - static_cast<int>( BaseKeyNo / 12 ) ) ) / ((kNesSystemClock / 72) );
  //    break;
  // VRC7は暫定的に矩形波の計算式を使用する
  case 'A':                                                                     // 2A03 矩形波
  case 'B':                                                                     // 2A03 矩形波
  case 'a':                                                                     // MMC5 矩形波
  case 'b':                                                                     // MMC5 矩形波
  case 'M':                                                                     // VRC6 矩形波
  case 'N':                                                                     // VRC6 矩形波
  default:                                                                      // その他の値
    // 周波数（レジスタ値）＝ 整数化（システムクロック／音符の周波数）を下位4ビットカット
    note_frequency = static_cast<int>(kNesSystemClock / freq) >> 4;
    break;
  }
  // ビット範囲チェック
  if (0 > note_frequency) {
    note_frequency = 0;
/*  } else {
    int  max;                                                                   // 最大値
    switch ( Ch ) {                                                             // チャンネル
    case 'P':                                                                   // N106
    case 'Q':                                                                   // N106
    case 'R':                                                                   // N106
    case 'S':                                                                   // N106
    case 'T':                                                                   // N106
    case 'U':                                                                   // N106
    case 'V':                                                                   // N106
    case 'W':                                                                   // N106
      max = 262143;                                                             // 18bit
      break;

    case 'G':                                                                   // VRC7
    case 'H':                                                                   // VRC7
    case 'I':                                                                   // VRC7
    case 'J':                                                                   // VRC7
    case 'K':                                                                   // VRC7
    case 'L':                                                                   // VRC7
      max = 511;                                                                // 9bit
      break;

    case 'F':                                                                   // FDS
    case 'M':                                                                   // VRC6 矩形波
    case 'N':                                                                   // VRC6 矩形波
    case 'O':                                                                   // VRC6 鋸波
    case 'X':                                                                   // FME7
    case 'Y':                                                                   // FME7
    case 'Z':                                                                   // FME7
      max = 4095;                                                               // 12bit
      break;

    case 'A':                                                                   // 2A03 矩形波
    case 'B':                                                                   // 2A03 矩形波
    case 'C':                                                                   // 2A03 三角波
    case 'a':                                                                   // MMC5 矩形波
    case 'b':                                                                   // MMC5 矩形波
    default :                                                                   // その他
      max = 2047;                                                               // 11bit
      break;
    }
    if ( max < NoteFrequency ) {                                                // 最大値を超えたら
      NoteFrequency = max;                                                      // 範囲内に収める
    }
*/
  }
  return static_cast<int>(note_frequency);
}

/** @brief ピッチベンドを反映した周波数レジスタ値を計算する */
uint32_t Mml::PitchFrequencyCalc(                                               // ピッチベンドを反映した周波数レジスタ値を計算する
    const uint8_t& key_no,                                                      ///< (i)MIDIキー番号
    const uint8_t& ch,                                                          ///< (i)チャンネル
    const uint8_t& pitch_bend_sensitivity,                                      ///< (i)ピッチベンドセンシティヴィティ
    const short& pitch_bend) const noexcept                                     ///< (i)ピッチベンド(範囲-8192～0～8191)
{
  uint8_t sensitive_key_no(key_no);                                             ///< センシティヴィティでのMIDIキー番号
  if (0 > pitch_bend) {                                                         // 音符のキー番号＋ピッチベンドが負の数
    sensitive_key_no -= pitch_bend_sensitivity;                                 // センシティヴィティでのMIDIキー番号－＝ピッチベンドセンシティヴィティ
  } else {
    sensitive_key_no += pitch_bend_sensitivity;                                 // センシティヴィティでのMIDIキー番号＋＝ピッチベンドセンシティヴィティ
  }
  uint32_t note_frequency = NoteFrequencyCalc(key_no, ch, sensitive_key_no);    // 音符の周波数（レジスタ値）計算
  uint32_t sensitive_frequency = NoteFrequencyCalc(sensitive_key_no, ch, sensitive_key_no); // ピッチベンドセンシティヴィティの周波数（レジスタ値）計算
  int pitch_bend_frequency;                                                     ///< ピッチベンドの周波数差分
  if (0 > pitch_bend) {                                                         // ピッチベンドが負の数なら
    pitch_bend_frequency = static_cast<int>(                                    // ピッチベンドの周波数差分＝整数化（
        static_cast<int>(note_frequency - sensitive_frequency) * (pitch_bend / 8192.)); // （音符の周波数　－　ピッチベンドセンシティヴィティの周波数）×（ピッチベンド　／　8192(ピッチベンド最大値)））
  } else {                                                                      // ピッチベンドが正の数なら
    pitch_bend_frequency = static_cast<int>(                                    // ピッチベンドの周波数差分＝整数化（
        -static_cast<int>(note_frequency - sensitive_frequency) * (pitch_bend / 8191.)); // －（音符の周波数　－　ピッチベンドセンシティヴィティの周波数）×（ピッチベンド　／　8191(ピッチベンド最大値)））
  }
  switch (ch) {
  case 'F':                                                                     // FDS
  //  case 'G':
  //  case 'H':
  //  case 'I':
  //  case 'J':
  //  case 'K':
  //  case 'L':
  case 'P':                                                                     // N106
  case 'Q':                                                                     // N106
  case 'R':                                                                     // N106
  case 'S':                                                                     // N106
  case 'T':                                                                     // N106
  case 'U':                                                                     // N106
  case 'V':                                                                     // N106
  case 'W':                                                                     // N106
    // 音声周波数とレジスタ値が比例する音源
    pitch_bend_frequency = -pitch_bend_frequency;                               // プラマイ反転
  }
  return note_frequency + pitch_bend_frequency;                                 // ピッチ周波数（レジスタ値）＝音符の周波数＋ピッチベンドの周波数差分
}

/** @brief ピッチエンベロープへ周波数差分を設定する */
void Mml::SetPitchEnvelope(                                                     // ピッチエンベロープへ周波数差分を設定する
    vector<char>& pitch_envelope,                                               ///< (io)ピッチエンべロープ
    uint32_t frame,                                                             ///< (i)フレーム
    int pitch_diff) const                                                       ///< (i)ピッチ周波数（レジスタ値）差分
{
  for ( ; (frame < 511) && (pitch_diff != 0); ++frame) {                        // フレームが511未満、かつ、ピッチ周波数差分が０以外である間ループ
    if (frame >= pitch_envelope.size()) {                                       // フレームの要素が存在しないなら
      pitch_envelope.resize(frame + 1);                                         // ピッチエンべロープの要素を追加
    }
    pitch_diff += pitch_envelope[frame];                                        // ピッチ周波数差分にピッチエンべロープ[フレーム]の値を足す
    if (pitch_diff < 0) {                                                       // ピッチ周波数差分がマイナス値なら
      if (pitch_diff < -127) {                                                  // ピッチ周波数差分が－１２８より小さければ
        pitch_envelope[frame] = -127;                                           // ピッチエンヴェロープ[フレーム]に－１２８を設定し
        pitch_diff += 127;                                                      // その分ピッチ周波数差分に１２８を足す
      } else {                                                                  // ピッチ周波数差分が－１２８以上なら
        pitch_envelope[frame] = pitch_diff;                                     // ピッチエンヴェロープ[フレーム]にピッチ周波数差分を設定し
        pitch_diff = 0;                                                         // その分ピッチ周波数差分を０にする
      }
    } else {                                                                    // ピッチ周波数差分がプラス値なら
      if (pitch_diff > 126) {                                                   // ピッチ周波数差分が１２７より大きければ
        pitch_envelope[frame] = 126;                                            // ピッチエンヴェロープ[フレーム]に１２７を設定し
        pitch_diff -= 126;                                                      // その分ピッチ周波数差分に１２７を引く
      } else {                                                                  // ピッチ周波数差分が１２７以下なら
        pitch_envelope[frame] = pitch_diff;                                     // ピッチエンヴェロープ[フレーム]にピッチ周波数差分を設定し
        pitch_diff = 0;                                                         // その分ピッチ周波数差分を０にする
      }
    }
  }
}

/** @brief 音符長の使用回数を集計する */
void Mml::UseLenCnt(                                                            // 音符長の使用回数を集計する
    map<uint8_t, uint32_t>& use_len,                                            ///< (io)音符長の数
    const uint32_t& note_len,                                                   ///< (i)音符の長さ
    const uint16_t& whole_note) const                                           ///< (i)全音符の長さ
{
  uint32_t par_note = whole_note / note_len;                                    ///< 何分音符か求める
  if (1 > par_note) {                                                           // 全音符以上なら
    use_len[0] += note_len / whole_note;                                        // 全音符の数をカウント
  } else {                                                                      // それ以外
    int sh;                                                                     ///< シフト数
    for (sh = 0; par_note >> sh; ++sh);                                         // 最上位ビットの位置を探索
    ++use_len[--sh];                                                            // 使用レングスカウント
  }
}

/** @brief 音色コマンドと定義を登録する */
void Mml::Tone::Set(                                                            // 音色コマンドと定義を登録する
    const uint8_t& ch,                                                          ///< (i)チャンネル
    const uint8_t& prg_no,                                                      ///< (i)プログラム番号
    uint8_t volume)                                                             ///< (i)音量
{
  // 音色(@@コマンド、その他)
  uint16_t shift_no(prg_no);                                                    ///< 重複しないプログラム番号
  switch (ch) {
  case 'A':                                                                     // 2A03 矩形波
  case 'B':                                                                     // 2A03 矩形波
  case 'a':                                                                     // MMC5 矩形波
  case 'b':                                                                     // MMC5 矩形波
    // 矩形波Duty0～3用
    if ((color_cmd_abab_mn_.find(prg_no) == color_cmd_abab_mn_.end()) &&        // 音色が登録されておらず、
        (127 >= serial_no_abab_mn_)) {                                          // 最大登録件数以内なら
      ostringstream cmd;                                                        ///< コマンド作成
      cmd << "@@" << serial_no_abab_mn_;
      color_cmd_abab_mn_[prg_no] = cmd.str();                                   // コマンド追加
      ostringstream def;                                                        ///< 定義作成
      def << "@" << serial_no_abab_mn_ << "  \t={"
          << (tone_def_vct_square_[prg_no].empty() ? "1" : tone_def_vct_square_[prg_no])
          << "}\t\t\t\t\t\t\t\t\t\t" << CommentEdit(ch, prg_no);
      color_def_abab_mn_[serial_no_abab_mn_] = def.str();                       // 定義追加
      ++serial_no_abab_mn_;                                                     // 管理番号カウントアップ
    }
    break;

  case 'M':                                                                     // VRC6 矩形波
  case 'N':                                                                     // VRC6 矩形波
    // 矩形波Duty0～7用
    shift_no = 128 + prg_no;                                                    // 重複しないプログラム番号
    if ((color_cmd_abab_mn_.find(shift_no) == color_cmd_abab_mn_.end()) &&      // 音色が登録されておらず、
        (127 >= serial_no_abab_mn_)) {                                          // 最大登録件数以内なら
      ostringstream cmd;                                                        ///< コマンド作成
      cmd << "@@" << serial_no_abab_mn_;
      color_cmd_abab_mn_[shift_no] = cmd.str();                                 // コマンド追加
      ostringstream def;                                                        ///< 定義作成
      def << "@" << serial_no_abab_mn_ << "  \t={"
          << (tone_def_vct_vrc6_[prg_no].empty() ? "3" : tone_def_vct_vrc6_[prg_no])
          << "}\t\t\t\t\t\t\t\t\t\t" << CommentEdit(ch, prg_no);
      color_def_abab_mn_[serial_no_abab_mn_] = def.str();                       // 定義追加
      ++serial_no_abab_mn_;                                                     // 管理番号カウントアップ
    }
    break;

  case 'D':                                                                     // 2A03 ノイズ
    shift_no = ParMapChg(prg_no);                                               // パーカッション未登録部分をマップ
    if (note_cmd_d_.find(shift_no) == note_cmd_d_.end()) {                      // 音符が登録されていなければ
      ostringstream note;                                                       ///< 音符コマンド作成
      note << tone_def_vct_noise_[prg_no];                                      // 音色定義ノイズを取得
      note_cmd_d_[shift_no] = note.str();                                       // 音符コマンド追加
      ostringstream cmd;                                                        ///< コマンド作成
      cmd << "D255";
      color_cmd_d_[shift_no] = cmd.str();                                       // コマンド追加
      ++serial_no_d_;                                                           // 管理番号カウントアップ
    }
    break;

  case 'E':                                                                     // 2A03 DPCM
    shift_no = ParMapChg(prg_no);                                               // パーカッション未登録部分をマップ
    if ((note_cmd_e_.find(shift_no) == note_cmd_e_.end()) &&                    // 音符が登録されておらず、
        (63 >= serial_no_e_)) {                                                 // 最大登録件数以内なら
      ostringstream note;                                                       ///< 音符コマンド作成
      note << "n" << serial_no_e_;
      note_cmd_e_[shift_no] = note.str();                                       // 音符コマンド追加
      ostringstream def;                                                        ///< 定義作成
      def << "@DPCM" << serial_no_e_ << "\t={\"dmc\\" << static_cast<uint16_t>(shift_no)
          << ".dmc\"\t,15}\t\t\t\t\t\t// Ch.E\t\t\tPrgNo." << static_cast<uint16_t>(shift_no)
          << "\t" << kDrumName[static_cast<uint16_t>(shift_no)] << "\n";
      color_def_e_[serial_no_e_] = def.str();                                   // 定義追加
      ++serial_no_e_;                                                           // 管理番号カウントアップ
    }
    break;

  case 'F':                                                                     // FDS
    if (color_cmd_f_.find(prg_no) == color_cmd_f_.end()) {                      // 音色が登録されていなければ
      ostringstream cmd;                                                        ///< コマンド作成
      cmd << "@@" << serial_no_f_;
      color_cmd_f_[prg_no] = cmd.str();                                         // コマンド追加
      ostringstream def;                                                        ///< 定義作成
      def << "@FM" << serial_no_f_ << "\t={\t\t\t\t\t\t\t"
          << CommentEdit(ch, prg_no)
          << "      63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,\n"
          << "      63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,\n"
          << "      00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,\n"
          << "      00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00}\n";
      color_def_f_[serial_no_f_] = def.str();                                   // 定義追加
      ++serial_no_f_;                                                           // 管理番号カウントアップ
    }
    break;

  case 'G':                                                                     // VRC7
    // Gチャンネルでのみユーザー音色を登録する
    if (color_cmd_ghijkl_.find(prg_no) == color_cmd_ghijkl_.end()) {            // 音色が登録されていなければ
      if (!tone_def_vct_vrc7_[prg_no].empty()) {                                // 音色定義VRC7が登録されていたら
        ostringstream cmd;                                                      ///< コマンド作成
        cmd << "OP" << serial_no_ghijkl_;                                       // 管理番号を設定
        color_cmd_ghijkl_[prg_no] = cmd.str();                                  // コマンド追加
        ostringstream def;                                                      ///< 定義作成
        def << "@OP" << serial_no_ghijkl_                                       // 管理番号を設定
            << "\t={" << tone_def_vct_vrc7_[prg_no]                             // 音色定義VRC7を設定
            << "}\t\t\t" << CommentEdit(ch, prg_no);                            // コメントを設定
        color_def_ghijkl_[serial_no_ghijkl_] = def.str();                       // 定義追加
        ++serial_no_ghijkl_;                                                    // 管理番号カウントアップ
      }
    }
    break;

  case 'H':                                                                     // VRC7
  case 'I':                                                                     // VRC7
  case 'J':                                                                     // VRC7
  case 'K':                                                                     // VRC7
  case 'L':                                                                     // VRC7
    // Gチャンネル以外ではユーザー音色を登録しない
    break;

  case 'P':                                                                     // N106
  case 'Q':                                                                     // N106
  case 'R':                                                                     // N106
  case 'S':                                                                     // N106
  case 'T':                                                                     // N106
  case 'U':                                                                     // N106
  case 'V':                                                                     // N106
  case 'W':                                                                     // N106
    shift_no += static_cast<int>(ch - 'P') * 128;                               // 同じ波形でもチャンネル毎にバッファ番号を変える必要がある為
    if ((color_cmd_pqrstuvw_.find(shift_no) == color_cmd_pqrstuvw_.end()) &&    // 音色が登録されておらず、
        (127 >= serial_no_pqrstuvw_)) {                                         // 最大登録件数以内なら
      ostringstream cmd;                                                        ///< コマンド作成
      cmd << "@@" << serial_no_pqrstuvw_;
      color_cmd_pqrstuvw_[shift_no] = cmd.str();                                // コマンド追加
      ostringstream def;                                                        ///< 定義作成
      if (!tone_def_vct_n106_[prg_no].empty()) {                                // 定義が有るなら
        def << "@N" << serial_no_pqrstuvw_ << " \t={" << ch - 'P' << ","
            << tone_def_vct_n106_[prg_no] << "}\t" << CommentEdit(ch, prg_no);
      } else {                                                                  // 定義が無いなら
        def << "@N" << serial_no_pqrstuvw_ << " \t={" << ch - 'P'
            << ",15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0}\t"
            << CommentEdit(ch, prg_no);
      }
      color_def_pqrstuvw_[serial_no_pqrstuvw_] = def.str();                     // 定義追加
      ++serial_no_pqrstuvw_;                                                    // 管理番号カウントアップ
    }
    break;

  case 'X':                                                                     // FME7
  case 'Y':                                                                     // FME7
  case 'Z':                                                                     // FME7
    if (color_cmd_xyz_.find(prg_no) == color_cmd_xyz_.end()) {                  // 音色が登録されていなければ
      ostringstream cmd;                                                        ///< コマンド作成
      cmd << "@1";
      color_cmd_xyz_[prg_no] = cmd.str();                                       // コマンド追加
      ++serial_no_xyz_;                                                         // 管理番号カウントアップ
    }
    break;

  case 'C':                                                                     // 2A03 三角波
  case 'O':                                                                     // VRC6 鋸波
    // 音色無し
    break;

  default:                                                                      // その他の値
    cerr << "異常なチャンネル'" << ch << "'がありました。\n";
    break;
  }
  // LFO(MPコマンド)
  switch (ch) {
  case 'A':                                                                     // 2A03 矩形波
  case 'B':                                                                     // 2A03 矩形波
  case 'a':                                                                     // MMC5 矩形波
  case 'b':                                                                     // MMC5 矩形波
  case 'C':                                                                     // 2A03 三角波
  case 'F':                                                                     // FDS
  case 'G':                                                                     // VRC7
  case 'H':                                                                     // VRC7
  case 'I':                                                                     // VRC7
  case 'J':                                                                     // VRC7
  case 'K':                                                                     // VRC7
  case 'L':                                                                     // VRC7
  case 'M':                                                                     // VRC6 矩形波
  case 'N':                                                                     // VRC6 矩形波
  case 'O':                                                                     // VRC6 鋸波
  case 'P':                                                                     // N106
  case 'Q':                                                                     // N106
  case 'R':                                                                     // N106
  case 'S':                                                                     // N106
  case 'T':                                                                     // N106
  case 'U':                                                                     // N106
  case 'V':                                                                     // N106
  case 'W':                                                                     // N106
  case 'X':                                                                     // FME7
  case 'Y':                                                                     // FME7
  case 'Z':                                                                     // FME7
    // メロディLFO用
    if ((lfo_cmd_.find(prg_no) == lfo_cmd_.end()) &&                            // LFOコマンドが登録されておらず、
        (63 >= serial_no_lfo_)) {                                               // 最大登録件数以内なら
      ostringstream cmd;                                                        ///< コマンド作成
      cmd << "MP" << serial_no_lfo_;
      lfo_cmd_[prg_no] = cmd.str();                                             // コマンド追加
      ostringstream def;                                                        ///< 定義作成
      def << "@MP" << serial_no_lfo_ << "\t={29,2,1,0}\t\t\t\t\t\t\t\t\t"
          << CommentEdit(ch, prg_no);
      lfo_def_[serial_no_lfo_] = def.str();                                     // 定義追加
      ++serial_no_lfo_;                                                         // 管理番号カウントアップ
    }
    break;

  case 'D':                                                                     // 2A03 ノイズ
    // ノイズLFO用
    // 使わない
    break;

  case 'E':                                                                     // 2A03 DPCM
    // LFO無し
    break;
  }
  // 音量(エンヴェロープ @vコマンド)
  if (VolumeMode::kToneBased & volume_mode_) {                                  // 音色音量モードなら
    shift_no = prg_no;                                                          // 重複しないプログラム番号
    switch (ch) {
    case 'D':                                                                   // 2A03 ノイズ
      shift_no = ParMapChg(shift_no);                                           // DPCMマップ(未登録部分の置換)
      shift_no += 128;                                                          // 重複しないプログラム番号
      [[fallthrough]];                                                          // VRC7用の範囲へ続行

    case 'G':                                                                   // VRC7
    case 'H':                                                                   // VRC7
    case 'I':                                                                   // VRC7
    case 'J':                                                                   // VRC7
    case 'K':                                                                   // VRC7
    case 'L':                                                                   // VRC7
      shift_no += 128;                                                          // 重複しないプログラム番号
      [[fallthrough]];                                                          // 64段階音量用の範囲へ続行

    case 'F':                                                                   // FDS
    case 'O':                                                                   // VRC6 鋸波
      // Volume 0～63用
      shift_no += 128;                                                          // 重複しないプログラム番号
      [[fallthrough]];                                                          // 15段階音量用の共通処理へ続行

    case 'A':                                                                   // 2A03 矩形波
    case 'B':                                                                   // 2A03 矩形波
    case 'a':                                                                   // MMC5 矩形波
    case 'b':                                                                   // MMC5 矩形波
    case 'M':                                                                   // VRC6 矩形波
    case 'N':                                                                   // VRC6 矩形波
    case 'P':                                                                   // N106
    case 'Q':                                                                   // N106
    case 'R':                                                                   // N106
    case 'S':                                                                   // N106
    case 'T':                                                                   // N106
    case 'U':                                                                   // N106
    case 'V':                                                                   // N106
    case 'W':                                                                   // N106
    case 'X':                                                                   // FME7
    case 'Y':                                                                   // FME7
    case 'Z':                                                                   // FME7
      // Volume 0～15用
    {
      uint16_t vol = VolumeConvert(ch, volume);                                 ///< 音量をMMLの音量に変換
      auto vd_mit = volume_def_.find(shift_no);                                 // 音量定義イテレータ、音量(エンヴェロープ)を検索
      if (vd_mit == volume_def_.end()) {                                        // 音量(エンヴェロープ)が登録されていない場合
        map<uint16_t, VolumeInfo> vol_map;                                      ///< 音量マップを作成
        VolumeInfo vol_info;                                                    ///< 音量情報を作成
        vol_info.ch = ch;                                                       // チャンネルを登録
        vol_map[vol] = vol_info;                                                // 音量マップに音量情報を登録
        volume_def_[shift_no] = vol_map;                                        // 音量定義マップに音量マップを登録
      } else {                                                                  // 音量(エンヴェロープ)が登録されている場合
        map<uint16_t, VolumeInfo>& vol_map = vd_mit->second;                    ///< 音量マップの参照
        if (vol_map.find(vol) == vol_map.end()) {                               // 音量マップが登録されていない場合
          VolumeInfo vol_info;                                                  ///< 音量情報を作成
          vol_info.ch = ch;                                                     // チャンネルを登録
          vol_map[vol] = vol_info;                                              // 音量マップに音量情報を登録
        }
      }
    }
    break;

    case 'C':                                                                   // 2A03 三角波
    case 'E':                                                                   // 2A03 DPCM
    default:                                                                    // その他の値
      // 音量定義無し
      break;
    }
  }
}

/**
 * @brief MIDIベロシティをMML音量へ変換する
 * @param ch (i)対象チャンネル
 * @param vel (i)MIDIベロシティ
 * @return MML音量
 */
uint8_t Mml::Tone::VolumeConvert(                                               // MIDIベロシティをMML音量へ変換する
    const uint8_t& ch,                                                          ///< (i)チャンネル
    const uint8_t& vel) const noexcept                                          ///< (i)ベロシティ
{
  uint8_t vol(vel);                                                             ///< 音量
  switch (ch) {
  case 'A':                                                                     // 2A03 矩形波
  case 'B':                                                                     // 2A03 矩形波
  case 'a':                                                                     // MMC5 矩形波
  case 'b':                                                                     // MMC5 矩形波
    vol = static_cast<uint8_t>(vol * .097);                                     // MIDIベロシティ127で最大12
    if (15 < vol) vol = 15;
    break;

  case 'M':                                                                     // VRC6 矩形波
  case 'N':                                                                     // VRC6 矩形波
    vol = static_cast<uint8_t>(vol * .097);                                     // MIDIベロシティ127で最大12
    if (15 < vol) vol = 15;
    break;

  case 'G':                                                                     // VRC7
  case 'H':                                                                     // VRC7
  case 'I':                                                                     // VRC7
  case 'J':                                                                     // VRC7
  case 'K':                                                                     // VRC7
  case 'L':                                                                     // VRC7
    vol = static_cast<uint8_t>(vol * .097);                                     // MIDIベロシティ127で最大12
    if (15 < vol) vol = 15;
    break;

  case 'P':                                                                     // N106
  case 'Q':                                                                     // N106
  case 'R':                                                                     // N106
  case 'S':                                                                     // N106
  case 'T':                                                                     // N106
  case 'U':                                                                     // N106
  case 'V':                                                                     // N106
  case 'W':                                                                     // N106
    vol = static_cast<uint8_t>(vol * .1255);                                    // MIDIベロシティ127で最大15
    if (15 < vol) vol = 15;
    break;

  case 'X':                                                                     // FME7
  case 'Y':                                                                     // FME7
  case 'Z':                                                                     // FME7
    vol = static_cast<uint8_t>(vol * .111);                                     // MIDIベロシティ127で最大14
    if (15 < vol) vol = 15;
    break;

  case 'F':                                                                     // FDS
    vol = static_cast<uint8_t>(vol * .146);                                     // MIDIベロシティ127で最大18
    if (31 < vol) vol = 31;
    break;

  case 'O':                                                                     // VRC6 鋸波
    vol = static_cast<uint8_t>(vol * .3);                                       // MIDIベロシティ127で最大38
    if (63 < vol) vol = 63;
    break;

  case 'D':                                                                     // 2A03 ノイズ
    vol = static_cast<uint8_t>(vol * .17);                                      // MIDIベロシティ127で計算値21、上限適用後15
    if (vol > 15) {
      vol = 15;
    }
    break;

  default:                                                                      // その他
    vol >>= 3;                                                                  // MIDIベロシティ127で最大15
    break;
  }
  return vol;
}

/** @brief 音量定義を音色へ割り当てる */
void Mml::Tone::AssignVolume(                                                   // 音量定義を音色へ割り当てる
    uint16_t vol_def_threshold)                                                 ///< (i)音量登録数間引き閾値
{
  // 音色定義割当て
  uint16_t serial_no(0);                                                        ///< 管理番号
  for (auto& [Definition, VolMap] : volume_def_) {                              // 音色・音量定義マップループ
    for (auto& [Volume, VolInfo] : VolMap) {                                    // 音量マップループ
      VolInfo.serial_no = serial_no++;                                          // 管理番号を割当て
      const uint16_t definition_index = Definition & 0x7F;                      ///< 音色定義配列の添字
      switch (VolInfo.ch) {
      case 'D':                                                                 // 2A03 ノイズ
        VolInfo.define = AdjustVolume(                                          // 音量定義を割当て
            static_cast<float>(Volume / 15.),
            volume_def_vct_noise_[Definition & 0x7F].empty() ?
              "15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0" :
              volume_def_vct_noise_[Definition & 0x7F]);                        // 音量定義配列[音色別]を音量調節する
        break;

      case 'G':                                                                 // VRC7
      case 'H':                                                                 // VRC7
      case 'I':                                                                 // VRC7
      case 'J':                                                                 // VRC7
      case 'K':                                                                 // VRC7
      case 'L':                                                                 // VRC7
        VolInfo.define = AdjustVolume(                                          // 音量定義を割当て、音量定義配列[音色別]を音量調節する
            static_cast<float>(Volume / 15.),
            volume_def_vct_vrc7_pre_[definition_index].empty() ?
              "15" :
              volume_def_vct_vrc7_pre_[definition_index]);
        break;

      case 'O':                                                                 // VRC6 鋸波
        VolInfo.define = AdjustVolume(                                          // 音量定義を割当て
            static_cast<float>(Volume / 63.),                                   // MIDI音量を音量定義の倍率へ変換
            volume_def_vct_common64_[definition_index].empty() ?
              "63,58,58,54,54,54,50,50,50,46" :
              volume_def_vct_common64_[definition_index]);                      // 64段階音量定義を音量調節する
        break;

      case 'F':                                                                 // FDS
        VolInfo.define = AdjustVolume(                                          // 音量定義を割当て
            static_cast<float>(Volume / 31.),
            volume_def_vct_common32_[definition_index].empty() ?
              "31,30,30,27,27,27,25,25,25,23" :
              volume_def_vct_common32_[definition_index]);                      // 32段階音量定義を音量調節する
        break;

      default:                                                                  // それ以外
        VolInfo.define = AdjustVolume(                                          // 音量定義を割当て
            static_cast<float>(Volume / 15.),
            volume_def_vct_common_[definition_index].empty() ?
              "15 14 14 13 13 13 12 12 12 11" :
              volume_def_vct_common_[definition_index]);                        // 音量定義配列[音色別]を音量調節する
        break;
      }
    }
  }
  // 音色音量モード用の設定
  if (VolumeMode::kToneBased == volume_mode_) {                                 // 音量モードが音色音量モードの場合
    volume_def_mask_ = 0x7;                                                     // 最初から0～14を間引きする設定にする
    serial_no = 128;                                                            // 間引き処理を必ずやる設定にする
  }
  // 間引き処理
  while ((vol_def_threshold <= serial_no) && (0x10 > volume_def_mask_)) {       // 登録件数が多過ぎで間引きが可能なら
    volume_def_mask_ = (volume_def_mask_ << 1) | 0x1;                           // 音量マスクのビットを増やす
    serial_no = 0;                                                              // 管理番号を最初から
    for (auto& [Definition, VolMap] : volume_def_) {                            // 音量定義マップループ
      for (auto vol_map_it  = VolMap.begin();
                vol_map_it != VolMap.end();) {                                  // 音量マップを先頭から最後まで確認し、ループ内で進める
        VolumeInfo& vol_info = vol_map_it->second;                              ///< 音量情報の参照
        if ((~vol_map_it->first) & volume_def_mask_) {                          // この定義が間引きの対象なら
          uint8_t dest_vol = vol_map_it->first | volume_def_mask_;              ///< 移動先音量
          auto fit = VolMap.find(dest_vol);                                     ///< 移動先音量定義の存在を確認
          if (fit == VolMap.end()) {                                            // 無ければ
            VolMap[dest_vol] = vol_info;                                        // この定義をコピー
          }
          vol_map_it = VolMap.erase(vol_map_it);                                // この定義を削除
        } else {                                                                // この定義が残す対象なら
          vol_info.serial_no = serial_no++;                                     // 管理番号を割当て、カウント
          ++vol_map_it;                                                         // 次へ
        }
      }
    }
  }
}

/** @brief 音量定義の値を指定割合で調整する */
string Mml::Tone::AdjustVolume(                                                 // 音量定義の値を指定割合で調整する
    const float& ratio,                                                         ///< (i)割合
    const string& define) const                                                 ///< (i)定義
{
  string edit_def(define);                                                      ///< 編集用
  replace(edit_def.begin(), edit_def.end(), ',', ' ');                          // ','をスペースに変換
  istringstream iss(edit_def);                                                  ///< 文字列切り出し
  ostringstream oss;                                                            ///< 文字列編集
  while (!iss.fail() && !iss.eof()) {                                           // エラーでも終端でもないあいだループ
    uint16_t val;                                                               ///< 数値
    iss >> val;                                                                 // 読み込み
    if (iss.fail()) {                                                           // エラーなら
      iss.clear();                                                              // エラーをクリア
      string token;                                                             ///< 文節
      iss >> token;                                                             // 文字列として読み込み
      oss << token;                                                             // そのまま出力
    } else {                                                                    // エラーがなければ
      val = static_cast<uint16_t>(val * ratio);                                 // 割合を掛ける
      oss << val;                                                               // 出力
    }
    if (!iss.eof()) {                                                           // 終端でなければ
      oss << ",";                                                               // 区切りを出力
    }
  }
  return oss.str();                                                             // 音量調節した定義を返す
}

/** @brief 登録済みの音質コマンドを取得する */
string Mml::Tone::Get(                                                          // 登録済みの音質コマンドを取得する
    const uint8_t& ch,                                                          ///< (i)チャンネル
    const uint8_t& prg_no) const                                                ///< (i)プログラム番号
{
  string cmd_str;                                                               ///< 返却する音質・LFOコマンド文字列
  uint16_t shift_no(prg_no);                                                    ///< 重複しないプログラム番号
  switch (ch) {
  case 'M':                                                                     // VRC6 矩形波
  case 'N':                                                                     // VRC6 矩形波
    // 矩形波Duty0～7用
    shift_no += 128;                                                            // 重複しないプログラム番号
    [[fallthrough]];                                                            // Duty0～3用の共通処理へ続行

  case 'A':                                                                     // 2A03 矩形波
  case 'B':                                                                     // 2A03 矩形波
  case 'a':                                                                     // MMC5 矩形波
  case 'b':                                                                     // MMC5 矩形波
    // 矩形波Duty0～3用
    if (const auto it = color_cmd_abab_mn_.find(shift_no);                      // 矩形波音色を検索し、
        it != color_cmd_abab_mn_.end()) {                                       // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    } else {                                                                    // 未登録なら
      ostringstream oss;                                                        ///< メッセージ編集
      oss << "/* 未登録PrgNo=" << static_cast<uint16_t>(prg_no) << " */";
      cmd_str = oss.str();                                                      // メッセージを取得
    }
    break;

  case 'D':                                                                     // 2A03 ノイズ
    shift_no = ParMapChg(prg_no);                                               // パーカッション未登録部分をマップ
    if (const auto it = color_cmd_d_.find(shift_no);                            // ノイズ音色を検索し、
        it != color_cmd_d_.end()) {                                             // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    }
    break;

  case 'F':                                                                     // FDS
    if (const auto it = color_cmd_f_.find(prg_no);                              // FDS音色を検索し、
        it != color_cmd_f_.end()) {                                             // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    } else {                                                                    // 未登録なら
      ostringstream oss;                                                        ///< メッセージ編集
      oss << "/* 未登録PrgNo=" << static_cast<uint16_t>(prg_no) << " */";
      cmd_str = oss.str();                                                      // メッセージを取得
    }
    break;

  case 'G':                                                                     // VRC7
  case 'H':                                                                     // VRC7
  case 'I':                                                                     // VRC7
  case 'J':                                                                     // VRC7
  case 'K':                                                                     // VRC7
  case 'L':                                                                     // VRC7
    // VRC7音色コマンドを検索
    if (const auto it = color_cmd_ghijkl_.find(prg_no);                         // VRC7音色を検索し、
        it != color_cmd_ghijkl_.end()) {                                        // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    }
    // 未登録なら空文字列を返す（受け取り側でプリセット音色を選択するため）
    break;

  case 'P':                                                                     // N106
  case 'Q':                                                                     // N106
  case 'R':                                                                     // N106
  case 'S':                                                                     // N106
  case 'T':                                                                     // N106
  case 'U':                                                                     // N106
  case 'V':                                                                     // N106
  case 'W':                                                                     // N106
    // N106音色コマンドをチャンネル別に検索
    shift_no += static_cast<int>(ch - 'P') * 128;                               // 同じ波形でもチャンネル毎にバッファ番号を変えるため
    if (const auto it = color_cmd_pqrstuvw_.find(shift_no);                     // N106音色を検索し、
        it != color_cmd_pqrstuvw_.end()) {                                      // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    } else {                                                                    // 未登録なら
      ostringstream oss;                                                        ///< メッセージ編集
      oss << "/* 未登録PrgNo=" << static_cast<uint16_t>(prg_no) << " */";
      cmd_str = oss.str();                                                      // メッセージを取得
    }
    break;

  case 'X':                                                                     // FME7
  case 'Y':                                                                     // FME7
  case 'Z':                                                                     // FME7
    // FME7音色コマンドを検索
    if (const auto it = color_cmd_xyz_.find(prg_no);                            // FME7音色を検索し、
        it != color_cmd_xyz_.end()) {                                           // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    } else {                                                                    // 未登録なら
      ostringstream oss;                                                        ///< メッセージ編集
      oss << "/* 未登録PrgNo=" << static_cast<uint16_t>(prg_no) << " */";
      cmd_str = oss.str();                                                      // メッセージを取得
    }
    break;

  case 'E':                                                                     // 2A03 DPCM
  case 'C':                                                                     // 2A03 三角波
  case 'O':                                                                     // VRC6 鋸波
    // 音色無し
    break;

  default:                                                                      // その他の値
    break;
  }
  // LFO(MPコマンド)
  switch (ch) {
  case 'A':                                                                     // 2A03 矩形波
  case 'B':                                                                     // 2A03 矩形波
  case 'a':                                                                     // MMC5 矩形波
  case 'b':                                                                     // MMC5 矩形波
  case 'C':                                                                     // 2A03 三角波
  case 'F':                                                                     // FDS
  case 'G':                                                                     // VRC7
  case 'H':                                                                     // VRC7
  case 'I':                                                                     // VRC7
  case 'J':                                                                     // VRC7
  case 'K':                                                                     // VRC7
  case 'L':                                                                     // VRC7
  case 'M':                                                                     // VRC6 矩形波
  case 'N':                                                                     // VRC6 矩形波
  case 'O':                                                                     // VRC6 鋸波
  case 'P':                                                                     // N106
  case 'Q':                                                                     // N106
  case 'R':                                                                     // N106
  case 'S':                                                                     // N106
  case 'T':                                                                     // N106
  case 'U':                                                                     // N106
  case 'V':                                                                     // N106
  case 'W':                                                                     // N106
  case 'X':                                                                     // FME7
  case 'Y':                                                                     // FME7
  case 'Z':                                                                     // FME7
    // メロディLFO用
    if (const auto it = lfo_cmd_.find(prg_no);                                  // LFOコマンドを検索し、
        it != lfo_cmd_.end()) {                                                 // 登録されていたら
      cmd_str += it->second;                                                    // コマンドを取得
    } else {                                                                    // 未登録なら
      ostringstream oss;                                                        ///< メッセージ編集
      oss << "/* Lfo未登録PrgNo=" << static_cast<uint16_t>(prg_no) << " */";
      cmd_str += oss.str();                                                     // メッセージを取得
    }
    break;

  case 'D':                                                                     // 2A03 ノイズ
    // ノイズLFO用
    // 使わない
    break;

  case 'E':                                                                     // 2A03 DPCM
    // LFO無し
    break;
  }
  return EncodeSourceText(cmd_str);                                             // 出力文字コードへ変換したコマンドを返す
}

/** @brief 登録済みの音符コマンドを取得する */
string Mml::Tone::GetNote(                                                      // 登録済みの音符コマンドを取得する
    const uint8_t& ch,                                                          ///< (i)チャンネル
    const uint8_t& prg_no) const                                                ///< (i)プログラム番号
{
  string cmd_str;                                                               ///< 返却する音符コマンド文字列
  uint16_t shift_no(prg_no);                                                    ///< 重複しないプログラム番号
  // 音符
  switch (ch) {
  case 'D':                                                                     // 2A03 ノイズ
    shift_no = ParMapChg(prg_no);                                               // パーカッション未登録部分をマップ
    if (const auto it = note_cmd_d_.find(shift_no);                             // ノイズ音符を検索し、
        it != note_cmd_d_.end()) {                                              // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    } else {                                                                    // 未登録なら
      ostringstream oss;                                                        ///< メッセージ編集
      oss << "/* 未登録PrgNo=" << static_cast<uint16_t>(prg_no) << " */";
      cmd_str = oss.str();                                                      // メッセージを取得
    }
    break;

  case 'E':                                                                     // 2A03 DPCM
    shift_no = ParMapChg(prg_no);                                               // パーカッション未登録部分をマップ
    if (const auto it = note_cmd_e_.find(shift_no);                             // DPCM音符を検索し、
        it != note_cmd_e_.end()) {                                              // 登録されていたら
      cmd_str = it->second;                                                     // コマンドを取得
    } else {                                                                    // 未登録なら
      ostringstream oss;                                                        ///< メッセージ編集
      oss << "/* 未登録PrgNo=" << static_cast<uint16_t>(prg_no) << " */";
      cmd_str = oss.str();                                                      // メッセージを取得
    }
    break;

  default:                                                                      // それ以外
    // 直接指定するので、ここでは取らない
    break;
  }
  return EncodeSourceText(cmd_str);                                             // 出力文字コードへ変換した音符コマンドを返す
}

/** @brief 登録済みの音量コマンドを取得する */
string Mml::Tone::GetVolume(                                                    // 登録済みの音量コマンドを取得する
    const uint8_t& ch,                                                          ///< (i)チャンネル
    const uint8_t& prg_no,                                                      ///< (i)プログラム番号
    const uint8_t& volume) const                                                ///< (i)音量
{
  // 音量(エンヴェロープ @vコマンド)
  string cmd_str;                                                               ///< 返却する音量コマンド文字列
  if (VolumeMode::kToneBased & volume_mode_) {                                  // 音色音量モードなら
    uint16_t shift_no(prg_no);                                                  ///< 重複しないプログラム番号
    switch (ch) {
    case 'D':                                                                   // 2A03 ノイズ
      shift_no = ParMapChg(shift_no);                                           // DPCMマップ(未登録部分の置換)
      shift_no += 128;                                                          // 重複しないプログラム番号
      [[fallthrough]];                                                          // VRC7用の範囲へ続行

    case 'G':                                                                   // VRC7
    case 'H':                                                                   // VRC7
    case 'I':                                                                   // VRC7
    case 'J':                                                                   // VRC7
    case 'K':                                                                   // VRC7
    case 'L':                                                                   // VRC7
      // VRC7音量用の管理番号範囲へ移動
      shift_no += 128;                                                          // 重複しないプログラム番号
      [[fallthrough]];                                                          // 64段階音量用の範囲へ続行

    case 'F':                                                                   // FDS
    case 'O':                                                                   // VRC6 鋸波
      // 64段階音量用の管理番号範囲へ移動
      shift_no += 128;                                                          // 重複しないプログラム番号
      [[fallthrough]];                                                          // 15段階音量用の共通処理へ続行

    case 'A':                                                                   // 2A03 矩形波
    case 'B':                                                                   // 2A03 矩形波
    case 'a':                                                                   // MMC5 矩形波
    case 'b':                                                                   // MMC5 矩形波
    case 'M':                                                                   // VRC6 矩形波
    case 'N':                                                                   // VRC6 矩形波
    case 'P':                                                                   // N106
    case 'Q':                                                                   // N106
    case 'R':                                                                   // N106
    case 'S':                                                                   // N106
    case 'T':                                                                   // N106
    case 'U':                                                                   // N106
    case 'V':                                                                   // N106
    case 'W':                                                                   // N106
    case 'X':                                                                   // FME7
    case 'Y':                                                                   // FME7
    case 'Z':                                                                   // FME7
      // 15段階音量用
    {
      auto vd_mit = volume_def_.find(shift_no);                                 // 音量定義イテレータ、音量(エンヴェロープ)を検索
      ostringstream oss;                                                        ///< 文字列編集用
      if (vd_mit != volume_def_.end()) {                                        // 音量(エンヴェロープ)が登録されている場合
        const map<uint16_t, VolumeInfo>& vol_map = vd_mit->second;              ///< 音量マップの参照
        auto vol_map_it = vol_map.find(volume | volume_def_mask_);              // 音量マップイテレータ、音量マップを検索
        if (vol_map_it != vol_map.end()) {                                      // 音量マップが登録されている場合
          const VolumeInfo& vol_info = vol_map_it->second;                      ///< 音量情報の参照
          oss << "@v" << vol_info.serial_no;                                    // コマンド編集
        } else {
          oss << "/* 音量コマンド取得で未登録PrgNo." << static_cast<uint16_t>(0x7F & prg_no)
              << " Vol." << static_cast<uint16_t>(volume) << " */";
        }
      } else {                                                                  // 音量(エンヴェロープ)が登録されていない場合
        oss << "/* 音量コマンド取得で未登録PrgNo." << static_cast<uint16_t>(prg_no) << " */";
      }
      cmd_str = oss.str();                                                      // コマンドを取得
    }
    break;

    case 'C':                                                                   // 2A03 三角波
    case 'E':                                                                   // 2A03 DPCM
      // 音量コマンド無し
      break;

    default:                                                                    // その他の値
      break;
    }
  }
  return EncodeSourceText(cmd_str);                                             // 出力文字コードへ変換した音量コマンドを返す
}

/** @brief 未登録のパーカッション音色を補完する */
uint16_t Mml::Tone::ParMapChg(                                                  // 未登録のパーカッション音色を補完する
    const uint16_t& prg_no) const noexcept                                      ///< (i)プログラム番号
{
  switch (prg_no) {                                                             // プログラム番号で分岐
  case 0: return 36;                                                            // Standard 1 Kick 1 → Bass Drum 1
  case 1: return 35;                                                            // Standard 1 Kick 2 → Bass Drum 2
  case 2: return 36;                                                            // Standard 2 Kick 1 → Bass Drum 1
  case 3: return 35;                                                            // Standard 2 Kick 2 → Bass Drum 2
  case 4: return 36;                                                            // Kick Drum 1 → Bass Drum 1
  case 5: return 35;                                                            // Kick Drum 2 → Bass Drum 2
  case 6: return 36;                                                            // Jazz Kick 1 → Bass Drum 1
  case 7: return 35;                                                            // Jazz Kick 2 → Bass Drum 2
  case 8: return 36;                                                            // Room Kick 1 → Bass Drum 1
  case 9: return 35;                                                            // Room Kick 2 → Bass Drum 2
  case 10: return 36;                                                           // Power Kick 1 → Bass Drum 1
  case 11: return 35;                                                           // Power Kick 2 → Bass Drum 2
  case 12: return 35;                                                           // Electric Kick 2 → Bass Drum 2
  case 13: return 36;                                                           // Electric Kick 1 → Bass Drum 1
  case 14: return 36;                                                           // TR-808 Kick → Bass Drum 1
  case 15: return 35;                                                           // TR-909 Kick → Bass Drum 2
  case 16: return 36;                                                           // Dance Kick → Bass Drum 1
  case 17: return 83;                                                           // Voice One → Jingle Bell
  case 18: return 83;                                                           // Voice Two → Jingle Bell
  case 19: return 83;                                                           // Voice Three → Jingle Bell
  case 20: return 83;                                                           // ? → Jingle Bell
  case 21: return 83;                                                           // ? → Jingle Bell
  case 22: return 47;                                                           // MC-500 Beep1 → Long Whistle
  case 23: return 47;                                                           // MC-500 Beep2 → Long Whistle
  case 24: return 52;                                                           // Concert DS → Chinese Cymbal
  case 88: return 72;                                                           // Applause 2 → Long Whistle
  case 89: return 78;                                                           // Mute Cuica → Mute Cuica
  case 90: return 79;                                                           // Open Cuica → Open Cuica
  case 91: return 80;                                                           // Mute Triangle → Mute Triangle
  case 92: return 81;                                                           // Open Triangle → Open Triangle
  case 93: return 73;                                                           // Short Guiro → Short Guiro
  case 94: return 74;                                                           // Long Guiro → Long Guiro
  case 95: return 69;                                                           // Cabasa Up → Cabasa
  case 96: return 69;                                                           // Cabasa Down → Cabasa
  case 97: return 38;                                                           // Standard 1 Snare 1 → Snare Drum 1
  case 98: return 40;                                                           // Standard 1 Snare 2 → Snare Drum 2
  case 99: return 38;                                                           // Standard 2 Snare 1 → Snare Drum 1
  case 100: return 40;                                                          // Standard 2 Snare 2 → Snare Drum 2
  case 101: return 40;                                                          // Snare Drum 2 → Snare Drum 2
  case 102: return 38;                                                          // Standard 1 Snare 1 → Snare Drum 1
  case 103: return 40;                                                          // Standard 1 Snare 2 → Snare Drum 2
  case 104: return 40;                                                          // Standard 1 Snare 3 → Snare Drum 2
  case 105: return 38;                                                          // Jazz Snare 1 → Snare Drum 1
  case 106: return 40;                                                          // Jazz Snare 2 → Snare Drum 2
  case 107: return 38;                                                          // Room Snare 1 → Snare Drum 1
  case 108: return 40;                                                          // Room Snare 2 → Snare Drum 2
  case 109: return 38;                                                          // Power Snare 1 → Snare Drum 1
  case 110: return 40;                                                          // Power Snare 2 → Snare Drum 2
  case 111: return 38;                                                          // Gated Snare → Snare Drum 1
  case 112: return 38;                                                          // Dance Snare 1 → Snare Drum 1
  case 113: return 40;                                                          // Dance Snare 2 → Snare Drum 2
  case 114: return 38;                                                          // Disco Snare → Snare Drum 1
  case 115: return 40;                                                          // Electric Snare 2 → Snare Drum 2
  case 116: return 38;                                                          // Electric Snare → Snare Drum 1
  case 117: return 40;                                                          // Electric Snare 3 → Snare Drum 2
  case 118: return 38;                                                          // TR-707 Snare 1 → Snare Drum 1
  case 119: return 38;                                                          // TR-808 Snare 1 → Snare Drum 1
  case 120: return 40;                                                          // TR-808 Snare 2 → Snare Drum 2
  case 121: return 38;                                                          // TR-909 Snare 1 → Snare Drum 1
  case 122: return 40;                                                          // TR-909 Snare 2 → Snare Drum 2
  case 123: return 38;                                                          // Rap Snare → Snare Drum 1
  case 124: return 38;                                                          // Jungle Snare 1 → Snare Drum 1
  case 125: return 38;                                                          // House Snare 1 → Snare Drum 1
  case 126: return 40;                                                          // House Snare → Snare Drum 2
  case 127: return 40;                                                          // House Snare 2 → Snare Drum 2
  }
  return prg_no;
}

/** @brief ピッチエンベロープ定義を登録し、EPコマンド文字列を返す */
string Mml::Pitch::Regist(                                                      // ピッチエンベロープ定義を登録し、EPコマンド文字列を返す
    vector<char>& envelope,                                                     ///< (io)エンヴェロープ
    uint8_t ch)                                                                 ///< (i)チャンネル
{
  // 末尾の０の連続を削除
  uint32_t reg_no = envelope_def_.size();                                       ///< 登録番号
  for (uint32_t i = envelope.size(); i > 0;) {                                  ///< エンベロープの添字
    --i;                                                                        // １つ手前へ
    char& val = envelope[i];                                                    ///< エンヴェロープの値を参照
    if (val) {                                                                  // エンヴェロープの値が０以外なら
      break;                                                                    // ループを抜ける
    }
    envelope.pop_back();                                                        // その０を削除する
  }
  if (envelope.empty()) {                                                       // エンヴェロープが空なら
    return "EPOF";                                                              // EPOFコマンド文字列を返す
  }
  // 定義文字列作成、及び、変化量最大最小の検索
  int frequency(0), max(0), min(0);                                             ///< 周波数変化量の累積値・最大値・最小値
  ostringstream oss;                                                            ///< 文字列編集用
  for (const auto& val : envelope) {                                            // エンヴェロープループ
    oss << static_cast<int>(val) << ",";                                        // エンヴェロープを編集
    frequency += val;                                                           // 現在の相対周波数
    if (max < frequency) max = frequency;                                       // 最大値更新
    if (min > frequency) min = frequency;                                       // 最小値更新
  }
  // ピッチエンヴェロープ変化量チェック
  if (register_threshold_ > (max - min)) {                                      // 変化量が、ピッチエンヴェロープ登録閾値（変化量下限）未満なら
    return "EPOF";                                                              // EPOFコマンド文字列を返す
  }
  // 同じ登録内容を検索
  string def_str = oss.str();                                                   ///< 定義文字列を取り出し
  uint32_t same_no(0xFFFFFFFF);                                                 ///< 同じ登録番号（不一致を設定）
  uint32_t i;                                                                   ///< ループ用添字
  for (i = 0; i < reg_no; ++i) {                                                // 既に登録されているものをチェック
    if (def_str == envelope_def_[i]) {                                          // 同じ登録内容なら
      same_no = i;                                                              // 登録番号に見つかった番号を取得
      break;
    }
  }
  // 検索結果により分岐
  if (same_no != i) {                                                           // 同じ登録内容が見つからない
    if (reg_no < register_max_) {                                               // 最大登録数未満なら
      envelope_def_.push_back(oss.str());                                       // エンヴェロープ定義文字列を登録
      oss.str("");                                                              // 文字列をクリアする
      oss << "EP" << reg_no;                                                    // コマンド文字列を編集
    } else {                                                                    // 最大登録数に達していたら
      oss.str("");                                                              // 文字列をクリアする
      oss << "EPOF";                                                            // EPOFコマンド文字列を編集
    }
  } else {                                                                      // 同じ登録内容が見つかった場合
    oss.str("");                                                                // 文字列をクリアする
    oss << "EP" << same_no;                                                     // 見つかった登録番号のEPコマンド文字列を編集
  }
  return oss.str();                                                             // コマンド文字列を返す
}

/** @brief 複数のLコマンドから最後のループ位置を決定する */
void Mml::LoopPointConclusion() {                                               // 複数のLコマンドから最後のループ位置を決定する
  int last_l_pos = 0;                                                           ///< Lコマンド最終位置
  {                                                                             // Lコマンド最終位置探索
    auto ch_it = ch_map_.begin();                                               ///< チャンネルマップの先頭
    // 音符ベクタ
    for (const auto& note : ch_it->second.note_vector) {                        // 音符ベクタループ
      if ("L" == note.str) {                                                    // 音符情報がループなら
        ++last_l_pos;                                                           // Lコマンド最終位置を更新
      }
    }
  }
  if (0 == last_l_pos) {                                                        // Lコマンドが無ければ
    return;                                                                     // 終わり
  }
  // 最終Lコマンド以外を削除
  for (auto& ch_entry : ch_map_) {                                              // チャンネルマップループ
    int l_pos = 0;                                                              ///< Lコマンド位置
    for (auto note_it  = ch_entry.second.note_vector.begin();                   // 音符ベクタループ
              note_it != ch_entry.second.note_vector.end();) {                  // ここではイテレータを進めない
      NoteInfo& note = *note_it;                                                ///< 音符情報を参照
      if ("L" == note.str) {                                                    // 音符情報がLコマンドなら
        ++l_pos;                                                                // Lコマンド位置更新
        if (last_l_pos == l_pos) {                                              // Lコマンド最終位置でないなら
          break;                                                                // 音符ベクタループから抜ける
        }
        note_it = ch_entry.second.note_vector.erase(note_it);                   // このLコマンドを削除
      } else {                                                                  // Lコマンド以外なら
        ++note_it;
      }
    }
  }
}
// セーブ ----------------------------------------------------------
/** @brief MMLファイルへ保存する */
extern const char kToolName[];                                                  // ツール名
extern const char kAuthor[];                                                    // 著作者
int Mml::Save(                                                                  // MMLファイルへ保存する
    const string& file_path,                                                    ///< (i)UTF-8のファイルパス
    uint8_t one_line_bar) const                                                 ///< (i)一行に出力する小節数
{
  // UTF-8由来の文字列をMIDIテキストに合わせて変換
  const auto tool_name = EncodeUtf8ForMml(kToolName, midi_text_encoding_);      ///< 出力用ツール名
  const auto file_name = EncodeUtf8ForMml(file_name_, midi_text_encoding_);     ///< 出力用MIDIファイル名
  const auto progamer = EncodeUtf8ForMml(programmer_, midi_text_encoding_);     ///< 出力用打ち込み者名
  if (!tool_name || !file_name || !progamer) {                                  // UTF-8由来文字列を変換できなければ
    cerr << "MMLへ出力する文字列の文字コードを変換できませんでした。\n";
    return -1;
  }
  const string& title = title_.empty() ? *file_name : title_;                   ///< 出力用タイトル
  // ファイルオープン
  ofstream ofs(file_path, ios::out);                                            ///< 出力ファイルストリーム
  if (!ofs) {
    cerr << "MMLファイルの出力先を開けませんでした。\n";
    return -1;
  }
  // タイトル情報出力
  ofs << "// " << *tool_name << " " << kAuthor << '\n';
  ofs << "//  mid2mml " << *file_name << " -c:" << ch_str_ << '\n';             // コマンド表示
  ofs << "#TITLE\t\t" << title << '\n';                                         // タイトル情報
  ofs << "#COMPOSER\t" << composer_ << '\n';                                    // 作曲者名
  ofs << "#MAKER\t\t" << maker_ << '\n';                                        // 原著作者名
  ofs << "#PROGRAMER\t" << *tool_name << " & " << *progamer << '\n';            // 打ち込み者名
  // 固定宣言
  ofs << "#AUTO-BANKSWITCH 0\n";                                                // 自動バンク設定
  ofs << "#PITCH-CORRECTION\n";                                                 // ピッチエンヴェロープ、LFO、ディチューン方向整順化
  // 音源別使用宣言
  PutChUseDef(ofs, "C", "#GATE-DENOM 16");                                      // 三角波使用時はクオンタイズ分母指定
  PutChUseDef(ofs, "E", "#DPCM-RESTSTOP");                                      // DPCM用宣言
  PutChUseDef(ofs, "F", "#EX-DISKFM");                                          // FDS 音源使用宣言
  PutChUseDef(ofs, "GHIJKL", "#EX-VRC7");                                       // VRC7音源使用宣言
  PutChUseDef(ofs, "MNO", "#EX-VRC6");                                          // VRC6音源使用宣言
  PutChUseDef(ofs, "PQRSTUVW", "#EX-NAMCO106 8");                               // N106音源使用宣言
  PutChUseDef(ofs, "XYZ", "#EX-FME7");                                          // FME7音源使用宣言
  PutChUseDef(ofs, "ab", "#EX-MMC5");                                           // MMC5音源使用宣言
  // 音質定義出力
  tone_.PutToneDef(ofs, tone_.color_def_abab_mn_);                              // 音色：矩形波
  tone_.PutToneDef(ofs, tone_.color_def_e_);                                    // 音色：DPCM
  tone_.PutToneDef(ofs, tone_.color_def_f_);                                    // 音色：FDS
  tone_.PutToneDef(ofs, tone_.color_def_ghijkl_);                               // 音色：VRC7
  tone_.PutToneDef(ofs, tone_.color_def_pqrstuvw_);                             // 音色：N106
  tone_.PutToneDef(ofs, tone_.lfo_def_);                                        // LFO
  if (VolumeMode::kToneBased & tone_.volume_mode_) {                            // 音色音量モードなら
    tone_.PutVolumeDef(ofs, tone_.volume_def_);                                 // 音量
  }
  // ピッチエンヴェロープ定義出力
  pitch_.PutDef(ofs);                                                           // ピッチエンヴェロープ定義出力
  ofs << '\n';                                                                  // 空行
  // 最初のテンポ出力
  if (first_tempo_ && (120 != first_tempo_)) {                                  // テンポが取得できており、最初のテンポが120以外なら
    string ch_str;                                                              ///< チャンネル文字列
    for (const auto& ch_entry : ch_map_) {                                      // チャンネルマップループ
      string str(" ");                                                          ///< 文字列
      str[0] = ch_entry.first;                                                  // チャンネルを文字列にセット
      ch_str += str;                                                            // チャンネル文字列に追加
    }
    ofs << ch_str << "\tt" << first_tempo_ << '\n';                             // テンポを出力
  }
  // チャンネル先頭のコマンド出力
  for (const auto& ch_entry : ch_map_) {                                        // チャンネルマップループ
    string ch_str("? ");                                                        ///< 編集用チャンネル文字
    ch_str[0] = ch_entry.first;                                                 // チャンネルをセット
    ofs << ch_str;                                                              // チャンネルを出力
    // デフォルトレングス
    if (4 != ch_entry.second.default_len) {                                     // デフォルトレングスが４以外なら
      ofs << "l" << static_cast<uint16_t>(ch_entry.second.default_len);         // 'l'コマンド出力
    } else {
      ofs << "\t";
    }
    ofs << "\t";
    // オクターブ
    if (('D' != ch_str[0]) && ('E' != ch_str[0])) {                             // パーカッション以外
      ofs << "o" << static_cast<uint16_t>(ch_entry.second.first_octave);        // 'o'コマンド出力
    }
    ofs << "\t";
    // 音量
    if (('C' != ch_str[0]) && ('E' != ch_str[0])) {                             // 音量のあるチャンネル
      if (VolumeMode::kToneBased & tone_.volume_mode_) {                        // 音色音量モードなら
        ofs << tone_.GetVolume(ch_str[0], ch_entry.second.first_prg_no,         // '@v'コマンド出力
                               static_cast<uint16_t>(ch_entry.second.first_volume)) << " ";
      } else {                                                                  // 音色音量モードでないなら
        ofs << "v" << static_cast<uint16_t>(ch_entry.second.first_volume);      // 'v'コマンド出力
      }
    } else {
      ofs << "\t";
    }
    // 音色
    if (('G' <= ch_str[0]) && ('L' >= ch_str[0])) {                             // VRC7の場合
      string cmd = tone_.Get(ch_str[0], ch_entry.second.first_prg_no);          ///< 音色コマンド
      if (!cmd.empty()) {                                                       // ユーザー音色が登録されていたら
        ofs << "\t@@0";                                                         // ユーザー音色を出力
        // if ( 'G' == ChStr[0] ) {                                             // Gチャンネルなら
        ofs << cmd;                                                             // 更にOPコマンドを出力
        // }
      } else {                                                                  // ユーザー音色が登録されてなければ
        ofs << "\t@@" << kToneDefVrc7Preset[ch_entry.second.first_prg_no];      // プリセット音色を出力
      }
    } else if ('D' == ch_str[0]) {                                              // ノイズの場合
      ofs << "\t";
      if ("D255" != tone_.Get(ch_str[0], ch_entry.second.first_prg_no)) {       // 音色が"D255"以外なら
        ofs << tone_.Get(ch_str[0], ch_entry.second.first_prg_no);              // 音色を出力
      }
    } else {                                                                    // その他チャンネルの場合
      ofs << "\t" << tone_.Get(ch_str[0], ch_entry.second.first_prg_no);        // 音色を出力
    }
    // ピッチシフト量
    if (('P' <= ch_str[0]) && ('W' >= ch_str[0])) {                             // N106の場合
      ofs << "\tSA7";                                                           // ピッチシフト量設定
    }
    // クォンタイズ
    if ('C' == ch_str[0]) {                                                     // 三角波の場合
      ofs << "\tq15";                                                           // クォンタイズ設定
    }
    ofs << '\n';
  }
  // コマンド出力
  const uint16_t whole_note = time_base_ << 2;                                  ///< 全音符の長さ
  for (const auto& ch_entry : ch_map_) {                                        // チャンネルマップループ
    double rhythm_time(1);                                                      ///< 拍子(全音符に対する一小節の割合 4/4=1 3/4=0.75)
    uint32_t one_line_time = static_cast<uint32_t>(whole_note * rhythm_time * one_line_bar); ///< 1行分の出力時間 一行の時間（全音符の時間×拍子×一行に出力する小節数）
    uint32_t line_time(0);                                                      ///< 行内の時間
    uint32_t tempo(first_tempo_);                                               ///< テンポ
    uint8_t octave(ch_entry.second.first_octave);                               ///< オクターブ
    uint8_t volume(ch_entry.second.first_volume);                               ///< 音量
    uint8_t prg_no(ch_entry.second.first_prg_no);                               ///< プログラム番号
    uint8_t loop_prg_no(255);                                                   ///< ループ時点のプログラム番号
    uint8_t loop_volume(255);                                                   ///< ループ時点の音量
    string comment;                                                             ///< コメント文字列
    string ch_str("? ");                                                        ///< 編集用チャンネル文字
    ch_str[0] = ch_entry.first;                                                 // チャンネルをセット
    ofs << '\n' << ch_str;                                                      // チャンネルを出力
    for (const auto& note : ch_entry.second.note_vector) {                      ///< 音符ベクタループ
      switch (note.str[0]) {                                                    // 音符情報の先頭１文字を判定
      case 'c':                                                                 // ド
      case 'd':                                                                 // レ
      case 'e':                                                                 // ミ
      case 'f':                                                                 // ファ
      case 'g':                                                                 // ソ
      case 'a':                                                                 // ラ
      case 'b':                                                                 // シ
      case 'n':                                                                 // 音符番号指定
        // 音量
        if (('C' != ch_str[0]) && ('E' != ch_str[0])) {                         // 音量のあるチャンネル
          if (VolumeMode::kVariable == tone_.volume_mode_) {                    // 可変音量モードで
            if (note.vol < volume) {                                            // 音量が小さければ
              ofs << "v-";                                                      // 下げる
              if ((volume - note.vol) > 1) {                                    // 差が１より多ければ
                ofs << static_cast<uint16_t>(volume - note.vol);                // 差分を数値で出力
              }
              volume = note.vol;                                                // 音量を更新
            } else if (note.vol > volume) {                                     // 音量が大きければ
              ofs << "v+";                                                      // 上げる
              if ((note.vol - volume) > 1) {                                    // 差が１より多ければ
                ofs << static_cast<uint16_t>(note.vol - volume);                // 差分を数値で出力
              }
              volume = note.vol;                                                // 音量を更新
            }
          } else if (VolumeMode::kToneAndVariable == tone_.volume_mode_) {      // 音色可変音量モードで
            if (volume != note.vol) {                                           // 音量が変わったら、または音色変更直後なら
              volume = note.vol;                                                // 音量を更新
              ofs << tone_.GetVolume(ch_str[0], prg_no, volume);                // '@v'コマンド出力
            }
          } else if (VolumeMode::kToneBased == tone_.volume_mode_) {            // 音色音量モードで
            if (volume == 255) {                                                // 音色変更直後なら
              volume = note.vol;                                                // 音量を更新
              ofs << tone_.GetVolume(ch_str[0], prg_no, volume);                // '@v'コマンド出力
            }
          }
        }
        // オクターブ
        if (('D' != ch_str[0]) && ('E' != ch_str[0])) {                         // パーカッション以外
          if (note.oct < octave) {                                              // オクターブが低ければ
            for (; note.oct < octave; --octave) {                               // 同じになるまで
              ofs << "<";                                                       // 下げる
            }
          } else if (note.oct > octave) {                                       // オクターブが高ければ
            for (; note.oct > octave; ++octave) {                               // 同じになるまで
              ofs << ">";                                                       // 上げる
            }
          }
        }
        [[fallthrough]];                                                        // 音符・休符の出力処理へ続行

      case 'r':                                                                 // 通常休符
      case 'w':                                                                 // DPCM休符
        /// 残りの音符または休符の長さ
        for (uint32_t full_len = note.len; 0 < full_len;) {                     // 音符全体長が残っている限りループ
          // 音符長を決める
          /// 音符または休符の長さ
          uint32_t note_len = (full_len > whole_note) ? whole_note : full_len;  // 音符長は、音符全体長が全音符より長ければ、全音符の長さ、そうでないなら音符全体長とする
          uint32_t blank_len = one_line_time - line_time;                       ///< 改行までの時間を求める
          if (note_len > blank_len) {                                           // 音符長が改行までの時間を超えたら
            note_len = blank_len;                                               // 音符長を改行までの時間とする
          }
          full_len -= note_len;                                                 // 音符全体長から音符長を引く
          line_time += note_len;                                                // 行内の時間に音符長を足す
          // 音符を出力する
          uint16_t note_top(1);                                                 ///< 音符の先頭フラグ
          uint16_t repeat(0);                                                   ///< 連続フラグ
          uint16_t par_len;                                                     ///< 分割長
          bool n_comma_output(false);                                           ///< 'n'コマンドの','出力済みフラグ
          bool note_output(false);                                              ///< 音符を出力したかどうか（&出力判定用）
          // ↑音符が短すぎて出力されない時でも'&'を出力してしまう対策
          /// 音符長を分割する位置
          for (uint16_t sh = 0; (par_len = whole_note >> sh) || note_len; ++sh) { // 全音符を分割してゆく
            if (par_len > note_len) {                                           // この分音符を含まないなら
              repeat = 0;                                                       // 連続しないをセット
              continue;                                                         // 次へ
            }
            if (note_top) {                                                     // 音符の先頭なら
              ofs << note.str;                                                  // 音符の先頭を出力
              uint16_t len(1 << sh);                                            ///< 音長を求める
              if (ch_entry.second.default_len != len) {                         // 音長がデフォルトレングスでなければ
                if (('n' == note.str[0]) && !n_comma_output) {                  // 'n'コマンドで、','未出力なら
                  ofs << ",";                                                   // ','を出力
                  n_comma_output = true;                                        // ','出力済み
                }
                ofs << len;                                                     // 音長を出力
              }
              note_top = 0;                                                     // 音符の続きをセット
              repeat = 1;                                                       // 連続フラグを立てる
              note_output = true;                                               // 音符出力済み
            } else if (repeat) {                                                // 連続するなら
              if (('n' == note.str[0]) && !n_comma_output) {                    // 'n'コマンドで、','未出力なら
                ofs << ",";                                                     // ','を出力
                n_comma_output = true;                                          // ','出力済み
              }
              ofs << ".";                                                       // 付点音符
            } else {
              if (('n' == note.str[0]) && !n_comma_output) {                    // 'n'コマンドで、','未出力なら
                ofs << ",";                                                     // ','を出力
                n_comma_output = true;                                          // ','出力済み
              }
              ofs << "^" << (1 << sh);                                          // 音符の続きを出力
            }
            note_len -= par_len;                                                // 音符の残り長さを求める
          }
          if (full_len && note_output && ('r' != note.str[0]) && ('w' != note.str[0])) { // 音符に続きがあり、音符出力済みで休符以外なら
            ofs << "&";                                                         // タイを出力
          }
          // 改行判定
          if (line_time >= one_line_time) {                                     // 改行位置に来たら
            if (!comment.empty()) {                                             // コメントがあるなら
              ofs << "\t" << comment;                                           // コメント出力
              comment = "";                                                     // コメントをクリア
            }
            ofs << '\n' << ch_str;                                              // 改行、チャンネル文字を出力
            line_time = 0;                                                      // 行内の時間を０に戻す
          }
        }
        break;

      case 'L':                                                                 // 繰り返し
        ofs << note.str;                                                        // そのまま出力
        // プログラム番号出力のための前準備
        loop_prg_no = prg_no;                                                   // ループ時点の音色を記録する
        loop_volume = volume;                                                   // ループ時点の音量を記録する
        break;

      case '$':                                                                 // プログラム番号（内部表現）
        if (prg_no != note.oct) {                                               // プログラム番号が変わったら
          prg_no = note.oct;                                                    // プログラム番号を更新
          if (('G' <= ch_str[0]) && ('L' >= ch_str[0])) {                       // VRC7の場合
            string cmd = tone_.Get(ch_str[0], prg_no);                          ///< ユーザー音色取得
            if (!cmd.empty()) {                                                 // ユーザー音色が登録されていたら
              ofs << "@@0";                                                     // ユーザー音色を出力
              if ('G' == ch_str[0]) {                                           // Gチャンネルなら
                ofs << cmd;                                                     // 更にOPコマンドを出力
              }
            } else {                                                            // ユーザー音色が登録されてなければ
              ofs << "@@" << kToneDefVrc7Preset[prg_no];                        // プリセット音色を出力
            }
          } else {                                                              // その他チャンネルの場合
            ofs << tone_.Get(ch_str[0], prg_no);                                // 音色コマンドを出力する
          }

          if (VolumeMode::kToneBased & tone_.volume_mode_) {                    // 音色音量モードなら
            volume = 255;                                                       // 次の音符で音量コマンドを出力する設定とする
          }
        }
        break;

      case '/':                                                                 // コメント
        comment += note.str;                                                    // コメントをセット
        break;

      case '!':                                                                 // 拍子（内部表現）
        if (line_time) {                                                        // この行に音符があれば
          ofs << '\n' << ch_str;                                                // 改行しチャンネル文字を出力
        }
        rhythm_time = static_cast<double>(note.oct) / note.vol;                 // 拍子を計算
        one_line_time = static_cast<uint32_t>(whole_note * rhythm_time * one_line_bar); ///< 1行分の出力時間 一行の時間（全音符の時間×拍子×一行に出力する小節数）
        break;

      case 't':                                                                 // テンポ
        if (tempo != note.len) {                                                // テンポが変わったら
          ofs << note.str;                                                      // そのまま出力
          tempo = note.len;                                                     // テンポを更新
        }
        break;

      default:                                                                  // その他のコマンド
        ofs << note.str;                                                        // そのまま出力
        break;
      }
    }
    // 曲の最後

    // ループ時の音色出力
    if ((loop_prg_no != 255) && (loop_prg_no != prg_no)) {                      // プログラム番号がループ時点のプログラム番号と一致しなければ
      prg_no = loop_prg_no;                                                     // プログラム番号をループ時点のプログラム番号に更新
      if (('G' <= ch_str[0]) && ('L' >= ch_str[0])) {                           // VRC7の場合
        string cmd = tone_.Get(ch_str[0], prg_no);                              ///< ユーザー音色取得
        if (!cmd.empty()) {                                                     // ユーザー音色が登録されていたら
          ofs << "@@0";                                                         // ユーザー音色を出力
          if ('G' == ch_str[0]) {                                               // Gチャンネルなら
            ofs << cmd;                                                         // 更にOPコマンドを出力
          }
        } else {                                                                // ユーザー音色が登録されてなければ
          ofs << "@@" << kToneDefVrc7Preset[prg_no];                            // プリセット音色を出力
        }
      } else {                                                                  // その他チャンネルの場合
        ofs << tone_.Get(ch_str[0], prg_no);                                    // 音色コマンドを出力する
      }

      if (VolumeMode::kToneBased & tone_.volume_mode_) {                        // 音色音量モードなら
        volume = 255;                                                           // 次の音符で音量コマンドを出力する設定とする
      }
    }

    // ループ時の音量出力
    if ((loop_volume != 255) && (loop_volume != volume)) {                      // プログラム番号がループ時点のプログラム番号と一致しなければ
      if (VolumeMode::kToneBased & tone_.volume_mode_) {                        // 音色音量モードなら
        if (('C' != ch_str[0]) && ('E' != ch_str[0])) {                         // 音量のあるチャンネル
          if (VolumeMode::kVariable == tone_.volume_mode_) {                    // 可変音量モードで
            if (loop_volume < volume) {                                         // 音量が小さければ
              ofs << "v-";                                                      // 下げる
              if ((volume - loop_volume) > 1) {                                 // 差が１より多ければ
                ofs << static_cast<uint16_t>(volume - loop_volume);             // 差分を数値で出力
              }
              volume = loop_volume;                                             // 音量を更新
            } else if (loop_volume > volume) {                                  // 音量が大きければ
              ofs << "v+";                                                      // 上げる
              if ((loop_volume - volume) > 1) {                                 // 差が１より多ければ
                ofs << static_cast<uint16_t>(loop_volume - volume);             // 差分を数値で出力
              }
              volume = loop_volume;                                             // 音量を更新
            }
          } else if (VolumeMode::kToneAndVariable == tone_.volume_mode_) {      // 音色可変音量モードで
            if (volume != loop_volume) {                                        // 音量が変わったら、または音色変更直後なら
              volume = loop_volume;                                             // 音量を更新
              ofs << tone_.GetVolume(ch_str[0], prg_no, volume);                // '@v'コマンド出力
            }
          } else if (VolumeMode::kToneBased == tone_.volume_mode_) {            // 音色音量モードで
            if (volume == 255) {                                                // 音色変更直後なら
              volume = loop_volume;                                             // 音量を更新
              ofs << tone_.GetVolume(ch_str[0], prg_no, volume);                // '@v'コマンド出力
            }
          }
        }
      }
    }

    ofs << '\n';                                                                // 改行
  }
  return 0;
}

/** @brief 使用する音源の宣言をストリームへ出力する */
void Mml::PutChUseDef(                                                          // 使用する音源の宣言をストリームへ出力する
    ostream& os,                                                                ///< (o)出力ストリーム
    const string& chk_ch,                                                       ///< (i)対象チャンネル文字列
    const string& def) const                                                    ///< (i)音源使用宣言
{
  for (const char ch : chk_ch) {                                                // 対象チャンネル文字列を先頭から順に処理
    if (ch_map_.find(static_cast<uint8_t>(ch)) != ch_map_.end()) {              // このチャンネルがあるなら
      os << def << '\n';                                                        // 音源使用宣言
      break;                                                                    // ループを抜ける
    }
  }
}

/** @brief 音色定義をストリームへ出力する */
void Mml::Tone::PutToneDef(                                                     // 音色定義をストリームへ出力する
    ostream& os,                                                                ///< (o)出力ストリーム
    const map<uint16_t, string>& def_map) const                                 ///< (i)音質定義マップ
{
  for (const auto& [Key, ToneDef] : def_map) {                                  // 定義マップループ
    os << ToneDef;                                                              // 定義を出力
  }
  os.flush();
}

/** @brief 音量定義をストリームへ出力する */
void Mml::Tone::PutVolumeDef(                                                   // 音量定義をストリームへ出力する
    ostream& os,                                                                ///< (o)出力ストリーム
    const map<uint16_t, map<uint16_t, VolumeInfo>>& def_map) const              ///< (i)音量定義マップ
{
  for (const auto& [Definition, VolMap] : def_map) {                            // 音量定義マップループ
    for (const auto& [Volume, VolInfo] : VolMap) {                              // 音量マップループ
      uint16_t prg_no = (0x7F & Definition);                                    ///< プログラム番号を取り出す
      os << "@v" << VolInfo.serial_no                                           // 定義を出力
         << " \t={" << VolInfo.define << "}\t\t\t\t\t// Ch." << VolInfo.ch
         << " Vol." << Volume << "\tPrgNo." << prg_no;
      if (('D' == VolInfo.ch) || ('E' == VolInfo.ch)) {
        os << "\t" << kDrumName[prg_no];
      } else {
        os << "\t" << kToneName[prg_no];
      }
      os << '\n';
    }
  }
}

/** @brief ピッチエンベロープ定義をストリームへ出力する */
void Mml::Pitch::PutDef(                                                        // ピッチエンベロープ定義をストリームへ出力する
    ostream& os) const                                                          ///< (o)出力ストリーム
{
  size_t reg_no = 0;                                                            ///< エンベロープ定義番号
  for (const auto& definition : envelope_def_) {                                // エンベロープ定義ループ
    os << "@EP" << reg_no++ << " \t={" << definition << "0}\n";                 // エンベロープ定義先頭編集を出力
  }
}
