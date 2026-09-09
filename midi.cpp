// Midiクラス
#include "pch.h"
#include <fstream>
#include <iomanip>
#include <stack>
#include <vector>
#include "midi.h"
#include "endian.h"
using namespace std;

// ロード ----------------------------------------------------------
//	MIDIファイルを読み込む
int
Midi::Load (
	const char	*FilePath )											// (i)SMFファイルパス
{
	// ファイルオープン
	ifstream    ifs( FilePath, ios::in | ios::binary );				// ファイルストリーム
	if ( !ifs ) {
		cerr << "File Openできなかった" << endl;
 		return -1;
	}
	// FileHeader
	FileHeader	fh;
	ifs.read( reinterpret_cast<char*>(&fh), FHSIZE );
	if ( !ifs ) {
		cerr << "File Header 読み込みでエラー" << endl;
 		return -1;
	}
	reverse_endian( fh.dwKeyWord );									// エンディアン反転
	if ( fh.dwKeyWord != 'MThd' ) {									// MIDIファイルヘッダ"MThd"であること
		cerr << "File Header チャンク'MThd'を認識できない、これ midiファイルじゃないよ" << endl;
 		return -1;
	}
	reverse_endian( fh.dwMainHeaderSize );							// エンディアン反転
	cerr << "; Main Header Size " << fh.dwMainHeaderSize << " byte." << endl;
	if ( MHSIZE > fh.dwMainHeaderSize								// 小さ過ぎ
			|| 0x1000 < fh.dwMainHeaderSize ) {						// でか過ぎ
		cerr << "Main Headerのサイズ(" << fh.dwMainHeaderSize << "BYTE)が異常だよ？" << endl;
 		return -1;
	}
	// MainHeader
	MainHeader mh;
	ifs.read( reinterpret_cast<char*>(&mh), MHSIZE );
	if ( !ifs ) {
		cerr << "Main Header 読み込みでエラー" << endl;
 		return -1;
	}
	reverse_endian( mh.wFormat );									// エンディアン反転
	reverse_endian( mh.wTrackCount );
	reverse_endian( mh.wTimeBase );
	cerr << "; midi format "	<< mh.wFormat
		 << "\n; track count "	<< mh.wTrackCount
		 << "\n; Time base   "	<< mh.wTimeBase << endl;
	mFormat = mh.wFormat;											// フォーマット
	mTimeBase = mh.wTimeBase;										// 分解能を

	ifs.seekg( FHSIZE + fh.dwMainHeaderSize, ios::beg );			// MainHeaderの終わりまで読み飛ばす
	if ( !ifs ) {
		cerr << "Main Header の後が途切れてる" << endl;
 		return -1;
	}
	// Track
	for ( WORD n = 0 ; n < mh.wTrackCount ; n++ ){
		cerr << "; Track " << n << "\t";
		// TrackHeader
		TrackHeader	th;												// トラックヘッダ
		ifs.read( reinterpret_cast<char*>(&th), THSIZE );
		if ( !ifs ) {
			cerr << "Track Header 読み込みでエラー" << endl;
	 		return -1;
		}
		reverse_endian( th.dwKeyWord );								// エンディアン反転
		if ( th.dwKeyWord != 'MTrk' ) {								// トラックヘッダ"MTrk"であること
			cerr << "Track Header チャンク'MTrk'を認識できない、トラックが壊れてる" << endl;
	 		return -1;
		}
		reverse_endian( th.dwTrackSize );							// エンディアン反転
		cerr << "Track Data Size " << th.dwTrackSize << " byte." << endl;
		// TrackData
		if ( Decode( ifs, th.dwTrackSize ) ) {						// トラック内解析
			return -1;
		}
	}
	return 0;
}

// トラック複合化
//	ストリームからMIDI形式１トラック分を解読しトラックリストを取得する
int
Midi::Decode (														// ファイルストリームから読み込みトラック内解析する
	istream		&is,												// (i)入力ストリーム
	const DWORD	&dwTrackSize)										// (i)トラックサイズ（トラックの終了位置判定に利用）
{
	list<Operate>	OpeList;
	BYTE		StatusBackup = 0x00;								// ステータスバックアップ
	streampos	LastPos = is.tellg();								// 現在位置
	LastPos += dwTrackSize;											// トラックの終了位置を求める
	while ( LastPos > is.tellg() ) {								// トラックの終了位置までループ
		Operate			Ope;
		// デルタタイム取得
		Ope.Time = TimeDecode ( is );								// 曲先頭からの時間を解析
		if ( 0 > Ope.Time) break;
		// ステータス取得
		is.read( reinterpret_cast<char*>(&(Ope.Status)), 1 );		// ステータス取得
		if ( !is ) {
			cerr << "Status 読み込みでエラー" << endl;
			break;
		}
		// ランニングステータスの処理
		// 参考：http://www2s.biglobe.ne.jp/~yyagi/material/smfspec.html#RunningStatus
		BYTE &Status = reinterpret_cast<BYTE&>( Ope.Status );		// Alius
		if ( 0x80 & Status ) {										// ステータス省略してなければ
			StatusBackup = Status;									// ステータスをバックアップ
		} else {													// ステータス省略されてたら
			Status = StatusBackup;									// 前回のステータスで補完
			is.seekg( -1, ios::cur );								// 1バイト戻す
		}
		// ステータス別処理
		switch ( 0xF0 & Status ) {									// ステータス処理
		// 2ByteStatus
		case 0xC0:	// プログラムチェンジ
		case 0xD0:	// チャンネルプレッシャー
			is.read( reinterpret_cast<char*>(&(Ope.Status1)), 1 );	// ステータス1取得
			if ( !is ) break;
			OpeList.push_back( Ope );								// 制御をリストに追加
			break;
		// 3ByteStatus
		case 0x80:	// ノートオフ
		case 0x90:	// ノートオン
		case 0xA0:	// ポリフォニックキープレッシャー
		case 0xB0:	// コントロールチェンジ
		case 0xE0:	// ピッチベンド
			is.read( reinterpret_cast<char*>(&(Ope.Status1)), 1 );	// ステータス1取得
			if ( !is ) break;
			is.read( reinterpret_cast<char*>(&(Ope.Status2)), 1 );	// ステータス2取得
			if ( !is ) break;
			OpeList.push_back( Ope );								// 制御をリストに追加
			break;
		// ControlStatus
		case 0xF0:	// SysEx
			if ( 0xFF == Status ) {									// メタイベントなら
				is.read( reinterpret_cast<char*>(&(Ope.Status1)), 1 );// ステータス1取得
				if ( !is ) break;
			}
			{
				long lCount = static_cast<long>( TimeDecode ( is ) );// メッセージ長を取得
				if ( 0 > lCount ) break;
				char* p= new char[lCount] ;							// 作業領域確保
				is.read( p, lCount );								// SysEx取得
				Ope.ExData.assign( p, lCount );						// 格納
				delete[] p;
				if ( !is ) break;
			}
			// テキスト表示
			switch ( Ope.Status1 ) {								// ステータス1がテキストなら
			case 0x01:	// テキスト
			case 0x02:	// 著作権表示
			case 0x03:	// 曲名/トラック名
			case 0x04:	// 楽器名
			case 0x05:	// 歌詞
			case 0x08:	// プログラム名(音色名)
			case 0x09:	// デバイス名(音源名)
				if ( !Ope.ExData.empty() ) {						// 空じゃなければ
					if ( '\0' == Ope.ExData[ Ope.ExData.length()-1 ] ) {// NULL文字削除
						Ope.ExData = Ope.ExData.substr( 0, Ope.ExData.length()-1 );
					}
				}
				if ( !Ope.ExData.empty() ) {						// 空じゃなければ
					char *TextName[]={								// テキスト名
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
					};
					cerr << TextName[Ope.Status1]					// テキスト名
						 << ":\"" << Ope.ExData.c_str() << "\"" << endl;// テキスト表示
					OpeList.push_back( Ope );						// 制御をリストに追加
				}
				break;
			case 0x2F:	// トラック終了の場合
				OpeList.push_back( Ope );							// 制御をリストに追加
				mTrackList.push_back( OpeList );					// 制御リストをトラックリストに追加
				OpeList.clear();									// 制御リストをクリア
				break;
			default:
				OpeList.push_back( Ope );							// 制御をリストに追加
			}
			break;
		default:	// MIDIフォーマット異常
			cerr << "警告:MIDIフォーマット異常 Status("
				 << hex << (WORD)Status << ")処理強行" << endl;
			break;	// 処理は強行
		}
		if ( !is ) {												// 読み込みエラー抜け出し
			cerr << "読み込みエラー、midiファイル壊れてませんか？" << endl;
			break;
		}
	}
	// トラックリストに追加
	if ( !OpeList.empty() ) {										// 制御リストに何か残っていたら
		mTrackList.push_back( OpeList );							// 制御リストをトラックリストに追加
	}
	return ( !is ? -1: 0 );											// エラーなら-1
}

// デルタタイム複合化
//	ストリームからデルタタイムを解読し時間を取得する。
DWORD
Midi::TimeDecode (													// ファイルストリームから読み込みデルタタイム解析し時間を返す
	istream		&is)												// (i)入力ストリーム
{
	DWORD	Time(0);
	BYTE	Data;
	do {
		is.read( reinterpret_cast<char*>(&Data), 1 );
		if ( !is ) {												// エラーなら
			cerr << "デルタタイム読み込みでエラー、midiファイル壊れてませんか？" << endl;
			return -1;												// -1を返す
		}
		Time = ( Time << 7 ) | ( Data & 0x7F );
	} while ( Data & 0x80 );
	return Time;													// 時間を返す
}

