#include "text_encoding.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>

#ifdef _WIN32                                                                   // Windows
#include <Windows.h>
#else                                                                           // Linux・MacOS
#include <iconv.h>
#endif

using std::any_of, std::min, std::nullopt, std::optional;
using std::size_t, std::string, std::string_view, std::wstring;

namespace {

/** @brief 文字列がUTF-8として有効か判定する */
bool IsValidUtf8(                                                               // 文字列がUTF-8として有効か判定する
    string_view text) noexcept {                                                ///< (i)判定対象の文字列
  const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());      ///< 判定対象のバイト列
  size_t length{};                                                              ///< UTF-8文字のバイト数
  for (size_t i = 0; i < text.size(); i += length) {                            // 全バイト判定ループ
    length = 0;
    const unsigned char first = bytes[i];                                       ///< UTF-8文字の先頭バイト
    unsigned int code_point{};                                                  ///< Unicodeコードポイント
    if (first <= 0x7F) {                                                        // ASCIIなら
      length = 1;
      code_point = first;
    } else if ((first >= 0xC2) && (first <= 0xDF)) {                            // 2バイト文字なら
      length = 2;
      code_point = first & 0x1F;
    } else if ((first >= 0xE0) && (first <= 0xEF)) {                            // 3バイト文字なら
      length = 3;
      code_point = first & 0x0F;
    } else if ((first >= 0xF0) && (first <= 0xF4)) {                            // 4バイト文字なら
      length = 4;
      code_point = first & 0x07;
    } else {
      return false;
    }
    if (i + length > text.size()) {                                             // 後続バイトが不足するなら
      return false;
    }
    for (size_t j = 1; j < length; ++j) {                                       // 後続バイトループ
      if ((bytes[i + j] & 0xC0) != 0x80) {                                      // UTF-8の後続バイトでなければ
        return false;
      }
      code_point = (code_point << 6) | (bytes[i + j] & 0x3F);
    }
    if (((length == 2) && (code_point < 0x80)) ||                               // 過剰長表現なら
        ((length == 3) && (code_point < 0x800)) ||
        ((length == 4) && (code_point < 0x10000)) ||
        ((code_point >= 0xD800) && (code_point <= 0xDFFF)) ||                   // UTF-16サロゲートなら
        (code_point > 0x10FFFF)) {                                              // Unicode範囲外なら
      return false;
    }
  }
  return true;
}

/** @brief 文字列がShift-JISとして有効か判定する */
bool IsValidShiftJis(                                                           // 文字列がShift-JISとして有効か判定する
    string_view text) noexcept {                                                ///< (i)判定対象の文字列
  const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());      ///< 判定対象のバイト列
  for (size_t i = 0; i < text.size(); ++i) {                                    // 全バイト判定ループ
    const unsigned char first = bytes[i];                                       ///< Shift-JIS文字の先頭バイト
    if ((first <= 0x7F) || ((first >= 0xA1) && (first <= 0xDF))) {              // 1バイト文字なら
      continue;
    }
    if (!(((first >= 0x81) && (first <= 0x9F)) ||                               // 2バイト文字の先頭でなければ
          ((first >= 0xE0) && (first <= 0xFC))) ||
        (++i >= text.size())) {                                                 // 後続バイトがなければ
      return false;
    }
    const unsigned char second = bytes[i];                                      ///< Shift-JIS文字の後続バイト
    if (!(((second >= 0x40) && (second <= 0x7E)) ||                             // 有効な後続バイトでなければ
          ((second >= 0x80) && (second <= 0xFC)))) {
      return false;
    }
  }
  return true;
}

