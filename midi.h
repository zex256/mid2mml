#ifndef _MIDI
#define _MIDI
#include <list>
#include "type.h"
using namespace std;

// MIDIクラス
class Midi {
protected:
	// midi file headers
	struct FileHeader {
		DWORD	dwKeyWord;											// "MThd"
		DWORD	dwMainHeaderSize;									// MainHeader size
	};
	struct MainHeader {
		WORD	wFormat;											// midi format 0/1
		WORD	wTrackCount;										// track count
		WORD	wTimeBase;											// 分解能：4分音符とする数値(480が一般的)
	};
	struct TrackHeader {
		DWORD	dwKeyWord;											// "MTrk"
		DWORD	dwTrackSize;										// track size
	};
	enum {
		FHSIZE = sizeof( FileHeader ),
		MHSIZE = sizeof( MainHeader ),
		THSIZE = sizeof( TrackHeader),
	};
public:
	// 制御クラス
	class Operate {													// 基底クラス
	public:
		DWORD	Time;												// 直前の制御からの時間
		BYTE	Status;												// ステータス
		BYTE	Status1;											// ステータス1
		BYTE	Status2;											// ステータス2
		string	ExData;												// システムエクスクルーシブメッセージ
		// コンストラクタ
		Operate(													// コンストラクタで初期化
			DWORD	_Time	= 0,
			BYTE	_Status	= 0x00,
			BYTE	_Status1= 0x00,
			BYTE	_Status2= 0x00,
			string	_ExData	= "")
		:	Time	(_Time),
			Status	(_Status),
			Status1	(_Status1),
			Status2	(_Status2),
			ExData	(_ExData) {}
		// オペレーター＜	(list::sortやlist::mergeが使用する)
		bool operator < (
			const Operate &Ope) {
			// 絶対時間により判定するが同じ時間の場合はチャンネルで判定する
			return ( (Time < Ope.Time)? TRUE: ( (Time > Ope.Time)? FALSE: 
					((0xf & Status) < (0xf & Ope.Status))? TRUE: ((0xf & Status) > (0xf & Ope.Status))? FALSE:
					((0xf & Status1) < (0xf & Ope.Status1)) ) );	// それも同じなら低音順
			//return ( Time < Ope.Time );
		}
/* 非効率なのでやめた
		// オペレーター＝＝	(list::removeが使用する)
		bool operator == (
			const Operate &Ope) {
			// ステータス別に一致を判定する
			if ( Status != Ope.Status ) return FALSE;				// ステータスが違う
			switch ( 0xF0 & Status ) {
			// 多バイト
			case 0xF0:	// システムエクスクルーシブメッセージ
				if ( 0xFF == Status ) {	// メタイベントなら
					if ( Status1 != Ope.Status1 ) return FALSE;		// ステータス１が違う
				}
				if ( ExData != Ope.ExData ) return FALSE;			// システムエクスクルーシブメッセージが違う
				break;
			// 3ByteStatus
			case 0x80:	// ノートオフ
			case 0x90:	// ノートオン
			case 0xA0:	// ポリフォニックキープレッシャー
			case 0xB0:	// コントロールチェンジ
			case 0xE0:	// ピッチベンド
				if ( Status2 != Ope.Status2 ) return FALSE;			// ステータス２が違う
				// ※breakせず続行
			// 2ByteStatus
			case 0xC0:	// プログラムチェンジ
			case 0xD0:	// チャンネルプレッシャー
				if ( Status1 != Ope.Status1 ) return FALSE;			// ステータス１が違う
			//default:	// それ以外は異常なステータスだが、その場合ステータスのみの判定とする
			}
			return TRUE;											// 一致と認める
		}
*/
	};
	// メンバ変数
	WORD		mFormat;											// midi format 0/1
	WORD		mTimeBase;											// 分解能
	enum TIMETYPE {													// 時間タイプ列挙型
		TT_RELATIVE,												// 　相対時間
		TT_ABSOLUTE,												// 　絶対時間
	}			mTimeType;											// 時間タイプ FALSE:相対時間 TRUE:絶対時間
	list<list<Operate>> mTrackList;									// トラックデータ
	// コンストラクタ
	Midi():															// コンストラクタ
		mFormat(),
		mTimeBase(),
		mTimeType(),
		mTrackList() {}
	Midi(															// ファイル名付きコンストラクタ
		const char		*FilePath ):								// (i)SMFファイルパス
		mFormat(),
		mTimeBase(),
		mTimeType(),
		mTrackList() {
		Load( FilePath );											// ロードする
	}
	// デストラクタ
	~Midi(){}
	// ロード
	int Load (														// SMFを読み込む
		const char		*FilePath );								// (i)ファイルパス
	// セーブ
	int Save (														// SMFを書き出す
		const char		*FilePath );								// (i)ファイルパス
	// 時間伸縮
	void TimeExpand (												// トラック内の時間を伸縮する
		const float		&Ratio );									// (i)伸縮率
	// 分解能変更
	void ChangeResolution (											// 分解能変更
		const WORD		&NewTimeBase );								// (i)新しい分解能
	// フォーマット変更
	void ChengeFormat (												// フォーマットを変更する
		const WORD		&Format );									// (i)フォーマット
	// 時間タイプ変更
	void ChengeTimeType (											// 時間タイプを変更する
		const TIMETYPE	&NewTimeType );								// (i)新しい時間タイプ
	// SysEx削除
	void DeleteSysEx (												// SysEx削除
		const WORD		Mode = 0x3 );								// (i)SysEx削除モード
	// 未使用制御削除
	void DeleteUnusedOperate ( void );								// 未使用制御削除
	// 空トラック削除
	void DeleteEmptyTrack ( void );									// 空トラック削除
	// ノートエンド表現変更
	void ChengeNoteEndType (										// ノートエンド表現変更
		const WORD		NewNoteEnd = 0x90);							// (i)新しいノートエンド表現 0x80 / 0x90
	// パーカッションを９ｃｈに統一
	void UnifyPercussionCh9 ( void );								// パーカッションを９ｃｈに統一
	// パーカッションチャンネルをトラック分離
	void DividePercussion (											// パーカッションチャンネルをトラック分割
		const BYTE		*NoteMap = NULL);							// (i)割り当て表[128byte配列] 0:ノイズ 1:DPCM
	// パーカッションチャンネルの重複音符削除
	void DeleteCh9RepeatNote (										// パーカッションチャンネルの重複音符削除
		const BYTE		*Priority = NULL);							// (i)優先度表
	// 音符の延長
	void ExtendNote (												// 音符の延長
		const WORD		ChMask = 0x0200 );							// (i)チャンネルマスク(９ｃｈ)
	// 重複音符をトラック分離
	void DivideRepeatNote (											// 重複音符をトラック分割
		const float		TailRatioMax = .25,							// (i)尻尾割合最大
		const WORD		ChMask = 0xFDFF);							// (i)チャンネルマスク(９ｃｈ以外)
	// テンポ／拍子／マーカーを全トラックにコピー
	void CopyTempoAllTrack ( void );								// テンポ／拍子／マーカーを全トラックにコピー
	// 曲先頭の空白を切詰
	void BrankTrim ( void );										// 曲先頭の空白を切詰