// セーブ ----------------------------------------------------------
//	MIDIファイルに保存する
int
Midi::Save (
	const char	*FilePath )											// (i)SMFファイルパス
{
	// 相対時間に設定
	ChengeTimeType( TT_RELATIVE );									// 相対時間に変更
	// 文字化けしないおまじない
	locale::global(locale("japanese"));								// 文字化けしないおまじない
	// ファイルオープン
	ofstream    ofs( FilePath, ios::out | ios::binary );			// ファイルストリーム
	if ( !ofs ) {
		cerr << "File Openできなかった" << endl;
 		return -1;
	}
	// FileHeader
	FileHeader	fh;
	fh.dwKeyWord = convert_endian( 'MThd' );						// ファイルヘッダチャンク識別を設定
	fh.dwMainHeaderSize = convert_endian( MHSIZE );					// MainHeaderサイズを設定
	ofs.write( reinterpret_cast<char*>(&fh), FHSIZE );
	if ( !ofs ) {
		cerr << "File Header 出力でエラー" << endl;
 		return -1;
	}
	// MainHeader
	MainHeader	mh;
	mh.wFormat		= convert_endian( mFormat );					// フォーマット１に設定
	WORD TrackCount	= static_cast<WORD>(mTrackList.size());			// トラック数
	mh.wTrackCount	= convert_endian( TrackCount );					// トラック数を設定
	mh.wTimeBase	= convert_endian( mTimeBase );					// 分解能を設定
	ofs.write( reinterpret_cast<char*>(&mh), MHSIZE );
	if ( !ofs ) {
		cerr << "Main Header 出力でエラー" << endl;
 		return -1;
	}
	// Track
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		// TrackData
		// 説明：トラックサイズを先に求める必要があるため、先にトラックデータを符号化しておく（ファイル出力は後で）
		stringstream ss;											// 一時領域
		if ( 0 > Encode( ss, *TrackIt ) ) {							// トラックデータ符号化
			cerr << "トラックエンコードでエラー" << endl;
			return -1;
		}
		int TrackSize = static_cast<int>(ss.tellp());									// トラックサイズ取得
		if ( 0 > TrackSize ) {										// もしマイナス値ならトラックが空なので
			continue;												// 出力せず、次のトラックへ
		}
		// TrackHeader
		TrackHeader	th;												// トラックヘッダ
		th.dwKeyWord	= convert_endian( 'MTrk' );					// トラックヘッダチャンク識別を設定
		th.dwTrackSize	= convert_endian( TrackSize );				// トラックサイズを設定
		ofs.write( reinterpret_cast<char*>(&th), THSIZE );
		if ( !ofs ) {
			cerr << "Track Header 出力でエラー" << endl;
			return -1;
		}
		// トラックデータをファイルに流し込む
		ofs << ss.str() << flush;									// 一時領域をファイルに出力
		if ( !ofs ) {
			cerr << "Track Data 出力でエラー" << endl;
			return -1;
		}
	}
	return ( !ofs ? -1: 0 );										// エラーなら-1
}

// トラック符号化
//	１トラックの制御リストをMIDI形式のデータに変換しストリームに出力する
int
Midi::Encode(														// トラックデータを符号化し一時領域に格納する
		ostream		  &os,											// (o)出力ストリーム
		list<Operate> &OpeList)										// (i)制御リスト
{
	BYTE	StatusBackup(0x00);										// ステータスバックアップ
	// 制御リストループ
	for ( list<Operate>::iterator OpeIt = OpeList.begin();			// イテレータに制御リストの先頭をセットし
		  OpeIt != OpeList.end();									// 制御リストの最後まで
		  ++OpeIt ) {												// イテレータを進める
		// 出力処理
		Operate &Ope = *OpeIt;										// 制御
		TimeEncode( os, Ope.Time );									// 時間を符号化
		if ( !os ) return -1;
		BYTE &Status = reinterpret_cast<BYTE&>( Ope.Status );		// Alius
		// ランニングステータスの処理
		if (StatusBackup != Status ) {								// ステータスが前回と違うなら
			StatusBackup = Status;									// ステータスをバックアップ
			os.write( reinterpret_cast<char*>(&Ope.Status), 1);		// ステータスを出力
			if ( !os ) return -1;
		}
		// ステータス別処理
		switch ( 0xF0 & Status ) {
		// 2ByteStatus
		case 0xC0:	// プログラムチェンジ
		case 0xD0:	// チャンネルプレッシャー
			os.write( reinterpret_cast<char*>(&(Ope.Status1)), 1 );	// ステータス1出力
			if ( !os ) return -1;
			break;
		// 3ByteStatus
		case 0x80:	// ノートオフ
		case 0x90:	// ノートオン
		case 0xA0:	// ポリフォニックキープレッシャー
		case 0xB0:	// コントロールチェンジ
		case 0xE0:	// ピッチベンド
			os.write( reinterpret_cast<char*>(&(Ope.Status1)), 1 );	// ステータス1出力
			if ( !os ) return -1;
			os.write( reinterpret_cast<char*>(&(Ope.Status2)), 1 );	// ステータス2出力
			if ( !os ) return -1;
			break;
		// ControlStatus
		case 0xF0:	// システムSysEx
			StatusBackup = 0x00;									// ランニングステータスにしない
			if ( 0xFF == Status ) {									// メタイベントなら
				os.write( reinterpret_cast<char*>(&(Ope.Status1)), 1 );// ステータス1出力
				if ( !os ) return -1;
			}
			TimeEncode( os, Ope.ExData.size() );					// メッセージ長を出力
			if ( !os ) return -1;
			os.write( Ope.ExData.c_str(), Ope.ExData.size() );		// システムSysEx出力
			if ( !os ) return -1;
		}
	}
	return 0;														// 正常
}
// デルタタイム符号化
//	時間をデルタタイムに変換しストリームに出力する
int
Midi::TimeEncode (													// 時間をデルタタイムに符号化しストリームに出力する
		ostream		&os,											// (o)出力ストリーム
		DWORD		Time)											// (i)時間
{
	// 下位ビットからスタックに積む
	stack<BYTE>		stk;											// スタック
	bool			final( TRUE );									// 最後に設定
	BYTE			data;
	do {
		data = Time & 0x7F;											// 下７ビット取り出し
		Time >>= 7;													// Time下７ビットを詰める
		if ( final ) {												// 最後なら
			final = FALSE;											// 途中に設定
		} else {													// 途中なら
			data |= 0x80;											// 続きありのビットを立てる
		}
		stk.push( data );											// スタックに積む
	} while ( Time );												// Time有効桁が無くなるまでループ
	// 逆順に出力
	while ( !stk.empty() ) {										// 空になるまでループ
		data = stk.top();											// スタック取得
		os.write( reinterpret_cast<char*>(&data), 1 );				// 出力
		if ( !os ) {												// エラーなら
			cerr << "デルタタイム出力でエラー" << endl;
			return -1;												// -1を返す
		}
		stk.pop();													// スタックから抜く
	}
	return 0;
}

// 時間伸縮 --------------------------------------------------------
//	TimeBase（分解能）は変更せず、全イベントの時間を同比率で伸縮します。
//	使用目的は小節と音符長の比率が合ってない場合の補正等です。
void
Midi::TimeExpand  (													// トラック内の時間を伸張する
	const float	&Ratio )											// (i)伸縮率
{
	ChengeTimeType(TT_RELATIVE);									// 相対時間にしておく
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		double	sum(0),												// 積算時間
				ex_sum(0);											// 積算時間伸縮値
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			// 時間を伸縮
			// ※伸縮による時間のズレを解消するため、曲先頭からの時間を求め伸縮を行う
			Operate &Ope = *OpeIt;
			sum += Ope.Time;										// 積算時間に今回の時間を加算
			double	now_ex_sum = sum * Ratio;						// 今回の積算時間伸縮値を計算
			Ope.Time = static_cast<DWORD>( now_ex_sum )				// 今回と前回の積算時間伸縮値の差を新しい時間とする
					 - static_cast<DWORD>( ex_sum );				// ※丸め処理した値で差を取るのがズレを解消するミソ
			ex_sum = now_ex_sum;									// 今回の積算時間伸縮値を保管
			// テンポ補正
			// ※時間を伸縮すると曲の速度も変わるため、テンポを補正する
			// 参考：http://www2s.biglobe.ne.jp/~yyagi/material/smfspec.html#tempo
			if ( (0xFF == Ope.Status) && (0x51 == Ope.Status1) ) {	//テンポイベントなら
				DWORD	tempo
					= (static_cast<BYTE>(Ope.ExData[0]) << 16)		// テンポを取り出す
					| (static_cast<BYTE>(Ope.ExData[1]) << 8)
					|  static_cast<BYTE>(Ope.ExData[2]);
				tempo= static_cast<DWORD>(1 / Ratio * tempo);		// テンポを伸縮
				Ope.ExData[0] = static_cast<char>(tempo >> 16);		// テンポを格納
				Ope.ExData[1] = static_cast<char>(tempo >> 8);
				Ope.ExData[2] = static_cast<char>(tempo);
			}
		}
	}
}
// 分解能変更
//	TimeBase（分解能）と、それに合わせ全イベントの時間を同比率で変更します。
//	体感的には分解能は最低 24 くらいが限界でこれ以下にすると音符の微妙なズレを感じる。（曲のテンポにもよる）
void
Midi::ChangeResolution (											// 分解能変更
	const WORD	&NewTimeBase )										// (i)新しい分解能
{
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	double Ratio = static_cast<double>(NewTimeBase) / mTimeBase;	// 伸縮率を計算
	
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			// 時間を伸縮
			Operate &Ope = *OpeIt;
			Ope.Time = static_cast<DWORD>(Ope.Time * Ratio + .5);	// 伸縮 ＋ 補正
		}
	}
	mTimeBase = NewTimeBase;										// 新しい分解能に更新
}

