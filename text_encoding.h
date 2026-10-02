#pragma once

#include <optional>
#include <string>
#include <string_view>

using std::optional, std::string, std::string_view;

/** @brief 判別可能なMIDIテキストの文字コード */
enum class TextEncoding {
  kAscii,                                                                       ///< ASCII
  kShiftJis,                                                                    ///< Shift-JIS
  kUtf8,                                                                        ///< UTF-8
};

/** @brief 複数のMIDIテキストから文字コードを判別するクラス */
class TextEncodingDetector {
 public:
  /** @brief 判別対象のMIDIテキストを追加する */
  void Add(                                                                     // 判別対象のMIDIテキストを追加する
      string_view text) noexcept;                                               ///< (i)判別対象のMIDIテキスト

  /** @brief 追加されたMIDIテキストの文字コードを返す */
  [[nodiscard]] optional<TextEncoding> Detect() const noexcept;                 // 追加されたMIDIテキストの文字コードを返す

 private:
  bool has_non_ascii_{};                                                        ///< ASCII以外のバイトが存在するか
  bool utf8_valid_{true};                                                       ///< 全テキストがUTF-8として有効か
  bool shift_jis_valid_{true};                                                  ///< 全テキストがShift-JISとして有効か
};

/**
 * @brief UTF-8文字列をMMLの出力文字コードへ変換する
 * @param text (i)UTF-8文字列
 * @param midi_text_encoding (i)MIDI由来テキストの文字コード
 * @return 変換後の文字列、変換失敗時は空のoptional
 */
[[nodiscard]] optional<string> EncodeUtf8ForMml(                                // UTF-8文字列をMMLの出力文字コードへ変換する
    string_view text,                                                           ///< (i)UTF-8文字列
    const optional<TextEncoding>& midi_text_encoding);                          ///< (i)MIDI由来テキストの文字コード

#ifdef _WIN32                                                                   // Windows
/** @brief Windowsコンソールの出力コードページを一時的にUTF-8へ変更するクラス */
class ConsoleOutputCodePageGuard {
 public:
  /** @brief 出力コードページをUTF-8へ変更する */
  ConsoleOutputCodePageGuard() noexcept;                                        // 出力コードページをUTF-8へ変更する

  /** @brief 変更した出力コードページを元に戻す */
  ~ConsoleOutputCodePageGuard() noexcept;                                       // 変更した出力コードページを元に戻す

  /** @brief コピーコンストラクタを禁止する */
  ConsoleOutputCodePageGuard(                                                   // コピーコンストラクタを禁止する
      const ConsoleOutputCodePageGuard&) = delete;
  /** @brief コピー代入演算子を禁止する */
  ConsoleOutputCodePageGuard& operator=(                                        // コピー代入演算子を禁止する
      const ConsoleOutputCodePageGuard&) = delete;

 private:
  unsigned int original_code_page_{};                                           ///< 変更前の出力コードページ
  bool changed_{};                                                              ///< 出力コードページを変更したか
};

#endif
