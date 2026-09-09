// MMLクラス
#include "pch.h"
#include <cstdlib>													// 環境変数取得用
#include <fstream>													// ファイル
#include <algorithm>												// replaceとかで使用
#include <math.h>													// 乗数計算に使用
#include "mml.h"
using namespace std;

// Midiクラスからのロード
//	※音符の重複が無いこと
//	MIDIクラスから必要な全情報を読み取り、中間的MML形式でクラス内に溜め込みます。
//	・環境変数"USERNAME"を取得します。（#PROGRAMER定義で使用）
//	・引数指定のチャンネル文字列でチャンネルを割り当てます。
//	・チャンネル毎に一番多く使われてる音符長を調べます（lコマンドで使用）
//	・音符、休符を採譜します。
//	・プログラムチェンジの番号を基にMMLの音色情報を登録します。（パーカッションの場合はキー番号による）
void
Mml::Load (															// Midiクラスからのロード
	const Midi &midi,												// (i)Midiクラス
	string	ChStr)													// (i)チャンネル文字列(※D,Eチャンネルは9chからの変換固定のため含てはならない)
{
	// 環境変数の取得
	{
#if _MSC_VER >=1300													// VC++2003以降
		char	env[256];
		size_t	requiredSize = sizeof(env);
		getenv_s(&requiredSize, env, requiredSize, "USERNAME");		// 環境変数"USERNAME"を取得
#else																// それ以外
		const char* env = getenv( "USERNAME" );						// 環境変数"USERNAME"を取得
#endif
		if ( env ) {												// 環境変数があるなら
			mProgamer = env;										// 打ち込み者名に設定
		}
	}
	mChStr = ChStr;													// チャンネル文字列を保存
	mTimeBase = midi.mTimeBase;										// Midiクラスからタイムベースを取得する
	const WORD	WholeNote = mTimeBase << 2;							// 全音符の長さ
	string PerChStr( "DE" );										// パーカッションチャンネル文字列
	const string NoteChar[]={"c","c+","d","d+","e","f","f+","g","g+","a","a+","b"};// 音符文字
	// トラックリストループ
	for ( list<list<Midi::Operate>>::const_iterator TrackIt = midi.mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != midi.mTrackList.end();							// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		ChInfo	Channel;											// チャンネル情報作成
		map<BYTE,DWORD>	UseLen;										// 使用レングスカウント[0]全音符[1]2分[2]4分[3]8分[4]16分[5]32分[6]64分[7]128分
		list<Midi::Operate>::const_iterator	NoteOn = (*TrackIt).end();// ノートオンの位置
		list<Midi::Operate>::const_iterator	NoteOff= (*TrackIt).end();// ノートオフの位置
		WORD	Tempo(120);											// テンポ
		BYTE	MmlCh(0);											// MMLでのチャンネル
		BYTE	PrgNo(0);											// プログラム(音色)番号
		BYTE	PrevPrgNo(255);										// １つ前の音符のプログラム(音色)番号
		// ↓ピッチエンヴェロープ用
		BYTE	RPN_LSB(0);											// RPN LSB(コントロールチェンジ100)
		BYTE	RPN_MSB(0);											// RPN MSB(コントロールチェンジ101)
		BYTE	PitchBendSensitivity(2);							// ピッチベンドセンシティヴィティ
		short	PitchBend(0);										// ピッチベンド(範囲-8192～0～8191)
		DWORD	PrevPitchFrequency(0);								// 直前のピッチ周波数（レジスタ値）
		int		InsideNote(0);										// 音符中 0:音符外 1:音符内
		vector<char> PitchEnvelope;									// ピッチエンべロープ
		string	PrevEPcommand("EPOF");								// 直前のEPコマンド
		// ↑ピッチエンヴェロープ用
		// 制御リストループ
		for ( list<Midi::Operate>::const_iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			const Midi::Operate &Ope = *OpeIt;
			BYTE Sta = 0xF0 & Ope.Status;							// イベント抽出
			switch ( Sta ) {
			case 0xF0:	// SysEx
				if ( 0xFF == Ope.Status ) {							// メタイベント
					switch ( Ope.Status1 ) {						// テキスト種類別処理
					case 0x03:	// タイトル
						if ( TrackIt == midi.mTrackList.begin()		// 先頭トラックでかつ
							 && mTitle.empty() ) {					// タイトルが空なら
							mTitle = Ope.ExData;					// タイトルを取得
							replace( mTitle.begin(), mTitle.end(), '\0', ' ');// ヌル文字をスペースにする
						}
						break;
					case 0x02:	// 著作権表示
						{
							string::size_type idx1 = Ope.ExData.find("(C)");		// "(C)"を検索
							string::size_type idx2 = Ope.ExData.find("Copyright");	// "Copyright"を検索
							if ( (idx1 == string::npos) &&
								 (idx2 == string::npos) ) {			// 含まれていなければ
								if ( mComposer.empty() ) {			// 作曲者名が空なら
									mComposer = Ope.ExData;			// 作曲者名を取得
									replace( mComposer.begin(), mComposer.end(), '\0', ' ');// ヌル文字をスペースにする
								}
							} else {
								if ( mMaker.empty() ) {				// 原著作者名が空なら
									mMaker = Ope.ExData;			// 原著作者名を取得
									replace( mMaker.begin(), mMaker.end(), '\0', ' ');// ヌル文字をスペースにする
								}
							}
						}
						break;
					case 0x2F:	// トラックエンド
						{
							DWORD NoteLen = Ope.Time -				// 音符の長さを求める
								((NoteOff != (*TrackIt).end())? (*NoteOff).Time: 0);
							if ( NoteLen ) {						// 音符の長さが０以外なら
								UseLenCnt( UseLen, NoteLen, WholeNote );// 音符長を数える
								NoteInfo Note( (MmlCh != 'E')?"r":"w", NoteLen );// この休符情報を作成
								Channel.NoteVector.push_back( Note );// 音符ベクタに音符情報を登録
							}
						}
						break;
					case 0x51:	// テンポ設定
						{
							DWORD NoteLen = Ope.Time -				// 音符の長さを求める
								((NoteOff != (*TrackIt).end())? (*NoteOff).Time: 0);
							if ( NoteLen ) {						// 音符の長さが０以外なら
								UseLenCnt( UseLen, NoteLen, WholeNote );// 音符長を数える
								NoteInfo Note( (MmlCh != 'E')?"r":"w", NoteLen );// この休符情報を作成
								Channel.NoteVector.push_back( Note );// 音符ベクタに音符情報を登録
								NoteOff = OpeIt;					// ノートオフの位置を記録
							}
							Tempo = 60000000 / (					// テンポを取り出す
								(static_cast<BYTE>(Ope.ExData[0]) << 16) |
								(static_cast<BYTE>(Ope.ExData[1]) << 8) |
								 static_cast<BYTE>(Ope.ExData[2]) );
							ostringstream cmd;						// コマンド文字列
							cmd << "t" << Tempo;					// コマンド文字列を編集
							NoteInfo Note( cmd.str(), Tempo );		// この音符情報を作成
							Channel.NoteVector.push_back( Note );	// 音符ベクタに音符情報を登録
							if ( 0 == mFirstTempo ) {				// 最初のテンポが未設定なら
								mFirstTempo = Tempo;				// 最初のテンポに設定
							}
						}
						break;
					case 0x58:	// 拍子
						{
							DWORD NoteLen = Ope.Time -				// 音符の長さを求める
								((NoteOff != (*TrackIt).end())? (*NoteOff).Time: 0);
							if ( NoteLen ) {						// 音符の長さが０以外なら
								UseLenCnt( UseLen, NoteLen, WholeNote );// 音符長を数える
								NoteInfo Note( (MmlCh != 'E')?"r":"w", NoteLen );// この休符情報を作成
								Channel.NoteVector.push_back( Note );// 音符ベクタに音符情報を登録
								NoteOff = OpeIt;					// ノートオフの位置を記録
							}
							BYTE	mol = Ope.ExData[0];			// 分子を取り出す
							BYTE	denom = 1 << Ope.ExData[1];		// 分母を取り出す
							ostringstream cmd;						// コマンド文字列
							cmd << "!" << (WORD)mol << "/" << (WORD)denom;// コマンド文字列を編集(拍子の内部表現を'!'とする)
							NoteInfo Note( cmd.str(), 0, mol, denom );// この音符情報を作成
							Channel.NoteVector.push_back( Note );	// 音符ベクタに音符情報を登録
						}
						break;
					case 0x06:	// マーカー
						{
							DWORD NoteLen = Ope.Time -				// 音符の長さを求める
								((NoteOff != (*TrackIt).end())? (*NoteOff).Time: 0);
							if ( NoteLen ) {						// 音符の長さが０以外なら
								UseLenCnt( UseLen, NoteLen, WholeNote );// 音符長を数える
								NoteInfo Note( (MmlCh != 'E')?"r":"w", NoteLen );// この休符情報を作成
								Channel.NoteVector.push_back( Note );// 音符ベクタに音符情報を登録
								NoteOff = OpeIt;					// ノートオフの位置を記録
							}
							NoteInfo Note( "L" );					// この音符情報を作成
							Channel.NoteVector.push_back( Note );	// 音符ベクタに音符情報を登録
						}
						break;
					}
					if ( (0x01 <= Ope.Status1) &&					// テキストなら
						 (0x1F >= Ope.Status1) ) {
						ostringstream cmd;							// コマンド文字列
						cmd << "// " << Ope.ExData;					// コマンド文字列を編集
						NoteInfo Note( cmd.str() );					// キーを登録し音質コマンド取得し、この音符情報を作成
						Channel.NoteVector.push_back( Note );		// 音符ベクタに音符情報を登録
					}
				}
				continue;											// 次の制御へ
			}
			BYTE Ch = 0xF & Ope.Status;								// このトラックのMIDIチャンネル抽出
			if ( 0x9 != Ch ) {										// MIDIチャンネルが9ch以外で
				if ( !MmlCh ) {										// チャンネルが決まってなく
					if ( !ChStr.empty() ) {							// チャンネル文字列が残っていたら
						MmlCh = ChStr[0];							// チャンネル文字列の先頭１文字を取り出しMMLのチャンネルとする
						ChStr = ChStr.erase(0,1);					// チャンネル文字列の先頭１文字を切り詰める
					} else {										// チャンネル文字列が残ってなければ
						break;										// 次のトラックへ
					}
				}
				switch ( Sta ) {									// イベント別処理
				case 0xC0:	// プログラムチェンジ
					PrgNo = Ope.Status1;							// プログラム(音色)番号を記録
					break;
				case 0xB0:	// コントロールチェンジ
					switch ( Ope.Status1 ) {						// コントローラナンバーで分岐
					case 100:	// RPN LSB
						RPN_LSB = Ope.Status2;						// RPN LSBを設定
						break;
					case 101:	// RPN MSB
						RPN_MSB = Ope.Status2;						// RPN MSBを設定
						break;
					case   6:	// データエントリーMSB
						if ( (0==RPN_LSB) && (0==RPN_MSB) ) {		// RPN LSB MSBが共に０なら
							PitchBendSensitivity = Ope.Status2;		// ピッチベンドセンシティヴィティを設定
						}
						break;
					}
					break;
				case 0xE0:	// ピッチベンド
					PitchBend = ( (static_cast<int>(Ope.Status2) << 7)
							  | Ope.Status1 ) -8192;				// ピッチベンドを取り出す(範囲-8192～0～8191)
					if ( InsideNote ) {								// 音符内なら
						WORD Frame = static_cast<WORD>(				// 音符先頭からのフレーム　＝　整数化（
							3600./Tempo *							// 3600フレーム（１分間のフレーム数）／テンポ　＊
							(Ope.Time - (*NoteOn).Time) / mTimeBase );// 音符先頭からここまでの時間／分解能　）
						PitchEnvelope.resize( Frame +1 );			// 音符先頭からのフレーム数だけピッチエンべロープの要素を確保
						DWORD PitchFrequency = PitchFrequencyCalc (	// ピッチ周波数（レジスタ値）計算
							(*NoteOn).Status1, MmlCh,				// (i)音符のキー番号,(i)チャンネル,
							PitchBendSensitivity, PitchBend);		// (i)ピッチベンドセンシティヴィティ,(i)ピッチベンド
						SetPitchEnvelope( PitchEnvelope, Frame,		// ピッチエンべロープ[フレーム]に設定する
							PrevPitchFrequency - PitchFrequency );	// 直前のピッチ周波数　－　ピッチ周波数　
						PrevPitchFrequency = PitchFrequency;		// 直前のピッチ周波数を更新
					}
					break;
				case 0x90:	// ノートオン
					if ( 0x00 != Ope.Status2 ) {					// ベロシティ０でなければ
						InsideNote = 1;								// 音符内を設定
						NoteOn = OpeIt;								// ノートオンの位置を記録
						DWORD NoteLen = Ope.Time -					// 音符の長さを求める
							((NoteOff != (*TrackIt).end())? (*NoteOff).Time: 0);
						// ピッチエンヴェロープの処理
						PitchEnvelope.clear();						// ピッチエンヴェロープをクリア
						PrevPitchFrequency = NoteFrequencyCalc (	// 音符の周波数（レジスタ値）計算し直前の周波数とする
							(*NoteOn).Status1, MmlCh );				// (i)音符のキー番号,(i)チャンネル
						if ( PitchBend ) {							// ピッチベンドが０じゃなければ
							DWORD PitchFrequency = PitchFrequencyCalc (// ピッチ周波数（レジスタ値）計算
								(*NoteOn).Status1, MmlCh,			// (i)音符のキー番号,(i)チャンネル,
								PitchBendSensitivity, PitchBend);	// (i)ピッチベンドセンシティヴィティ,(i)ピッチベンド
							SetPitchEnvelope( PitchEnvelope, 0,		// ピッチエンべロープに設定する
								PrevPitchFrequency - PitchFrequency );// 直前のピッチ周波数　－　ピッチ周波数　
							PrevPitchFrequency = PitchFrequency;	// 直前のピッチ周波数を更新
						}
						// 休符の処理
						if ( !NoteLen ) {							// 音符の長さが０なら
							continue;								// 次の制御へ
						}
						UseLenCnt( UseLen, NoteLen, WholeNote );	// 音符長を数える
						NoteInfo Note( "r",	NoteLen );				// この休符情報を作成
						Channel.NoteVector.push_back( Note );		// 音符ベクタに音符情報を登録
						break;
					}		// ベロシティ０ならノートオフなので続行
				case 0x80:	// ノートオフ
					if ( Ope.Status1 != (*NoteOn).Status1 ) {		// ノートオンとキーが一致しているなら
						cerr << "音符が重なっています、正しく変換できません。" << endl;
						break;
					}
					{
						InsideNote = 0;								// 音符外を設定
						NoteOff = OpeIt;							// ノートオフの位置を記録
						DWORD NoteLen = Ope.Time -					// 音符の長さを求める
							((NoteOn != (*TrackIt).end())? (*NoteOn).Time: 0);
						if ( !NoteLen ) {							// 音符の長さが０なら
							continue;								// 次の制御へ
						}
						UseLenCnt( UseLen, NoteLen, WholeNote );	// 音符長を数える
						// 音色
						mTone.Set( MmlCh, PrgNo, (*NoteOn).Status2 );// プログラム番号を登録
						if ( PrevPrgNo != PrgNo ) {					// １つ前の音符のプログラム（音色）番号と違うなら
							PrevPrgNo = PrgNo;						// １つ前の音符のプログラム（音色）番号を更新
							NoteInfo Note( "$", 0, PrgNo );			// プログラム番号とする内部表現'$'で音符情報を作成
							Channel.NoteVector.push_back( Note );	// 音符ベクタに音符情報を登録
						}
						// ピッチ
						string Command = mPitch.Regist( PitchEnvelope, MmlCh );// ピッチエンヴェロープを登録し、EPコマンド文字列を受け取る
						if ( (!Command.empty()) &&					// コマンドが空じゃなく、かつ
							 (PrevEPcommand != Command) ) {			// 直前のEPコマンドと違うなら
							PrevEPcommand = Command;				// 直前のEPコマンドを更新
							NoteInfo Note( Command );				// EPコマンドの音符情報を作る
							Channel.NoteVector.push_back( Note );	// 音符ベクタに音符情報を登録
						}
						// 音符
						NoteInfo Note( NoteChar[ Ope.Status1 % 12 ],// 音符情報を作成
							NoteLen, Ope.Status1 / 12 -1,
							mTone.VolumeConvert( MmlCh, (*NoteOn).Status2 ) );// MIDI音量をMML音量に変換
						if ( ('C' == MmlCh) || ('P' <= MmlCh) && ('W' >= MmlCh) ) {// C,P～Wチャンネルなら
							++Note.Oct;								// １オクターブ上げる
						}
						if ( (signed char)Note.Oct < 0 ) {			// オクターブがマイナスなら
							Note.Oct = 0;							// 0オクターブに調整
						}
						Channel.NoteVector.push_back( Note );		// 音符ベクタに音符情報を登録
						// 最初の値収集
						if ( 255 == Channel.FirstOctave ) {			// 最初のオクターブが未設定なら
							Channel.FirstOctave = Note.Oct;			// 最初のオクターブを設定
							Channel.FirstVolume = Note.Vol;			// 最初の音量を設定
							Channel.FirstPrgNo = PrgNo;				// 最初のプログラム番号を設定
						}
					}
					break;
				}
			} else {												// MIDIチャンネルが9chなら
				if ( !MmlCh ) {										// チャンネルが決まってなく
					if ( !PerChStr.empty() ) {						// パーカッションチャンネル文字列が残っていたら
						MmlCh = PerChStr[0];						// パーカッションチャンネル文字列の先頭１文字を取り出しMMLのチャンネルとする
						PerChStr = PerChStr.erase(0,1);				// パーカッションチャンネル文字列の先頭１文字を切り詰める
					} else {										// パーカッションチャンネル文字列が残ってなければ
						break;										// 次のトラックへ
					}
				}
				switch ( Sta ) {									// イベント別処理
				case 0x90:	// ノートオン
					if ( 0x00 != Ope.Status2 ) {					// ベロシティ０でなければ
						NoteOn = OpeIt;								// ノートオンの位置を記録
						DWORD NoteLen = Ope.Time -					// 音符の長さを求める
							((NoteOff != (*TrackIt).end())? (*NoteOff).Time: 0);
						if ( !NoteLen ) {							// 音符の長さが０なら
							continue;								// 次の制御へ
						}
						UseLenCnt( UseLen, NoteLen, WholeNote );	// 音符長を数える
						NoteInfo Note( (MmlCh != 'E')?"r":"w", NoteLen );// この休符情報を作成
						Channel.NoteVector.push_back( Note );		// 音符ベクタに音符情報を登録
						break;
					}		// ベロシティ０ならノートオフなので続行
				case 0x80:	// ノートオフ
					if ( Ope.Status1 != (*NoteOn).Status1 ) {		// ノートオンとキーが一致しているなら
						cerr << "音符が重なっています、正しく変換できません。" << endl;
						break;
					}
					{
						NoteOff = OpeIt;							// ノートオフの位置を記録
						DWORD NoteLen = Ope.Time -					// 音符の長さを求める
							((NoteOn != (*TrackIt).end())? (*NoteOn).Time: 0);
						if ( !NoteLen ) {							// 音符の長さが０なら
							continue;								// 次の制御へ
						}
						UseLenCnt( UseLen, NoteLen, WholeNote );	// 音符長を数える
						mTone.Set( MmlCh, Ope.Status1, (*NoteOn).Status2 );// キーを登録する
						if ( PrevPrgNo != Ope.Status1 ) {			// １つ前の音符のキー（音色）番号と違うなら
							PrevPrgNo = Ope.Status1;				// １つ前の音符のキー（音色）番号を更新
							if ( 'D' == MmlCh ) {					// ノイズチャンネルなら
								NoteInfo Note( "$", 0, Ope.Status1 );// プログラム番号とする内部表現'$'で音符情報を作成
								Channel.NoteVector.push_back( Note );// 音符ベクタに音符情報を登録
							}
						}
						NoteInfo Note( mTone.GetNote( MmlCh, Ope.Status1 ),
							NoteLen, Ope.Status1 / 12 -1,			// キーを登録し音質コマンド取得し、この音符情報を作成
							mTone.VolumeConvert( MmlCh, (*NoteOn).Status2 ) );// MIDI音量をMML音量に変換
						Channel.NoteVector.push_back( Note );		// 音符ベクタに音符情報を登録
						// ↓使わないから意味無いんだけど一応
						if ( 255 == Channel.FirstOctave ) {			// 最初のオクターブが未設定なら
							Channel.FirstOctave = Note.Oct;			// 最初のオクターブを設定
							Channel.FirstVolume = Note.Vol;			// 最初の音量を設定
							Channel.FirstPrgNo = Ope.Status1;		// 最初のプログラム番号を設定
						}
					}
					break;
				}
			}
		}
		if ( MmlCh ) {												// チャンネルが決まっていたなら
			// 一番多く使われているレングスをデフォルトレングスとする
			DWORD	MaxCnt(0);										// 最大使用数をカウント
			for ( map<BYTE,DWORD>::iterator UseLenIt = UseLen.begin();// 使用レングスの最初から
				  UseLenIt != UseLen.end();							// 最後まで
				  ++UseLenIt) {										// 使用レングスイテレータを進める
				if ( MaxCnt < (*UseLenIt).second ) {				// より多く使われていたら
					MaxCnt = (*UseLenIt).second;					// 最大使用数を更新
					Channel.DefaultLen = 1 << (*UseLenIt).first;	// そのレングス(分音符)を更新
				}
			}
			mChMap[ MmlCh ] = Channel;								// チャンネルマップにチャンネル情報を追加
		}
		if ( ChStr.empty() && PerChStr.empty() ) {					// チャンネル文字列が空なら
			break;													// これ以降のトラックを処理しない
		}
	}
}