// 時間タイプ変更 --------------------------------------------------
//	全イベントの時間情報を、相対時間／絶対時間に変更する。（MIDIファイルはデフォルトで相対時間）
void
Midi::ChengeTimeType (												// 時間タイプを変更する
	const TIMETYPE	&NewTimeType )									// (i)新しい時間タイプ FALSE:相対時間 TRUE:絶対時間
{
	if ( NewTimeType == mTimeType )	return;							// 変更なければ帰れ
	// 絶対時間に変更する場合
	if ( NewTimeType ) {
		// トラックリストループ
		for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
			  TrackIt != mTrackList.end();							// トラックの最後まで
			  ++TrackIt ) {											// イテレータを進める
			DWORD	sum(0);											// 絶対時間
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();						// 制御リストの最後まで
				  ++OpeIt ) {										// イテレータを進める
				Operate &Ope = *OpeIt;
				sum += Ope.Time;									// 絶対時間に相対時間を加算し
				Ope.Time = sum;										// 絶対時間を格納する
			}
		}
	}
	// 相対時間に変更する場合
	else {
		// トラックリストループ
		for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
			  TrackIt != mTrackList.end();							// トラックの最後まで
			  ++TrackIt ) {											// イテレータを進める
			DWORD	last(0);										// 直前の時間
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();						// 制御リストの最後まで
				  ++OpeIt ) {										// イテレータを進める
				Operate &Ope = *OpeIt;
				DWORD	now = Ope.Time;								// 今回絶対時間を退避
				Ope.Time -= last ;									// 相対時間は今回から前回を引いた時間を格納する
				last = now;											// 次の前回絶対時間に今回絶対時間をセット
			}
		}
	}
	mTimeType = NewTimeType;										// 目的の時間タイプをセット
}

// フォーマット変更 ------------------------------------------------
//	フォーマット０／１の変更を行う。
//	フォーマット０では全トラックを先頭トラックにマージする。
//	フォーマット１ではイベントをチャンネル別に分類しトラック分けを行う。
//		この場合システムSysExはチャンネル情報を持たないため、
//		同一トラックの他のイベントと同じチャンネルに分類する。
//		同一トラックの他のイベントが無い場合にはConductorTrack（先頭トラック）に分類する。
//		但しテンポ設定・拍子の設定の場合は強制的にConductorTrack（先頭トラック）に分類する。
void
Midi::ChengeFormat (												// フォーマットを変更する
	const WORD	&Format )											// (i)フォーマット
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// フォーマット０に変更----
	if ( 0 == Format ) {
		// ２番トラック以降を先頭トラックにマージ
		list<list<Operate>>::iterator FirstTrackIt = mTrackList.begin();// 先頭トラック
		(*FirstTrackIt).sort();
		list<list<Operate>>::iterator TrackIt = FirstTrackIt;		// それ以降のトラック
		for ( ++TrackIt; TrackIt != mTrackList.end();	) {			// トラック削除により手前に詰められるためitを進める必要がない
			(*TrackIt).sort();
			(*FirstTrackIt).merge( *TrackIt );						// 先頭トラックにマージ
			TrackIt = mTrackList.erase( TrackIt );					// 不要になったトラックを破棄
		}
		mFormat = 0;												// フォーマット０にする
	}
	// フォーマット１に変更----
	else if ( 1 == Format ) {
		// トラックにある全ての制御をチャンネル毎に分類し移し変える
		list<Operate>	ChList[17];									// チャンネルリスト
		// トラックリストループ
		for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
			  TrackIt != mTrackList.end();							// トラックの最後まで
			  ++TrackIt ) {											// イテレータを進める
			// 一番多いチャンネルを、このトラックのチャンネルとする
			int	 ThisCh(0);											// 一番多いチャンネル
			if ( mFormat == 1 ) {									// フォーマットが１だったら
				DWORD ChCnt[16]={0};								// チャンネルカウンタ
				// 制御リストループ
				for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
					  OpeIt != (*TrackIt).end();					// 制御リストの最後まで
					  ++OpeIt ) {									// イテレータを進める
					if ( 0xF0 != (0xF0 & (*OpeIt).Status) ) {		// システムSysEx以外なら
						++ChCnt[(0xF & (*OpeIt).Status)];			// チャンネルを数える
					}
				}
				// 一番多いチャンネルを検索
				DWORD	ChMax(0);									// チャンネル数最大値
				for ( int ch = 0; ch < 16; ++ch ) {					// 全チャンネルループ
					if ( ChMax < ChCnt[ch] ) {						// 最大値より多ければ
						ChMax = ChCnt[ch];							// 最大値更新
						ThisCh = ch+1;								// 一番多いチャンネル更新
					}
				}
			}
			// 制御をチャンネルに分類する
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  ! (*TrackIt).empty();								// 制御リストが空になるまで
				  OpeIt = (*TrackIt).begin() ) {					// イテレータに制御リストの先頭をセット
				int ch;
				BYTE Sta = 0xF0 & (*OpeIt).Status;
				if ( 0xF0 == Sta ) {								// システムSysExなら
					if ( (0xFF == (*OpeIt).Status) &&				// メタイベントで
						 ( (0x06 == (*OpeIt).Status1) ||			// マーカー　または
						   (0x51 == (*OpeIt).Status1) ||			// テンポ設定　または
						   (0x58 == (*OpeIt).Status1) )	) {			// 拍子の設定　なら
						ch = 0;										// Conductor Trackに分類
					} else {										// それ以外なら
						ch = ThisCh;								// このトラックのチャンネルに分類
					}
				} else {											// システムSysEx以外は
					ch = (0xF & (*OpeIt).Status) +1;				// ステータスの下４ビットがチャンネル
				}
				ChList[ch].splice( ChList[ch].end(), *TrackIt, OpeIt );// トラックリストからチャンネルリストに制御を移す
			}
		}
		// トラックリストをクリア
		mTrackList.clear();
		// 全チャンネルをトラックに戻す
		for ( int ch = 0; ch < 17; ++ch ) {							// 全チャンネルループ
			if ( ! ChList[ch].empty() ) {							// このチャンネルが空じゃなければ
				ChList[ch].sort();									// ソートをかける
				mTrackList.push_back( ChList[ch] );					// トラックにチャンネルを追加し		(メモリ効率悪っ)
				ChList[ch].clear();									// このチャンネルをクリア
			}
		}
		mFormat = 1;												// フォーマット１にする
	}
	// トラックエンド付け替え
	AppendAllTrackEnd ( DeleteAllTrackEnd() );						// トラックエンド付け替え
}
// 全トラックエンドを削除、及びトラック最後の時間を採取
//	全トラックエンドを削除し、全トラックのイベントで最後となる時間を採取する。
DWORD																// 戻り値：トラック最後の時間
Midi::DeleteAllTrackEnd ( void )									// 全トラックエンドを削除、及びトラック最後の時間を採取
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// トラックリストループ
	DWORD LastTime(0);												// 最後の時間
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ) {													// ここではイテレータを進めない
			Operate &Ope = *OpeIt;
			if ( LastTime < Ope.Time ) {							// 最後の時間なら
				LastTime = Ope.Time;								// 更新する
			}
			if ( (0xFF == Ope.Status) && (0x2F == Ope.Status1) ) {	// トラックエンドなら
				OpeIt = (*TrackIt).erase( OpeIt );					// 削除
			} else {												// それ以外なら
				++OpeIt;											// イテレータを進める
			}
		}
	}
	return ( LastTime );											// トラック最後の時間
}

// 全トラックトラックエンドを追加
//	全トラックにトラックエンドを追加する。
void
Midi::AppendAllTrackEnd (											// 全トラックトラックエンド追加
	const DWORD		&LastTime )										// (i)トラックエンド時間
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	Operate TrackEnd( LastTime, 0xFF, 0x2F );						// トラックエンドを作成
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		(*TrackIt).push_back( TrackEnd );							// トラックエンドを追加
	}
}

