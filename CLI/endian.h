#pragma once

#include <cstddef>
#include <type_traits>

using std::is_integral, std::make_unsigned, std::size_t;

/**
 * @brief 整数値のバイト順を反転する
 * @tparam T バイト順を反転する整数型
 * @param value (i)変換する値
 * @return バイト順を反転した値
 */
template <typename T>
T ByteSwap(                                                                     // 整数値のバイト順を反転する
    T value) noexcept {                                                         ///< (i)変換する値
  static_assert(is_integral<T>::value, "ByteSwap requires an integral type");
  static_assert(sizeof(T) > 1, "ByteSwap requires a multi-byte type");

  using Unsigned = typename make_unsigned<T>::type;                             ///< 符号なし作業型
  Unsigned input = static_cast<Unsigned>(value);                                ///< 変換前の値
  Unsigned output = 0;                                                          ///< バイト順を反転した値

  for (size_t index = 0; index < sizeof(T); ++index) {                          ///< 処理中のバイト位置
    output = static_cast<Unsigned>((output << 8) | (input & 0xFF));
    input >>= 8;
  }

  return static_cast<T>(output);
}

/**
 * @brief 整数値をその場でバイト順反転する
 * @tparam T バイト順を反転する整数型
 * @param value (io)変換対象の値
 */
template <typename T>
void ReverseEndian(                                                             // 整数値をその場でバイト順反転する
    T& value) noexcept {                                                        ///< (io)変換対象の値
  value = ByteSwap(value);
}

/**
 * @brief 整数値をエンディアン変換する
 * @tparam T エンディアン変換する整数型
 * @param value (i)変換する値
 * @return エンディアン変換後の値
 */
template <typename T>
T ConvertEndian(                                                                // 整数値をエンディアン変換する
    T value) noexcept {                                                         ///< (i)変換する値
  return ByteSwap(value);
}
