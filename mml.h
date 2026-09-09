#ifndef _MML
#define _MML
#include <string>
#include <vector>
#include <map>
#include "ToneDef.h"
#include "midi.h"
using namespace std;
// MMLクラス
class Mml {
public:
	// メンバ変数
	enum VOL_MODE {													// 音量モード
		VOL_CONSTANT,												// 　固定音量モード
		VOL_VARIABLE,												// 　可変音量モード
		VOL_COLORFIT,												// 　音色音量モード
		VOL_COL_VAR,												// 　音色可変音量モード
	};
	string				mChStr;										// チャンネル文字列
	string				mFileName;									// ファイル名
	string				mTitle;										// タイトル
	string				mComposer;									// 作曲者名
	string				mMaker;										// 原著作者名
	string				mProgamer;									// 打ち込み者名
	WORD				mFirstTempo;								// 最初のテンポ
	// 音質管理
	class Tone {													// 音質管理
	public:
		VOL_MODE			VolMode;								// 音量モード
		// 音色
		// ABabチャンネル MNチャンネル(音色番号+128)
		WORD				SerialNoABabMN;							// 管理番号
		map<WORD,string>	ColorCmdABabMN;							// コマンド
		map<WORD,string>	ColorDefABabMN;							// 定義
		// Dチャンネル
		WORD				SerialNoD;								// 管理番号
		map<WORD,string>	ColorCmdD;								// コマンド
		map<WORD,string>	NoteCmdD;								// 音符コマンド
		// Eチャンネル
		WORD				SerialNoE;								// 管理番号
		map<WORD,string>	NoteCmdE;								// 音符コマンド
		map<WORD,string>	ColorDefE;								// 定義
		// Fチャンネル
		WORD				SerialNoF;								// 管理番号
		map<WORD,string>	ColorCmdF;								// コマンド
		map<WORD,string>	ColorDefF;								// 定義
		// GHIJKLチャンネル
		WORD				SerialNoGHIJKL;							// 管理番号
		map<WORD,string>	ColorCmdGHIJKL;							// コマンド
		map<WORD,string>	ColorDefGHIJKL;							// 定義
		// PQRSTUVWチャンネル
		WORD				SerialNoPQRSTUVW;						// 管理番号
		map<WORD,string>	ColorCmdPQRSTUVW;						// コマンド
		map<WORD,string>	ColorDefPQRSTUVW;						// 定義
		// XYZチャンネル
		WORD				SerialNoXYZ;							// 管理番号
		map<WORD,string>	ColorCmdXYZ;							// コマンド

		// LFO
		WORD				SerialNoLfo;							// 管理番号
		map<WORD,string>	LfoCmd;									// コマンド
		map<WORD,string>	LfoDef;									// 定義
		