// SysEx削除 -------------------------------------------------------
//	SysEx削除モードに対応した種類のSysExを削除する。
//		0x0:削除しない
//		0x1:TEXT削除(タイトル以外)(FF,01～1F)
//		0x2:拍子・調削除(FF,58・59)
//		0x4:機種依存イベント削除(F0～FEとFF,7F)
//		0x8:その他テンポを除くSysExの削除(FF,00・20～50・52～7E)※正常に鳴らなくなる危険
// 各モードはビットに対応し複数のモードを同時使用可能
void
Midi::DeleteSysEx (													// SysEx削除
	const WORD		Mode )											// (i)SysEx削除モード
{
	if ( 0 == Mode ) return;										// モード０なら削除しない
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	string Title;													// タイトル
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ) {													// ここではイテレータを進めない
			Operate &Ope = *OpeIt;
			BYTE	Sta = 0xF0 & Ope.Status;						// イベント抽出
			if ( 0xF0 != Sta ) {									// SysEx以外
				++OpeIt;											// 制御リストのイテレータを進める
				continue;											// 次の制御へ
			}
			else if ( 0xFF == Ope.Status ) {						// メタイベントなら
				switch ( Ope.Status1 ) {							// ステータス１が
				case 0x03:	// 曲名/トラック名
					if ( Title.empty() ) {							// タイトル未取得なら
						Title = Ope.ExData;							// タイトルを取る
						++OpeIt;									// イテレータを進める
						break;										// このタイトル情報は残す
					}
					// タイトル取得済みなら続行
				case 0x01:	// テキスト
				case 0x02:	// 著作権表示
				case 0x04:	// 楽器名
				case 0x05:	// 歌詞
				case 0x08:	// プログラム名(音色名)
				case 0x09:	// デバイス名(音源名)
				case 0x0A:
				case 0x0B:
				case 0x0C:
				case 0x0D:
				case 0x0E:
				case 0x0F:
				case 0x10:
				case 0x11:
				case 0x12:
				case 0x14:
				case 0x15:
				case 0x18:
				case 0x19:
				case 0x1A:
				case 0x1B:
				case 0x1C:
				case 0x1D:
				case 0x1E:
				case 0x1F:
					if ( 0x1 & Mode )								// モード１なら
						OpeIt = (*TrackIt).erase( OpeIt );			// 制御を削除
					else											// それ以外なら
						++OpeIt;									// 制御リストのイテレータを進める
					break;
				case 0x7F:	// シーケンサ特定メタイベント
					if ( 0x4 & Mode )								// モード４なら
						OpeIt = (*TrackIt).erase( OpeIt );			// 制御を削除
					else											// それ以外なら
						++OpeIt;									// 制御リストのイテレータを進める
					break;
				case 0x06:	// マーカー
				case 0x58:	// 拍子
				case 0x59:	// 調
					if ( 0x2 & Mode )								// モード２なら
						OpeIt = (*TrackIt).erase( OpeIt );			// 制御を削除
					else {											// それ以外なら
						if ( TrackIt != mTrackList.begin() )		// 先頭トラック以外
							OpeIt = (*TrackIt).erase( OpeIt );		// 制御を削除
						else
							++OpeIt;								// 制御リストのイテレータを進める
					}
					break;
				case 0x51:	// テンポ設定
					if ( TrackIt != mTrackList.begin() )			// 先頭トラック以外
						OpeIt = (*TrackIt).erase( OpeIt );			// 制御を削除
					else
						++OpeIt;									// 制御リストのイテレータを進める
					break;
				case 0x2F:	// トラックエンド
					++OpeIt;										// 制御リストのイテレータを進める
					break;
				default:	// それ以外
					if ( 0x8 & Mode )								// モード８なら
						OpeIt = (*TrackIt).erase( OpeIt );			// 制御を削除
					else											// それ以外なら
						++OpeIt;									// 制御リストのイテレータを進める
					break;
				}
			} else {												// メタイベント以外のSysExなら
				if ( 0x4 & Mode )									// モード４なら
					OpeIt = (*TrackIt).erase( OpeIt );				// 制御を削除
				else												// それ以外なら
					++OpeIt;										// 制御リストのイテレータを進める
			}
		}
	}
}
// 未使用制御削除
//	音符またはSysExが無いチャンネルを対象とする無駄な制御を削除する。
//	つまり、プログラムチェンジやコントロールチェンジ等を使用しているのに音符が無い場合
//	これらを設定する意味は無く、無駄なデータなので削除する。
//	厳密には、プログラムチェンジやコントロールチェンジ等の対象チャンネルと、
//	トラックが違っていても同じチャンネルを対象とする音符があるならば、その制御は削除しない。
void
Midi::DeleteUnusedOperate ( void )									// 未使用制御削除
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// チャンネルの音符数をカウント
	int ChCnt[16]={0};												// チャンネルの音符数
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			BYTE	Sta = 0xF0 & Ope.Status;						// イベント抽出
			if ( (0x90 == Sta) ||									// ノートオン または
				 (0x80 == Sta) ) {									// ノートオフ ならば
				++ChCnt[0xF & Ope.Status];							// そのチャンネルの音符カウント
			}
		}
	}
	// 未使用チャンネルを対象とするトラックを削除
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt) {												// イテレータを進める
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ) {													// ここではイテレータを進めない
			Operate &Ope = *OpeIt;
			BYTE	Sta = 0xF0 & Ope.Status;						// イベント抽出
			if ( (0x90 == Sta) ||									// ノートオン または
				 (0x80 == Sta) ||									// ノートオフ または
				 (0xF0 == Sta) ) {									// SysExなら
				++OpeIt;											// 制御リストのイテレータを進める
			}
			else if ( ChCnt[0xF & Ope.Status] ) {					// それ以外なら そのチャンネルに音符があれば
				++OpeIt;											// 制御リストのイテレータを進める
			} else {												// そのチャンネルに音符が無ければ
				OpeIt = (*TrackIt).erase( OpeIt );					// その制御を削除
			}
		}
	}
}
// 空トラック削除
//	何もイベントが無いトラック、またはトラックエンドしかないトラックを削除する。
void
Midi::DeleteEmptyTrack ( void )										// 空トラック削除
{
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		) {															// ここではイテレータを進めない
		if ( (*TrackIt).empty() ) {									// 空トラックなら
			TrackIt = mTrackList.erase( TrackIt ) ;					// このトラックを削除
		} else {
			list<Operate>::iterator OpeIt = (*TrackIt).begin();		// イテレータに制御リストの先頭をセット
			Operate &Ope = *OpeIt;
			if ( (0xFF == Ope.Status) && (0x2F == Ope.Status1) ) {	// 先頭がトラックエンドなら空トラックなので
				TrackIt = mTrackList.erase( TrackIt ) ;				// このトラックを削除
			} else {												// それ以外なら
				++TrackIt;											// トラックイテレータを進める
			}
		}
	}
}
// ノートエンド表現変更 --------------------------------------------
//	ノートエンドの表現 0x80 または 0x90ベロシティ０ を切り替える
void
Midi::ChengeNoteEndType (											// ノートエンド表現変更
	const WORD		NewNoteEnd )									// (i)新しいノートエンド表現 0x80 / 0x90
{
	// ノートエンド表現 0x80 に変更する場合
	if ( 0x80 == NewNoteEnd ) {
		// トラックリストループ
		for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
			  TrackIt != mTrackList.end();							// トラックの最後まで
			  ++TrackIt ) {											// イテレータを進める
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();						// 制御リストの最後まで
				  ++OpeIt ) {										// イテレータを進める
				Operate &Ope = *OpeIt;
				BYTE	Sta = 0xF0 & Ope.Status;					// イベント抽出
				if ( (0x90 == Sta) && (0x00 == Ope.Status2) ) {		// ノートエンド 0x90ベロシティ０なら
					Ope.Status = 0x80 | (0xF & Ope.Status);			// ノートエンド 0x80に変更
				}
			}
		}
	}
	// ノートエンド表現 0x90ベロシティ０ に変更する場合
	else {
		// トラックリストループ
		for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
			  TrackIt != mTrackList.end();							// トラックの最後まで
			  ++TrackIt ) {											// イテレータを進める
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();						// 制御リストの最後まで
				  ++OpeIt ) {										// イテレータを進める
				Operate &Ope = *OpeIt;
				BYTE	Sta = 0xF0 & Ope.Status;					// イベント抽出
				if ( 0x80 == Sta ) {								// ノートエンド 0x80なら
					Ope.Status = 0x90 | (0xF & Ope.Status);			// ノートエンド 0x90に変更
					Ope.Status2 = 0x00;								// ベロシティ０に変更
				}
			}
		}
	}
}

