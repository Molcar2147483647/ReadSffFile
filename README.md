# ReadSffFile
## 基本仕様
SAEツールで使用される.sffファイルを読み取り、スプライトリスト等のデータを取得できます  
SFFv1のみで使用可能です　SFFv2/SFFv2.1は想定していません

## 想定環境
Windows11  
C++言語標準：ISO C++17標準  

上記の環境で動作することを確認しています  
上記以外の環境での動作は保証しません  

## クラス/名前空間の概要
### class SAELib::SFF
読み込んだSFFファイルのデータが格納される  
インスタンスを生成して使用する  

### class SAELib::SFF::SpriteData
格納されたデータのパラメータを取得する際に使用するクラス  

### class SAELib::SFFConfig
ReadSffFileライブラリの動作設定が可能  
インスタンス生成不可  

### namespace SAELib::SFFError
本ライブラリが扱うエラー情報のまとめ  
throwされた例外をcatchするために使用する  

## クラス/名前空間の関数一覧
## class SAELib::SFF
### デフォルトコンストラクタ
コンストラクタの引数を指定した場合、指定した引数でLoadSFF関数を実行します  
引数を指定しない場合、ファイル読み込みは行いません  
```
SAELib::SFF sff;
```
### 指定されたSFFファイルを読み込み
実行ファイルから子階層へファイル名を検索して読み込みます  
第二引数指定時は指定した階層からファイル名を検索します(SFFConfigよりも優先されます)  
実行時に既存の要素は初期化、上書きされます  
```
sff.LoadSFF("kfm.sff");                 // 実行ファイルの階層から検索
sff.LoadSFF("kfm.sff", "C:/MugenData"); // 指定パスから検索
```
引数1 const std::string& FileName ファイル名(拡張子 .sff は省略可)  
引数2 const std::string& FilePath 対象のパス(省略時は実行ファイルの子階層を探索)  
戻り値 bool 読み込み結果 (false = 失敗：true = 成功)

### 指定番号の存在確認
読み込んだSFFデータを検索し、指定番号が存在するかを確認します  
```
sff.ExistSpriteNumber(9000, 0); // 画像番号9000-0が存在するか確認
```
引数1 int32_t GroupNo グループ番号  
引数2 int32_t ImageNo イメージ番号  
戻り値 bool 検索結果 (false = 存在なし : true = 存在あり)

### 指定番号のデータへのアクセス
指定したグループ番号とイメージ番号のSFFデータへアクセスします  
対象が存在しない場合はSFFConfig::SetThrowErrorの設定に準拠します  
```
sff.GetSpriteData(9000, 0); // 画像番号9000-0のデータを取得
```
引数1 int32_t GroupNo グループ番号  
引数2 int32_t ImageNo イメージ番号  
戻り値1 対象が存在する GetSpriteData(GroupNo, ImageNo)のデータ  
戻り値2 対象が存在しない SFFConfig::SetThrowError (false = ダミーデータの参照：true = 例外を投げる)  

### 指定インデックスデータの存在確認
読み込んだSFFデータを検索し、指定インデックスのデータ存在するかを確認します  
```
air.ExistSpriteDataIndex(0); // 0番目のデータが存在するか確認
```
引数1 int32_t index データ配列インデックス  
戻り値 bool 検索結果 (false = 存在なし : true = 存在あり)

### 指定インデックスのデータへアクセス
SFFデータへ指定したインデックスでアクセスします  
対象が存在しない場合はSFFConfig::SetThrowErrorの設定に準拠します  
```
sff.GetSpriteDataIndex(0); // 0番目の画像データを取得
```
引数1 int32_t Index インデックス  
戻り値1 対象が存在する GetSpriteData(Index)のデータ  
戻り値2 対象が存在しない SFFConfig::SetThrowError (false = ダミーデータの参照：true = 例外を投げる)  

### 指定番号の画像をBMP出力
指定番号のSFFデータをBMPファイルとして出力します  
出力先のファイルは SFFConfig::SetSAELibPath の設定に準拠します  
```
sff.ExportToBMP(9000, 0); // 画像番号9000-0の画像をBMP出力
```
引数1 int32_t GroupNo グループ番号  
引数2 int32_t ImageNo イメージ番号  
戻り値 bool 出力結果 (true = 成功：false = 失敗)  

### 全ての格納画像をBMP出力
読み込んだSFFデータ全てをBMPファイルとして出力します  
出力先のファイルは SFFConfig::SetSAELibPath の設定に準拠します  
```
sff.ExportToBMP(true); // 取得画像をBMP出力
```
引数1 bool 重複した画像を出力するか (false = 含まない：true = 含む)  
戻り値 bool 出力結果 (true = 成功：false = 失敗)  