// 音符の周波数（レジスタ値）計算
#define NES_MASTER_CLOCK	(21477272.7272)							// ファミコンのマスタークロック
#define NES_SYSTEM_CLOCK	(NES_MASTER_CLOCK / 12)					// ファミコンのシステムクロック
DWORD 
Mml::NoteFrequencyCalc(												// 音符の周波数（レジスタ値）計算
	const BYTE			&KeyNo,										// (i)MIDIキー番号
	const BYTE			&Ch,										// (i)チャンネル
	const BYTE			BaseKeyNo)									// (i)基準MIDIキー番号(VRC7用)
{
	static const double Nes_System_Clock ( NES_SYSTEM_CLOCK );		// ファミコンのシステムクロック
	double	Freq = 440 * pow( 2, ( KeyNo - 69 ) / 12. );			// 音声周波素
	int		NoteFrequency;											// 音符の周波数（レジスタ値）
	switch ( Ch ) {													// チャンネル分岐
	case 'C':
		Freq /= 2;													// 音声周波数を半分に（1オクターブ低い）
		// 周波数（レジスタ値）＝ 整数化（システムクロック／音符の周波数）を下位5ビットカット
		NoteFrequency = static_cast<int>( Nes_System_Clock / Freq ) >> 5;
		break;
	case 'O':
		// 周波数（レジスタ値）＝ 整数化（システムクロック／音符の周波数）／14
		NoteFrequency = static_cast<int>( Nes_System_Clock / Freq ) / 14;
		break;
	case 'X':
	case 'Y':
	case 'Z':
		// 周波数（レジスタ値）＝ 整数化（システムクロック／２／音符の周波数）を下位4ビットカット
		NoteFrequency = static_cast<int>( NES_SYSTEM_CLOCK / 2 / Freq ) >> 4;
		break;
	case 'F':
		// 周波数（レジスタ値）＝ 整数化（音符の周波数／（システムクロック／65536／64））
		NoteFrequency = static_cast<int>( Freq / (NES_SYSTEM_CLOCK / 65536 / 64) );
		break;
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
		Freq /= 2;													// 音声周波数を半分に（16サンプルでは1オクターブ低い）
		// 周波数（レジスタ値）＝ 整数化（音声周波数(Hz) × $40000 × 45 × 有効チャンネル数 × サンプル数 ÷ マスタークロック）
		NoteFrequency = static_cast<int>( Freq * (0x40000 * 45 * 8 * 16 / NES_MASTER_CLOCK) ) >> 7;	// SA6用の設定
		break;
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
		// 周波数（レジスタ値）＝ 整数化（音声周波数(Hz) × 2＾(18－オクターブ) ÷ (システムクロック÷72)）
//		NoteFrequency = static_cast<int>( Freq * pow ( 2., (18 - 1 - static_cast<int>( BaseKeyNo / 12 ) ) ) / ((NES_SYSTEM_CLOCK / 72) );
//		break;
// ☆ダメダメなので代わりに矩形波の計算式で計算する↓
	case 'A':
	case 'B':
	case 'a':
	case 'b':
	case 'M':
	case 'N':
	default :
		// 周波数（レジスタ値）＝ 整数化（システムクロック／音符の周波数）を下位4ビットカット
		NoteFrequency = static_cast<int>( Nes_System_Clock / Freq ) >> 4;
		break;
	}
	// ビット範囲チェック
	if ( 0 > NoteFrequency ) {
		NoteFrequency = 0;
/*	} else {
		int	max;													// 最大値
		switch ( Ch ) {												// チャンネル
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':	// N106
			max = 262143;											// 18bit
			break;
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':	// VRC7
			max = 511;												// 9bit
			break;
		case 'F':	// FDS
		case 'M':
		case 'N':
		case 'O':	// VRC6
		case 'X':
		case 'Y':
		case 'Z':	// SUNSOFT 5B
			max = 4095;												// 12bit
			break;
		case 'A':
		case 'B':
		case 'C':	// 2A03
		case 'a':
		case 'b':	// MMC5
		default :	// その他
			max = 2047;												// 11bit
			break;
		}
		if ( max < NoteFrequency ) {								// 最大値を超えたら
			NoteFrequency = max;									// 範囲内に収める
		}
*/	}
	return static_cast<int>( NoteFrequency );
}
// ピッチ周波数（レジスタ値）計算
//	MIDIキー番号、チャンネル、ピッチベンドセンシティヴィティ、ピッチベンドに対応する周波数（レジスタ値）を求める処理
DWORD
Mml::PitchFrequencyCalc(											// ピッチ周波数（レジスタ値）計算
	const BYTE			&KeyNo,										// (i)MIDIキー番号
	const BYTE			&Ch,										// (i)チャンネル
	const BYTE			&PitchBendSensitivity,						// (i)ピッチベンドセンシティヴィティ
	const short			&PitchBend)									// (i)ピッチベンド(範囲-8192～0～8191)
{
	BYTE SensitiveKeyNo (KeyNo);									// センシティヴィティでのMIDIキー番号
	if ( 0 > PitchBend ) {											// 音符のキー番号＋ピッチベンドが負の数
		SensitiveKeyNo -= PitchBendSensitivity;						// センシティヴィティでのMIDIキー番号－＝ピッチベンドセンシティヴィティ
	} else {
		SensitiveKeyNo += PitchBendSensitivity;						// センシティヴィティでのMIDIキー番号＋＝ピッチベンドセンシティヴィティ
	}
	DWORD NoteFrequency = NoteFrequencyCalc (KeyNo, Ch, SensitiveKeyNo);// 音符の周波数（レジスタ値）計算
	DWORD SensitiveFrequency = NoteFrequencyCalc (SensitiveKeyNo, Ch, SensitiveKeyNo);// ピッチベンドセンシティヴィティの周波数（レジスタ値）計算
	int  PitchBendFrequency;										// ピッチベンドの周波数差分
	if ( 0 > PitchBend ) {											// ピッチベンドが負の数なら
		PitchBendFrequency = static_cast<int>(						// ピッチベンドの周波数差分＝整数化（
			static_cast<int>(NoteFrequency - SensitiveFrequency) *	// （音符の周波数　－　ピッチベンドセンシティヴィティの周波数）×
			(PitchBend / 8192.) );									// （ピッチベンド　／　8192(ピッチベンド最大値)））
	} else {														// ピッチベンドが正の数なら
		PitchBendFrequency = static_cast<int>(						// ピッチベンドの周波数差分＝整数化（
			-static_cast<int>(NoteFrequency - SensitiveFrequency) *	// －（音符の周波数　－　ピッチベンドセンシティヴィティの周波数）×
			(PitchBend / 8191.) );									// （ピッチベンド　／　8191(ピッチベンド最大値)））
	}
	switch ( Ch ) {
	case 'F':	// 音声周波数とレジスタ値が比例するチャンネル
//	case 'G':
//	case 'H':
//	case 'I':
//	case 'J':
//	case 'K':
//	case 'L':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
		PitchBendFrequency = -PitchBendFrequency;					// プラマイ反転
	}
	return NoteFrequency + PitchBendFrequency;						// ピッチ周波数（レジスタ値）＝音符の周波数＋ピッチベンドの周波数差分
}