		// 音量(エンヴェロープ) FOチャンネル(音色番号+128) Dチャンネル(音色番号+256)
		BYTE				VolDefMask;								// 音量マスク（間引き）
		struct VolumeInfo {											// 音量情報
			WORD			SerialNo;								// 管理番号
			BYTE			Ch;										// チャンネル
			string			Define;									// 定義
		};
		map<WORD,map<WORD,VolumeInfo>>	VolumeDef;					// 定義
		// 音色定義
		string				ToneDefVctNoize[128];					// 音色定義ノイズ
		string				ToneDefVctSquare[128];					// 音色定義矩形波
		string				ToneDefVctVRC6[128];					// 音色定義VRC6矩形波
		string				ToneDefVctVRC7[128];					// 音色定義VRC7
		string				ToneDefVctN106[128];					// 音色定義N106
		string				ToneDefVctFDS[128];						// 音色定義FDS
		// 音色別音量エンヴェロープ定義
		string				VolDefVctNoise[128];					// 音色別音量エンヴェロープ定義ノイズ
		string				VolDefVctCommon[128];					// 音色別音量エンヴェロープ定義共通
		string				VolDefVctComm64[128];					// 音色別音量エンヴェロープ定義共通64段階
		string				VolDefVctVRC7Pre[128];						// 音色別音量エンヴェロープ定義VRC7
		// コンストラクタ
		Tone():
			VolMode( VOL_COL_VAR ),
			SerialNoABabMN(),
			ColorCmdABabMN(),
			ColorDefABabMN(),
			SerialNoD(),
			ColorCmdD(),
			NoteCmdD(),
			SerialNoE(),
			NoteCmdE(),
			ColorDefE(),
			SerialNoF(),
			ColorCmdF(),
			ColorDefF(),
			SerialNoGHIJKL(),
			ColorCmdGHIJKL(),
			ColorDefGHIJKL(),
			SerialNoPQRSTUVW(),
			ColorCmdPQRSTUVW(),
			ColorDefPQRSTUVW(),
			SerialNoXYZ(),
			ColorCmdXYZ(),
			SerialNoLfo(),
			LfoCmd(),
			LfoDef(),
			VolDefMask(),
			VolumeDef()
			{
				for ( int i=128 ; i--;) {
					// 音色定義
					ToneDefVctNoize[i] = TONE_DEF_NOIZE[i];			// 音色定義ノイズに初期値を設定
					ToneDefVctSquare[i]= TONE_DEF_SQUARE[i];		// 音色定義矩形波に初期値を設定
					ToneDefVctVRC6[i]  = TONE_DEF_VRC6[i];			// 音色定義VRC6に初期値を設定
					ToneDefVctVRC7[i]  = TONE_DEF_VRC7_USER[i];		// 音色定義VRC7に初期値を設定
					ToneDefVctN106[i]  = TONE_DEF_N106[i];			// 音色定義N106に初期値を設定
					ToneDefVctFDS[i]   = TONE_DEF_FDS[i];			// 音色定義FDSに初期値を設定
					// 音色別音量エンヴェロープ定義
					VolDefVctNoise[i]  = VOL_DEF_NOIZE[i];			// 音色別音量エンヴェロープ定義ノイズ
					VolDefVctCommon[i] = VOL_DEF_COMMON[i];			// 音色別音量エンヴェロープ定義共通
					VolDefVctVRC7Pre[i]= VOL_DEF_VRC7[i];			// 音色別音量エンヴェロープ定義VRC7
				}
			}
		// 音質コマンド、定義登録
		void Set (													// 音質コマンド、定義登録
			const BYTE			&Ch,								// (i)チャンネル
			const BYTE			&PrgNo,								// (i)プログラム番号
			const BYTE			Volume = 0);						// (i)音量
		// 音量変換
		BYTE VolumeConvert (										// 音量変換
			const BYTE			&Ch,								// チャンネル
			const BYTE			&Vel);								// ベロシティ
		// 音量定義割当て
		void AssignVolume (											// 音量割当て
			const WORD			VolDefThreshold = 64);				// (i)音量登録数間引き閾値
		// 音量調節
		string AdjustVolume (										// 音量調節
			const float			&Ratio,								// (i)割合
			const string		&Define);							// (i)定義
		// 音質コマンド取得
		string Get (												// 音質コマンド取得
			const BYTE			&Ch,								// (i)チャンネル
			const BYTE			&PrgNo);							// (i)プログラム番号
		// 音符コマンド取得
		string GetNote (											// 音符コマンド取得
			const BYTE			&Ch,								// (i)チャンネル
			const BYTE			&PrgNo);							// (i)プログラム番号
		// 音量コマンド取得
		string GetVolume (											// 音量コマンド取得
			const BYTE			&Ch,								// (i)チャンネル
			const BYTE			&PrgNo,								// (i)プログラム番号
			const BYTE			&Volume);							// (i)音量
		// 音質定義出力
		void PutToneDef (											// 音質定義出力
			ostream				&os,								// (o)出力ストリーム
			map<WORD,string>	&DefMap);							// (i)音質定義マップ
		// 音量定義出力
		void PutVolumeDef (											// 音量定義出力
			ostream				&os,								// (o)出力ストリーム
			map<WORD,map<WORD,VolumeInfo>>	&DefMap);				// (i)音量定義マップ
	protected:
		// パーカッション未登録部分をマップ
		WORD ParMapChg (											// パーカッション未登録部分をマップ
			const WORD	&PrgNo);									// (i)プログラム番号
		// コメント編集
		string CommentEdit(											// コメント編集
			const BYTE			&Ch,								// (i)チャンネル
			const BYTE			&PrgNo)								// (i)プログラム番号
		{
			char	ChStr[2] = " ";									// チャンネル文字列
			ChStr[0] = Ch;
			ostringstream	oss;									// 編集用
			oss << "// Ch." << ChStr << "\t\t\tPrgNo." << 
				static_cast<WORD>(PrgNo +1) << "\t" << TONE_NAME[PrgNo] << endl;
			return oss.str();										// コメントを出力
		}
	}	mTone;														// 音質管理クラス実体
	//--------------------------------------------------------------
	// ピッチ管理
	class Pitch {													// ピッチ管理
	public:
		WORD				mRegistMax;								// 最大登録数
		WORD				mRegistThreshold;						// 登録閾値（ピッチエンヴェロープ変化量下限）
		vector<string>		EnvelopeDef;							// 定義
		// コンストラクタ
		Pitch():
			mRegistMax(128),
			mRegistThreshold(1),
			EnvelopeDef() {}
		// ピッチエンヴェロープ定義登録
		string Regist (												// ピッチエンヴェロープ定義登録
			vector<char>		&Envelope,							// (io)ピッチエンヴェロープ
			const BYTE			Ch = '?');							// (i)チャンネル
		// ピッチエンヴェロープ定義出力
		void PutDef (												// ピッチエンヴェロープ定義出力
			ostream			&os);									// (o)出力ストリーム
	} mPitch;
	//--------------------------------------------------------------
	// 音符情報
	struct NoteInfo {												// 音符情報
		string	Str;												// 音符その他(c,c+,d,d+,e,f,f+,g,g+,a,a+,b,n,r,w,&,@...)
		DWORD	Len;												// 長さ
		BYTE	Oct;												// 音程(-1～10)
		BYTE	Vol;												// 音量(0～15,0～63)
		// コンストラクタ
		NoteInfo (
			string	_Str = "",
			DWORD	_Len = 0,
			BYTE	_Oct = 0,
			BYTE	_Vol = 0):
			Str( _Str ),
			Len( _Len ),
			Oct( _Oct ),
			Vol( _Vol ) {}
	};
	// チャンネル情報構造体
	struct ChInfo {													// チャンネル情報
		BYTE				DefaultLen;								// デフォルトレングス
		BYTE				FirstOctave;							// 最初のオクターブ
		BYTE				FirstVolume;							// 最初の音量
		BYTE				FirstPrgNo;								// 最初のプログラム番号
		vector<NoteInfo>	NoteVector;								// 音符ベクタ
		// コンストラクタ
		ChInfo():
			DefaultLen(),
			FirstOctave(255),
			FirstVolume(255),
			FirstPrgNo(0),
			NoteVector() {}
	};
	map<BYTE,ChInfo>		mChMap;									// チャンネルマップ
	WORD					mTimeBase;								// 分解能
	// メンバ関数
	// コンストラクタ
	Mml ():															// デフォルトコンストラクタ
		mChStr(),
		mFileName(),
		mTitle(),
		mComposer(),
		mMaker(),
		mProgamer(),
		mFirstTempo(),
		mTone(),
		mPitch(),
		mChMap(),
		mTimeBase(64) {}
	Mml (															// 引数付きコンストラクタ
		const Midi			&midi ):								// (i)Midiクラス
		mChStr(),
		mFileName(),
		mTitle(),
		mComposer(),
		mMaker(),
		mProgamer(),
		mFirstTempo(),
		mTone(),
		mPitch(),
		mChMap(),
		mTimeBase(64)
	{
		Load ( midi );												// ロードを呼ぶ
	}
	Mml (															// 引数付きコンストラクタ
		const Midi			&midi,	 								// (i)Midiクラス
		string				ChStr):									// (i)チャンネル文字列(※D,Eチャンネルは9chからの変換固定のため含てはならない)
		mChStr(),
		mFileName(),
		mTitle(),
		mComposer(),
		mMaker(),
		mProgamer(),
		mFirstTempo(),
		mTone(),
		mPitch(),
		mChMap(),
		mTimeBase(64)
	{
		Load ( midi, ChStr );										// ロードを呼ぶ
	}
	// ロード
	void Load (														// Midiクラスからのロード
		const Midi			&midi,									// (i)Midiクラス
		string				ChStr = "ABabMNOCXYZPQRSTUVWGHIJKL");	// (i)チャンネル文字列(※D,Eチャンネルは9chからの変換固定のため含てはならない)
	// セーブ
	int Save (														// セーブ
		const char			*FilePath,								// (i)ファイルパス
		BYTE				OneLineBar = 1);						// (i)一行に出力する小節数
	// ファイル名設定
	void SetFileName (
		const string		FileName)								// (i)ファイル名
	{
		mFileName = FileName;
	}	
	// 音量モード設定
	void SetVolMode (												// 音量モード設定
		const VOL_MODE		VolMode)								// (i)音量モード
	{
		mTone.VolMode = VolMode;									// 音量モードを変更する
	}
	// 音量定義割当て
	void AssignVolume (												// 音量割当て
			const WORD		VolDefThreshold = 64)					// (i)音量登録数間引き閾値
	{
		mTone.AssignVolume( VolDefThreshold );						// 音量割当て
	}
	// ループ位置決定
	void LoopPointConclusion ();									//	'L'コマンドが複数ある場合、最後の'L'コマンドのみを残し、それ以外を削除
protected:
	//--------------------------------------------------------------
	// 音符の周波数（レジスタ値）計算
	DWORD NoteFrequencyCalc(										// 音符の周波数（レジスタ値）計算
		const BYTE			&KeyNo,									// (i)MIDIキー番号
		const BYTE			&Ch,									// (i)チャンネル
		const BYTE			BaseKeyNo = 1);							// (i)基準MIDIキー番号(VRC7用)
	// ピッチ周波数（レジスタ値）計算
	DWORD PitchFrequencyCalc(										// ピッチ周波数（レジスタ値）計算
		const BYTE			&KeyNo,									// (i)MIDIキー番号
		const BYTE			&Ch,									// (i)チャンネル
		const BYTE			&PitchBendSensitivity,					// (i)ピッチベンドセンシティヴィティ
		const short			&PitchBend);							// (i)ピッチベンド(範囲-8192～0～8191)
	// ピッチエンヴェロープ設定
	void SetPitchEnvelope (											// ピッチエンヴェロープ設定
		vector<char>		&PitchEnvelope,							// (io)ピッチエンべロープ
		DWORD				Frame,									// (i)フレーム
		int					PitchDiff);								// (i)ピッチ周波数（レジスタ値）差分
	//--------------------------------------------------------------
	// 音符長を数える
	void UseLenCnt(													// 音符長を数える
		map<BYTE,DWORD>		&UseLen,								// (io)音符長の数
		const DWORD			&NoteLen,								// (i)音符の長さ
		const WORD			&WholeNote);							// (i)全音符の長さ
	// 音源使用宣言出力
	void PutChUseDef (												// 音源使用宣言出力
		ostream				&os,									// (o)出力ストリーム
		string				ChkCh,									// (i)対象チャンネル文字列
		string				Def);									// (i)音源使用宣言
};
#endif	// MML