### SFFデータの画像グループ数を取得
読み込んだSFFデータの画像グループ数を返します  
```
sff.NumGroup(); // 画像グループ数を取得
```
戻り値 int32_t NumGroup 画像グループ数  

### SFFデータの画像数を取得
読み込んだSFFデータの画像数を返します  
```
sff.NumImage(); // 画像数を取得
```
戻り値 int32_t NumImage 画像数  

### SFFデータのファイル名を取得
読み込んだSFFデータの拡張子を除いたファイル名を返します  
```
sff.FileName(); // ファイル名を取得
```
戻り値 const std::string& FileName ファイル名  

### SFFデータの初期化
読み込んだSFFデータを初期化します  
```
sff.clear(); // SFFデータの初期化
```
戻り値 なし(void)  

### SFFデータの存在確認
読み込んだSFFデータの空かを判定します  
```
sff.empty(); // SFFデータの存在確認
```
戻り値 bool 判定結果 (false = データが空：true = データが存在)  

### SFFデータのデータサイズを取得
読み込んだSFFデータのデータサイズを返します  
```
sff.size(); // SFFデータサイズを取得
```
戻り値 size_t SFFDataSize SFFデータサイズ  

## class SAELib::SFF::SpriteData
### ダミーデータ判断
自身がダミーデータであるかを確認します  
SFFConfig::SetThrowErrorの設定がOFFの場合にエラー回避のために使用されます  
```
sff.GetSpriteData(XXX).IsDummy(); // ダミーデータ判断
```
戻り値 bool 判定結果 (false = 自身が正常なデータ：true = 自身がダミーデータ))  

### 軸座標Xの取得
SAEで設定した軸座標Xを返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).AxisX(); // 軸座標Xを取得
```
戻り値 int32_t AxisX 軸座標X  

### 軸座標Yの取得
SAEで設定した軸座標Yを返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).AxisY(); // 軸座標Yを取得
```
戻り値 int32_t AxisY 軸座標Y  

### グループ番号の取得
SAEで設定したグループ番号を返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).GroupNo(); // グループ番号を取得
```
戻り値 int32_t GroupNo グループ番号  

### イメージ番号の取得
SAEで設定したイメージ番号を返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).ImageNo(); // イメージ番号を取得
```
戻り値 int32_t ImageNo イメージ番号  

### ピクセルデータの取得
画像のピクセルデータ配列を返します  
ダミーデータの場合は DummyBinaryData を返します  
DummyBinaryData は常に長さ1の配列で内容は {0} です  
```
sff.GetSpriteData(XXX).PixelBinaryData(); // イメージ番号を取得
```
戻り値1 const unsigned char* const PixelBinaryData ピクセルデータ配列  
戻り値2 const unsigned char* const DummyBinaryData ダミーデータ配列  

### ピクセルデータサイズの取得
ピクセルデータのバイトサイズを返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).PixelBinaryDataByteSize(); // ピクセルデータバイトサイズを取得
```
戻り値 size_t PixelBinaryDataByteSize ピクセルデータバイトサイズ 

### パレットデータの取得
画像のパレットデータ配列を返します  
ダミーデータの場合は DummyBinaryData を返します  
DummyBinaryData は常に長さ1の配列で内容は {0} です  
```
sff.GetSpriteData(XXX).PaletteBinaryData(); // イメージ番号を取得
```
戻り値1 const unsigned char* const PaletteBinaryData ピクセルデータ配列  
戻り値2 const unsigned char* const DummyBinaryData ダミーデータ配列  

### BMPデータの取得
画像をBMP形式に変換したデータを返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).BuildBMPBinaryData(); // BMPデータを取得
```
戻り値 std::vector\<unsigned char> BuildBMPBinaryData BMPデータ 