#ifdef _WIN32                                                                   // Windows
/** @brief Shift-JIS文字列をUTF-8文字列へ変換する */
optional<string> ShiftJisToUtf8(                                                // Shift-JIS文字列をUTF-8文字列へ変換する
    string_view text) {                                                         ///< (i)Shift-JIS文字列
  if (text.empty()) {                                                           // 空文字列なら
    return string{};
  }
  const int text_size = static_cast<int>(text.size());                          ///< UTF-8変換前のバイト数
  const int wide_size = MultiByteToWideChar(932, MB_ERR_INVALID_CHARS,          ///< CP932からUTF-16の文字数を取得
      text.data(), text_size, nullptr, 0);
  if (wide_size <= 0) {                                                         // UTF-16へ変換できなければ
    return nullopt;
  }
  wstring wide_text(static_cast<size_t>(wide_size), L'\0');                     ///< UTF-16文字列
  if (MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, text.data(), text_size,
      wide_text.data(), wide_size) <= 0) {                                      // UTF-16への変換に失敗したら
    return nullopt;
  }
  const int utf8_size = WideCharToMultiByte(CP_UTF8, 0,                         ///< UTF-8変換後のバイト数を取得
      wide_text.data(), wide_size, nullptr, 0, nullptr, nullptr);
  if (utf8_size <= 0) {                                                         // UTF-8へ変換できなければ
    return nullopt;
  }
  string result(static_cast<size_t>(utf8_size), '\0');                          ///< UTF-8変換結果
  if (WideCharToMultiByte(CP_UTF8, 0, wide_text.data(), wide_size,
      result.data(), utf8_size, nullptr, nullptr) <= 0) {                       // UTF-8への変換に失敗したら
    return nullopt;
  }
  return result;
}

/** @brief UTF-8文字列をShift-JIS文字列へ変換する */
optional<string> Utf8ToShiftJis(                                                // UTF-8文字列をShift-JIS文字列へ変換する
    string_view text) {                                                         ///< (i)UTF-8文字列
  if (text.empty()) {                                                           // 空文字列なら
    return string{};
  }
  const int text_size = static_cast<int>(text.size());                          ///< UTF-8文字列のバイト数
  const int wide_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,      // UTF-16変換後の文字数を取得
      text.data(), text_size, nullptr, 0);
  if (wide_size <= 0) {                                                         // UTF-16へ変換できなければ
    return nullopt;
  }
  wstring wide_text(static_cast<size_t>(wide_size), L'\0');                     ///< UTF-16文字列
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), text_size,
      wide_text.data(), wide_size) <= 0) {                                      // UTF-16への変換に失敗したら
    return nullopt;
  }
  BOOL used_default_character = FALSE;                                          ///< 代替文字を使用したか
  const int shift_jis_size = WideCharToMultiByte(932, WC_NO_BEST_FIT_CHARS,     // Shift-JIS変換後のバイト数を取得
      wide_text.data(), wide_size, nullptr, 0, "?", &used_default_character);
  if (shift_jis_size <= 0) {                                                    // Shift-JISへ変換できなければ
    return nullopt;
  }
  string result(static_cast<size_t>(shift_jis_size), '\0');                     ///< Shift-JIS変換結果
  if (WideCharToMultiByte(932, WC_NO_BEST_FIT_CHARS, wide_text.data(), wide_size,
      result.data(), shift_jis_size, "?", &used_default_character) <= 0) {      // Shift-JISへの変換に失敗したら
    return nullopt;
  }
  return result;
}
#else                                                                           // Linux・MacOS
/** @brief Shift-JIS文字列をUTF-8文字列へ変換する */
optional<string> ShiftJisToUtf8(                                                // Shift-JIS文字列をUTF-8文字列へ変換する
    string_view text) {                                                         ///< (i)Shift-JIS文字列
  iconv_t converter = iconv_open("UTF-8", "CP932");                             ///< 文字コード変換器
  if (converter == reinterpret_cast<iconv_t>(-1)) {                             // CP932変換器を作成できなければ
    converter = iconv_open("UTF-8", "SHIFT-JIS");
  }
  if (converter == reinterpret_cast<iconv_t>(-1)) {                             // 変換器を作成できなければ
    return nullopt;
  }
  string result(text.size() * 4 + 1, '\0');                                     ///< 1入力バイトあたり最大4バイトのUTF-8出力領域
  char* input = const_cast<char*>(text.data());                                 ///< iconvへ渡す未変換部分の先頭
  size_t input_size = text.size();                                              ///< 未変換部分のバイト数
  char* output = result.data();                                                 ///< 変換結果の書き込み位置
  size_t output_size = result.size();                                           ///< 変換結果の空きバイト数
  const size_t status = iconv(                                                  ///< CP932の文字列をUTF-8へ変換する
      converter, &input, &input_size, &output, &output_size);
  iconv_close(converter);
  if (status == static_cast<size_t>(-1)) {                                      // 未定義文字などで変換に失敗したら
    return nullopt;
  }
  result.resize(result.size() - output_size);
  return result;
}

