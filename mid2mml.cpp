#include <iostream>
#include <sstream>
#include "midi.h"
#include "mml.h"
using namespace std;
const char *TOOLNAME="mid2mml for ppmck Ver1.0β";			// ツール名
const char *AUTHER	="by ZEX";								// 著作者
int main ( int argc, char* argv[] )
{
	cerr << TOOLNAME << " " << AUTHER << endl;
	string	FilePath;										// ファイルパス
	string	ChStr( "GHIJKLABCPQRSTUVWabMNOXYZ" );			// チャンネル文字列
	WORD	Resolution( 32 );								// 解像度128分音符の場合TimeBase32
	DWORD	VolMode( Mml::VOL_COL_VAR );					// 音量モード
	WORD	VolDefThreshold( 60 );							// 音量登録数間引き閾値
	WORD	PitchRegistMax( 15 );							// ピッチエンヴェロープ最大登録数
	WORD	PitchRegistThreshold( 5 );						// ピッチエンヴェロープ登録閾値（変化量下限）
	float	TrimRatioMax( .25 );							// 重複音符切詰率最大
	WORD	MergeMode( 1 );									// パーカッションを９ｃｈに統一モード
	WORD	DrumMode( 3 );									// ドラムモード
	for ( int No = 1; No < argc; ++No ) {					// 引数の数ループ
		if ( '/' == argv[No][0] ) {							// 先頭が'/'なら
			switch ( argv[No][1] ) {						// ２番目により分岐
			case 'C':
			case 'c':	// Channel	/c:ABabMNOCXYZPQRSTUVWGHIJKL | F
				ChStr = argv[No];							// 文字列を取り出し
				ChStr.erase( 0, 3 );						// 先頭３文字を削除
				break;
			case 'R':
			case 'r':	// Resolution	/r4 8 16 32 64 128 256
				{
					string ResoStr( argv[No] );				// 文字列を取り出し
					ResoStr.erase( 0, 2 );					// 先頭２文字を削除
					istringstream( ResoStr ) >> Resolution;	// 解像度を取得
					Resolution /= 4;						// 128の場合32(TimeBase)
				}
				break;
			case 'V':
			case 'v':	// 音量設定
				switch ( argv[No][2] ) {
				case 'M':
				case 'm':	// VolumeMode	/vm0:固定音量モード 1:可変音量モード 2:音色音量モード 3:音色可変音量モード
					{
						string VolModeStr( argv[No] );			// 文字列を取り出し
						VolModeStr.erase( 0, 3 );				// 先頭３文字を削除
						istringstream( VolModeStr ) >> VolMode;	// 音量モードを取得
						VolMode &= 0x3;
					}
					break;
				case 'T':
				case 't':	// VolDefThreshold	音量登録数間引き閾値 /vt64
					{
						string VolDefThresholdStr( argv[No] );	// 文字列を取り出し
						VolDefThresholdStr.erase( 0, 3 );		// 先頭３文字を削除
						istringstream( VolDefThresholdStr ) >> VolDefThreshold;// 音量登録数間引き閾値を取得
					}
					break;
				}
				break;
			case 'P':
			case 'p':	// ピッチ設定
				switch ( argv[No][2] ) {
				case 'M':
				case 'm':	// PitchRegistMax	ピッチエンヴェロープ最大登録数 /pm128　（範囲0～128）
					{
						string PitchRegistMaxStr( argv[No] );	// 文字列を取り出し
						PitchRegistMaxStr.erase( 0, 3 );		// 先頭３文字を削除
						istringstream( PitchRegistMaxStr ) >> PitchRegistMax;	// ピッチエンヴェロープ最大登録数を取得
						PitchRegistMax = (PitchRegistMax > 128)? 128: PitchRegistMax;	// 最大値補正
					}
					break;
				case 'T':
				case 't':	// PitchRegistThreshold	ピッチエンヴェロープ登録閾値（変化量下限） /pt1	（範囲0～65535）
					{
						string PitchRegistThresholdStr( argv[No] );	// 文字列を取り出し
						PitchRegistThresholdStr.erase( 0, 3 );		// 先頭３文字を削除
						istringstream( PitchRegistThresholdStr ) >> PitchRegistThreshold;// 音量登録数間引き閾値を取得
					}
					break;
				}
				break;
			case 'N':
			case 'n':	// RepeatNoteTrimRatioMax 重複音符切詰率最大 /n25
				{
					string TrimRatioMaxStr( argv[No] );		// 文字列を取り出し
					TrimRatioMaxStr.erase( 0, 2 );			// 先頭２文字を削除
					istringstream( TrimRatioMaxStr ) >> TrimRatioMax;// 音量登録数間引き閾値を取得
					TrimRatioMax /= 100;					// 百分率にする
				}
				break;
			case 'M':
			case 'm':	// MergeMode	パーカッションを９ｃｈに統一モード	/m0 /m1
				{
					string MergeModeStr( argv[No] );		// 文字列を取り出し
					MergeModeStr.erase( 0, 2 );				// 先頭２文字を削除
					istringstream( MergeModeStr ) >> MergeMode;	// パーカッションを９ｃｈに統一モードを取得
				}
				break;
			case 'D':
			case 'd':	// DrumMode	ドラムモード	/d1 ノイズ使用	/d2 DPCM使用	/d3 ノイズ＆DPCM使用
				{
					string DrumModeStr( argv[No] );			// 文字列を取り出し
					DrumModeStr.erase( 0, 2 );				// 先頭２文字を削除
					istringstream( DrumModeStr ) >> DrumMode;// ドラムモードを取得
					DrumMode &= 0x3;						// 範囲内にする
					if ( 0 == DrumMode ) {					// ドラムモード0だったら
						DrumMode = 3;						// ドラムモード3にする
					}
				}
				break;
			}
		} else {											// それ以外なら
			FilePath = argv[No];							// ファイルパスとする
		}
	}
	if ( FilePath.empty() ) {								// ファイルパスが指定されてなければ
		cerr << "MIDI File を指定して" << endl;
	} else {
		// midiファイル読み込み
		Midi midi;
		if ( midi.Load ( FilePath.c_str() ) ) {
			exit(-1);
		}
		// パーカッションを９ｃｈに統一
		if ( MergeMode ) {									// パーカッションを９ｃｈに統一モードが有効なら
			midi.UnifyPercussionCh9();						// パーカッションを９ｃｈに統一
			// ※パーカッション指定した後、解除しても９ｃｈに統合される問題がある
		}
		// SysEx削除
		midi.DeleteSysEx(0xC);								// 拍子／マーカーは削除しない設定
		// フォーマット変更
		midi.ChengeFormat(1);								// フォーマット１にし、チャンネル毎にトラックを統合する
		// 未使用制御削除
		midi.DeleteUnusedOperate();
		// 空トラック削除
		midi.DeleteEmptyTrack();
		// ノートエンド表現変更
		midi.ChengeNoteEndType();
		// パーカッションチャンネルをトラック分割
		switch ( DrumMode ) {
		case 1:
			{	const BYTE ALL_NOIZE_MAP[128] =					// 全ノイズ割り当て表
				  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
				midi.DividePercussion( ALL_NOIZE_MAP );
			}
			break;
		case 2:
			{	const BYTE ALL_DPCM_MAP[128] =					// 全DPCM割り当て表
				  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
				midi.DividePercussion( ALL_DPCM_MAP );
			}
			break;
		case 3:
		default:
			midi.DividePercussion();
		}
		// パーカッションチャンネルの重複音符削除
		midi.DeleteCh9RepeatNote();
		// 音符の延長
		midi.ExtendNote();
		// 重複音符のトラック分割
		midi.DivideRepeatNote( TrimRatioMax );
		// テンポ／拍子／マーカーを全トラックにコピー
		midi.CopyTempoAllTrack();
		// 出力midiファイル名編集
		string ofilepath( FilePath );						// 出力midiファイル名は入力ファイル名の
		int pos = ofilepath.find_last_of( "." );			// 拡張子の前に
		ofilepath.insert( pos, "_" );						// '_'を追加する
		// midiファイル出力
		midi.Save( ofilepath.c_str() );
		// 曲先頭の空白を切詰める
		midi.BrankTrim();
		// 分解能を下げる
		midi.ChangeResolution( Resolution );				// 分解能を修正
		// 絶対時間に設定
		midi.ChengeTimeType( midi.TT_ABSOLUTE );			// 絶対時間に変更
		// MMLを生成
		Mml	mml;
		// MMLのピッチエンヴェロープ最大登録数を設定
		mml.mPitch.mRegistMax = PitchRegistMax;				// ピッチエンヴェロープ最大登録数を設定
		// MMLのピッチエンヴェロープ登録閾値（変化量下限）を設定
		mml.mPitch.mRegistThreshold = PitchRegistThreshold;	// ピッチエンヴェロープ登録閾値（変化量下限）を設定
		// MMLにロード
		mml.Load( midi, ChStr );							// チャンネルを指定してMMLロード
		// ループ位置決定
		mml.LoopPointConclusion();							// 最後のLコマンド以外を削除する
		// 音量モード設定
		mml.SetVolMode( static_cast<Mml::VOL_MODE>(VolMode) );// 音量モードを可変音量モードに変更する
		// 音量定義割当て
		mml.AssignVolume( VolDefThreshold );				// 音量定義割当て
		// 出力midiファイル名編集
		ofilepath = FilePath;								// 出力midiファイル名は入力ファイル名の
		pos = ofilepath.find_last_of( "." );				// 拡張子の位置
		if (pos != string::npos) {							// 拡張子があるなら
			ofilepath = ofilepath.substr( 0, pos );			// 拡張子を切り落とす
		}
		// ファイル名抽出
		string FileName(ofilepath);							// ファイル名
		pos = FileName.find_last_of( "\\" );				// ファイル名の位置
		if (pos != string::npos) {							// pathがあるなら
			FileName = FileName.substr( pos+1 );			// pathを切り落とす
		}
		mml.SetFileName( FileName );						// ファイル名を設定
		
		ofilepath += ".mml";								// ".mml"に挿げ替える
		// MMLファイル出力
		mml.Save( ofilepath.c_str() );						// MMLファイル出力
		cerr << "done." << endl;
	}	// ここで開放
	return 0;
}