	// 全制御表示
	void PrintAllOperate ( void );									// 全制御表示
	//　デバッグ用（異常な値をチェックする）
	void CheckAllOperate ( void );									// 全制御表示
	void FindViewOperate (											// デバッグ用（特定の制御検索し表示する）
		const BYTE		&Status,									// (i)検索する制御
		const BYTE		&Status1 = 0x00,							// (i)検索する制御ステータス１
		const BYTE		&Status2 = 0x00);							// (i)検索する制御ステータス２
	
protected:// 内部関数
	// トラック複合化
	int Decode (													// ファイルストリームから読み込みトラック内解析する
		istream			&is,										// (i)入力ストリーム
		const DWORD		&TrackSize);								// (i)トラックサイズ（トラックの終了位置判定に利用）
	// デルタタイム複合化
	DWORD TimeDecode (												// ファイルストリームから読み込みデルタタイム解析し時間を返す
		istream			&is);										// (i)入力ストリーム
	// トラック符号化
	int Encode(														// トラックデータを符号化しストリームに出力する
		ostream			&os,										// (o)出力ストリーム
		list<Operate>	&OpeList);									// (i)制御リスト
	// デルタタイム符号化
	int TimeEncode (												// 時間をデルタタイムに符号化しストリームに出力する
		ostream			&os,										// (o)出力ストリーム
		DWORD			Time);										// (i)時間
	// 全トラックエンドを削除
	DWORD DeleteAllTrackEnd ( void );								// 全トラックエンドを削除、及びトラック最後の時間を採取
	// 全トラックトラックエンド追加
	void  AppendAllTrackEnd (										// 全トラックトラックエンド追加
		const DWORD		&LastTime );								// (i)トラックエンド時間
};
#endif	// MIDI