// パーカッションを９ｃｈに統一 ------------------------------------
//	9ch以外のパーカッションを9chに統一する
//	ここで言う９ｃｈとは一般的なMIDIマニュアルで言う１０ｃｈのこと
//	※SysEx削除より先にやらないと意味がない
//	参照：複数のパートでドラムを使えるようにする http://cmsi.ddss.jp/midi.html
void
Midi::UnifyPercussionCh9 ( void )									// パーカッションを９ｃｈに統一
{
	// パーカッションチャンネルを検出する
	int PerCh[16] = {0};											// パーカッションチャンネル
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			if ( ( 0xF0 == (0xF0 & Ope.Status) ) &&					// SysExでかつ
				 ( 0xFF != Ope.Status) &&							// メタイベント以外でかつ
				 (    9 <= Ope.ExData.size() ) &&					// 9Byte以上かつ
				 ( "\x41\x10\x42\x12\x40" == Ope.ExData.substr(0,5) ) &&// 先頭 5Byteがパーカッション指定なら
				 ( 0x10 == (0xF0 & Ope.ExData[5]) ) &&				// 上位ビットも確認
				 ( 0x15 == Ope.ExData[6] ) ) {						// ここも確認
				int No = 0xF & Ope.ExData[5];						// パートを抽出
				if ( 0 == No )										// パートが０なら
					No = 9;											// チャンネルは９だよ
				else if ( 9 >= No )									// パート９以下なら
					--No;											// チャンネルはパートの１つ下
				PerCh[ No ] = Ope.ExData[8];						// パーカッションチャンネルのフラグOn/Off
			}
		}
	}
	// パーカッションチャンネルを9chに統一する
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			if ( (0xF0 != (Ope.Status & 0xF0)) && PerCh[ 0xF & Ope.Status ] ) {	// SysEx以外で　かつ　パーカッションチャンネルなら
				Ope.Status = 9 | (0xF0 & Ope.Status);				// チャンネルを９にする
			}
		}
	}
}
// パーカッションチャンネルをトラック分割 ------------------------------------
//	パーカッションチャンネルをノイズ音源向け／DPCM音源向けにトラックを分割する
//	※事前にフォーマット１に変換しておくこと
void
Midi::DividePercussion (											// パーカッションチャンネルをトラック分割
	const BYTE	*NoteMap)											// (i)割り当て表[128byte配列] 0:ノイズ 1:DPCM
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// デフォルト割り当て表
	const BYTE DEFAULTMAP[128] =									// デフォルト割り当て表
	  { 0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,1,1,1,1,1,1,0,0,0,1,
		1,1,1,1,1,1,0,1,0,1,0,1,
		1,0,1,0,0,0,0,0,1,0,1,0,
		1,1,1,1,1,1,1,1,1,0,0,1,
		1,1,1,1,1,1,1,1,0,0,0,1,
		0,1,1,1,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0 };
	if ( NULL == NoteMap ) {										// 省略されたら
		 NoteMap = DEFAULTMAP;										// デフォルト割り当て表を使う
	}
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt) {												// トラックイテレータを進める
		list<Operate>			DPCMTrack;							// DPCM用トラック
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ) {													// ここでは制御リストのイテレータは進めない
			Operate	&Ope = *OpeIt;
			BYTE	Ch = 0xF & Ope.Status;							// チャンネル抽出
			if ( 9 != Ch ) {										// パーカッションチャンネル以外は
				++OpeIt;											// 制御イテレータを進める
				continue;											// 対象外
			}
			BYTE	Sta = 0xF0 & Ope.Status;						// イベント抽出
			if ( 0xB0 <= Sta ) {									// 音符／ポリフォニックキープレッシャー以外
				++OpeIt;											// 制御イテレータを進める
				continue;											// そのまま
			}
			if ( !NoteMap[ Ope.Status1 ] ) {						// このキーがノイズ向けなら
				++OpeIt;											// 制御イテレータを進める
				continue;											// そのまま
			}
			// DPCM用の音符をDPCM用トラックに移す
			list<Operate>::iterator ItWork = OpeIt;					// splice用のイテレータにセット
			++OpeIt;												// 制御イテレータを進める
			DPCMTrack.splice( DPCMTrack.end(), *TrackIt, ItWork );	// この制御をDPCM用トラックに移す
		}
		// DPCM用トラックに１つでも音符が入ってたらトラックリストに挿入
		if ( !DPCMTrack.empty() ) {									// DPCM用トラックが空じゃなければ
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();						// 制御リストの最後まで
				  ++OpeIt ) {										// 制御イテレータを進める
				Operate	&Ope = *OpeIt;
				BYTE	Sta = 0xF0 & Ope.Status;					// イベント抽出
				if ( 0xB0 <= Sta ) {								// コントロールチェンジ・プログラムチェンジ・チャンネルプレッシャー・ピッチベンド・SysExなら
					DPCMTrack.push_back( Ope );						// DPCM用トラックに追加
				}
			}
			DPCMTrack.sort();										// DPCM用トラックにソートをかける
			++TrackIt;												// トラックイテレータを進める
			TrackIt = mTrackList.insert( TrackIt, DPCMTrack );		// トラックリストにDPCM用トラックを挿入
			DPCMTrack.clear();										// DPCM用トラックをクリア
		}
	}
}
// パーカッションチャンネルの重複音符削除 --------------------------
//	重複した音符は優先度の高い音符を残し他は削除する。
void
Midi::DeleteCh9RepeatNote (											// パーカッションチャンネルの重複音符削除
	const BYTE	*Priority)											// (i)優先度表 0～127 大きいほど優先
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// デフォルト優先度表
	const BYTE DEFAULTPRIORITY[128] =								// デフォルト優先度表
		{ 0,	//   0:
		  1,	//   1:
		  2,	//   2:
		  3,	//   3:
		  4,	//   4:
		  5,	//   5:
		  6,	//   6:
		  7,	//   7:
		  8,	//   8:
		  9,	//   9:
		 10,	//  10:
		 11,	//  11:
		 12,	//  12:
		 13,	//  13:
		 14,	//  14:
		 15,	//  15:
		 16,	//  16:
		 17,	//  17:
		 18,	//  18:
		 19,	//  19:
		 20,	//  20:
		 21,	//  21:
		 22,	//  22:
		 23,	//  23:
		 68,	//  24:
		 69,	//  25:Snare Roll
		 70,	//  26:Finger Snap
		 71,	//  27:High Q
		 72,	//  28:Slap
		 73,	//  29:Scratch Push
		 74,	//  30:Scratch Pull
		 75,	//  31:Sticks
		 76,	//  32:Square Click
		 77,	//  33:Metronome Click
		 78,	//  34:Metronome Bell
		103,	//  35:Bass Drum 2
		104,	//  36:Bass Drum 1
		106,	//  37:Side Stick
		108,	//  38:Snare Drum 1
		111,	//  39:Hand Clap
		109,	//  40:Snare Drum 2
		112,	//  41:Low Tom 2
		113,	//  42:Closed Hi-hat
		114,	//  43:Low Tom 1
		115,	//  44:Pedal Hi-hat
		116,	//  45:Mid Tom 2
		117,	//  46:Open Hi-hat
		119,	//  47:Mid Tom 1
		120,	//  48:High Tom 2
		121,	//  49:Crash Cymbal 1
		122,	//  50:High Tom 1
		123,	//  51:Ride Cymbal 1
		102,	//  52:Chinese Cymbal
		124,	//  53:Ride Bell
		125,	//  54:Tambourine
		126,	//  55:Splash Cymbal
		110,	//  56:Cowbell
		127,	//  57:Crash Cymbal 2
		105,	//  58:Vibra Slap
		107,	//  59:Ride Cymbal 2
		101,	//  60:High Bongo
		100,	//  61:Low Bongo
		 99,	//  62:Mute High Conga
		 98,	//  63:Open High Conga
		 97,	//  64:Low Conga
		 96,	//  65:High Timbale
		 95,	//  66:Low Timbale
		 94,	//  67:High Agogo
		 93,	//  68:Low Agogo
		 92,	//  69:Cabasa
		 91,	//  60:Maracas
		 90,	//  71:Short Whistle
		 89,	//  72:Long Whistle
		 88,	//  73:Short Guiro
		 87,	//  74:Long Guiro
		 86,	//  75:Claves
		 85,	//  76:High Wood Block
		 84,	//  77:Low Wood Block
		 83,	//  78:Mute Cuica
		 82,	//  79:Open Cuica
		 81,	//  70:Mute Triangle
		 80,	//  81:Open Triangle
		118,	//  82:Shaker
		 79,	//  83:Jingle Bell
		 67,	//  84:Bell Tree
		 66,	//  85:Castanets
		 65,	//  86:Mute Surdo
		 64,	//  87:Open Surdo
		 63,	//  88:Low Whistle
		 62,	//  89:Mute Cuica
		 61,	//  90:Open Cuica
		 60,	//  91:Mute Triangle
		 59,	//  92:Open Triangle
		 58,	//  93:Short Guiro
		 57,	//  94:Long Guiro
		 56,	//  95:Cabasa Up
		 55,	//  96:Cabasa Down
		 54,	//  97:Claves
		 53,	//  98:High Wood Block
		 52,	//  99:Low Wood Block
		 51,	// 100:
		 50,	// 101:
		 49,	// 102:
		 48,	// 103:
		 47,	// 104:
		 46,	// 105:
		 45,	// 106:
		 44,	// 107:
		 43,	// 108:
		 42,	// 109:
		 41,	// 110:
		 40,	// 111:
		 39,	// 112:
		 38,	// 113:
		 37,	// 114:
		 36,	// 115:
		 35,	// 116:
		 34,	// 117:
		 33,	// 118:
		 32,	// 119:
		 31,	// 120:
		 30,	// 121:
		 29,	// 122:
		 28,	// 123:
		 27,	// 124:
		 26,	// 125:
		 25,	// 126:
		 24 };	// 127:
	if ( NULL == Priority ) {										// 省略されたら
		 Priority = DEFAULTPRIORITY;								// デフォルト割り当て表を使う
	}
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt) {												// トラックイテレータを進める
		enum {
			C_IDLE,													// 空状態
			C_SEARCHSUBJECTNOTEOFF,									// 対象音符オフ探索状態
			C_SEARCHREPEATNOTE,										// 重複音符探索状態
			C_SEARCHDELETENOTEOFF,									// 削除音符オフ探索状態
		} Condition ( C_IDLE );										// 状態
		list<Operate>::iterator	SubjectNoteOn;						// 対象音符ノートオンの位置
		list<Operate>::iterator	SubjectNoteOff;						// 対象音符ノートオフの位置
		DWORD					SubjectTailTime;					// 対象音符の尻尾時間（これ以降に音符が重複したら尻尾を切り落とす）
		BYTE					DeleteNoteKey;						// 削除音符のキー
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ) {													// ここでは制御イテレータを進めない
			Operate	&Ope = *OpeIt;
			BYTE	Ch = 0xF & Ope.Status;							// チャンネル抽出
			if ( 9 != Ch ) {										// チャンネルが９以外
				++OpeIt;											// 制御イテレータを進める
				continue;											// 対象外
			}
			BYTE	Sta = 0xF0 & Ope.Status;						// イベント抽出
			switch ( Condition ) {									// 状態
			case C_IDLE:	// 空状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン（対象音符を見つけた）
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						Condition = C_SEARCHSUBJECTNOTEOFF;			// 対象音符オフ探索状態に移行する
						SubjectNoteOn = OpeIt;						// 対象音符ノートオンの位置を記録
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					cerr << "空状態でノートオンが無いノートオフがありました" << endl;
					break;
				}
				++OpeIt;											// 制御イテレータを進める
				break;
			case C_SEARCHSUBJECTNOTEOFF:	// 対象音符オフ探索状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					if ( Ope.Status1 == (*SubjectNoteOn).Status1 ) {// 対象音符のキーなら
						// 尻尾時間を求める
						DWORD Length = Ope.Time - (*SubjectNoteOn).Time;// 対象音符の長さを求める
						float TailRatio = 1.F - ( mTimeBase / 480000.F * Length );// 尻尾割合を求める(４分音符なら48%)
						if ( .05F > TailRatio ) {					// 5%以下なら
							TailRatio = .05F;						// 5%とする
						}
						SubjectTailTime = Ope.Time -				// 尻尾時間＝対象音符オフ時間－
							static_cast<DWORD>( TailRatio *	Length );// 尻尾割合×対象音符の長さ
						SubjectNoteOff = OpeIt;						// 対象音符ノートオフの位置を記録
						OpeIt = SubjectNoteOn;						// 対象音符ノートオンの位置に戻る
						Condition = C_SEARCHREPEATNOTE;				// 重複音符探索状態
					}
					break;
				}
				++OpeIt;											// 制御イテレータを進める
				break;
			case C_SEARCHREPEATNOTE:	// 重複音符探索状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン（重複音符を見つけた）
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						if ( SubjectTailTime < Ope.Time ) {			// 尻尾に重複したら 尻尾切り落とし
							(*SubjectNoteOff).Time = Ope.Time;		// 対象音符の時間を重複音符の時間に切り詰める
							(*TrackIt).splice( OpeIt, *TrackIt, SubjectNoteOff );// 対象音符ノートオフを重複音符の直前に移動
							// ここでは制御イテレータは進めない
							Condition = C_IDLE;						// 空状態に移行する
							cerr << "." << flush;
						} else {									// 完全重複
							if (  Priority[(*SubjectNoteOn).Status1]// 対象音符の優先度が
								> Priority[Ope.Status1] ) {			// 重複音符の優先度より高ければ
								DeleteNoteKey =	Ope.Status1;		// 削除音符のキーは重複音符のキーを記録
							} else {								// 重複音符の優先度が高ければ
								DeleteNoteKey =	(*SubjectNoteOn).Status1;// 削除音符のキーは対象音符のキーを記録
								swap( OpeIt, SubjectNoteOn );		// 対象の音符に戻る、この音符を対象の音符ノートオンとする（戻り先）
							}
							OpeIt = (*TrackIt).erase( OpeIt );		// この制御を削除
							Condition = C_SEARCHDELETENOTEOFF;		// 削除音符オフ探索状態に移行する
						}
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					if ( Ope.Status1 == (*SubjectNoteOn).Status1 ) {// 対象音符のキーなら
						Condition = C_IDLE;							// 空状態に移行する
					} else {										// 対象音符以外のキーなら
						cerr << "重複音符探索状態でノートオンが無いノートオフがありました" << endl;
					}
					// ↓続行
				default:	// その他
					++OpeIt;										// 制御イテレータを進める
				}
				break;
			case C_SEARCHDELETENOTEOFF:	// 削除音符オフ探索状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						++OpeIt;									// 制御イテレータを進める
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					if ( DeleteNoteKey == Ope.Status1 ) {			// 削除音符のキーなら
						(*TrackIt).erase( OpeIt );					// この制御を削除
						OpeIt = SubjectNoteOn;						// 対象音符ノートオンの位置に戻る
						Condition = C_SEARCHSUBJECTNOTEOFF;			// 対象音符オフ探索状態に移行する
					}
					++OpeIt;										// 制御イテレータを進める
					break;
				case 0xA0:	// ポリフォニックキープレッシャー
					if ( DeleteNoteKey == Ope.Status1 ) {			// 重複音符のキーなら
						OpeIt = (*TrackIt).erase( OpeIt );			// この制御を削除
					} else {
						++OpeIt;									// 制御イテレータを進める
					}
					break;
				default:
					++OpeIt;										// 制御イテレータを進める
				}
				break;
			}
		}
	}
}
// 音符の延長 ------------------------------------------------------
//	※トラックエンドがあること
void
Midi::ExtendNote (													// 音符の延長
	const WORD ChMask )												// (i)チャンネルマスク
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	DWORD NoteMaxLen = 4 * mTimeBase;								// 音符の最大長（全音符）
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		DWORD	MaxExtendTime(0);									// 最大延長時間
		DWORD	NoteOnTime(0);										// ノートオン時間
		list<Operate>::iterator	NoteOff = (*TrackIt).end();			// ノートオフの位置
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			if ( NoteOff != (*TrackIt).end() ) {					// ノートオフがあるなら
				if ( MaxExtendTime > Ope.Time ) {					// この時間が最大延長時間未満なら
					(*NoteOff).Time = Ope.Time;						// 前の音符のノートエンドはこの時間とする
				} else {
					(*NoteOff).Time = MaxExtendTime;				// 前の音符のノートエンドは最大延長時間とする
				}
				NoteOff = (*TrackIt).end();							// ノートオフ無しに設定
			}
			BYTE	Ch = 0xF & Ope.Status;							// チャンネル抽出
			BYTE	Sta = 0xF0 & Ope.Status;						// イベント抽出
			switch ( Sta ) {										// イベント判定
			case 0x90:	// ノートオン
				if ( 0 < Ope.Status2 ) {							// ベロシティが０以外なら
					if ( !( (1 << Ch) & ChMask) ) {					// このチャンネルが対象じゃなければ
						continue;									// 対象外
					}
					NoteOnTime = Ope.Time;							// ノートオン時間を取得
					break;
				}
				// ベロシティが０ならノートオフなので↓続行
			case 0x80:	// ノートオフ
				if ( !( (1 << Ch) & ChMask) ) {						// このチャンネルが対象じゃなければ
					continue;										// 対象外
				}
				if ( (Ope.Time - NoteOnTime) < NoteMaxLen ) {		// 音符の長さが音符の最大長未満なら
					NoteOff = OpeIt;								// ノートオフの位置を記録
					MaxExtendTime = NoteOnTime + NoteMaxLen;		// 最大延長時間を計算
				}
				break;
			}
		}
	}
}
// 重複音符をトラック分割 ------------------------------------------
//	音符の前部が重複する場合にトラック分割する。
//	但し、音符の後方（尻尾）が重複する場合には、その音符を切り詰め、重複しない状態にする。
//	分離したトラックにはコントロールチェンジ・プログラムチェンジ・チャンネルプレッシャー・ピッチベンド・SysExをコピー
void
Midi::DivideRepeatNote (											// 重複音符をトラック分割
	const float TailRatioMax,										// (i)尻尾割合最大
	const WORD	ChMask)												// (i)チャンネルマスク
{
	// 絶対時間に設定
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// トラックリストループ
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ) {														// ここではトラックイテレータは進めない
		enum {
			C_IDLE,													// 空状態
			C_SEARCHSUBJECTNOTEOFF,									// 対象音符オフ探索状態
			C_SEARCHREPEATNOTE,										// 重複音符探索状態
			C_SEARCHREPEATNOTEOFF,									// 重複音符オフ探索状態
		} Condition ( C_IDLE );										// 状態
		list<Operate>::iterator	SubjectNoteOn;						// 対象音符ノートオンの位置
		list<Operate>::iterator	SubjectNoteOff;						// 対象音符ノートオフの位置
		DWORD					SubjectTailTime;					// 対象音符の尻尾時間（これ以降に音符が重複したら尻尾を切り落とす）
		list<Operate>			RepeatTrack;						// 重複音符用トラック
		BYTE					RepeatKey;							// 重複音符のキー
		DWORD					LastTime;							// トラック最後の時間
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// 制御イテレータを進める
			Operate	&Ope = *OpeIt;
			BYTE	Ch = 0xF & Ope.Status;							// チャンネル抽出
			if ( !( (1 << Ch) & ChMask) ) {							// このチャンネルが対象じゃなければ
				continue;											// 対象外
			}
			LastTime = Ope.Time;									// トラック最後の時間を更新
			BYTE	Sta = 0xF0 & Ope.Status;						// イベント抽出
			switch ( Condition ) {									// 状態
			case C_IDLE:	// 空状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン（対象音符を見つけた）
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						Condition = C_SEARCHSUBJECTNOTEOFF;			// 対象音符オフ探索状態に移行する
						SubjectNoteOn = OpeIt;						// 対象音符ノートオンの位置を記録
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					cerr << "空状態でノートオンが無いノートオフがありました" << endl;
					break;
				}
				break;
			case C_SEARCHSUBJECTNOTEOFF:	// 対象音符オフ探索状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					if ( Ope.Status1 == (*SubjectNoteOn).Status1 ) {// 対象音符のキーなら
						// 尻尾時間を求める
						DWORD Length = Ope.Time - (*SubjectNoteOn).Time;// 対象音符の長さを求める
						float TailRatio = static_cast<float>(Length) / mTimeBase;// 尻尾割合を求める(４分音符なら48%を切り落とす対象とする)
						if ( TailRatioMax < TailRatio ) {			// 尻尾割合最長を超えたら
							TailRatio = TailRatioMax;				// 尻尾割合最長とする
						}
						SubjectTailTime = Ope.Time -				// 尻尾時間＝対象音符オフ時間－
							static_cast<DWORD>( TailRatio *	Length );// 尻尾割合×対象音符の長さ
						SubjectNoteOff = OpeIt;						// 対象音符ノートオフの位置を記録
						OpeIt = SubjectNoteOn;						// 対象音符ノートオンの位置に戻る
						Condition = C_SEARCHREPEATNOTE;				// 重複音符探索状態
					}
					break;
				}
				break;
			case C_SEARCHREPEATNOTE:	// 重複音符探索状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン（重複音符を見つけた）
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						if ( SubjectTailTime < Ope.Time ) {			// 尻尾に重複したら 尻尾切り落とし
							(*SubjectNoteOff).Time = Ope.Time;		// 対象音符の時間を重複音符の時間に切り詰める
							(*TrackIt).splice( OpeIt, *TrackIt, SubjectNoteOff );// 対象音符ノートオフを重複音符の直前に移動
							--OpeIt;								// 必然的に１つ進んでしまうため戻す
							Condition = C_IDLE;						// 空状態に移行する
							cerr << "." << flush;
						} else {									// 完全重複
							RepeatKey = Ope.Status1;				// 重複音符のキーを記録
							list<Operate>::iterator ItWork = OpeIt;	// splice用のイテレータにセット
							--OpeIt;								// イテレータを１つ前に退避
							RepeatTrack.splice( RepeatTrack.end(), *TrackIt, ItWork );// この制御を重複音符用トラックに移す
							Condition = C_SEARCHREPEATNOTEOFF;		// 重複音符オフ探索状態に移行する
						}
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					if ( Ope.Status1 == (*SubjectNoteOn).Status1 ) {// 対象音符のキーなら
						Condition = C_IDLE;							// 空状態に移行する
					} else {										// 対象音符以外のキーなら
						cerr << "重複音符探索状態でノートオンが無いノートオフがありました" << endl;
					}
					break;
				}
				break;
			case C_SEARCHREPEATNOTEOFF:	// 重複音符オフ探索状態
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン
					if ( 0 < Ope.Status2 ) {						// ベロシティが０以外なら
						break;
					}
					// ベロシティが０ならノートオフなので↓続行
				case 0x80:	// ノートオフ
					if ( RepeatKey == Ope.Status1 ) {				// 重複音符のキーなら
						if ( OpeIt == SubjectNoteOff ) {			// このノートオフが対象音符のノートオフなら
							break;									// 対象外とする
						}
						RepeatTrack.splice( RepeatTrack.end(), *TrackIt, OpeIt );// この制御を重複音符用トラックに移す
						OpeIt = SubjectNoteOn;						// 対象音符ノートオンの位置に戻る
						Condition = C_SEARCHREPEATNOTE;				// 重複音符探索状態に移行する
					}
					break;
				case 0xA0:	// ポリフォニックキープレッシャー
					if ( RepeatKey == Ope.Status1 ) {				// 重複音符のキーなら
						list<Operate>::iterator ItWork = OpeIt;		// splice用のイテレータにセット
						--OpeIt;									// イテレータを１つ前に退避
						RepeatTrack.splice( RepeatTrack.end(), *TrackIt, ItWork );// この制御を重複音符用トラックに移す
					}
					break;
				}
				break;
			}
		}
		// 重複トラックに１つでも音符が入ってたらトラックリストに挿入
		if ( !RepeatTrack.empty() ) {								// 重複トラックが空じゃなければ
			cerr << "重複音符がありましたトラック分割します。" << endl;
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();						// 制御リストの最後まで
				  ++OpeIt ) {										// 制御イテレータを進める
				Operate	&Ope = *OpeIt;
				BYTE	Sta = 0xF0 & Ope.Status;					// イベント抽出
				if ( 0xB0 <= Sta ) {								// コントロールチェンジ・プログラムチェンジ・チャンネルプレッシャー・ピッチベンド・SysExなら
					RepeatTrack.push_back( Ope );					// 重複トラックに追加
				}
			}
			RepeatTrack.sort();										// 重複トラックにソートをかける
			++TrackIt;												// トラックイテレータを進める
			TrackIt = mTrackList.insert( TrackIt, RepeatTrack );	// トラックリストに重複トラックを挿入
			RepeatTrack.clear();									// 重複トラックをクリア
		} else {													// 空なら
			++TrackIt;												// トラックイテレータを進める
		}
	}
}