// ピッチエンヴェロープ設定
void
Mml::SetPitchEnvelope (												// ピッチエンヴェロープ設定
	vector<char>	&PitchEnvelope,									// (io)ピッチエンべロープ
	DWORD			Frame,											// (i)フレーム
	int				PitchDiff)										// (i)ピッチ周波数（レジスタ値）差分
{
	for ( ; (Frame < 511) && (PitchDiff != 0) ; ++Frame ) {			// フレームが511未満、かつ、ピッチ周波数差分が０以外である間ループ
		if ( Frame >= PitchEnvelope.size() ) {						// フレームの要素が存在しないなら
			PitchEnvelope.resize( Frame +1 );						// ピッチエンべロープの要素を追加
		}
		PitchDiff += PitchEnvelope[ Frame ];						// ピッチ周波数差分にピッチエンべロープ[フレーム]の値を足す
		if ( PitchDiff < 0 ) {										// ピッチ周波数差分がマイナス値なら
			if ( PitchDiff < -127 ) {								// ピッチ周波数差分が－１２８より小さければ
				PitchEnvelope[ Frame ] = -127;						// ピッチエンヴェロープ[フレーム]に－１２８を設定し
				PitchDiff += 127;									// その分ピッチ周波数差分に１２８を足す
			} else {												// ピッチ周波数差分が－１２８以上なら
				PitchEnvelope[ Frame ] = PitchDiff;					// ピッチエンヴェロープ[フレーム]にピッチ周波数差分を設定し
				PitchDiff = 0;										// その分ピッチ周波数差分を０にする
			}
		} else {													// ピッチ周波数差分がプラス値なら
			if ( PitchDiff > 126 ) {								// ピッチ周波数差分が１２７より大きければ
				PitchEnvelope[ Frame ] = 126;						// ピッチエンヴェロープ[フレーム]に１２７を設定し
				PitchDiff -= 126;									// その分ピッチ周波数差分に１２７を引く
			} else {												// ピッチ周波数差分が１２７以下なら
				PitchEnvelope[ Frame ] = PitchDiff;					// ピッチエンヴェロープ[フレーム]にピッチ周波数差分を設定し
				PitchDiff = 0;										// その分ピッチ周波数差分を０にする
			}
		}
	}
}
// 音符長を数える
void
Mml::UseLenCnt(														// 音符長を数える
	map<BYTE,DWORD>	&UseLen,										// (io)音符長の数
	const DWORD		&NoteLen,										// (i)音符の長さ
	const WORD		&WholeNote)										// (i)全音符の長さ
{
	DWORD ParNote = WholeNote / NoteLen;							// 何分音符か求める
	if ( 1 > ParNote ) {											// 全音符以上なら
		UseLen[0]+= NoteLen / WholeNote;							// 全音符の数をカウント
	} else {														// それ以外
		int sh;														// シフト数
		for (sh = 0; ParNote >> sh; ++sh);							// 最上位ビットの位置を探索
		++UseLen[--sh];												// 使用レングスカウント
	}
}
// 音質データ登録
//	指定したプログラム番号の登録を確認し、未登録なら登録する。
void
Mml::Tone::Set (													// 音質データ登録
	const BYTE	&Ch,												// (i)チャンネル
	const BYTE	&PrgNo,												// (i)プログラム番号
	const BYTE	Volume)												// (i)音量
{
	// 音色(@@コマンド、その他)
	WORD	ShiftNo(PrgNo);											// 重複しないプログラム番号
	map<WORD,string>::iterator it;									// イテレータ
	switch ( Ch ) {
	case 'A':
	case 'B':
	case 'a':
	case 'b':	// 矩形波Duty0～3用
		it = ColorCmdABabMN.find( PrgNo );							// 音色の登録を確認
		if ( (it == ColorCmdABabMN.end()) && (127 >= SerialNoABabMN) ) {// 登録されておらず、最大登録件数以内なら
			ostringstream	cmd;									// コマンド作成
			cmd << "@@" << SerialNoABabMN;
			ColorCmdABabMN[ PrgNo ] = cmd.str();					// コマンド追加
			ostringstream	def;									// 定義作成
			def << "@" << SerialNoABabMN << "  \t={"
				<< ( ToneDefVctSquare[PrgNo].empty() ? "1": ToneDefVctSquare[PrgNo])
				<< "}\t\t\t\t\t\t\t\t\t\t" << CommentEdit( Ch, PrgNo );
			ColorDefABabMN[ SerialNoABabMN ] = def.str();			// 定義追加
			++SerialNoABabMN;										// 管理番号カウントアップ
		}
		break;
	case 'M':
	case 'N':	// 矩形波Duty0～7用
		ShiftNo = 128 + PrgNo;										// 重複しないプログラム番号
		it = ColorCmdABabMN.find( ShiftNo );						// 音色の登録を確認
		if ( (it == ColorCmdABabMN.end()) && (127 >= SerialNoABabMN) ) {// 登録されておらず、最大登録件数以内なら
			ostringstream	cmd;									// コマンド作成
			cmd << "@@" << SerialNoABabMN;
			ColorCmdABabMN[ ShiftNo ] = cmd.str();					// コマンド追加
			ostringstream	def;									// 定義作成
			def << "@" << SerialNoABabMN << "  \t={"
				<< ( ToneDefVctVRC6[PrgNo].empty() ? "3": ToneDefVctVRC6[PrgNo])
				<< "}\t\t\t\t\t\t\t\t\t\t" << CommentEdit( Ch, PrgNo );
			ColorDefABabMN[ SerialNoABabMN ] = def.str();			// 定義追加
			++SerialNoABabMN;										// 管理番号カウントアップ
		}
		break;
	case 'D':	// ノイズ用
		ShiftNo = ParMapChg( PrgNo );								// パーカッション未登録部分をマップ
		it = NoteCmdD.find( ShiftNo );								// 音符の登録を確認
		if ( it == NoteCmdD.end() ) {								// 登録されていなければ
			ostringstream	note;									// 音符コマンド作成
			note << ToneDefVctNoize[PrgNo];							// 音色定義ノイズを取得
			NoteCmdD[ ShiftNo ] = note.str();						// 音符コマンド追加
			ostringstream	cmd;									// コマンド作成
			cmd << "D255";
			ColorCmdD[ ShiftNo ] = cmd.str();						// コマンド追加
			++SerialNoD;											// 管理番号カウントアップ
		}
		break;
	case 'E':	// DPCM用
		ShiftNo = ParMapChg( PrgNo );								// パーカッション未登録部分をマップ
		it = NoteCmdE.find( ShiftNo );								// 音符の登録を確認
		if ( (it == NoteCmdE.end()) && (63 >= SerialNoE) ) {		// 登録されておらず、最大登録件数以内なら
			ostringstream	note;									// 音符コマンド作成
			note << "n" << SerialNoE;
			NoteCmdE[ ShiftNo ] = note.str();						// 音符コマンド追加
			ostringstream	def;									// 定義作成
			def << "@DPCM" << SerialNoE << "\t={\"dmc\\" << (WORD)ShiftNo << ".dmc\"\t,15}\t\t\t\t\t\t// Ch.E\t\t\tPrgNo." << (WORD)ShiftNo << "\t" << DRUM_NAME[(WORD)ShiftNo] << "\n";
			ColorDefE[ SerialNoE ] = def.str();						// 定義追加
			++SerialNoE;											// 管理番号カウントアップ
		}
		break;
	case 'F':	// FDS用
		it = ColorCmdF.find( PrgNo );								// 音色の登録を確認
		if ( it == ColorCmdF.end() ) {								// 登録されていなければ
			ostringstream	cmd;									// コマンド作成
			cmd << "@@" << SerialNoF;
			ColorCmdF[ PrgNo ] = cmd.str();							// コマンド追加
			ostringstream	def;									// 定義作成
			def << "@FM" << SerialNoF << "\t={\t\t\t\t\t\t\t" << CommentEdit( Ch, PrgNo )
				<< "		  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,\n"
				<< "		  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,\n"
				<< "		  00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,\n"
				<< "		  00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00}\n";
			ColorDefF[ SerialNoF ] = def.str();						// 定義追加
			++SerialNoF;											// 管理番号カウントアップ
		}
		break;
	case 'G':	// VRC7用 Gチャンネルでのみ、ユーザー音色のみを登録する
		it = ColorCmdGHIJKL.find( PrgNo );							// 音色の登録を確認
		if ( it == ColorCmdGHIJKL.end() ) {							// 登録されていなければ
			if ( ! ToneDefVctVRC7[PrgNo].empty() ) {				// 音色定義VRC7が登録されていたら
				ostringstream	cmd;								// コマンド作成
				cmd << "OP" << SerialNoGHIJKL;						// 管理番号を設定
				ColorCmdGHIJKL[ PrgNo ]	= cmd.str();				// コマンド追加
				ostringstream	def;								// 定義作成
				def << "@OP" << SerialNoGHIJKL						// 管理番号を設定
					<< "\t={" << ToneDefVctVRC7[PrgNo]				// 音色定義VRC7を設定
					<< "}\t\t\t" << CommentEdit( Ch, PrgNo );		// コメントを設定
				ColorDefGHIJKL[ SerialNoGHIJKL ] = def.str();		// 定義追加
				++SerialNoGHIJKL;									// 管理番号カウントアップ
			}
		}
	case 'H':	// VRC7で、Gチャンネル以外は登録しない
	case 'I':
	case 'J':
	case 'K':
	case 'L':
		break;
	case 'W':	ShiftNo += 128;										// 同じ波形でもチャンネル毎にバッファ番号を変える必要がある為
	case 'V':	ShiftNo += 128;
	case 'U':	ShiftNo += 128;
	case 'T':	ShiftNo += 128;
	case 'S':	ShiftNo += 128;
	case 'R':	ShiftNo += 128;
	case 'Q':	ShiftNo += 128;
	case 'P':	// N106用
		it = ColorCmdPQRSTUVW.find( ShiftNo );						// 音色の登録を確認
		if ( (it == ColorCmdPQRSTUVW.end()) && (127 >= SerialNoPQRSTUVW) ) {// 登録されておらず、最大登録件数以内なら
			ostringstream	cmd;									// コマンド作成
			cmd << "@@" << SerialNoPQRSTUVW;
			ColorCmdPQRSTUVW[ ShiftNo ] = cmd.str();				// コマンド追加
			ostringstream	def;									// 定義作成
			if ( ! ToneDefVctN106[PrgNo].empty() ) {				// 定義が有るなら
				def << "@N" << SerialNoPQRSTUVW << " \t={" << Ch - 'P' << "," << ToneDefVctN106[PrgNo] << "}\t" << CommentEdit( Ch, PrgNo );
			} else {												// 定義が無いなら
				def << "@N" << SerialNoPQRSTUVW << " \t={" << Ch - 'P' << ",15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0}\t" << CommentEdit( Ch, PrgNo );
			}
			ColorDefPQRSTUVW[ SerialNoPQRSTUVW ] = def.str();		// 定義追加
			++SerialNoPQRSTUVW;										// 管理番号カウントアップ
		}
		break;
	case 'X':
	case 'Y':
	case 'Z':	// FME7用
		it = ColorCmdXYZ.find( PrgNo );								// 音色の登録を確認
		if ( it == ColorCmdXYZ.end() ) {							// 登録されていなければ
			ostringstream	cmd;									// コマンド作成
			cmd << "@1";
			ColorCmdXYZ[ PrgNo ] = cmd.str();						// コマンド追加
			++SerialNoXYZ;											// 管理番号カウントアップ
		}
		break;
	case 'C':	// 三角波
	case 'O':	// 鋸波
		// 音色無し
		break;
	default:
		cerr << "異常なチャンネル'" << Ch << "'がありました。" << endl;
		break;
	}
	// LFO(MPコマンド)
	switch ( Ch ) {
	case 'A':
	case 'B':
	case 'a':
	case 'b':
	case 'C':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':	// メロディLFO用
		it = LfoCmd.find( PrgNo );									// 音量(エンヴェロープ)の登録を確認
		if ( (it == LfoCmd.end()) && (63 >= SerialNoLfo) ) {		// 登録されておらず、最大登録件数以内なら
			ostringstream	cmd;									// コマンド作成
			cmd << "MP" << SerialNoLfo;
			LfoCmd[ PrgNo ] = cmd.str();							// コマンド追加
			ostringstream	def;									// 定義作成
			def << "@MP" << SerialNoLfo << "\t={29,2,1,0}\t\t\t\t\t\t\t\t\t" << CommentEdit( Ch, PrgNo );
			LfoDef[ SerialNoLfo ] = def.str();						// 定義追加
			++SerialNoLfo;											// 管理番号カウントアップ
		}
		break;
	case 'D':	// ノイズLFO用
		// 使わない
		break;
	case 'E':	// DPCM
		// LFO無し
		break;
	}
	// 音量(エンヴェロープ @vコマンド)
	if ( VOL_COLORFIT & VolMode ) {									// 音色音量モードなら
		ShiftNo = PrgNo;											// 重複しないプログラム番号
		switch ( Ch ) {
		case 'D':	// ノイズ用
			ShiftNo = ParMapChg( ShiftNo );							// DPCMマップ(未登録部分の置換)
			ShiftNo += 128;											// 重複しないプログラム番号
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':	// VRC7用
			ShiftNo += 128;											// 重複しないプログラム番号
		case 'F':
		case 'O':	// Volume 0～63用
			ShiftNo += 128;											// 重複しないプログラム番号
		case 'A':
		case 'B':
		case 'a':
		case 'b':
		case 'M':
		case 'N':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':	// Volume 0～15用
			{
				WORD Vol = VolumeConvert( Ch, Volume);				// 音量をMMLの音量に変換
				map<WORD,map<WORD,VolumeInfo>>::iterator VDMit		// 音量(エンヴェロープ)イテレータを作成
					= VolumeDef.find( ShiftNo );					// 音量(エンヴェロープ)を検索
				if ( VDMit == VolumeDef.end()) {					// 音量(エンヴェロープ)が登録されていない場合
					map<WORD,VolumeInfo>	VolMap;					// 音量マップを作成
					VolumeInfo				VolInfo;				// 音量情報を作成
					VolInfo.Ch =			Ch;						// チャンネルを登録
					VolMap[ Vol ] =			VolInfo;				// 音量マップに音量情報を登録
					VolumeDef[ ShiftNo ] =	VolMap;					// 音量定義マップに音量マップを登録
				} else {											// 音量(エンヴェロープ)が登録されている場合
					map<WORD,VolumeInfo>	&VolMap = (*VDMit).second;// 音量マップの参照
					if ( VolMap.find( Vol ) == VolMap.end() ) {		// 音量マップが登録されていない場合
						VolumeInfo			VolInfo;				// 音量情報を作成
						VolInfo.Ch =		Ch;						// チャンネルを登録
						VolMap[ Vol ] =		VolInfo;				// 音量マップに音量情報を登録
					}
				}
			}
		case 'C':
		case 'E':	// 音量無し
		default:
			break;
		}
	}
}
// 音量変換
//	MIDIのベロシティからMMLの各チャンネルに対応した音量に変換する
BYTE
Mml::Tone::VolumeConvert (											// 音量変換
	const BYTE	&Ch,												// チャンネル
	const BYTE	&Vel)												// ベロシティ
{
	BYTE	Vol(Vel);												// 音量
	switch ( Ch ) {
	case 'A':
	case 'B':
	case 'a':
	case 'b':
		Vol = static_cast<BYTE>(Vol * .097);
		break;
	case 'M':
	case 'N':	// 矩形波
		Vol = static_cast<BYTE>(Vol * .097);
		break;
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':	// VRC7用
		Vol = static_cast<BYTE>(Vol * .097);
		break;
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':	// N106
		Vol = static_cast<BYTE>(Vol * .1255);
		break;
	case 'X':
	case 'Y':
	case 'Z':	// FME7用
		Vol = static_cast<BYTE>(Vol * .111);
		if ( 15 < Vol ) Vol = 15;
		break;
	case 'F':	// FDS用
	case 'O':	// 鋸波
		Vol = static_cast<BYTE>(Vol * .3);
		break;
	case 'D':	// ノイズ用
		Vol =  static_cast<BYTE>(Vol * .17);
		if (Vol > 15) {
			Vol = 15;
		}
		break;
	default:	// その他
		Vol >>= 3;
		break;
	}
	return Vol;
}
// 音量割当て
void
Mml::Tone::AssignVolume (											// 音量割当て
	const WORD		VolDefThreshold )								// (i)音量登録数間引き閾値
{
	// 音色定義割当て
	WORD	SerialNo(0);											// 管理番号
	for ( map<WORD,map<WORD,VolumeInfo>>::iterator VDMit = VolumeDef.begin();// 先頭から
		  VDMit != VolumeDef.end();									// 最後まで
		  ++VDMit) {												// 進める
		map<WORD,VolumeInfo>	&VolMap = (*VDMit).second;			// 音量マップの参照
		for ( map<WORD,VolumeInfo>::iterator VolMapIt = VolMap.begin();	// 先頭から
			  VolMapIt != VolMap.end();								// 最後まで
			  ++VolMapIt) {											// 進める
			VolumeInfo	&VolInfo = (*VolMapIt).second;				// 音量情報の参照
			VolInfo.SerialNo = SerialNo++;							// 管理番号を割当て
			switch ( VolInfo.Ch ) {
			case 'D':	// ノイズチャンネルなら
				VolInfo.Define =									// 音量定義を割当て
					AdjustVolume( static_cast<float>( (*VolMapIt).first / 15.),
					( VolDefVctNoise[ (*VDMit).first & 0x7F ].empty()?
					"15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0":
					VolDefVctNoise[ (*VDMit).first & 0x7F ]) );		// 音量定義配列[音色別]を音量調節する
				break;
			case 'G':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
			case 'L':	// VRC7用
				VolInfo.Define =									// 音量定義を割当て
					AdjustVolume( static_cast<float>( (*VolMapIt).first / 15.),
					( VolDefVctVRC7Pre[ (*VDMit).first & 0x7F ].empty()?
					"15":
					VolDefVctVRC7Pre[ (*VDMit).first & 0x7F ]) );	// 音量定義配列[音色別]を音量調節する
				break;
			case 'O':
			case 'F':	// 音量64のチャンネルなら
				VolInfo.Define =									// 音量定義を割当て
					AdjustVolume( static_cast<float>( (*VolMapIt).first / 15.),	// ←15で正しい(VolDefVctCommonの15段階から64段階に変換するため)
					( VolDefVctCommon[ (*VDMit).first & 0x7F ].empty()?
					"15 14 14 13 13 13 12 12 12 11":
					VolDefVctCommon[ (*VDMit).first & 0x7F ]) );	// 音量定義配列[音色別]を音量調節する
				break;
			default:	// それ以外
				VolInfo.Define =									// 音量定義を割当て
					AdjustVolume( static_cast<float>( (*VolMapIt).first / 15.),
					( VolDefVctCommon[ (*VDMit).first & 0x7F ].empty()?
					"15 14 14 13 13 13 12 12 12 11":
					VolDefVctCommon[ (*VDMit).first & 0x7F ]) );	// 音量定義配列[音色別]を音量調節する
				break;
			}
		}
	}
	// 音色音量モード用の設定
	if ( VOL_COLORFIT == VolMode ) {								// 音量モードが音色音量モードの場合
		VolDefMask = 0x7;											// 最初から0～14を間引きする設定にする
		SerialNo = 128;												// 間引き処理を必ずやる設定にする
	}
	// 間引き処理
	while ( (VolDefThreshold <= SerialNo) && (0x10 > VolDefMask) ) {// 登録件数が多過ぎで間引きが可能なら
		VolDefMask = (VolDefMask << 1) | 0x1;						// 音量マスクのビットを増やす
		SerialNo = 0;												// 管理番号を最初から
		for ( map<WORD,map<WORD,VolumeInfo>>::iterator VDMit = VolumeDef.begin();// 先頭から
			  VDMit != VolumeDef.end();								// 最後まで
			  ++VDMit) {											// 進める
			map<WORD,VolumeInfo>	&VolMap = (*VDMit).second;		// 音量マップの参照
			for ( map<WORD,VolumeInfo>::iterator VolMapIt = VolMap.begin();	// 先頭から
				  VolMapIt != VolMap.end();							// 最後まで
				  ) {												// ここでは進めない
				VolumeInfo	&VolInfo = (*VolMapIt).second;			// 音量情報の参照
				if ( (~(*VolMapIt).first) & VolDefMask ) {			// この定義が間引きの対象なら
					BYTE DestVol = (*VolMapIt).first | VolDefMask;	// 移動先音量
					map<WORD,VolumeInfo>::iterator fit =			// 検索用イテレータ
						VolMap.find( DestVol );						// 移動先音量定義の存在を確認
					if ( fit == VolMap.end() ) {					// 無ければ
						VolMap[ DestVol ] = VolInfo;				// この定義をコピー
					}
					VolMapIt = VolMap.erase( VolMapIt );			// この定義を削除
				} else {											// この定義が残す対象なら
					VolInfo.SerialNo = SerialNo++;					// 管理番号を割当て、カウント
					++VolMapIt;										// 次へ
				}
			}
		}
	}
}
// 音量調節
string
Mml::Tone::AdjustVolume (											// 音量調節
	const float		&Ratio,											// (i)割合
	const string	&Define)										// (i)定義
{
	string		EditDef( Define );									// 編集用
	replace( EditDef.begin(), EditDef.end(), ',', ' ' );			// ','をスペースに変換
	istringstream iss( EditDef );									// 文字列切り出し
	ostringstream oss;												// 文字列編集
	while ( !iss.fail() && !iss.eof() ) {							// エラーでも終端でもないあいだループ
		WORD	val;												// 数値
		iss >> val;													// 読み込み
		if ( iss.fail() ) {											// エラーなら
			iss.clear();											// エラーをクリア
			string	token;											// 文節
			iss >> token;											// 文字列として読み込み
			oss << token;											// そのまま出力
		} else {													// エラーがなければ
			val = static_cast<WORD>(val * Ratio);					// 割合を掛ける
			oss << val;												// 出力
		}
		if ( !iss.eof() ) {											// 終端でなければ
			oss << ",";												// 区切りを出力
		}
	}
	return oss.str();												// 音量調節した定義を返す
}
// 音質コマンド取得
//	登録されたコマンドを返す。
string
Mml::Tone::Get (													// 音質データ取得
	const BYTE	&Ch,												// (i)チャンネル
	const BYTE	&PrgNo)												// (i)プログラム番号
{
	string	CmdStr;													// コマンド文字列return用
	WORD	ShiftNo(PrgNo);											// 重複しないプログラム番号
	map<WORD,string>::iterator it;									// イテレータ
	switch ( Ch ) {
	case 'M':
	case 'N':	// 矩形波Duty0～7用
		ShiftNo += 128;												// 重複しないプログラム番号
	case 'A':
	case 'B':
	case 'a':
	case 'b':	// 矩形波Duty0～3用
		it = ColorCmdABabMN.find( ShiftNo );						// 音色の登録を確認
		if ( it != ColorCmdABabMN.end() ) {							// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		} else {													// 未登録なら
			ostringstream oss;										// メッセージ編集
			oss << "/* 未登録PrgNo=" << (WORD)PrgNo << " */";
			CmdStr = oss.str();										// メッセージを取得
		}
		break;
	case 'D':	// ノイズ用
		ShiftNo = ParMapChg( PrgNo );								// パーカッション未登録部分をマップ
		it = ColorCmdD.find( ShiftNo );								// 音色の登録を確認
		if ( it != ColorCmdD.end() ) {								// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		}
		break;
	case 'F':	// FDS用
		it = ColorCmdF.find( PrgNo );								// 音色の登録を確認
		if ( it != ColorCmdF.end() ) {								// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		} else {													// 未登録なら
			ostringstream oss;										// メッセージ編集
			oss << "/* 未登録PrgNo=" << (WORD)PrgNo << " */";
			CmdStr = oss.str();										// メッセージを取得
		}
		break;
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':	// VRC7用
		it = ColorCmdGHIJKL.find( PrgNo );							// 音色の登録を確認
		if ( it != ColorCmdGHIJKL.end() ) {							// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		//} else {													// 未登録なら
			// CmdStr = "";											// 空を返す（受け取り側で未登録を判断するため）
		}
		break;
	case 'W':	ShiftNo += 128;										// 同じ波形でもチャンネル毎にバッファ番号を変える必要がある為
	case 'V':	ShiftNo += 128;
	case 'U':	ShiftNo += 128;
	case 'T':	ShiftNo += 128;
	case 'S':	ShiftNo += 128;
	case 'R':	ShiftNo += 128;
	case 'Q':	ShiftNo += 128;
	case 'P':	// N106用
		it = ColorCmdPQRSTUVW.find( ShiftNo );						// 音色の登録を確認
		if ( it != ColorCmdPQRSTUVW.end() ) {						// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		} else {													// 未登録なら
			ostringstream oss;										// メッセージ編集
			oss << "/* 未登録PrgNo=" << (WORD)PrgNo << " */";
			CmdStr = oss.str();										// メッセージを取得
		}
		break;
	case 'X':
	case 'Y':
	case 'Z':	// FME7用
		it = ColorCmdXYZ.find( PrgNo );								// 音色の登録を確認
		if ( it != ColorCmdXYZ.end() ) {							// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		} else {													// 未登録なら
			ostringstream oss;										// メッセージ編集
			oss << "/* 未登録PrgNo=" << (WORD)PrgNo << " */";
			CmdStr = oss.str();										// メッセージを取得
		}
		break;
	case 'E':	// DPCM
	case 'C':	// 三角波
	case 'O':	// 鋸波
		// 音色無し
		break;
	default:
		break;
	}
	// LFO(MPコマンド)
	switch ( Ch ) {
	case 'A':
	case 'B':
	case 'a':
	case 'b':
	case 'C':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':	// メロディLFO用
		it = LfoCmd.find( PrgNo );									// 音量(エンヴェロープ)の登録を確認
		if ( it != LfoCmd.end() ) {									// 登録されていたら
			CmdStr += (*it).second;									// コマンドを取得
		} else {													// 未登録なら
			ostringstream oss;										// メッセージ編集
			oss << "/* Lfo未登録PrgNo=" << (WORD)PrgNo << " */";
			CmdStr += oss.str();									// メッセージを取得
		}
		break;
	case 'D':	// ノイズLFO用
		// 使わない
		break;
	case 'E':	// DPCM
		// LFO無し
		break;
	}
	return CmdStr;													// コマンドを返す
}
// 音符コマンド取得
//	登録された音符コマンドを返す
string
Mml::Tone::GetNote (												// 音符コマンド取得
	const BYTE	&Ch,												// (i)チャンネル
	const BYTE	&PrgNo)												// (i)プログラム番号
{
	string	CmdStr;													// コマンド文字列return用
	WORD	ShiftNo(PrgNo);											// 重複しないプログラム番号
	map<WORD,string>::iterator it;									// イテレータ
	// 音符
	switch ( Ch ) {
	case 'D':	// ノイズ用
		ShiftNo = ParMapChg( PrgNo );								// パーカッション未登録部分をマップ
		it = NoteCmdD.find( ShiftNo );								// 音符の登録を確認
		if ( it != NoteCmdD.end() ) {								// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		} else {													// 未登録なら
			ostringstream oss;										// メッセージ編集
			oss << "/* 未登録PrgNo=" << (WORD)PrgNo << " */";
			CmdStr = oss.str();										// メッセージを取得
		}
		break;
	case 'E':	// DPCM用
		ShiftNo = ParMapChg( PrgNo );								// パーカッション未登録部分をマップ
		it = NoteCmdE.find( ShiftNo );								// 音符の登録を確認
		if ( it != NoteCmdE.end() ) {								// 登録されていたら
			CmdStr = (*it).second;									// コマンドを取得
		} else {													// 未登録なら
			ostringstream oss;										// メッセージ編集
			oss << "/* 未登録PrgNo=" << (WORD)PrgNo << " */";
			CmdStr = oss.str();										// メッセージを取得
		}
		break;
	default:	// それ以外
		// 直接指定するので、ここでは取らない
		break;
	}
	return CmdStr;													// コマンド２を返す
}
// 音量コマンド取得
//	登録された音量コマンドを返す
string
Mml::Tone::GetVolume (												// 音量コマンド取得
	const BYTE	&Ch,												// (i)チャンネル
	const BYTE	&PrgNo,												// (i)プログラム番号
	const BYTE	&Volume)											// (i)音量
{
	// 音量(エンヴェロープ @vコマンド)
	string	CmdStr;													// コマンド文字列return用
	// 音量(エンヴェロープ @vコマンド)
	if ( VOL_COLORFIT & VolMode ) {									// 音色音量モードなら
		WORD	ShiftNo( PrgNo );									// 重複しないプログラム番号
		switch ( Ch ) {
		case 'D':	// ノイズ用
			ShiftNo = ParMapChg( ShiftNo );							// DPCMマップ(未登録部分の置換)
			ShiftNo += 128;											// 重複しないプログラム番号
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':	// VRC7用
			ShiftNo += 128;											// 重複しないプログラム番号
		case 'F':
		case 'O':	// Volume 0～63用
			ShiftNo += 128;											// 重複しないプログラム番号
		case 'A':
		case 'B':
		case 'a':
		case 'b':
		case 'M':
		case 'N':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':	// Volume 0～15用
			{
				map<WORD,map<WORD,VolumeInfo>>::iterator VDMit		// 音量(エンヴェロープ)イテレータを作成
					= VolumeDef.find( ShiftNo );					// 音量(エンヴェロープ)を検索
				ostringstream oss;									// 文字列編集用
				if ( VDMit != VolumeDef.end()) {					// 音量(エンヴェロープ)が登録されている場合
					map<WORD,VolumeInfo>	&VolMap = (*VDMit).second;// 音量マップの参照
					map<WORD,VolumeInfo>::iterator VolMapIt			// 音量マップイテレータを作成
						= VolMap.find( Volume | VolDefMask );		// 音量マップを検索
					if ( VolMapIt != VolMap.end() ) {				// 音量マップが登録されている場合
						VolumeInfo	&VolInfo = (*VolMapIt).second;	// 音量情報の参照
						oss << "@v" << VolInfo.SerialNo;			// コマンド編集
					} else {
						oss << "/* 音量コマンド取得で未登録PrgNo." << (WORD)(0x7F & PrgNo) << " Vol." << (WORD)Volume << " */";
					}
				} else {											// 音量(エンヴェロープ)が登録されていない場合
					oss << "/* 音量コマンド取得で未登録PrgNo." << (WORD)PrgNo << " */";
				}
				CmdStr = oss.str();									// コマンドを取得
			}
		case 'C':
		case 'E':	// 音量無し
		default:
			break;
		}
	}
	return CmdStr;													// コマンド２を返す
}
// DPCMマップ(未登録部分の置換)
WORD
Mml::Tone::ParMapChg (												// DPCMマップ(未登録部分の置換)
	const WORD	&PrgNo)												// (i)プログラム番号
{
	switch ( PrgNo ) {												// プログラム番号で分岐
	case   0:	return 36;											// Standard 1 Kick 1 → Bass Drum 1
	case   1:	return 35;											// Standard 1 Kick 2 → Bass Drum 2
	case   2:	return 36;											// Standard 2 Kick 1 → Bass Drum 1
	case   3:	return 35;											// Standard 2 Kick 2 → Bass Drum 2
	case   4:	return 36;											// Kick Drum 1 → Bass Drum 1
	case   5:	return 35;											// Kick Drum 2 → Bass Drum 2
	case   6:	return 36;											// Jazz Kick 1 → Bass Drum 1
	case   7:	return 35;											// Jazz Kick 2 → Bass Drum 2
	case   8:	return 36;											// Room Kick 1 → Bass Drum 1
	case   9:	return 35;											// Room Kick 2 → Bass Drum 2
	case  10:	return 36;											// Power Kick 1 → Bass Drum 1
	case  11:	return 35;											// Power Kick 2 → Bass Drum 2
	case  12:	return 35;											// Electric Kick 2 → Bass Drum 2
	case  13:	return 36;											// Electric Kick 1 → Bass Drum 1
	case  14:	return 36;											// TR-808 Kick → Bass Drum 1
	case  15:	return 35;											// TR-909 Kick → Bass Drum 2
	case  16:	return 36;											// Dance Kick → Bass Drum 1
	case  17:	return 83;											// Voice One → Jingle Bell
	case  18:	return 83;											// Voice Two → Jingle Bell
	case  19:	return 83;											// Voice Three → Jingle Bell
	case  20:	return 83;											// ? → Jingle Bell
	case  21:	return 83;											// ? → Jingle Bell
	case  22:	return 47;											// MC-500 Beep1 → Long Whistle
	case  23:	return 47;											// MC-500 Beep2 → Long Whistle
	case  24:	return 52;											// Concert DS → Chinese Cymbal
	case  88:	return 72;											// Applause 2 → Long Whistle
	case  89:	return 78;											// Mute Cuica → Mute Cuica
	case  90:	return 79;											// Open Cuica → Open Cuica
	case  91:	return 80;											// Mute Triangle → Mute Triangle
	case  92:	return 81;											// Open Triangle → Open Triangle
	case  93:	return 73;											// Short Guiro → Short Guiro
	case  94:	return 74;											// Long Guiro → Long Guiro
	case  95:	return 69;											// Cabasa Up → Cabasa
	case  96:	return 69;											// Cabasa Down → Cabasa
	case  97:	return 38;											// Standard 1 Snare 1 → Snare Drum 1
	case  98:	return 40;											// Standard 1 Snare 2 → Snare Drum 2
	case  99:	return 38;											// Standard 2 Snare 1 → Snare Drum 1
	case 100:	return 40;											// Standard 2 Snare 2 → Snare Drum 2
	case 101:	return 40;											// Snare Drum 2 → Snare Drum 2
	case 102:	return 38;											// Standard 1 Snare 1 → Snare Drum 1
	case 103:	return 40;											// Standard 1 Snare 2 → Snare Drum 2
	case 104:	return 40;											// Standard 1 Snare 3 → Snare Drum 2
	case 105:	return 38;											// Jazz Snare 1 → Snare Drum 1
	case 106:	return 40;											// Jazz Snare 2 → Snare Drum 2
	case 107:	return 38;											// Room Snare 1 → Snare Drum 1
	case 108:	return 40;											// Room Snare 2 → Snare Drum 2
	case 109:	return 38;											// Power Snare 1 → Snare Drum 1
	case 110:	return 40;											// Power Snare 2 → Snare Drum 2
	case 111:	return 38;											// Gated Snare → Snare Drum 1
	case 112:	return 38;											// Dance Snare 1 → Snare Drum 1
	case 113:	return 40;											// Dance Snare 2 → Snare Drum 2
	case 114:	return 38;											// Disco Snare → Snare Drum 1
	case 115:	return 40;											// Electric Snare 2 → Snare Drum 2
	case 116:	return 38;											// Electric Snare → Snare Drum 1
	case 117:	return 40;											// Electric Snare 3 → Snare Drum 2
	case 118:	return 38;											// TR-707 Snare 1 → Snare Drum 1
	case 119:	return 38;											// TR-808 Snare 1 → Snare Drum 1
	case 120:	return 40;											// TR-808 Snare 2 → Snare Drum 2
	case 121:	return 38;											// TR-909 Snare 1 → Snare Drum 1
	case 122:	return 40;											// TR-909 Snare 2 → Snare Drum 2
	case 123:	return 38;											// Rap Snare → Snare Drum 1
	case 124:	return 38;											// Jungle Snare 1 → Snare Drum 1
	case 125:	return 38;											// House Snare 1 → Snare Drum 1
	case 126:	return 40;											// House Snare → Snare Drum 2
	case 127:	return 40;											// House Snare 2 → Snare Drum 2
	}
	return PrgNo;
}
// ピッチエンヴェロープ定義登録
string																// 戻り値：EPコマンド文字列
Mml::Pitch::Regist (												// ピッチエンヴェロープ定義登録
	vector<char>		&Envelope,									// (io)エンヴェロープ
	const BYTE			Ch)											// (i)チャンネル
{
	// 末尾の０の連続を削除
	DWORD RegNo = EnvelopeDef.size();								// 登録番号
	for ( DWORD i = Envelope.size(); i > 0 ; ) {					// エンヴェロープのループ
		--i;														// １つ手前へ
		char &Val = Envelope[i];									// エンヴェロープの値を参照
		if ( Val ) {												// エンヴェロープの値が０以外なら
			break;													// ループを抜ける
		}
		Envelope.pop_back();										// その０を削除する
	}
	if ( Envelope.empty() ) {										// エンヴェロープが空なら
		return "EPOF";												// EPOFコマンド文字列を返す
	}
	// 定義文字列作成、及び、変化量最大最小の検索
	int Frequency(0), Max(0), Min(0);
	ostringstream oss;												// 文字列編集用
	for ( vector<char>::const_iterator it = Envelope.begin();		// エンヴェロープの先頭から
		  it != Envelope.end();										// エンヴェロープの最後まで
		  ++it ) {													// イテレータを進める
		oss << static_cast<int>(*it) << ",";						// エンヴェロープを編集
		Frequency += *it;											// 現在の相対周波数
		if ( Max < Frequency )	Max = Frequency;					// 最大値更新
		if ( Min > Frequency )	Min = Frequency;					// 最小値更新
	}
	// ピッチエンヴェロープ変化量チェック
	if ( mRegistThreshold > (Max - Min) ) {							// 変化量が、ピッチエンヴェロープ登録閾値（変化量下限）以下なら
		return "EPOF";												// EPOFコマンド文字列を返す
	}
	// 同じ登録内容を検索
	string DefStr = oss.str();										// 定義文字列を取り出し
	DWORD SameNo(0xFFFFFFFF);										// 同じ登録番号（不一致を設定）
	DWORD i;														// ループ用添字
	for ( i = 0; i < RegNo; ++i ) {									// 既に登録されているものをチェック
		if ( DefStr == EnvelopeDef[i] ) {							// 同じ登録内容なら
			SameNo = i;												// 登録番号に見つかった番号を取得
			break;
		}
	}
	// 検索結果により分岐
	if ( SameNo != i ) {											// 同じ登録内容が見つからない
		if ( RegNo < mRegistMax ) {									// 最大登録数未満なら
			EnvelopeDef.push_back( oss.str() );						// エンヴェロープ定義文字列を登録
			oss.str("");											// 文字列をクリアする。
			oss << "EP" << RegNo;									// コマンド文字列を編集
		} else {													// 最大登録数に達していたら
			oss.str("");											// 文字列をクリアする。
			oss << "EPOF";											// EPOFコマンド文字列を編集
		}
	} else {														// 同じ登録内容が見つかった場合
		oss.str("");												// 文字列をクリアする。
		oss << "EP" << SameNo;										// 見つかった登録番号のEPコマンド文字列を編集
	}
	return oss.str();												// コマンド文字列を返す
}
// ループ位置決定 --------------------------------------------------
//	'L'コマンドが複数ある場合、最後の"L"コマンドのみを残し、それ以外を削除
void
Mml::LoopPointConclusion ()
{
	int LastLPos = 0;												// Lコマンド最終位置
	{ // Lコマンド最終位置探索
		map<BYTE,ChInfo>::iterator ChIt = mChMap.begin();			// チャンネルマップの先頭
		// 音符ベクタ
		for ( vector<NoteInfo>::iterator NoteIt = (*ChIt).second.NoteVector.begin();// 音符ベクタの先頭から
			  NoteIt != (*ChIt).second.NoteVector.end();			// 最後まで
			  ++NoteIt) {											// イテレータを進める
			NoteInfo	&Note = (*NoteIt);							// 音符情報を参照
			if ( "L" == Note.Str ) {								// 音符情報がループなら
				++LastLPos;											// Lコマンド最終位置を更新
			}
		}
	}
	if ( 0 == LastLPos ) {											// Lコマンドが無ければ
		return;														// 終わり
	}
	// 最終Lコマンド以外を削除
	for ( map<BYTE,ChInfo>::iterator ChIt = mChMap.begin();			// チャンネルマップの先頭から
		  ChIt != mChMap.end();										// 最後まで
		  ++ChIt) {													// イテレータを進める
		int LPos = 0;												// Lコマンド位置
		for ( vector<NoteInfo>::iterator NoteIt = (*ChIt).second.NoteVector.begin();// 音符ベクタの先頭から
			  NoteIt != (*ChIt).second.NoteVector.end();			// 最後まで
			  ) {													// ここではイテレータを進めない
			NoteInfo	&Note = (*NoteIt);							// 音符情報を参照
			if ( "L" == Note.Str ) {								// 音符情報がLコマンドなら
				++LPos;												// Lコマンド位置更新
				if ( LastLPos == LPos ) {							// Lコマンド最終位置でないなら
					break;											// 音符ベクタループから抜ける
				}
				NoteIt = (*ChIt).second.NoteVector.erase( NoteIt );	// このLコマンドを削除
			} else {												// Lコマンド以外なら
				++NoteIt;											// イテレータを進める
			}
		}
	}
}
// セーブ ----------------------------------------------------------
//	MMLファイルに出力します。
extern const char* TOOLNAME;										// ツール名
extern const char* AUTHER;											// 著作者
int
Mml::Save (															// セーブ
	const char	*FilePath,											// (i)ファイルパス
	BYTE		OneLineBar )										// (i)一行に出力する小節数
{
	// 文字化けしないおなじない
	locale::global(locale("japanese"));								// ファイル名が文字化けしないおまじない
	// ファイルオープン
	ofstream    ofs( FilePath, ios::out );							// 出力ファイルストリーム
	if ( !ofs ) {
		cerr << "File Openできなかった" << endl;
 		return -1;
	}
	// タイトル情報が無い場合ファイル名をタイトル
	if ( mTitle.empty() ) {											// タイトルが空なら
		mTitle = mFileName;											// ファイル名をタイトルにする
	}
	// タイトル情報出力
	ofs << "// " << TOOLNAME << " " << AUTHER << endl;
	ofs << "//  mid2mml " << mFileName << " /c:" << mChStr << endl; // コマンド表示
	ofs << "#TITLE		" << mTitle << endl;						// タイトル情報
	ofs << "#COMPOSER	" << mComposer << endl;						// 作曲者名
	ofs << "#MAKER		" << mMaker << endl;						// 原著作者名
	ofs << "#PROGRAMER	" << TOOLNAME << " & " << mProgamer << endl;// 打ち込み者名
	// 固定宣言
	ofs << "#AUTO-BANKSWITCH 0" << endl;							// 自動バンク設定
	ofs << "#PITCH-CORRECTION" << endl;								// ピッチエンヴェロープ、LFO、ディチューン方向整順化
	// 音源別使用宣言
	PutChUseDef( ofs, "C"		, "#GATE-DENOM 16" );				// 三角波使用時はクオンタイズ分母指定
	PutChUseDef( ofs, "E"		, "#DPCM-RESTSTOP" );				// DPCM用宣言
	PutChUseDef( ofs, "F"		, "#EX-DISKFM" );					// FDS 音源使用宣言
	PutChUseDef( ofs, "GHIJKL"  , "#EX-VRC7" );						// VRC7音源使用宣言
	PutChUseDef( ofs, "MNO"		, "#EX-VRC6" );						// VRC6音源使用宣言
	PutChUseDef( ofs, "PQRSTUVW", "#EX-NAMCO106 8" );				// N106音源使用宣言
	PutChUseDef( ofs, "XYZ"		, "#EX-FME7" );						// FME7音源使用宣言
	PutChUseDef( ofs, "ab"		, "#EX-MMC5" );						// MMC5音源使用宣言
	// 音質定義出力
	mTone.PutToneDef( ofs, mTone.ColorDefABabMN );					// 音色：矩形波
	mTone.PutToneDef( ofs, mTone.ColorDefE );						// 音色：DPCM
	mTone.PutToneDef( ofs, mTone.ColorDefF );						// 音色：FDS
	mTone.PutToneDef( ofs, mTone.ColorDefGHIJKL );					// 音色：VRC7
	mTone.PutToneDef( ofs, mTone.ColorDefPQRSTUVW );				// 音色：N106
	mTone.PutToneDef( ofs, mTone.LfoDef );							// LFO
	if ( VOL_COLORFIT & mTone.VolMode ) {							// 音色音量モードなら
		mTone.PutVolumeDef( ofs, mTone.VolumeDef );					// 音量
	}
	// ピッチエンヴェロープ定義出力
	mPitch.PutDef( ofs );											// ピッチエンヴェロープ定義出力
	ofs << endl;													// 空行
	// 最初のテンポ出力
	if ( mFirstTempo && (120 != mFirstTempo) ) {					// テンポが取得できており、最初のテンポが120以外なら
		string ChStr;												// チャンネル文字列
		for ( map<BYTE,ChInfo>::iterator ChIt = mChMap.begin();		// チャンネルマップの先頭から
			  ChIt != mChMap.end();									// 最後まで
			  ++ChIt) {												// イテレータを進める
			string Str(" ");										// 文字列
			Str[0] = (*ChIt).first;									// チャンネルを文字列にセット
			ChStr += Str;											// チャンネル文字列に追加
		}
		ofs << ChStr << "\tt" << mFirstTempo << endl;				// テンポを出力
	}
	// チャンネル先頭のコマンド出力
	for ( map<BYTE,ChInfo>::iterator ChIt = mChMap.begin();			// チャンネルマップの先頭から
		  ChIt != mChMap.end();										// 最後まで
		  ++ChIt) {													// イテレータを進める
		string	ChStr("? ");										// 編集用チャンネル文字
		ChStr[0] = (*ChIt).first;									// チャンネルをセット
		ofs << ChStr;												// チャンネルを出力
		// デフォルトレングス
		if ( 4 != (*ChIt).second.DefaultLen ) {						// デフォルトレングスが４以外なら
			ofs << "l" << (WORD)(*ChIt).second.DefaultLen;			// 'l'コマンド出力
		} else {
			ofs << "\t";
		}
		ofs << "\t";
		// オクターブ
		if ( ('D' != ChStr[0]) && ('E' != ChStr[0]) ) {				// パーカッション以外
			ofs << "o" << (WORD)(*ChIt).second.FirstOctave;			// 'o'コマンド出力
		}
		ofs << "\t";
		// 音量
		if ( ('C' != ChStr[0]) && ('E' != ChStr[0]) ) {				// 音量のあるチャンネル
			if ( VOL_COLORFIT & mTone.VolMode ) {					// 音色音量モードなら
				ofs << mTone.GetVolume( ChStr[0],					// '@v'コマンド出力
					(*ChIt).second.FirstPrgNo, (WORD)(*ChIt).second.FirstVolume) << " ";
			} else {												// 音色音量モードでないなら
				ofs << "v" << (WORD)(*ChIt).second.FirstVolume;		// 'v'コマンド出力
			}
		} else {
			ofs << "\t";
		}
		// 音色
		if ( ('G' <= ChStr[0]) && ('L' >= ChStr[0]) ) {				// VRC7の場合
			string Cmd = mTone.Get( ChStr[0], (*ChIt).second.FirstPrgNo );
			if ( 'M' != Cmd[0] ) {									// ユーザー音色が登録されていたら
				ofs << "\t@@0";										// ユーザー音色を出力
//				if ( 'G' == ChStr[0] ) {							// Gチャンネルなら
					ofs	<< Cmd;										// 更にOPコマンドを出力
//				}
			} else {												// ユーザー音色が登録されてなければ
				ofs << "\t@@" << TONE_DEF_VRC7_PRESET[(*ChIt).second.FirstPrgNo];//プリセット音色を出力
			}
		} else if ('D' == ChStr[0]) {								// ノイズの場合
			ofs << "\t";
			if ("D255" != mTone.Get( ChStr[0], (*ChIt).second.FirstPrgNo ) ) {	// 音色が"D255"以外なら
				ofs << mTone.Get( ChStr[0], (*ChIt).second.FirstPrgNo );// 音色を出力
			}
		} else {													// その他チャンネルの場合
			ofs << "\t" << mTone.Get( ChStr[0], (*ChIt).second.FirstPrgNo );// 音色を出力
		}
		// ピッチシフト量
		if ( ('P' <= ChStr[0]) && ('W' >= ChStr[0]) ) {				// N106の場合
			ofs << "\tSA7";											// ピッチシフト量設定
		}
		// クォンタイズ
		if ( 'C' == ChStr[0] ) {									// 三角波の場合
			ofs << "\tq15";											// クォンタイズ設定
		}
		ofs << endl;
	}
	// コマンド出力
	const WORD	WholeNote = mTimeBase << 2;							// 全音符の長さ
	for ( map<BYTE,ChInfo>::iterator ChIt = mChMap.begin();			// チャンネルマップの先頭から
		  ChIt != mChMap.end();										// 最後まで
		  ++ChIt) {													// イテレータを進める
		double	RhythmTime(1);										// 拍子(全音符に対する一小節の割合 4/4=1 3/4=0.75)
		DWORD	OneLineTime = static_cast<DWORD>(WholeNote * RhythmTime * OneLineBar);// 一行の時間（全音符の時間×拍子×一行に出力する小節数）
		DWORD	LineTime(0);										// 行内の時間
		WORD	Tempo( mFirstTempo );								// テンポ
		BYTE	Octave( (*ChIt).second.FirstOctave );				// オクターブ
		BYTE	Volume( (*ChIt).second.FirstVolume );				// 音量
		BYTE	PrgNo( (*ChIt).second.FirstPrgNo );					// プログラム番号
		BYTE	LoopPrgNo(255);										// ループ時点のプログラム番号
		BYTE	LoopVolume(255);									// ループ時点の音量
		string	Comment;											// コメント文字列
		string	ChStr("? ");										// 編集用チャンネル文字
		ChStr[0] = (*ChIt).first;									// チャンネルをセット
		ofs << endl << ChStr;										// チャンネルを出力
		for ( vector<NoteInfo>::iterator NoteIt = (*ChIt).second.NoteVector.begin();// 音符ベクタの先頭から
			  NoteIt != (*ChIt).second.NoteVector.end();			// 最後まで
			  ++NoteIt) {											// イテレータを進める
			NoteInfo	&Note = (*NoteIt);							// 音符情報を参照
			switch ( Note.Str[0] ) {								// 音符情報の先頭１文字を判定
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'g':
			case 'a':
			case 'b':
			case 'n':	// 音符
				// 音量
				if ( ('C' != ChStr[0]) && ('E' != ChStr[0]) ) {		// 音量のあるチャンネル
					if ( VOL_VARIABLE == mTone.VolMode ) {			// 可変音量モードで
						if ( Note.Vol < Volume ) {					// 音量が小さければ
							ofs << "v-";							// 下げる
							if ( (Volume - Note.Vol) > 1 ) {		// 差が１より多ければ
								ofs << (WORD)(Volume - Note.Vol);	// 差分を数値で出力
							}
							Volume = Note.Vol;						// 音量を更新
						} else if ( Note.Vol > Volume ) {			// 音量が大きければ
							ofs << "v+";							// 上げる
							if ( (Note.Vol - Volume) > 1 ) {		// 差が１より多ければ
								ofs << (WORD)(Note.Vol - Volume);	// 差分を数値で出力
							}
							Volume = Note.Vol;						// 音量を更新
						}
					} else if ( VOL_COL_VAR == mTone.VolMode ) {	// 音色可変音量モードで
						if ( Volume != Note.Vol ) {					// 音量が変わったら、または音色変更直後なら
							Volume = Note.Vol;						// 音量を更新
							ofs << mTone.GetVolume( ChStr[0], PrgNo, Volume);// '@v'コマンド出力
						}
					} else if ( VOL_COLORFIT == mTone.VolMode ) {	// 音色音量モードで
						if ( Volume == 255 ) {						// 音色変更直後なら
							Volume = Note.Vol;						// 音量を更新
							ofs << mTone.GetVolume( ChStr[0], PrgNo, Volume);// '@v'コマンド出力
						}
					}
				}
				// オクターブ
				if ( ('D' != ChStr[0]) && ('E' != ChStr[0]) ) {		// パーカッション以外
					if ( Note.Oct < Octave ) {						// オクターブが低ければ
						for (; Note.Oct < Octave; --Octave) {		// 同じになるまで
							ofs << "<";								// 下げる
						}
					} else if ( Note.Oct > Octave ) {				// オクターブが高ければ
						for (; Note.Oct > Octave; ++Octave) {		// 同じになるまで
							ofs << ">";								// 上げる
						}
					}
				}
				// 下記続行
			case 'r':
			case 'w':	// 休符
				for ( DWORD FullLen = Note.Len; 0 < FullLen; ) {	// 音符全体長が残っている限りループ
					// 音符長を決める
					DWORD NoteLen = ( FullLen > WholeNote )? WholeNote: FullLen;// 音符長は、音符全体長が全音符より長ければ、全音符の長さ、そうでないなら音符全体長とする
					DWORD BlankLen = OneLineTime - LineTime;		// 改行までの時間を求める
					if ( NoteLen > BlankLen ) {						// 音符長が改行までの時間を超えたら
						NoteLen = BlankLen;							// 音符長を改行までの時間とする
					}
					FullLen  -= NoteLen;							// 音符全体長から音符長を引く
					LineTime += NoteLen;							// 行内の時間に音符長を足す
					// 音符を出力する
					WORD NoteTop(1);								// 音符の先頭フラグ
					WORD Repeat(0);									// 連続フラグ
					WORD ParLen;									// 分割長
					BOOL NCommaOutput(FALSE);						// 'n'コマンドの','出力済みフラグ
					BOOL NoteOutput(FALSE);							// 音符出力済みフラグ('&'出力判定用)
																	// ↑音符が短すぎて出力されない時でも'&'を出力してしまう対策
					for (WORD sh = 0; (ParLen = WholeNote >> sh) || NoteLen; ++sh) {// 全音符を分割してゆく
						if ( ParLen > NoteLen ) {					// この分音符を含まないなら
							Repeat = 0;								// 連続しないをセット
 							continue;								// 次へ
 						}
 						if ( NoteTop ) {							// 音符の先頭なら
							ofs << Note.Str;						// 音符の先頭を出力
 							WORD Len(1 << sh);						// 音長を求める
 							if ( (*ChIt).second.DefaultLen != Len ) {// 音長がデフォルトレングスでなければ
 								if ( ('n' == Note.Str[0]) && !NCommaOutput ) {// 'n'コマンドで、','未出力なら
 									ofs << ",";						// ','を出力
 									NCommaOutput = TRUE;			// ','出力済み
 								}
								ofs << Len;							// 音長を出力
							}
							NoteTop = 0;							// 音符の続きをセット
							Repeat = 1;								// 連続フラグを立てる
							NoteOutput = TRUE;						// 音符出力済み
						} else if ( Repeat ) {						// 連続するなら
							if ( ('n' == Note.Str[0]) && !NCommaOutput ) {// 'n'コマンドで、','未出力なら
								ofs << ",";							// ','を出力
								NCommaOutput = TRUE;				// ','出力済み
							}
							ofs << ".";								// 付点音符
						} else {
							if ( ('n' == Note.Str[0]) && !NCommaOutput ) {// 'n'コマンドで、','未出力なら
								ofs << ",";							// ','を出力
								NCommaOutput = TRUE;				// ','出力済み
							}
							ofs << "^" << (1 << sh);				// 音符の続きを出力
						}
						NoteLen  -= ParLen;							// 音符の残り長さを求める
					}
					if ( FullLen && NoteOutput &&					// 音符に続きがあり、音符出力済みで
						('r' != Note.Str[0]) && ( 'w' != Note.Str[0]) ) {// 休符以外なら
						ofs << "&";									// タイを出力
					}
					// 改行判定
					if ( LineTime >= OneLineTime ) {				// 改行位置に来たら
						if ( !Comment.empty() ) {					// コメントがあるなら
							ofs << "\t" << Comment;					// コメント出力
							Comment = "";							// コメントをクリア
						}
						ofs << endl << ChStr;						// 改行、チャンネル文字を出力
						LineTime = 0;								// 行内の時間を０に戻す
					}
				}
				break;
			case 'L':	// 繰り返し
				ofs << Note.Str;									// そのまま出力
				// プログラム番号出力のための前準備
				LoopPrgNo = PrgNo;									// ループ時点の音色を記録する
				LoopVolume = Volume;								// ループ時点の音量を記録する
				break;
			case '$':	// プログラム番号（内部表現）
				if ( PrgNo != Note.Oct ) {							// プログラム番号が変わったら
					PrgNo = Note.Oct;								// プログラム番号を更新
					if ( ('G' <= ChStr[0]) && ('L' >= ChStr[0]) ) {	// VRC7の場合
						string Cmd = mTone.Get( ChStr[0], PrgNo );	// ユーザー音色取得
						if ( 'M' != Cmd[0] ) {						// ユーザー音色が登録されていたら
							ofs << "@@0";							// ユーザー音色を出力
							if ( 'G' == ChStr[0] ) {				// Gチャンネルなら
								ofs	<< Cmd;							// 更にOPコマンドを出力
							}
						} else {									// ユーザー音色が登録されてなければ
							ofs << "@@" << TONE_DEF_VRC7_PRESET[PrgNo];//プリセット音色を出力
						}
					} else {										// その他チャンネルの場合
						ofs << mTone.Get( ChStr[0], PrgNo );		// 音色コマンドを出力する
					}

					if ( VOL_COLORFIT & mTone.VolMode ) {			// 音色音量モードなら
						Volume = 255;								// 次の音符で音量コマンドを出力する設定とする
					}
				}
				break;
			case '/':	// コメント
				Comment += Note.Str;								// コメントをセット
				break;
			case '!':	// 拍子（内部表現）
				if ( LineTime ) {									// この行に音符があれば
					ofs << endl << ChStr;							// 改行しチャンネル文字を出力
				}
				RhythmTime = static_cast<double>(Note.Oct) / Note.Vol;// 拍子を計算
				OneLineTime = static_cast<DWORD>(WholeNote * RhythmTime * OneLineBar);// 一行の時間（全音符の時間×拍子×一行に出力する小節数）
				break;
			case 't':	// テンポ
				if ( Tempo != Note.Len ) {							// テンポが変わったら
					ofs << Note.Str;								// そのまま出力
					Tempo = static_cast<WORD>( Note.Len );			// テンポを更新
				}
				break;
			default:	// その他：'t'テンポ、音色
				ofs << Note.Str;									// そのまま出力
				break;
			}
		}
		// 曲の最後

		// ループ時の音色出力
		if ( (LoopPrgNo != 255) && (LoopPrgNo != PrgNo) ) {			// プログラム番号がループ時点のプログラム番号と一致しなければ
			PrgNo = LoopPrgNo;										// プログラム番号をループ時点のプログラム番号に更新
			if ( ('G' <= ChStr[0]) && ('L' >= ChStr[0]) ) {			// VRC7の場合
				string Cmd = mTone.Get( ChStr[0], PrgNo );			// ユーザー音色取得
				if ( 'M' != Cmd[0] ) {								// ユーザー音色が登録されていたら
					ofs << "@@0";									// ユーザー音色を出力
					if ( 'G' == ChStr[0] ) {						// Gチャンネルなら
						ofs	<< Cmd;									// 更にOPコマンドを出力
					}
				} else {											// ユーザー音色が登録されてなければ
					ofs << "@@" << TONE_DEF_VRC7_PRESET[PrgNo];		//プリセット音色を出力
				}
			} else {												// その他チャンネルの場合
				ofs << mTone.Get( ChStr[0], PrgNo );				// 音色コマンドを出力する
			}

			if ( VOL_COLORFIT & mTone.VolMode ) {					// 音色音量モードなら
				Volume = 255;										// 次の音符で音量コマンドを出力する設定とする
			}
		}

		// ループ時の音量出力
		if ( (LoopVolume != 255) && (LoopVolume != Volume) ) {		// プログラム番号がループ時点のプログラム番号と一致しなければ
			if ( VOL_COLORFIT & mTone.VolMode ) {					// 音色音量モードなら
				if ( ('C' != ChStr[0]) && ('E' != ChStr[0]) ) {		// 音量のあるチャンネル
					if ( VOL_VARIABLE == mTone.VolMode ) {			// 可変音量モードで
						if ( LoopVolume < Volume ) {				// 音量が小さければ
							ofs << "v-";							// 下げる
							if ( (Volume - LoopVolume) > 1 ) {		// 差が１より多ければ
								ofs << (WORD)(Volume - LoopVolume);	// 差分を数値で出力
							}
							Volume = LoopVolume;					// 音量を更新
						} else if ( LoopVolume > Volume ) {			// 音量が大きければ
							ofs << "v+";							// 上げる
							if ( (LoopVolume - Volume) > 1 ) {		// 差が１より多ければ
								ofs << (WORD)(LoopVolume - Volume);	// 差分を数値で出力
							}
							Volume = LoopVolume;					// 音量を更新
						}
					} else if ( VOL_COL_VAR == mTone.VolMode ) {	// 音色可変音量モードで
						if ( Volume != LoopVolume ) {				// 音量が変わったら、または音色変更直後なら
							Volume = LoopVolume;					// 音量を更新
							ofs << mTone.GetVolume( ChStr[0], PrgNo, Volume);// '@v'コマンド出力
						}
					} else if ( VOL_COLORFIT == mTone.VolMode ) {	// 音色音量モードで
						if ( Volume == 255 ) {						// 音色変更直後なら
							Volume = LoopVolume;					// 音量を更新
							ofs << mTone.GetVolume( ChStr[0], PrgNo, Volume);// '@v'コマンド出力
						}
					}
				}
			}
		}

		ofs << endl;												// 改行
	}
	return 0;
}
// 音源使用宣言出力
void
Mml::PutChUseDef (													// 音源使用宣言出力
	ostream		&os,												// (o)出力ストリーム
	string		ChkCh,												// (i)対象チャンネル文字列
	string		Def)												// (i)音源使用宣言
{
	for ( BYTE Ch;													// 対象チャンネル文字列
		  !ChkCh.empty(); ) {										// 文字があるかぎりループ
		Ch = ChkCh[0];												// 次の対象チャンネル文字列を1文字取り出し
		ChkCh.erase(0,1);											// 対象チャンネル文字列の先頭を切り落とす
		if ( mChMap.find( Ch ) != mChMap.end() ) {					// このチャンネルがあるなら
			os << Def << endl;										// 音源使用宣言
			break;													// ループを抜ける
		}
	}
}
// 音質定義出力
void
Mml::Tone::PutToneDef (												// 音質定義出力
	ostream				&os,										// (o)出力ストリーム
	map<WORD,string>	&DefMap)									// (i)音質定義マップ
{
	for ( map<WORD,string>::iterator it = DefMap.begin();			// 先頭から
		  it != DefMap.end();										// 最後まで
		  ++it) {													// 進める
		os << (*it).second << flush;								// 定義を出力
	}
}
// 音量定義出力
void
Mml::Tone::PutVolumeDef (											// 音量定義出力
	ostream				&os,										// (o)出力ストリーム
	map<WORD,map<WORD,VolumeInfo>>	&DefMap)						// (i)音量定義マップ
{
	for ( map<WORD,map<WORD,VolumeInfo>>::iterator VDMit = DefMap.begin();// 先頭から
		  VDMit != DefMap.end();									// 最後まで
		  ++VDMit) {												// 進める
		map<WORD,VolumeInfo>	&VolMap = (*VDMit).second;			// 音量マップの参照
		for ( map<WORD,VolumeInfo>::iterator VolMapIt = VolMap.begin();	// 先頭から
			  VolMapIt != VolMap.end();								// 最後まで
			  ++VolMapIt) {											// 進める
			VolumeInfo	&VolInfo = (*VolMapIt).second;				// 音量情報の参照
			WORD PrgNo = (0x7F & (*VDMit).first);					// プログラム番号を取り出す
			os  << "@v" << VolInfo.SerialNo							// 定義を出力
				<< " \t={" << VolInfo.Define
				<< "}\t\t\t\t\t// Ch." << VolInfo.Ch
				<< " Vol." << (*VolMapIt).first
				<< "\tPrgNo." << PrgNo;
			if ( ('D' == VolInfo.Ch) || ('E' == VolInfo.Ch) ) {
				os	<< "\t" << DRUM_NAME[PrgNo];
			} else {
				os	<< "\t" << TONE_NAME[PrgNo];
			}
			os << endl;
		}
	}
}

// ピッチエンヴェロープ定義出力
void 
Mml::Pitch::PutDef (												// ピッチエンヴェロープ定義出力
	ostream				&os)										// (o)出力ストリーム
{
	DWORD RegMax = EnvelopeDef.size();								// エンヴェロープ定義登録数を取得
	for ( DWORD i = 0; i < RegMax; ++i ) {							// エンヴェロープ定義でループ
		os << "@EP" << i << " \t={" << EnvelopeDef[i] << "0}"<< endl;// エンヴェロープ定義先頭編集を出力
	}
}