/** @brief UTF-8文字列をShift-JIS文字列へ変換する */
optional<string> Utf8ToShiftJis(                                                // UTF-8文字列をShift-JIS文字列へ変換する
    string_view text) {                                                         ///< (i)UTF-8文字列
  iconv_t converter = iconv_open("CP932", "UTF-8");                             ///< 文字コード変換器
  if (converter == reinterpret_cast<iconv_t>(-1)) {                             // CP932変換器を作成できなければ
    converter = iconv_open("SHIFT-JIS", "UTF-8");
  }
  if (converter == reinterpret_cast<iconv_t>(-1)) {                             // 変換器を作成できなければ
    return nullopt;
  }
  string result(text.size() * 2 + 16, '\0');                                    ///< Shift-JIS変換結果
  const char* input = text.data();                                              ///< 未変換部分の先頭
  size_t input_size = text.size();                                              ///< 未変換部分のバイト数
  char* output = result.data();                                                 ///< 変換結果の書き込み位置
  size_t output_size = result.size();                                           ///< 変換結果の空きバイト数
  while (input_size > 0) {                                                      // 全入力変換ループ
    char* mutable_input = const_cast<char*>(input);                             ///< iconvへ渡す入力位置
    if (iconv(converter, &mutable_input, &input_size, &output, &output_size) !=
        static_cast<size_t>(-1)) {                                              // 変換に成功したら
      input = mutable_input;
      continue;
    }
    input = mutable_input;
    if ((errno == EILSEQ) || (errno == EINVAL)) {                               // Shift-JISで表現できない文字なら
      if (output_size == 0) {                                                   // 代替文字の出力領域がなければ
        const size_t used_size = static_cast<size_t>(output - result.data());   ///< 変換済みバイト数
        result.resize(result.size() * 2);
        output = result.data() + used_size;
        output_size = result.size() - used_size;
      }
      *output++ = '?';
      --output_size;
      const unsigned char first = static_cast<unsigned char>(*input);           ///< 変換できなかったUTF-8文字の先頭バイト
      size_t skip_size = 1;                                                     ///< 読み飛ばすUTF-8文字のバイト数
      if ((first >= 0xC2) && (first <= 0xDF)) {
        skip_size = 2;
      } else if ((first >= 0xE0) && (first <= 0xEF)) {
        skip_size = 3;
      } else if ((first >= 0xF0) && (first <= 0xF4)) {
        skip_size = 4;
      }
      skip_size = min(skip_size, input_size);
      input += skip_size;
      input_size -= skip_size;
      iconv(converter, nullptr, nullptr, nullptr, nullptr);                     // 変換器の内部状態を初期化
      continue;
    }
    if (errno != E2BIG) {                                                       // 出力領域不足以外なら
      iconv_close(converter);
      return nullopt;
    }
    const size_t used_size = static_cast<size_t>(output - result.data());       ///< 変換済みバイト数
    result.resize(result.size() * 2);
    output = result.data() + used_size;
    output_size = result.size() - used_size;
  }
  const size_t converted_size = static_cast<size_t>(output - result.data());    ///< 変換後のバイト数
  iconv_close(converter);
  result.resize(converted_size);
  return result;
}
#endif

}                                                                               // namespace