// テンポ／拍子／マーカーを全トラックにコピー ----------------------
//	先頭トラックにあるテンポ／拍子／マーカーを２番以降のトラックにコピーする
//	コピー先が音符の途中だった場合は、音符を分断する。
void
Midi::CopyTempoAllTrack ( void )									// ノートエンド表現変更
{
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// 先頭トラックからテンポを探す
	list<list<Operate>>::iterator FirstTrackIt = mTrackList.begin();// 先頭トラック
	for ( list<Operate>::iterator FOpeIt = (*FirstTrackIt).begin();	// イテレータに制御リストの先頭をセットし
		  FOpeIt != (*FirstTrackIt).end();							// 制御リストの最後まで
		  ++FOpeIt ) {												// イテレータを進める
		Operate &FOpe = *FOpeIt;
		if ( !( (0xFF == FOpe.Status) && 
				( (0x51 == FOpe.Status1) ||							// （テンポまたは
				  (0x58 == FOpe.Status1) ||							// 拍子または
				  (0x06 == FOpe.Status1) ) ) ) {					// マーカー）以外は
			continue;												// 対象外
		}
		list<list<Operate>>::iterator TrackIt = FirstTrackIt;		// それ以降のトラック
		// トラックリストループ
		for ( ++TrackIt;											// ２番目にする
			  TrackIt != mTrackList.end();							// トラックの最後まで
			  ++TrackIt ) {											// イテレータを進める
			BYTE Note[128] = {0};									// 音符状態
			BYTE ch(0);												// このトラックの対象チャンネル
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();						// 制御リストの最後まで
				  ++OpeIt ) {										// イテレータを進める
				Operate &Ope = *OpeIt;
				BYTE Sta = 0xF0 & Ope.Status;						// イベント抽出
				// チャンネル抽出
				if ( 0xF0 != Sta ) {								// SysEx以外なら
					ch = 0xF & Ope.Status;							// このトラックのチャンネル抽出
				}
				// テンポ／拍子挿入位置確認
				if ( (Ope.Time > FOpe.Time ) ||						// 挿入位置以降または
					 ( (Ope.Time == FOpe.Time) &&					// 挿入位置で
					   ! ( (0x80 == Sta) ||							// （ノートオフまたは
					     ( (0x90 == Sta) &&							// 　ノートオンで
						   (0x00 == Ope.Status2) ) ) ) ) {			// 　ベロシティ０）以外なら
					// ノートオフ挿入
					for ( int Key = 0; Key < 128; ++Key ) {			// 全キーチェック
						if ( Note[Key] ) {							// このキーが音符の途中なら
							Operate OpeNote( FOpe.Time, 0x90 | ch, Key, 0x00);// ノートオフを作成
							OpeIt = (*TrackIt).insert( OpeIt, OpeNote );// ノートオフを挿入
							++OpeIt;								// 制御リストのイテレータを進める
						}
					}
					// テンポ／拍子／マーカー挿入
					OpeIt = (*TrackIt).insert( OpeIt, FOpe );		// テンポ／拍子／マーカーを挿入
					++OpeIt;										// 制御リストのイテレータを進める
					// ノートオン挿入
					for ( int Key = 0; Key < 128; ++Key ) {			// 全キーチェック
						if ( Note[Key] ) {							// このキーが音符の途中なら
							Operate OpeNote( FOpe.Time, 0x90 | ch, Key, Note[Key]);// ノートオンを作成
							OpeIt = (*TrackIt).insert( OpeIt, OpeNote );// ノートオンを挿入
							++OpeIt;								// 制御リストのイテレータを進める
						}
					}
					break;											// 次のトラックへ
				}
				// 音符状態確認
				switch ( Sta ) {									// イベント判定
				case 0x90:	// ノートオン
					if ( 0x00 != Ope.Status2 ) {					// ベロシティ０でなければ
						Note[ Ope.Status1 ] = Ope.Status2;			// ノートオン状態
						break;
					}		// ベロシティ０ならノートオフなので続行
				case 0x80:	// ノートオフ
					Note[ Ope.Status1 ] = 0;						// ノートオフ状態
					break;
				}
			}
		}
	}
}

