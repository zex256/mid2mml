#pragma once
#include <array>
#include <string>

using std::array, std::string;

/** @brief MIDIプログラム番号に対応する音色名 */
extern const array<string, 128> kToneName;
/** @brief MIDIドラム番号に対応するドラム名 */
extern const array<string, 128> kDrumName;
/** @brief ノイズ音源の音色定義 */
extern const array<string, 128> kToneDefNoise;
/** @brief 矩形波音源の音色定義 */
extern const array<string, 128> kToneDefSquare;
/** @brief VRC6音源の音色定義 */
extern const array<string, 128> kToneDefVrc6;
/** @brief VRC7ユーザー音色の定義 */
extern const array<string, 128> kToneDefVrc7User;
/** @brief VRC7プリセット音色の番号 */
extern const array<string, 128> kToneDefVrc7Preset;
/** @brief N106音源の音色定義 */
extern const array<string, 128> kToneDefN106;
/** @brief FDS音源の音色定義 */
extern const array<string, 128> kToneDefFds;
/** @brief ノイズ音源の音量定義 */
extern const array<string, 128> kVolumeDefNoise;
/** @brief 共通音源の音量定義 */
extern const array<string, 128> kVolumeDefCommon;
/** @brief VRC7音源の音量定義 */
extern const array<string, 128> kVolumeDefVrc7;