### 画像の幅を取得
ピクセルデータに記録されている画像の幅を返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).PixelWidth(); // 画像の幅を取得
```
戻り値 uint16_t PixelWidth 画像の幅  

### 画像の高さを取得
ピクセルデータに記録されている画像の高さを返します  
ダミーデータの場合は 0 を返します  
```
sff.GetSpriteData(XXX).PixelHeight(); // 画像の高さを取得
```
戻り値 uint16_t PixelHeight 画像の高さ  

## class SAELib::SFFConfig
### エラー出力切り替え設定/取得
このライブラリ関数で発生したエラーを例外として投げるかログとして記録するかを指定できます  
```
SAELib::SFFConfig::SetThrowError(bool flag); // エラー出力切り替え設定
```
引数1 bool (false = ログとして記録する：true = 例外を投げる)  
戻り値 なし(void)  
```
SAELib::SFFConfig::GetThrowError(); // エラー出力切り替え設定を取得
```
戻り値 bool (false = ログとして記録する：true = 例外を投げる)

### エラーログファイルを作成設定/取得
このライブラリ関数で発生したエラーのログファイルを出力するかどうか指定できます  
```
SAELib::SFFConfig::SetCreateLogFile(bool flag); // エラーログファイルを作成設定
```
引数1 bool (false = ログファイルを出力しない：true = ログファイルを出力する)  
戻り値 なし(void)  
```
SAELib::SFFConfig::GetCreateLogFile(); // エラーログファイルを作成設定を取得  
```
戻り値 bool (false = ログファイルを出力しない：true = ログファイルを出力する)    

### SAELibフォルダを作成設定/取得
ファイルの出力先としてSAELibファイルを使用するかを指定できます  
```
SAELib::SFFConfig::SetCreateSAELibFile(bool flag, const std::string& Path = ""); // SAELibフォルダを作成設定
```
引数1 bool (false = SAELibファイルを使用しない：true = SAELibファイルを使用する)  
引数2 const std::string& SAELibフォルダ作成先(省略時はパスの設定なし)  
戻り値 なし(void)  
```
SAELib::SFFConfig::GetCreateSAELibFile(); // SAELibフォルダを作成設定を取得  
```
戻り値 const std::string& CreateSAELibFile SAELibフォルダ作成先  

### SAELibフォルダのパス設定/取得
SAELibファイルの作成パスを指定できます  
```
SAELib::SFFConfig::SetSAELibFilePath(const std::string& Path = ""); // SAELibフォルダのパス設定
```
引数1 const std::string& SAELibFilePath SAELibフォルダ作成先  
戻り値 なし(void)  
```
SAELib::SFFConfig::GetSAELibFilePath(); // SAELibフォルダを作成パス取得  
```
戻り値 const std::string& SAELibFilePath SAELibフォルダ作成先  

### SFFファイルの検索パス設定/取得
SFFファイルの検索先のパスを指定できます  
SFFコンストラクタもしくはLoadSFF関数で検索先のパスを指定しない場合、この設定のパスで検索します  
```
SAELib::SFFConfig::SetSFFSearchPath(const std::string& Path = ""); // SFFファイルの検索パス設定  
```
引数1 const std::string& SFFSearchPath SFFファイルの検索先のパス  
戻り値 なし(void)  
```
SAELib::SFFConfig::GetSFFSearchPath(); // SFFファイルの検索パス取得  
```
戻り値 const std::string& SFFSearchPath SFFファイルの検索先のパス  

## namespace SAELib::SFFError
### エラーID情報  
このライブラリが出力するエラーIDのenumです  
```
enum ErrorID : int32_t {
	InvalidSFFExtension,
	LoadSFFInvalidPath,
	SFFSearchInvalidPath,
	SFFFileNotFound,
	SffFileSizeOver,
	EmptySffFilePath,
	OpenSffFileFailed,
	InvalidSFFSignature,
	UnsupportedSFFv2Version,
	UnsupportedSFFv2_1Version,
	BrokenSFFFile,
	DuplicateSpriteNumber,
	SpriteNumberNotFound,
	SpriteIndexNotFound,
	SAELibFolderInvalidPath,
	CreateSAELibFolderFailed,
	CreateErrorLogFileFailed,
	WriteErrorLogFileFailed,
	CloseErrorLogFileFailed,
	CreateExportBMPFolderFailed,
	CreateBMPFileFailed,
	WriteBMPFileFailed,
	CloseBMPFileFailed,
	CheckBuildBMPBinaryData,
};
```

### エラー情報配列
このライブラリが出力するエラー情報の配列です  
```
struct T_ErrorInfo {
public:
  const int32_t ID;
  const char* const Name;
  const char* const Message;
};
      