/** @brief 判別対象のMIDIテキストを追加する */
void TextEncodingDetector::Add(                                                 // 判別対象のMIDIテキストを追加する
    string_view text) noexcept {                                                ///< (i)判別対象のMIDIテキスト
  if (text.empty()) {                                                           // 空文字列なら
    return;
  }
  has_non_ascii_ = has_non_ascii_ || any_of(
      text.begin(), text.end(), [](unsigned char value) { return value >= 0x80; });
  utf8_valid_ = utf8_valid_ && IsValidUtf8(text);
  shift_jis_valid_ = shift_jis_valid_ && IsValidShiftJis(text);
}

/** @brief 追加されたMIDIテキストの文字コードを返す */
optional<TextEncoding> TextEncodingDetector::Detect() const noexcept {          // 追加されたMIDIテキストの文字コードを返す
  if (!has_non_ascii_) {                                                        // ASCII以外のバイトがなければ
    return TextEncoding::kAscii;
  }
  if (shift_jis_valid_) {                                                       // 全テキストがShift-JISとして有効なら
    return TextEncoding::kShiftJis;
  }
  if (utf8_valid_) {                                                            // 全テキストがUTF-8として有効なら
    return TextEncoding::kUtf8;
  }
  return nullopt;
}

/** @brief MIDIテキストをその場で判別し、画面表示用UTF-8へ変換する */
string EncodeMidiTextForConsole(                                                // MIDIテキストをその場で判別し、画面表示用UTF-8へ変換する
    string_view text) {                                                         ///< (i)元の文字コードのMIDIテキスト
  TextEncodingDetector detector;                                                ///< 表示対象の文字コード判別器
  detector.Add(text);
  const auto encoding = detector.Detect();                                      ///< Shift-JIS優先の判別結果
  if (!encoding) {                                                              // 文字コードを判別できなければ
    return "[文字コードを判別できないため、表示を省略しました。]";
  }
  if (encoding != TextEncoding::kShiftJis) {                                    // ASCIIまたはUTF-8なら
    return string{text};
  }
  const auto converted = ShiftJisToUtf8(text);                                  ///< 表示用のコピーだけをUTF-8へ変換する
  return converted.value_or("[文字コードを変換できないため、表示を省略しました。]");
}

/** @brief UTF-8文字列をMMLの出力文字コードへ変換する */
optional<string> EncodeUtf8ForMml(                                              // UTF-8文字列をMMLの出力文字コードへ変換する
    string_view text,                                                           ///< (i)UTF-8文字列
    const optional<TextEncoding>& midi_text_encoding) {                         ///< (i)MIDI由来テキストの文字コード
  if (midi_text_encoding != TextEncoding::kShiftJis) {                          // MIDIテキストがShift-JISでなければ
    return string{text};
  }
  return Utf8ToShiftJis(text);
}

#ifdef _WIN32                                                                   // Windows
/** @brief 出力コードページをUTF-8へ変更する */
ConsoleOutputCodePageGuard::ConsoleOutputCodePageGuard() noexcept               // 出力コードページをUTF-8へ変更する
    : original_code_page_(GetConsoleOutputCP()) {
  if ((original_code_page_ != 0) && (original_code_page_ != CP_UTF8)) {         // UTF-8以外のコンソールなら
    changed_ = SetConsoleOutputCP(CP_UTF8) != FALSE;
  }
}

/** @brief 変更した出力コードページを元に戻す */
ConsoleOutputCodePageGuard::~ConsoleOutputCodePageGuard() noexcept {            // 変更した出力コードページを元に戻す
  if (changed_) {                                                               // 出力コードページを変更していたら
    SetConsoleOutputCP(original_code_page_);
  }
}

#endif