// 曲先頭の空白を切詰 ----------------------------------------------
//	１小節毎に切り詰める	※先頭トラックの空白に拍子がある場合
void
Midi::BrankTrim ( void )											// 曲先頭の空白を切詰
{
	ChengeTimeType( TT_ABSOLUTE );									// 絶対時間に変更
	// 最初の音符位置を調べる
	DWORD FirstNoteTime( 0x7FFFFFFF );								// 最初の音符位置（初期値最大値）
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			if ( 0x90 == (0xF0 & Ope.Status) ) {					// 音符が見つかったら
				if ( FirstNoteTime > Ope.Time ) {					// より早い位置の音符ならば
					FirstNoteTime = Ope.Time;						// 最初の音符位置を更新する
				}
				break;												// 次のトラックへ
			}
		}
		if ( 0 == FirstNoteTime )									// 最初の音符位置が曲先頭なら
			return;													// 切詰処理無用
	}
	// 最初の音符までに在る最後の拍子の情報（位置と小節の長さ）を調べる
	DWORD RhythmTime( 0 );											// 拍子の時間（初期値先頭）
	DWORD BarLen( 4 * mTimeBase );									// 小節の長さ（初期値4/4拍子）
	{
		list<list<Operate>>::iterator TrackIt = mTrackList.begin();	// イテレータにトラックリストの先頭をセット
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			if ( FirstNoteTime < Ope.Time ) {						// 最初の音符の位置を超えたら
				break;												// ループを抜ける
			}
			if ( (0xFF == Ope.Status) && (0x58 == Ope.Status1) ) {	// 拍子が見つかったら
				RhythmTime = Ope.Time;								// 拍子の時間を更新
				BarLen = static_cast<DWORD>( (mTimeBase << 2) *		// 小節の長さ＝ 全音符×
						 Ope.ExData[0] /							// 拍子の分子 ／
						 static_cast<double>(1 << Ope.ExData[1]) );	// 拍子の分母
			}
		}
	}
	// 切詰る時間を求める
	// 例）	↓曲先頭↓拍子の時間			　↓最初の音符の時間
	//		┠4/4 ─╂5/4拍子 ─╂─────╂■────╂────
	//		│　	└小節の長さ┘			│
	//		└────切詰る時間──────┘
	DWORD TrimTime = ( ( FirstNoteTime - RhythmTime ) / BarLen )	// 切詰る時間＝（（最初の音符の時間－拍子の時間）％ 小節の長さ）
					* BarLen + RhythmTime;							//			 × 拍子の時間 ＋ 拍子の時間
	// 切り詰め処理
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			Ope.Time = ( Ope.Time < TrimTime )? 0:					// この制御が空白にあるなら制御の時間を０とする
						 Ope.Time - TrimTime;						// それ以外は空白を切詰める
		}
	}
	// 空白に複数のテンポ・拍子が在った場合、曲先頭に溜るので重複してるテンポ・拍子を削除する（最後の１つは残す）
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt ) {												// イテレータを進める
		int TempoCnt(0);											// テンポの数
		int RhythmCnt(0);											// 拍子の数
		// 曲先頭にあるテンポ・拍子の数をカウント
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			if ( 0 < Ope.Time )										// 時間が０以上の制御が現れたら
				break;												// カウント完了
			if ( 0xFF != Ope.Status )								// SysExでなければ
				continue;											// 次の制御へ
			switch ( Ope.Status1 ) {
			case 0x51:	// テンポなら
				++TempoCnt;											// テンポの数をカウント
				break;
			case 0x58:	// 拍子が見つかったら
				++RhythmCnt;										// 拍子の数をカウント
				break;
			}
		}
		// 重複しているテンポ・拍子を削除する（最後の１つは残す）
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ) {													// ここではイテレータを進めない
			if ( (2 > TempoCnt) && (2 > RhythmCnt) )				// テンポも拍子も重複してないなら
				break;												// ループを抜ける
			Operate &Ope = *OpeIt;
			if ( 0 < Ope.Time )										// 時間が０以上の制御が現れたら
				break;												// ループを抜ける
			if ( 0xFF != Ope.Status ) {								// SysExでなければ
				++OpeIt;											// 制御イテレータを進めて
				continue;											// 次へ
			}
			switch ( Ope.Status1 ) {
			case 0x51:	// テンポなら
				if ( 1 < TempoCnt ) {								// テンポが重複しているなら
					OpeIt = (*TrackIt).erase( OpeIt );				// この制御を削除
				} else {											// 重複していないなら
					++OpeIt;										// 制御イテレータを進めて
				}
				--TempoCnt;											// テンポの数を更新
				break;
			case 0x58:	// 拍子が見つかったら
				if ( 1 < RhythmCnt ) {								// 拍子が重複しているなら
					OpeIt = (*TrackIt).erase( OpeIt );				// この制御を削除
				} else {											// 重複していないなら
					++OpeIt;										// 制御イテレータを進めて
				}
				--RhythmCnt;										// 拍子の数を更新
				break;
			default:	// それ以外
				++OpeIt;											// 制御イテレータを進めて
			}
		}
	}
}