constexpr T_ErrorInfo ErrorInfo[] = {
	{ InvalidSFFExtension,			"InvalidSFFExtension",			"ファイルの拡張子が.sffではありません" },
	{ LoadSFFInvalidPath,			"LoadSFFInvalidPath",			"SFFファイル読み込み関数のパスが正しくありません" },
	{ SFFSearchInvalidPath,			"SFFSearchInvalidPath",			"SFFファイル検索フォルダのパスが正しくありません" },
	{ SFFFileNotFound,				"SFFFileNotFound",				"SFFファイルが見つかりません" },
	{ SffFileSizeOver,				"SffFileSizeOver",				"SFFファイルサイズが許容値を超えています" },
	{ EmptySffFilePath,				"EmptySffFilePath",				"SFFファイルパスが指定されていません" },
	{ OpenSffFileFailed,			"OpenSffFileFailed",			"SFFファイルが開けませんでした" },
	{ InvalidSFFSignature,			"InvalidSFFSignature",			"ファイルの内部形式がSFFファイルではありません" },
	{ UnsupportedSFFv2Version,		"UnsupportedSFFv2Version",		"SFFv2形式のファイルは対応していません" },
	{ UnsupportedSFFv2_1Version,	"UnsupportedSFFv2.1Version",	"SFFv2.1形式のファイルは対応していません" },
	{ BrokenSFFFile,				"BrokenSFFFile",				"SFFファイルが壊れている可能性があります" },
	{ DuplicateSpriteNumber,		"DuplicateSpriteNumber",		"スプライトリストの番号が重複しています" },
	{ SpriteNumberNotFound,			"SpriteNumberNotFound",			"指定した番号がスプライトリストから見つかりません" },
	{ SpriteIndexNotFound,			"SpriteIndexNotFound",			"指定したインデックスがスプライトリストから見つかりません" },
	{ SAELibFolderInvalidPath,		"SAELibFolderInvalidPath",		"SAELibフォルダのパスが正しくありません" },
	{ CreateSAELibFolderFailed,		"CreateSAELibFolderFailed",		"SAELibフォルダの作成に失敗しました" },
	{ CreateErrorLogFileFailed,		"CreateErrorLogFileFailed",		"エラーログファイルの作成に失敗しました" },
	{ WriteErrorLogFileFailed,		"WriteErrorLogFileFailed",		"エラーログファイルへの書き込みに失敗しました" },
	{ CloseErrorLogFileFailed,		"CloseErrorLogFileFailed",		"エラーログファイルへの書き込みが正常に終了しませんでした" },
	{ CreateExportBMPFolderFailed,	"CreateExportBMPFolderFailed",	"BMPファイル出力フォルダの作成に失敗しました" },
	{ CreateBMPFileFailed,			"CreateBMPFileFailed",			"BMPファイルの作成に失敗しました" },
	{ WriteBMPFileFailed,			"WriteBMPFileFailed",			"BMPファイルの書き込みに失敗しました" },
	{ CloseBMPFileFailed,			"CloseBMPFileFailed",			"BMPファイルの書き込みが正常に終了しませんでした" },
	{ CheckBuildBMPBinaryData,		"CheckBuildBMPBinaryData",		"BMPデータ構築中に想定外の値を確認しました" },
};

```

### エラー情報のサイズ取得
エラー情報の配列サイズを取得します　　
```
SAELib::SFFError::ErrorInfoSize; // エラー情報配列サイズ
```
戻り値 size_t ErrorInfoSize エラー情報配列サイズ

### エラー名取得
エラーIDに応じたエラー名を取得します  
```
SAELib::SFFError::ErrorName(ErrorID); // ErrorIDのエラー名を取得
```
引数1 int32_t ErrorID エラーID  
戻り値 const char* ErrorName エラー名  

### エラーメッセージ取得
エラーIDに応じたエラーメッセージを取得します  
```
SAELib::SFFError::ErrorMessage(ErrorID); // ErrorIDのエラーメッセージを取得
```
引数1 int32_t ErrorID エラーID  
戻り値 const char* ErrorMessage エラーメッセージ  

## 使用例
```
#include "h_ReadSFFFile.h"

int main()
{
	SAELib::SFFConfig::SetThrowError(false);		// このライブラリで発生したエラーを例外として処理しない
	SAELib::SFFConfig::SetCreateSAELibFile(true);	// SAELibファイルの作成を許可する
	SAELib::SFFConfig::SetSAELibFilePath();			// SAELibファイルの作成階層を指定
	SAELib::SFFConfig::SetCreateLogFile(true);		// このライブラリで発生したエラーログの作成を許可する
	SAELib::SFFConfig::SetSFFSearchPath("../../");	// SFFファイルの検索開始階層を指定

	// kfmのsffファイルを読み込む
	SAELib::SFF sff("kfm");

	// 画像番号9000-0が存在するか確認
	if (sff.ExistSpriteNumber(9000, 0)) {
		// 画像番号9000-0のXY軸を取得
		sff.GetSpriteData(9000, 0).AxisX();
		sff.GetSpriteData(9000, 0).AxisY();
	}
	// 画像番号9000-0の画像をBMP出力
	sff.ExportToBMP(9000, 0);

	// 取得画像をBMP出力(true = 重複あり)
	sff.ExportToBMP(true);

	return 0;
}
```