// 全制御表示 ------------------------------------------------------
void
Midi::PrintAllOperate ( void )										// 全制御表示
{
	// トラックリストループ
	int TrackNo(0);
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt, ++TrackNo ) {									// イテレータを進める
		cerr << "Track " << dec << TrackNo << ":";
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt ) {											// イテレータを進める
			Operate &Ope = *OpeIt;
			cerr << "(" << hex << Ope.Time
				 << "," << (WORD)Ope.Status
				 << "," << (WORD)Ope.Status1
				 << "," << (WORD)Ope.Status2;						// 表示
			if ( (0xFF == Ope.Status) &&							// SysExの
				 (0x01 <= Ope.Status1) &&							// テキストなら
				 (0x1F >= Ope.Status1) ) {
				cerr << ",\"" << Ope.ExData << "\"";				// 表示
			}
			cerr << ") " << flush;
		}
		cerr << endl;												// 改行
	}
	cerr << dec;													// １０進に戻す
}
// デバッグ用（異常な値をチェックする）
void
Midi::CheckAllOperate ( void )										// 全制御表示
{
	// トラックリストループ
	if ( mTimeType ) {	// 絶対時間
		cerr << "絶対時間" << endl;
		int TrackNo(0);
		for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
			  TrackIt != mTrackList.end();								// トラックの最後まで
			  ++TrackIt, ++TrackNo ) {									// イテレータを進める
			int OpeNo(0);
			DWORD	lasttime(0);
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
				  ++OpeIt, ++OpeNo ) {									// イテレータを進める
				Operate &Ope = *OpeIt;
				if ( Ope.Time < lasttime ) {
					cerr << "TrackNo." << TrackNo << " OpeNo." << OpeNo << "に異常なTime(" << Ope.Time << ")前のTime(" << lasttime << ")" <<endl; 
				}
				lasttime = Ope.Time;
			}
		}
	} else {	// 相対時間
		cerr << "相対時間" << endl;
		int TrackNo(0);
		for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
			  TrackIt != mTrackList.end();								// トラックの最後まで
			  ++TrackIt, ++TrackNo ) {									// イテレータを進める
			int OpeNo(0);
			// 制御リストループ
			for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
				  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
				  ++OpeIt, ++OpeNo ) {									// イテレータを進める
				Operate &Ope = *OpeIt;
				if ( Ope.Time > 0xaaaaaaa ) {
					cerr << "TrackNo." << TrackNo << " OpeNo." << OpeNo << "に異常なTime(" << Ope.Time << ")" << endl; 
				}
			}
		}
	}
}
// デバッグ用（特定の制御検索し表示する）
void
Midi::FindViewOperate (												// デバッグ用（特定の制御検索し表示する）
	const BYTE		&Status,										// (i)検索する制御ステータス
	const BYTE		&Status1,										// (i)検索する制御ステータス１
	const BYTE		&Status2)										// (i)検索する制御ステータス２
{
	cerr << "デバッグ（制御検索）:" << Status << ":"<< Status1 << ":" << Status2 << endl;
	// トラックリストループ
	int TrackNo(0);
	for ( list<list<Operate>>::iterator TrackIt = mTrackList.begin();// イテレータにトラックリストの先頭をセットし
		  TrackIt != mTrackList.end();								// トラックの最後まで
		  ++TrackIt, ++TrackNo ) {									// イテレータを進める
		int OpeNo(0);
		// 制御リストループ
		for ( list<Operate>::iterator OpeIt = (*TrackIt).begin();	// イテレータに制御リストの先頭をセットし
			  OpeIt != (*TrackIt).end();							// 制御リストの最後まで
			  ++OpeIt, ++OpeNo ) {									// イテレータを進める
			Operate &Ope = *OpeIt;
			if ( (Ope.Status  == Status ) &&
				 (Ope.Status1 == Status1) ) {						// 一致
				cerr << "TrackNo." << TrackNo << " OpeNo." << OpeNo // 表示
					 << "	Time:" << Ope.Time << "	Status:" 
					 << std::hex << Ope.Status << ":"<< Ope.Status1 << ":" << Ope.Status2 << std::dec << endl; 
			}
		}
	}
}
