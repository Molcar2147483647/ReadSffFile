#ifndef INCLUDEGUARD_READSFFFILE_HEADER
#define INCLUDEGUARD_READSFFFILE_HEADER

#include <stdint.h>		 // uint32_tとかのやつ
#include <string>		 // string系のやつ
#include <cstring>		 // memsetとかのやつ
#include <stdexcept>	 // runtime_errorのやつ
#include <fstream>		 // ファイル読み取り
#include <filesystem>	 // ファイル検索
#include <system_error>  // std::error_codeのやつ
#include <array>		 // 固定配列のやつ
#include <vector>		 // 可変長配列のやつ
#include <unordered_map> // ハッシュ的なやつ

namespace SAELib {
	namespace ReadSffFile_detail {
		using ksize_t = uint32_t;
		inline constexpr ksize_t KSIZE_MAX = static_cast<ksize_t>(~ksize_t{ 0 });

		namespace ReadSffFileFormat {
			inline constexpr double kVersion = 1.00;
			inline constexpr std::string_view kSystemDirectoryName = "SAELib";
			inline constexpr std::string_view kErrorLogFileName = "SAELib_SFFErrorLog";
		}

		namespace SFFFormat {
			inline constexpr std::string_view kExtension = ".sff";
			inline constexpr std::string_view kSignature = "ElecbyteSpr";
			inline constexpr uint32_t kSFFV1Version = 0x00010001;
			inline constexpr uint32_t kSFFV2Version = 0x00010002;
			inline constexpr uint32_t kSFFV2_1Version = 0x00020002;
			inline constexpr ksize_t kSubHeaderStart = 512;
			inline constexpr ksize_t kFileLength = 32;
			inline constexpr ksize_t kSFFPaletteSize = 768;
			inline constexpr ksize_t kSpriteBinaryPixelOffbits = 128;
			inline constexpr ksize_t kFileSizeLimit = 0xffffffff;
		}

		namespace DecodeBinary {
			[[nodiscard]] inline constexpr uint16_t UInt16LE(const unsigned char* const buffer) noexcept {
				return buffer[0] | (buffer[1] << 8);
			}
			[[nodiscard]] inline constexpr uint32_t UInt32LE(const unsigned char* const buffer) noexcept {
				return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
			}
			[[nodiscard]] inline constexpr uint16_t UInt16BE(const unsigned char* const buffer) noexcept {
				return buffer[1] | (buffer[0] << 8);
			}
			[[nodiscard]] inline constexpr uint32_t UInt32BE(const unsigned char* const buffer) noexcept { //（未使用）
				return buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);
			}
		}

		struct Convert {
		private: // 定数秘匿のため名前空間でなく構造体で定義
			inline static constexpr int32_t kAxisBit = 16;
			inline static constexpr int32_t kAxisXMask = (1 << kAxisBit) - 1;
			inline static constexpr int32_t kAxisXSignMask = (kAxisXMask >> 1) + 1;
			inline static constexpr int32_t kAxisYMask = -kAxisXMask - 1;

			inline static constexpr int32_t kIntHalfWidth = 16;
			inline static constexpr int32_t kIntHalfMask = (1 << kIntHalfWidth) - 1;

		public:
			[[nodiscard]] inline static constexpr int32_t EncodeSpriteAxis(int32_t AxisX, int32_t AxisY) noexcept {
				return ((AxisX & kAxisXMask) | ((AxisX < 0) * kAxisXSignMask)) | (((AxisY & kAxisXMask) | ((AxisY < 0) * kAxisXSignMask)) << kAxisBit);
			}
			[[nodiscard]] inline static constexpr int32_t DecodeSpriteAxisX(int32_t SpriteAxis) noexcept {
				return (SpriteAxis & (kAxisXSignMask - 1)) - (SpriteAxis & kAxisXSignMask);
			}
			[[nodiscard]] inline static constexpr int32_t DecodeSpriteAxisY(int32_t SpriteAxis) noexcept {
				return (SpriteAxis & kAxisYMask) >> kAxisBit;
			}

			[[nodiscard]] inline static constexpr int32_t EncodeIntHalf(int32_t GroupNo, int32_t ImageNo) noexcept {
				return (GroupNo & kIntHalfMask) | ((ImageNo << kIntHalfWidth) & ~kIntHalfMask);
			}
			[[nodiscard]] inline static constexpr int32_t DecodeSpriteGroupNo(int32_t SpriteNumber) noexcept {
				return (SpriteNumber & kIntHalfMask);
			}
			[[nodiscard]] inline static constexpr int32_t DecodeSpriteImageNo(int32_t SpriteNumber) noexcept {
				return (SpriteNumber >> kIntHalfWidth) & kIntHalfMask;
			}
		};

		struct T_Config {
		private:
			T_Config() = default;
			~T_Config() = default;
			T_Config(const T_Config&) = delete;
			T_Config& operator=(const T_Config&) = delete;

		private:
			int32_t BitFlag_ = {};
			// 設定一覧
			// &1 = このライブラリが例外を投げるか
			// &2 = エラーログファイルを生成するか
			// &4 = SAELibファイルを作成するか
			// &8 = 
			// 
			// SAELibファイルの生成パス指定
			// SFFデータ検索開始ディレクトリパス指定
			// 

			inline static constexpr int32_t kThrowError = 1 << 0;
			inline static constexpr int32_t kCreateLogFile = 1 << 1;
			inline static constexpr int32_t kCreateSAELibFile = 1 << 2;
			inline static constexpr int32_t kDefaultConfig = 0;

			// SAELibファイルのパス
			std::filesystem::path SAELibFilePath_ = {};

			// SFFファイル検索開始パス
			std::filesystem::path SFFSearchPath_ = {};

		public:
			[[nodiscard]] static T_Config& Instance() {
				static T_Config instance;
				return instance;
			}

		public:
			[[nodiscard]] int32_t BitFlag() const noexcept { return BitFlag_; }
			[[nodiscard]] bool ThrowError() const noexcept { return (BitFlag_ & kThrowError) != 0; }
			[[nodiscard]] bool CreateLogFile() const noexcept { return (BitFlag_ & kCreateLogFile) != 0; }
			[[nodiscard]] bool CreateSAELibFile() const noexcept { return (BitFlag_ & kCreateSAELibFile) != 0; }
			[[nodiscard]] const std::filesystem::path& SAELibFilePath() const noexcept { return SAELibFilePath_; }
			[[nodiscard]] const std::filesystem::path& SFFSearchPath() const noexcept { return SFFSearchPath_; }

			void InitConfig() { BitFlag_ = kDefaultConfig; }
			void ThrowError(bool flag) { BitFlag_ = (BitFlag_ & ~kThrowError) | (flag ? kThrowError : 0); }
			void CreateLogFile(bool flag) { BitFlag_ = (BitFlag_ & ~kCreateLogFile) | (flag ? kCreateLogFile : 0); }
			void CreateSAELibFile(bool flag) { BitFlag_ = (BitFlag_ & ~kCreateSAELibFile) | (flag ? kCreateSAELibFile : 0); }
			void SAELibFilePath(const std::filesystem::path& Path) { SAELibFilePath_ = (Path.empty() ? std::filesystem::current_path() : Path); }
			void SFFSearchPath(const std::filesystem::path& Path) { SFFSearchPath_ = (Path.empty() ? std::filesystem::current_path() : Path); }

		};


		// ユーザーが参照する範囲なのでkプレフィックスはなしで

		/**
		* @brief エラー情報
		*
		* 　このライブラリが使用するエラー情報の名前空間です
		*/
		namespace ErrorMessage {

			/**
			* @brief エラー情報構造体
			*
			* 　このライブラリが出力するエラー情報の構造体です
			*
			* @param const int32_t ID
			* @param const char* const Name
			* @param const char* const Message
			*/
			struct T_ErrorInfo {
			public:
				const int32_t ID;
				const char* const Name;
				const char* const Message;
			};

			/**
			* @brief エラーID情報
			*
			* 　このライブラリが出力するエラーIDのenumです
			*
			* @return enum ErrorID エラーID
			*/
			enum ErrorID : int32_t {
				InvalidSFFExtension,
				LoadSFFInvalidPath,
				SFFSearchInvalidPath,
				SFFFileNotFound,
				SFFFileSizeOver,
				EmptySFFFilePath,
				OpenSFFFileFailed,
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

			/**
			* @brief エラー情報配列
			*
			* 　このライブラリが出力するエラー情報の配列です
			*
			* @return T_ErrorInfo ErrorInfo エラー情報
			*/
			inline constexpr T_ErrorInfo ErrorInfo[] = {
				{ InvalidSFFExtension,			"InvalidSFFExtension",			"ファイルの拡張子が.sffではありません" },
				{ LoadSFFInvalidPath,			"LoadSFFInvalidPath",			"SFFファイル読み込み関数のパスが正しくありません" },
				{ SFFSearchInvalidPath,			"SFFSearchInvalidPath",			"SFFファイル検索フォルダのパスが正しくありません" },
				{ SFFFileNotFound,				"SFFFileNotFound",				"SFFファイルが見つかりません" },
				{ SFFFileSizeOver,				"SFFFileSizeOver",				"SFFファイルサイズが許容値を超えています" },
				{ EmptySFFFilePath,				"EmptySFFFilePath",				"SFFファイルパスが指定されていません" },
				{ OpenSFFFileFailed,			"OpenSFFFileFailed",			"SFFファイルが開けませんでした" },
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

			/**
			* @brief エラー情報のサイズ取得
			*
			* 　エラー情報の配列サイズを取得します
			*
			* @return size_t ErrorInfoSize エラー情報配列サイズ
			*/
			inline constexpr size_t ErrorInfoSize = sizeof(ErrorInfo) / sizeof(ErrorInfo[0]);

			/**
			* @brief エラー名の取得
			*
			* 　エラーIDに応じたエラー名を取得します
			*
			* @param int32_t ErrorID エラーID
			* @return const char* ErrorName エラー名
			*/
			inline constexpr const char* ErrorName(int32_t ID) { return ErrorInfo[ID].Name; }
		
			/**
			* @brief エラーメッセージの取得
			*
			* 　エラーIDに応じたエラーメッセージを取得します
			*
			* @param int32_t ErrorID エラーID
			* @return const char* ErrorMessage エラーメッセージ
			*/
			inline constexpr const char* ErrorMessage(int32_t ID) { return ErrorInfo[ID].Message; }
		}

		struct T_ErrorHandle {
		private:
			T_ErrorHandle() = default;
			~T_ErrorHandle() = default;
			T_ErrorHandle(const T_ErrorHandle&) = delete;
			T_ErrorHandle& operator=(const T_ErrorHandle&) = delete;
		
		private:
			struct T_ErrorList {
			private:
				const int32_t kErrorID;
				const int32_t kErrorValue;

			public:
				[[nodiscard]] int32_t ErrorID() const noexcept { return kErrorID; }
				[[nodiscard]] int32_t ErrorValue() const noexcept { return kErrorValue; }
				[[nodiscard]] const char* const ErrorName() const noexcept { return ErrorMessage::ErrorInfo[kErrorID].Name; }
				[[nodiscard]] const char* const ErrorMessage() const noexcept { return ErrorMessage::ErrorInfo[kErrorID].Message; }

				T_ErrorList(int32_t ErrorID)
					: kErrorID(ErrorID), kErrorValue(0) {
				}

				T_ErrorList(int32_t ErrorID, int32_t ErrorValue)
					: kErrorID(ErrorID), kErrorValue(ErrorValue) {
				}
			};
			std::vector<T_ErrorList> ErrorList = {};

			void InitErrorList() {
				ErrorList.clear();
			}

		public:
			[[nodiscard]] static T_ErrorHandle& Instance() {
				static T_ErrorHandle instance;
				return instance;
			}

		public:
			void AddErrorList(int32_t ErrorID) {
				ErrorList.emplace_back(T_ErrorList(ErrorID));
			}

			void AddErrorList(int32_t ErrorID, int32_t ErrorValue) {
				ErrorList.emplace_back(T_ErrorList(ErrorID, ErrorValue));
			}

			void AddErrorList(int32_t ErrorID, int32_t GroupNo, int32_t ImageNo) {
				ErrorList.emplace_back(T_ErrorList(ErrorID, Convert::EncodeIntHalf(GroupNo, ImageNo)));
			}

			[[noreturn]] void ThrowError(int32_t ErrorID) const {
				throw std::runtime_error(ErrorMessage::ErrorInfo[ErrorID].Name);
			}

			[[noreturn]] void ThrowError(int32_t ErrorID, int32_t ErrorValue) const {
				throw std::runtime_error(ErrorMessage::ErrorInfo[ErrorID].Name);
			}

			[[noreturn]] void ThrowError(int32_t ErrorID, int32_t GroupNo, int32_t ImageNo) const {
				throw std::runtime_error(ErrorMessage::ErrorInfo[ErrorID].Name);
			}

			void SetError(int32_t ErrorID) {
				if (!T_Config::Instance().ThrowError()) {
					AddErrorList(ErrorID);
					return;
				}
				ThrowError(ErrorID);
			}

			void SetError(int32_t ErrorID, int32_t ErrorValue) {
				if (!T_Config::Instance().ThrowError()) {
					AddErrorList(ErrorID, ErrorValue);
					return;
				}
				ThrowError(ErrorID, ErrorValue);
			}

			void SetError(int32_t ErrorID, int32_t GroupNo, int32_t ImageNo) {
				if (!T_Config::Instance().ThrowError()) {
					AddErrorList(ErrorID, GroupNo, ImageNo);
					return;
				}
				ThrowError(ErrorID, GroupNo, ImageNo);
			}

			void WriteErrorLog(std::ofstream& File) {
				File << "ReadSffFile ErrorLog" << "\n";
				File << "エラー数: " << ErrorList.size() << "\n";

				for (auto& Error : ErrorList) {
					File << "\nエラー名: " << Error.ErrorName() << "\n";
					File << "エラー内容: " << Error.ErrorMessage() << "\n";
					if (Error.ErrorID() == ErrorMessage::DuplicateSpriteNumber || Error.ErrorID() == ErrorMessage::SpriteNumberNotFound) {
						File << "エラー値: " << Convert::DecodeSpriteGroupNo(Error.ErrorValue()) << "-" << Convert::DecodeSpriteImageNo(Error.ErrorValue()) << "\n";
					}
					if (Error.ErrorID() == ErrorMessage::SpriteIndexNotFound) {
						File << "エラー値: " << Error.ErrorValue() << "\n";
					}
				}
				File.flush();

				if (File.fail() || File.bad()) {
					if (T_Config::Instance().ThrowError()) {
						ThrowError(ErrorMessage::WriteErrorLogFileFailed);
					}
				}
				File.close();
				if (File.fail() || File.bad()) {
					if (T_Config::Instance().ThrowError()) {
						ThrowError(ErrorMessage::CloseErrorLogFileFailed);
					}
				}
			}
		};

		// パスを扱う時の補助
		struct T_FilePathSystem {
		private:
			std::filesystem::path Path_ = {};
			std::error_code ErrorCode_ = {};

		public:
			const std::filesystem::path& Path() const noexcept { return Path_; }
			const std::error_code& ErrorCode() const noexcept { return ErrorCode_; }

			void SetPath(const std::filesystem::path& Path) {
				ErrorCode_.clear();
				Path_ = weakly_canonical(std::filesystem::absolute(Path), ErrorCode_);
				if (ErrorCode_) { Path_.clear(); }
			}

			void CreateDirectory(const std::filesystem::path& Path) {
				ErrorCode_.clear();
				if (std::filesystem::exists(Path)) { return; }
				std::filesystem::create_directories(Path, ErrorCode_);

			}

		public:
			T_FilePathSystem() = default;

			T_FilePathSystem(const std::filesystem::path& Path) {
				SetPath(Path);
			}

			[[nodiscard]] bool empty() const noexcept {
				return Path_.empty();
			}
		};

		// BMPバイナリ生成
		struct T_BuildBMPBinary {
		private:
			inline static constexpr ksize_t kBMPHeaderSize = 14 + 40;
			inline static constexpr ksize_t kBMPPaletteSize = 1024;
			inline static constexpr ksize_t kBMPPixelOffBits = kBMPHeaderSize + kBMPPaletteSize;

			const unsigned char* const kSpriteBinary;
			const unsigned char* const kPaletteBinary;
			const ksize_t kSpriteBinarySize;
			std::vector<unsigned char> BMPBinary;
			std::vector<unsigned char> DecideBinary;

			[[nodiscard]] unsigned char BitsPerPixel() const noexcept { return kSpriteBinary[3]; }
			[[nodiscard]] uint16_t Xmax() const noexcept { return 1 + DecodeBinary::UInt16LE(&kSpriteBinary[8]); }
			[[nodiscard]] uint16_t Ymax() const noexcept { return 1 + DecodeBinary::UInt16LE(&kSpriteBinary[10]); }
			[[nodiscard]] uint32_t HRes() const noexcept { return static_cast<uint32_t>(DecodeBinary::UInt16LE(&kSpriteBinary[12]) * 39.3701); } // 意味のある計算か不明
			[[nodiscard]] uint32_t VRes() const noexcept { return static_cast<uint32_t>(DecodeBinary::UInt16LE(&kSpriteBinary[14]) * 39.3701); }
			[[nodiscard]] ksize_t ImageDataSize() const noexcept { return ((Xmax() * BitsPerPixel() + 31) / 32) * 4 * Ymax(); }
			[[nodiscard]] ksize_t FileSize() const noexcept { return kBMPHeaderSize + kBMPPaletteSize + ImageDataSize(); }
			[[nodiscard]] uint16_t BytesPerLine() const noexcept { return DecodeBinary::UInt16LE(&kSpriteBinary[66]); }
			[[nodiscard]] uint8_t BMPScanlinePadding() const noexcept { return (4 - (BytesPerLine() % 4)) % 4; }
			[[nodiscard]] ksize_t BMPBinarySize() const noexcept { return kBMPPixelOffBits + (BytesPerLine() + BMPScanlinePadding()) * Ymax(); }

			[[nodiscard]] inline static constexpr unsigned char Buffer1(uint16_t value) noexcept { return (value & 0xFF); }
			[[nodiscard]] inline static constexpr unsigned char Buffer2(uint16_t value) noexcept { return (value >> 8) & 0xFF; }

			[[nodiscard]] inline static constexpr unsigned char Buffer1(uint32_t value) noexcept { return (value & 0xFF); }
			[[nodiscard]] inline static constexpr unsigned char Buffer2(uint32_t value) noexcept { return (value >> 8) & 0xFF; }
			[[nodiscard]] inline static constexpr unsigned char Buffer3(uint32_t value) noexcept { return (value >> 16) & 0xFF; }
			[[nodiscard]] inline static constexpr unsigned char Buffer4(uint32_t value) noexcept { return (value >> 24) & 0xFF; }

			[[nodiscard]] inline static constexpr unsigned char Buffer1(size_t value) noexcept { return (value & 0xFF); }
			[[nodiscard]] inline static constexpr unsigned char Buffer2(size_t value) noexcept { return (value >> 8) & 0xFF; }
			[[nodiscard]] inline static constexpr unsigned char Buffer3(size_t value) noexcept { return (value >> 16) & 0xFF; }
			[[nodiscard]] inline static constexpr unsigned char Buffer4(size_t value) noexcept { return (value >> 24) & 0xFF; }

			void InitBinaryBuffers() {
				BMPBinary.resize(BMPBinarySize());
				DecideBinary.resize(static_cast<size_t>(BytesPerLine()) * Ymax());
			}

			void SetBMPHeader() {
				unsigned char header[kBMPHeaderSize] = {
					'B', 'M'					// signeture "BM"
					, Buffer1(FileSize()), Buffer2(FileSize()), Buffer3(FileSize()), Buffer4(FileSize())	// ファイルサイズ
					, 0x00, 0x00				// 予約領域1
					, 0x00, 0x00				// 予約領域2
					, Buffer1(kBMPPixelOffBits), Buffer2(kBMPPixelOffBits), Buffer3(kBMPPixelOffBits), Buffer4(kBMPPixelOffBits)	// ファイル先頭からピクセルデータへのオフセット
					, 0x28, 0x00, 0x00, 0x00	// ヘッダーサイズ(40)
					, Buffer1(Xmax()), Buffer2(Xmax()), 0x00, 0x00	// ピクセルサイズX
					, Buffer1(Ymax()), Buffer2(Ymax()), 0x00, 0x00	// ピクセルサイズY
					, 0x01, 0x00				// カラープレーン数(1)
					, BitsPerPixel(), 0x00		// ビット数
					, 0x00, 0x00, 0x00, 0x00	// 圧縮形式
					, Buffer1(ImageDataSize()), Buffer2(ImageDataSize()), Buffer3(ImageDataSize()), Buffer4(ImageDataSize())	// 画像データサイズ
					, Buffer1(HRes()), Buffer2(HRes()), Buffer3(HRes()), Buffer4(HRes())	// 横方向解像度(ピクセル/m)
					, Buffer1(VRes()), Buffer2(VRes()), Buffer3(VRes()), Buffer4(VRes())	// 縦方向解像度(ピクセル/m)
					, 0x00, 0x00, 0x00, 0x00	// 使用パレット数
					, 0x00, 0x00, 0x00, 0x00	// 重要パレット数
				};
				std::memcpy(BMPBinary.data(), header, sizeof(header));
			}

			void AssignPaletteToBMP() {
				unsigned char* PalettePtr = BMPBinary.data() + kBMPHeaderSize;
				for (int32_t i = 0; i < 256; i++) {
					PalettePtr[i * 4 + 0] = kPaletteBinary[i * 3 + 2]; // B
					PalettePtr[i * 4 + 1] = kPaletteBinary[i * 3 + 1]; // G
					PalettePtr[i * 4 + 2] = kPaletteBinary[i * 3 + 0]; // R
					PalettePtr[i * 4 + 3] = 0x00;					   // A
				}
			}

			void DecodeSFFSpriteToBMP() {
				ksize_t SpriteBinaryOffSet = SFFFormat::kSpriteBinaryPixelOffbits; // 画像データ開始位置
				for (int32_t y = 0; y < Ymax(); ++y) {
					unsigned char* DecidePtr = DecideBinary.data() + y * BytesPerLine();
					uint16_t DecodePtrOffSet = 0;

					while (DecodePtrOffSet < BytesPerLine() && SpriteBinaryOffSet < kSpriteBinarySize) {
						uint8_t Byte = kSpriteBinary[SpriteBinaryOffSet++];
						if (SpriteBinaryOffSet >= kSpriteBinarySize) { // 想定外の値対策
							T_ErrorHandle::Instance().SetError(ErrorMessage::CheckBuildBMPBinaryData);
							break;
						}

						// RLE圧縮命令の判定
						if ((Byte & 0xC0) == 0xC0) {
							int32_t FillCount = Byte & 0x3F; // 書き込み数
							uint8_t FillValue = kSpriteBinary[SpriteBinaryOffSet++]; // 書き込む値
							int32_t CopyFillCount = (FillCount <= BytesPerLine() - DecodePtrOffSet ? FillCount : BytesPerLine() - DecodePtrOffSet); // 実際の書き込み数
							std::memset(DecidePtr + DecodePtrOffSet, FillValue, CopyFillCount);
							DecodePtrOffSet += CopyFillCount;
						}
						else {
							DecidePtr[DecodePtrOffSet++] = Byte;
						}
					}
				}
			}

			void WriteFlippedDecideToBMP() {
				unsigned char* PixelPtr = BMPBinary.data() + kBMPPixelOffBits;

				for (int32_t y = Ymax() - 1; y >= 0; --y) {
					std::memcpy(PixelPtr, &DecideBinary[static_cast<size_t>(y) * BytesPerLine()], BytesPerLine());
					PixelPtr += BytesPerLine();

					if (BMPScanlinePadding() > 0) {
						std::memset(PixelPtr, 0x00, BMPScanlinePadding());
						PixelPtr += BMPScanlinePadding();
					}
				}
			}

			void BuildBMPBinary() {
				InitBinaryBuffers();

				// ヘッダー構築
				SetBMPHeader();

				// パレットデータをBMPデータに配置
				AssignPaletteToBMP();

				// SFFデータをBMP形式に復元
				DecodeSFFSpriteToBMP();

				// 復元データを上下反転して書き込み
				WriteFlippedDecideToBMP();
			}

		public:
			T_BuildBMPBinary(const unsigned char* const SpriteBinary, const unsigned char* const PaletteBinary, ksize_t SpriteBinarySize)
				: kSpriteBinary(SpriteBinary), kPaletteBinary(PaletteBinary), kSpriteBinarySize(SpriteBinarySize)
			{
				BuildBMPBinary();
			}

			[[nodiscard]] const std::vector<unsigned char>& vecdata() const noexcept {
				return BMPBinary; 
			}

			[[nodiscard]] const unsigned char* const data() const noexcept {
				return BMPBinary.data();
			}

			[[nodiscard]] ksize_t size() const noexcept {
				return static_cast<ksize_t>(BMPBinary.size());
			}
		};

		// スプライトリストの画像番号の重複チェック＆存在確認
		struct T_UnorderedMap {
		private:
			std::unordered_map<int32_t, int32_t> UnorderedMap = {};

		public:
			void Register(int32_t value) {
				UnorderedMap[value] = static_cast<int32_t>(UnorderedMap.size());
			}

			void Register(int32_t value1, int32_t value2) {
				UnorderedMap[Convert::EncodeIntHalf(value1, value2)] = static_cast<int32_t>(UnorderedMap.size());
			}

			void Register(ksize_t value1, ksize_t value2) {
				UnorderedMap[Convert::EncodeIntHalf(static_cast<int32_t>(value1), static_cast<int32_t>(value2))] = static_cast<int32_t>(UnorderedMap.size());
			}

		public:
			T_UnorderedMap() = default;

			[[nodiscard]] int32_t find(int32_t input) {
				auto it = UnorderedMap.find(input);
				if (it != UnorderedMap.end()) { return it->second; }
				return -1;
			}
			[[nodiscard]] int32_t find(int32_t value1, int32_t value2) {
				return find(Convert::EncodeIntHalf(value1, value2));
			}
			[[nodiscard]] int32_t find(ksize_t value1, ksize_t value2) {
				return find(Convert::EncodeIntHalf(static_cast<int32_t>(value1), static_cast<int32_t>(value2)));
			}

			[[nodiscard]] bool exist(int32_t value) {
				return find(value) >= 0;
			}
			[[nodiscard]] bool exist(int32_t value1, int32_t value2) {
				return find(value1, value2) >= 0;
			}
			[[nodiscard]] bool exist(ksize_t value1, ksize_t value2) {
				return find(static_cast<int32_t>(value1), static_cast<int32_t>(value2)) >= 0;
			}

			void reserve(ksize_t value) {
				UnorderedMap.reserve(value);
			}

			void clear() {
				UnorderedMap.clear();
			}

			void shrink_to_fit() {
				UnorderedMap.rehash(0);
			}

			[[nodiscard]] bool empty() const noexcept {
				return UnorderedMap.empty();
			}

			[[nodiscard]] ksize_t size() const noexcept {
				return static_cast<ksize_t>(UnorderedMap.size());
			}
		};

		// 画像＆パレットのバイナリデータ管理
		struct T_SFFBinaryData {
		private:
			struct T_SpriteList {
			private:
				const ksize_t kSpriteStart;
				const ksize_t kSpriteSize;
			public:
				[[nodiscard]] ksize_t SpriteStart() const noexcept { return kSpriteStart; }
				[[nodiscard]] ksize_t SpriteSize() const noexcept { return kSpriteSize; }

				T_SpriteList(ksize_t SpriteStart, ksize_t SpriteSize)
					: kSpriteStart(SpriteStart), kSpriteSize(SpriteSize) {
				}
			};

			struct T_IndexList {
			private:
				const ksize_t kSpriteListIndex;
				const ksize_t kPaletteIndex;
			public:
				[[nodiscard]] ksize_t SpriteListIndex() const noexcept { return kSpriteListIndex; }
				[[nodiscard]] ksize_t PaletteIndex() const noexcept { return kPaletteIndex; }
				[[nodiscard]] ksize_t PaletteStart() const noexcept { return kPaletteIndex * SFFFormat::kSFFPaletteSize; }

				T_IndexList(ksize_t SpriteListIndex, ksize_t PaletteIndex)
					: kSpriteListIndex(SpriteListIndex), kPaletteIndex(PaletteIndex) {
				}
			};

			struct T_DataList {
			private:
				const ksize_t kIndexListNumber;	// 配列Index
				const int32_t kSpriteAxis;		// axisXY(-32768～32767) axisX(65535) axisY(65535)
				const int32_t kSpriteNumber;	// groupNo(65535) imageNo(65535)
			public:
				[[nodiscard]] ksize_t IndexListNumber() const noexcept { return kIndexListNumber; }
				[[nodiscard]] int32_t AxisX() const noexcept { return Convert::DecodeSpriteAxisX(kSpriteAxis); }
				[[nodiscard]] int32_t AxisY() const noexcept { return Convert::DecodeSpriteAxisY(kSpriteAxis); }
				[[nodiscard]] int32_t GroupNo() const noexcept { return Convert::DecodeSpriteGroupNo(kSpriteNumber); }
				[[nodiscard]] int32_t ImageNo() const noexcept { return Convert::DecodeSpriteImageNo(kSpriteNumber); }

				T_DataList(ksize_t IndexListNumber, int32_t AxisX, int32_t AxisY, int32_t GroupNo, int32_t ImageNo) noexcept
					: kIndexListNumber(IndexListNumber)
					, kSpriteAxis(Convert::EncodeSpriteAxis(AxisX, AxisY))
					, kSpriteNumber(Convert::EncodeIntHalf(GroupNo, ImageNo)) {
				}
			};

			std::vector<T_SpriteList> SpriteList_ = {};
			std::vector<T_IndexList> IndexList_ = {};
			std::vector<T_DataList> DataList_ = {};
			std::vector<unsigned char> Sprite_ = {};
			std::vector<unsigned char> Palette_ = {};
		
		public:
			[[nodiscard]] const std::vector<T_SpriteList>& SpriteList() const noexcept { return SpriteList_; }
			[[nodiscard]] const std::vector<T_IndexList>& IndexList() const noexcept { return IndexList_; }
			[[nodiscard]] const std::vector<T_DataList>& DataList() const noexcept { return DataList_; }
			[[nodiscard]] const std::vector<unsigned char>& Sprite() const noexcept { return Sprite_; }
			[[nodiscard]] const std::vector<unsigned char>& Palette() const noexcept { return Palette_; }
			[[nodiscard]] const T_SpriteList& SpriteList(ksize_t index) const noexcept { return SpriteList_[index]; }
			[[nodiscard]] const T_IndexList& IndexList(ksize_t index) const noexcept { return IndexList_[index]; }
			[[nodiscard]] const T_DataList& DataList(ksize_t index) const noexcept { return DataList_[index]; }
			[[nodiscard]] ksize_t NumSprite() const noexcept { return static_cast<ksize_t>(SpriteList_.size()); }
			[[nodiscard]] ksize_t NumPalette() const noexcept { return static_cast<ksize_t>(Palette_.size()) / SFFFormat::kSFFPaletteSize; }

			[[nodiscard]] const unsigned char* const Sprite(ksize_t index) const noexcept {
				return Sprite_.data() + SpriteList_[index].SpriteStart();
			}

			[[nodiscard]] const unsigned char* const IndexList_Sprite(ksize_t index) const noexcept {
				return Sprite(IndexList_[index].SpriteListIndex());
			}

			[[nodiscard]] const unsigned char* const DataList_Sprite(ksize_t index) const noexcept {
				return IndexList_Sprite(DataList_[index].IndexListNumber());
			}

			[[nodiscard]] const unsigned char* const Palette(ksize_t index) const noexcept {
				return Palette_.data() + index * SFFFormat::kSFFPaletteSize;
			}

			[[nodiscard]] const unsigned char* const IndexList_Palette(ksize_t index) const noexcept {
				return Palette_.data() + IndexList_[index].PaletteStart();
			}

			[[nodiscard]] const unsigned char* const DataList_Palette(ksize_t index) const noexcept {
				return IndexList_Palette(DataList_[index].IndexListNumber());
			}

			[[nodiscard]] ksize_t SpriteSize(ksize_t index) const noexcept {
				return SpriteList_[index].SpriteSize();
			}

			[[nodiscard]] ksize_t IndexList_SpriteSize(ksize_t index) const noexcept {
				return SpriteSize(IndexList_[index].SpriteListIndex());
			}

			[[nodiscard]] ksize_t DataList_SpriteSize(ksize_t index) const noexcept {
				return IndexList_SpriteSize(DataList_[index].IndexListNumber());
			}

			void AddIndexList(ksize_t SpriteListIndex, ksize_t PaletteIndex) {
				IndexList_.emplace_back(T_IndexList(SpriteListIndex, PaletteIndex));
			}

			void AddDataList(ksize_t IndexListNumber, int32_t AxisX, int32_t AxisY, int32_t GroupNo, int32_t ImageNo) {
				DataList_.emplace_back(T_DataList(IndexListNumber, AxisX, AxisY, GroupNo, ImageNo));
			}

			void AddSprite(const std::vector<unsigned char>& LoadSpriteData) {
				SpriteList_.emplace_back(T_SpriteList(static_cast<ksize_t>(Sprite_.size()), static_cast<ksize_t>(LoadSpriteData.size())));
				Sprite_.insert(Sprite_.end(), LoadSpriteData.begin(), LoadSpriteData.end());
			}

			void AddPalette(const std::array<unsigned char, SFFFormat::kSFFPaletteSize>& LoadPaletteData) {
				Palette_.insert(Palette_.end(), LoadPaletteData.begin(), LoadPaletteData.end());
			}

		public:
			T_SFFBinaryData() = default;

			void reserve(ksize_t NumImage, ksize_t FileSize, ksize_t PaletteSize) {
				SpriteList_.reserve(NumImage);
				IndexList_.reserve(NumImage);
				DataList_.reserve(NumImage);
				Sprite_.reserve(FileSize);
				Palette_.reserve(PaletteSize);
			}

			void clear() {
				SpriteList_.clear();
				IndexList_.clear();
				DataList_.clear();
				Sprite_.clear();
				Palette_.clear();
			}

			void shrink_to_fit() {
				SpriteList_.shrink_to_fit();
				IndexList_.shrink_to_fit();
				DataList_.shrink_to_fit();
				// Sprite_.shrink_to_fit(); プロセスメモリが2倍に跳ね上がったので無効
				Palette_.shrink_to_fit();
			}

			[[nodiscard]] bool empty() const noexcept {
				return SpriteList_.empty() && IndexList_.empty() && DataList_.empty() && Sprite_.empty() && Palette_.empty();
			}

			[[nodiscard]] ksize_t size() const noexcept {
				return static_cast<ksize_t>(Sprite_.size() + Palette_.size());
			}
		};

		// SFF読み込み時のヘッダー情報格納先
		struct T_LoadSFFHeader {
		private:
			const std::string kFileName = {};
			const std::string kFilePath = {};
			const uintmax_t kFileSize = 0;
			std::ifstream File = {};
			unsigned char buffer[33] = {};
			const bool kCheckError = false;
			// 0～11 識別子("ElecbyteSpr")
			// 12～ メイン情報
			// 33～ 余白
			// 36～ コメント等

			[[nodiscard]] const std::string EnsureSffExtension(const std::filesystem::path& FileName) const {
				std::filesystem::path FixedFileName = FileName;
				if (FixedFileName.extension() != SFFFormat::kExtension) {
					if (!FixedFileName.extension().empty()) {
						T_ErrorHandle::Instance().SetError(ErrorMessage::InvalidSFFExtension);
					}
					FixedFileName.replace_extension(SFFFormat::kExtension);
				}
				return FixedFileName.string();
			}

			[[nodiscard]] const std::string FindFilePathDown(const std::string& FilePath) const {
				T_FilePathSystem SFFFolder;
				if (!FilePath.empty()) {
					SFFFolder.SetPath(FilePath);
					if (SFFFolder.ErrorCode()) {
						T_ErrorHandle::Instance().SetError(ErrorMessage::LoadSFFInvalidPath);
					}
				}
				if (FilePath.empty() || SFFFolder.ErrorCode() && !T_Config::Instance().SFFSearchPath().empty()) {
					SFFFolder.SetPath(T_Config::Instance().SFFSearchPath());
					if (SFFFolder.ErrorCode()) {
						T_ErrorHandle::Instance().SetError(ErrorMessage::SFFSearchInvalidPath);
					}
				}
				const std::filesystem::path AbsolutePath = (std::filesystem::exists(SFFFolder.Path()) ? SFFFolder.Path() : std::filesystem::canonical(std::filesystem::current_path()));

				for (const auto& entry : std::filesystem::recursive_directory_iterator(
					AbsolutePath, std::filesystem::directory_options::skip_permission_denied)) {
					if (!entry.is_regular_file()) { continue; }
					if (entry.path().filename() == kFileName) {
						return entry.path().string();
					}
				}

				T_ErrorHandle::Instance().SetError(ErrorMessage::SFFFileNotFound);
				return {};
			}

			[[nodiscard]] bool CheckFileSize() const {
				if (kFileSize <= UINT32_MAX) { return false; }
				T_ErrorHandle::Instance().SetError(ErrorMessage::SFFFileSizeOver);
				return true;
			}
			[[nodiscard]] bool CheckFilePath() const {
				if (!FilePath().empty()) { return false; }
				T_ErrorHandle::Instance().SetError(ErrorMessage::EmptySFFFilePath);
				return true;
			}
			[[nodiscard]] bool CheckFileOpen() {
				File.open(FilePath(), std::ios::binary);
				if (File.is_open()) { return false; }
				T_ErrorHandle::Instance().SetError(ErrorMessage::OpenSFFFileFailed);
				return true;
			}

			[[nodiscard]] bool CheckSFFFormat() {
				// ファイル読み取りが絡むので一か所にまとめた
				File.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));

				if (Signature() != SFFFormat::kSignature) { 
					T_ErrorHandle::Instance().SetError(ErrorMessage::InvalidSFFSignature);
					return true;
				}
				if (Version() == SFFFormat::kSFFV2Version) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::UnsupportedSFFv2Version);
					return true;
				}
				if (Version() == SFFFormat::kSFFV2_1Version) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::UnsupportedSFFv2_1Version);
					return true;
				}
				if (Version() != SFFFormat::kSFFV1Version || 
					SubHeaderStart() != SFFFormat::kSubHeaderStart ||
					FileLength() != SFFFormat::kFileLength) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::BrokenSFFFile);
					return true;
				}

				return false;
			}

			[[nodiscard]] bool CheckFileError() { // CheckFileOpen → CheckSFFFormat の順で実行
				return CheckFileSize() || CheckFilePath() || CheckFileOpen() || CheckSFFFormat();
			}

		public:
			[[nodiscard]] const std::string& FileName() const noexcept { return kFileName; }
			[[nodiscard]] const std::string& FilePath() const noexcept { return kFilePath; }
			[[nodiscard]] ksize_t FileSize() const noexcept { return static_cast<ksize_t>(kFileSize); }
			[[nodiscard]] std::string_view Signature() const noexcept { return std::string_view(reinterpret_cast<const char*>(buffer), SFFFormat::kSignature.size()); }
			[[nodiscard]] uint32_t Version() const noexcept { return DecodeBinary::UInt32BE(&buffer[12]); }
			[[nodiscard]] uint32_t NumGroups() const noexcept { return DecodeBinary::UInt32LE(&buffer[16]); }
			[[nodiscard]] uint32_t NumImages() const noexcept { return DecodeBinary::UInt32LE(&buffer[20]); }
			[[nodiscard]] uint32_t SubHeaderStart() const noexcept { return DecodeBinary::UInt32LE(&buffer[24]); }
			[[nodiscard]] uint32_t FileLength() const noexcept { return DecodeBinary::UInt32LE(&buffer[28]); }
			[[nodiscard]] unsigned char SharedPal() const noexcept { return buffer[32]; }
			[[nodiscard]] bool CheckError() const noexcept { return kCheckError; }

		public:
			T_LoadSFFHeader(const std::string& FileName, const std::string& FilePath)
				: kFileName(EnsureSffExtension(FileName)), kFilePath(FindFilePathDown(FilePath))
				, kFileSize(kFilePath.empty() ? 0 : std::filesystem::file_size(kFilePath)), kCheckError(CheckFileError()) {
			}

			void seekg(std::streampos& _Pos, std::ios_base::seekdir _Way = std::ios::beg) {
				File.seekg(_Pos, _Way);
			}
			void seekg(uint32_t _Pos, std::ios_base::seekdir _Way = std::ios::beg) {
				File.seekg(_Pos, _Way);
			}

			void read(char* _Str, std::streamsize _Count) {
				File.read(_Str, _Count);
			}

			[[nodiscard]] std::streampos tellg() {
				return File.tellg();
			}
		};

		// SFF読み込み時のサブヘッダー情報格納先
		struct T_LoadSFFSubHeader {
		private:
			unsigned char buffer[19] = {}; // 主要部分の情報のみ格納
			// 0～18 メイン情報
			// 19～ ファイル名とか入ってる場合がある(余白)
			// 32～ 画像データ + パレットデータ(0 or 768 Byte)
			int32_t DuplicationCount_ = 0;
			T_LoadSFFHeader& File;
			std::vector<unsigned char> LoadSpriteData = {};
			std::array<unsigned char, SFFFormat::kSFFPaletteSize> LoadPaletteData = {};

			void AddDuplicationCount() {
				++DuplicationCount_;
			}

			void InitLoadSFFSubHeader() {
				File.seekg(File.SubHeaderStart());
			}

			[[nodiscard]] bool CheckReadError() { // データの末尾に到達orアドレスが不正な値
				return !NextAddress() || NextAddress() < File.tellg();
			}

			[[nodiscard]] bool ReadSubHeader() {
				constexpr unsigned long long SkipSize = SFFFormat::kFileLength - sizeof(buffer);
				File.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
				File.seekg(SkipSize, std::ios::cur); // データ余白をスキップ(スプライトバイナリデータ先まで相対移動)
				return CheckReadError();
			}

		public:
			[[nodiscard]] uint32_t NextAddress() const noexcept { return DecodeBinary::UInt32LE(&buffer[0]); }
			[[nodiscard]] uint32_t PCXDataSize() const noexcept { return DecodeBinary::UInt32LE(&buffer[4]); }
			[[nodiscard]] uint16_t AxisX() const noexcept { return DecodeBinary::UInt16LE(&buffer[8]); }
			[[nodiscard]] uint16_t AxisY() const noexcept { return DecodeBinary::UInt16LE(&buffer[10]); }
			[[nodiscard]] uint16_t GroupNo() const noexcept { return DecodeBinary::UInt16LE(&buffer[12]); }
			[[nodiscard]] uint16_t ImageNo() const noexcept { return DecodeBinary::UInt16LE(&buffer[14]); }
			[[nodiscard]] uint16_t SpriteIndex() const noexcept { return DecodeBinary::UInt16LE(&buffer[16]); }
			[[nodiscard]] unsigned char SharedPal() const noexcept { return buffer[18]; }
			[[nodiscard]] bool CheckError() { return false; } // 処理が思いつかないので保留
			[[nodiscard]] int32_t DuplicationCount() const noexcept { return DuplicationCount_; }

		public:
			T_LoadSFFSubHeader(T_LoadSFFHeader& LoadSFFHeader) : File(LoadSFFHeader) {
				InitLoadSFFSubHeader();
			}

			[[nodiscard]] bool ReadSpriteBinary(int32_t LoadNo, T_UnorderedMap& SpriteNumberUMap, T_UnorderedMap& SpriteDataUMap, T_SFFBinaryData& SFFBinaryData) {
				if (ReadSubHeader()) { return true; }

				// 取得した画像番号が重複
				if (SpriteNumberUMap.exist(GroupNo(), ImageNo())) {
					AddDuplicationCount();
					T_ErrorHandle::Instance().SetError(ErrorMessage::DuplicateSpriteNumber, GroupNo(), ImageNo());
					File.seekg(NextAddress());
					return false;
				}

				SpriteNumberUMap.Register(GroupNo(), ImageNo());
				ksize_t SpriteListIndex = 0;
				ksize_t PaletteListIndex = 0;
				ksize_t IndexListNumber = 0;

				// 画像データが存在
				if (PCXDataSize()) {
					bool FoundSpriteData = false;
					bool FoundPaletteData = false;
					const bool SharedPal_ = (!LoadNo ? false : !!SharedPal()); // 先頭画像は固有パレットとして扱う

					// 固有パレットなら画像データ末尾の768Byte(パレットデータ)を除外
					LoadSpriteData.resize(static_cast<size_t>(PCXDataSize()) - (!SharedPal_ ? SFFFormat::kSFFPaletteSize : 0));

					// 画像データ一時保存
					File.read(reinterpret_cast<char*>(LoadSpriteData.data()), LoadSpriteData.size());

					if (SharedPal_) {
						FoundPaletteData = true;
						// 画像番号(0,0)の場合先頭画像のパレットを適用
						// そうでなければ前画像のパレット引き継ぎ
						if (GroupNo() == 0 && ImageNo() == 0) {
							PaletteListIndex = 0;
						}
						else {
							PaletteListIndex = SFFBinaryData.IndexList(SFFBinaryData.DataList().back().IndexListNumber()).PaletteIndex();
						}
					}
					else {
						// パレットデータ一時保存
						File.read(reinterpret_cast<char*>(LoadPaletteData.data()), SFFFormat::kSFFPaletteSize);

						// パレットデータ重複チェック
						for (PaletteListIndex = 0; PaletteListIndex < SFFBinaryData.NumPalette(); ++PaletteListIndex) {
							// 既存のパレットデータの場合はインデックスを指定
							if (!std::memcmp(SFFBinaryData.Palette(PaletteListIndex), LoadPaletteData.data(), SFFFormat::kSFFPaletteSize)) {
								FoundPaletteData = true;
								break;
							}
						}

						// 新規パレットの場合はパレットデータを格納
						if (!FoundPaletteData) {
							PaletteListIndex = SFFBinaryData.NumPalette();
							SFFBinaryData.AddPalette(LoadPaletteData);
						}
					}

					// 画像データ重複チェック
					for (SpriteListIndex = 0; SpriteListIndex < SFFBinaryData.NumSprite(); ++SpriteListIndex) {
						// 既存の画像データの場合はインデックスを指定
						if (SFFBinaryData.SpriteSize(SpriteListIndex) == LoadSpriteData.size() && !std::memcmp(SFFBinaryData.Sprite(SpriteListIndex), LoadSpriteData.data(), LoadSpriteData.size())) {
							FoundSpriteData = true;
							break;
						}

					}
					// 新規画像の場合は画像データを格納
					if (!FoundSpriteData) {
						SFFBinaryData.AddSprite(LoadSpriteData);
					}

					// 画像とパレットの組み合わせが新規の場合インデックスリストへ登録
					if (!FoundPaletteData || !FoundSpriteData || !SpriteDataUMap.exist(SpriteListIndex, PaletteListIndex)) {
						SFFBinaryData.AddIndexList(SpriteListIndex, PaletteListIndex);
						SpriteDataUMap.Register(SpriteListIndex, PaletteListIndex);
						IndexListNumber = static_cast<ksize_t>(SFFBinaryData.IndexList().size()) - 1;
					}
					else {
						IndexListNumber = SpriteDataUMap.find(SpriteListIndex, PaletteListIndex);
					}
				}
				else { // PCXDataなしならコピー元のスプライトインデックス指定
					IndexListNumber = SFFBinaryData.DataList(SpriteIndex() - DuplicationCount()).IndexListNumber();
				}
				SFFBinaryData.AddDataList(IndexListNumber, AxisX(), AxisY(), GroupNo(), ImageNo());

				File.seekg(NextAddress());

				return false;
			}
		};

		// メイン情報
		struct T_SFFData {
		private:
			int32_t NumGroup_ = 0;
			int32_t NumImage_ = 0;
			std::string FileName_ = {};
			T_UnorderedMap SpriteNumberUMap = {};
			T_UnorderedMap SpriteDataUMap = {};
			T_SFFBinaryData SFFBinaryData = {};

			void NumGroup(int32_t value) noexcept { NumGroup_ = value; }
			void NumImage(int32_t value) noexcept { NumImage_ = value; }
			void FileName(const std::string& value) noexcept { FileName_ = value; }

			void ReserveSpriteData(T_LoadSFFHeader& LoadSFFHeader) {
				const ksize_t kNumImage = LoadSFFHeader.NumImages();
				const ksize_t kFileSize = LoadSFFHeader.FileSize();
				const ksize_t kPaletteSize = kNumImage * SFFFormat::kSFFPaletteSize;

				SpriteNumberUMap.reserve(kNumImage);
				SpriteDataUMap.reserve(kNumImage);
				SFFBinaryData.reserve(kNumImage, kFileSize, kPaletteSize);
			}

			void shrink_to_fit() {
				SpriteNumberUMap.shrink_to_fit();
				SpriteDataUMap.shrink_to_fit();
				SFFBinaryData.shrink_to_fit();
			}

			bool LoadSFFFile(const std::string& FileName_, const std::string& FilePath_) {
				if (!empty()) { clear(); }
				T_LoadSFFHeader LoadSFFHeader(FileName_, FilePath_);
				if (LoadSFFHeader.CheckError()) { return false; }
				T_LoadSFFSubHeader LoadSFFSubHeader(LoadSFFHeader);
				if (LoadSFFSubHeader.CheckError()) { return false; }

				ReserveSpriteData(LoadSFFHeader);

				for (int32_t LoadNo = 0; LoadNo < LoadSFFHeader.NumImages(); ++LoadNo) {
					if (LoadSFFSubHeader.ReadSpriteBinary(LoadNo, SpriteNumberUMap, SpriteDataUMap, SFFBinaryData)) { break; };
				}
				NumGroup(LoadSFFHeader.NumGroups());
				NumImage(static_cast<int32_t>(SpriteNumberUMap.size()));
				FileName(LoadSFFHeader.FileName());

				// 全てのロードが終了したら余分に確保したメモリを解放
				shrink_to_fit();

				// ログ出力
				if (T_Config::Instance().CreateLogFile()) {
					T_FilePathSystem SAELibFile(T_Config::Instance().SAELibFilePath() / (T_Config::Instance().CreateSAELibFile() ? ReadSffFileFormat::kSystemDirectoryName : ""));
					if (SAELibFile.ErrorCode()) {
						T_ErrorHandle::Instance().SetError(ErrorMessage::SAELibFolderInvalidPath);
						return false;
					}
					if (T_Config::Instance().CreateSAELibFile()) {
						SAELibFile.CreateDirectory(SAELibFile.Path());
						if (SAELibFile.ErrorCode()) {
							T_ErrorHandle::Instance().SetError(ErrorMessage::CreateSAELibFolderFailed);
							return false;
						}
					}

					const std::string ErrorLogFileName = std::string(ReadSffFileFormat::kErrorLogFileName) + "_" + FileName() + ".txt";
					std::ofstream ErrorLogFile(SAELibFile.Path() / ErrorLogFileName);
					if (!ErrorLogFile.is_open()) {
						T_ErrorHandle::Instance().SetError(ErrorMessage::CreateErrorLogFileFailed);
					}
					T_ErrorHandle::Instance().WriteErrorLog(ErrorLogFile);
				}

				return true;
			}

			// バイナリデータから出力(重複なし)
			[[nodiscard]] bool WriteBMPFile(ksize_t index, const std::filesystem::path& FullPath) const {
				// SFFのバイナリからBMPフォーマットへ組み立て
				T_BuildBMPBinary BMPBinary(SFFBinaryData.IndexList_Sprite(index), SFFBinaryData.IndexList_Palette(index), SFFBinaryData.IndexList_SpriteSize(index));
				std::ofstream File(FullPath, std::ios::binary);
				if (!File.is_open()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::CreateBMPFileFailed);
				}

				File.write(reinterpret_cast<const char*>(BMPBinary.data()), BMPBinary.size());
				File.flush();

				if (File.fail() || File.bad()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::WriteBMPFileFailed);
				}
				File.close();
				if (File.fail() || File.bad()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::CloseBMPFileFailed);
				}

				return File.good();
			}

			// スプライトリストから出力(重複有り)
			[[nodiscard]] bool WriteBMPFile(ksize_t index, const std::filesystem::path& FullPath, bool DuplicationSprite) const {
				// SFFのバイナリからBMPフォーマットへ組み立て
				T_BuildBMPBinary BMPBinary(SFFBinaryData.DataList_Sprite(index), SFFBinaryData.DataList_Palette(index), SFFBinaryData.DataList_SpriteSize(index));
				std::ofstream File(FullPath, std::ios::binary);
				if (!File.is_open()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::CreateBMPFileFailed);
				}

				File.write(reinterpret_cast<const char*>(BMPBinary.data()), BMPBinary.size());
				File.flush();

				if (File.fail() || File.bad()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::WriteBMPFileFailed);
				}
				File.close();
				if (File.fail() || File.bad()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::CloseBMPFileFailed);
				}

				return File.good();
			}

			// ユーザー向けのT_DataListアクセス手段
			struct T_AccessData {
			private:
				const T_SFFBinaryData* const kSFFBinaryDataPtr;
				const ksize_t kDataListIndex; // 配列Index(最大値のときダミーデータフラグとして使用)

				const auto& ParamRef() const noexcept { return kSFFBinaryDataPtr->DataList(kDataListIndex); }

				inline static constexpr unsigned char kDummyBinaryArray[1] = { 0 };
				inline static constexpr const unsigned char* kDummyBinaryData = kDummyBinaryArray;

			public:
				/**
				* @brief ダミーデータ判断
				*
				* 　自身がダミーデータであるかを確認します
				* 
				* 　SFFConfig::SetThrowErrorの設定がOFFの場合にエラー回避のために使用されます
				*
				* @return bool (false = 自身が正常なデータ：true = 自身がダミーデータ)
				*/
				bool IsDummy() const noexcept { return kDataListIndex == KSIZE_MAX; }

				/**
				* @brief 軸座標Xの取得
				*
				* 　SAEで設定した軸座標Xを返します
				*
				* 　ダミーデータの場合は 0 を返します
				*
				* @return int32_t AxisX 軸座標X
				*/
				int32_t AxisX() const noexcept { return (IsDummy() ? 0 : ParamRef().AxisX()); }
			
				/**
				* @brief 軸座標Yの取得
				*
				* 　SAEで設定した軸座標Yを返します
				*
				* 　ダミーデータの場合は 0 を返します
				*
				* @return int32_t AxisY 軸座標Y
				*/
				int32_t AxisY() const noexcept { return (IsDummy() ? 0 : ParamRef().AxisY()); }
			
				/**
				* @brief グループ番号の取得
				*
				* 　SAEで設定したグループ番号を返します
				*
				* 　ダミーデータの場合は 0 を返します
				*
				* @return int32_t GroupNo グループ番号
				*/
				int32_t GroupNo() const noexcept { return (IsDummy() ? 0 : ParamRef().GroupNo()); }

				/**
				* @brief イメージ番号の取得
				*
				* 　SAEで設定したイメージ番号を返します
				*
				* 　ダミーデータの場合は 0 を返します
				*
				* @return int32_t ImageNo イメージ番号
				*/
				int32_t ImageNo() const noexcept { return (IsDummy() ? 0 : ParamRef().ImageNo()); }
				
				/**
				* @brief ピクセルデータの取得
				*
				* 　画像のピクセルデータ配列を返します
				*
				* 　ダミーデータの場合は DummyBinaryData を返します
				* 
				* 　DummyBinaryData は常に長さ1の配列で内容は {0} です
				*
				* @return const unsigned char* const PixelBinaryData ピクセルデータ配列
				* @retval const unsigned char* const DummyBinaryData ダミーデータ配列
				*/
				const unsigned char* const PixelBinaryData() const noexcept { return (IsDummy() ? kDummyBinaryData : kSFFBinaryDataPtr->IndexList_Sprite(ParamRef().IndexListNumber())); }
				
				/**
				* @brief ピクセルデータサイズの取得
				*
				* 　ピクセルデータのバイトサイズを返します
				*
				* 　ダミーデータの場合は 0 を返します
				*
				* @return size_t PixelBinaryDataSize ピクセルデータバイトサイズ
				*/
				size_t PixelBinaryDataByteSize() const noexcept { return static_cast<size_t>(IsDummy() ? 0 : kSFFBinaryDataPtr->IndexList_SpriteSize(ParamRef().IndexListNumber())); }
			
				/**
				* @brief パレットデータの取得
				*
				* 　画像のパレットデータ配列を返します
				*
				* 　ダミーデータの場合は DummyBinaryData を返します
				* 
				* 　DummyBinaryData は常に長さ1の配列で内容は {0} です
				*
				* @return const unsigned char* const PaletteBinaryData パレットデータ配列
				* @retval const unsigned char* const DummyBinaryData ダミーデータ配列
				*/
				const unsigned char* const PaletteBinaryData() const noexcept { return (IsDummy() ? kDummyBinaryData : kSFFBinaryDataPtr->IndexList_Palette(ParamRef().IndexListNumber())); }
				
				/**
				* @brief BMPデータの取得
				*
				* 　画像をBMP形式に変換したデータを返します
				*
				* 　ダミーデータの場合は 0 を返します
				*
				* @return std::vector<unsigned char> BuildBMPBinaryData BMPデータ
				*/
				std::vector<unsigned char> BuildBMPBinaryData() const { return T_BuildBMPBinary(PixelBinaryData(), PaletteBinaryData(), static_cast<ksize_t>(PixelBinaryDataByteSize())).vecdata(); }
				
				/**
				* @brief 画像の幅を取得
				*
				* 　ピクセルデータに記録されている画像の幅を返します
				* 
				* 　ダミーデータの場合は 0 を返します
				*
				* @return uint16_t Width 画像の幅
				*/
				uint16_t PixelWidth() const noexcept { return (IsDummy() ? 0 : 1 + DecodeBinary::UInt16LE(&PixelBinaryData()[8])); }
				
				/**
				* @brief 画像の高さを取得
				*
				* 　ピクセルデータに記録されている画像の高さを返します
				*
				* 　ダミーデータの場合は 0 を返します
				*
				* @return uint16_t Height 画像の高さ
				*/
				uint16_t PixelHeight() const noexcept { return (IsDummy() ? 0 : 1 + DecodeBinary::UInt16LE(&PixelBinaryData()[10])); }

				T_AccessData(const T_SFFBinaryData* const SFFBinaryDataPtr, const ksize_t DataListIndex) : kSFFBinaryDataPtr(SFFBinaryDataPtr), kDataListIndex(DataListIndex) {}
			};

		public:
			/**
			* @brief SFFデータの画像グループ数を取得
			*
			* 　読み込んだSFFデータの画像グループ数を返します
			*
			* @return int32_t NumGroup 画像グループ数
			*/
			int32_t NumGroup() const noexcept { return NumGroup_; }
		
			/**
			* @brief SFFデータの画像数を取得
			*
			* 　読み込んだSFFデータの画像数を返します
			*
			* @return int32_t NumImage 画像数
			*/
			int32_t NumImage() const noexcept { return NumImage_; }
			
			/**
			* @brief SFFデータのファイル名を取得
			*
			* 　読み込んだSFFデータの拡張子を除いたファイル名を返します
			*
			* @return const std::string& FileName ファイル名
			*/
			const std::string& FileName() const noexcept { return FileName_; }

			/**
			* @brief SFFデータの初期化
			*
			* 　読み込んだSFFデータを初期化します
			*
			* @note
			*/
			void clear() {
				NumGroup(0);
				NumImage(0);
				FileName_.clear();
				SpriteNumberUMap.clear();
				SpriteDataUMap.clear();
				SFFBinaryData.clear();
			}

			/**
			* @brief SFFデータの存在確認
			*
			* 　読み込んだSFFデータの空かを判定します  
			*
			* @return bool 判定結果 (false = データが存在：true = データが空)
			*/
			bool empty() const noexcept {
				return FileName().empty() && SFFBinaryData.empty() && SpriteNumberUMap.empty() && SpriteDataUMap.empty();
			}

			/**
			* @brief SFFデータのデータサイズを取得
			*
			* 　読み込んだSFFデータのデータサイズを返します
			*
			* @return size_t SFFDataSize SFFデータサイズ
			*/
			size_t size() const noexcept {
				return SFFBinaryData.size();
			}

		public:
			using SpriteData = T_AccessData;

			T_SFFData() = default;

			T_SFFData(const std::string& FileName, const std::string& FilePath = "")
			{
				LoadSFFFile(FileName, FilePath);
			}

			/**
			* @brief 指定されたSFFファイルを読み込み
			*
			* 　実行ファイルから子階層へファイル名を検索して読み込みます
			*
			* 　第二引数指定時は指定した階層からファイル名を検索します(SFFConfigよりも優先されます)
			*
			* 　実行時に既存の要素は初期化、上書きされます
			*
			* @param const std::string& FileName ファイル名 (拡張子 .sff は省略可)
			* @param const std::string& FilePath 対象のパス (省略時は実行ファイルの子階層を探索)
			* @return bool 読み込み結果 (false = 失敗：true = 成功)
			*/
			bool LoadSFF(const std::string& FileName, const std::string& FilePath = "") {
				return LoadSFFFile(FileName, FilePath);
			}

			/**
			* @brief 指定番号の存在確認
			*
			* 　読み込んだSFFデータを検索し、指定番号が存在するかを確認します
			*
			* @param int32_t GroupNo グループ番号
			* @param int32_t ImageNo イメージ番号
			* @return bool 検索結果 (false = 存在なし : true = 存在あり)
			*/
			bool ExistSpriteNumber(int32_t GroupNo, int32_t ImageNo) {
				return SpriteNumberUMap.exist(GroupNo, ImageNo);
			}

			/**
			* @brief 指定番号のデータへアクセス
			*
			* 　指定したグループ番号とイメージ番号のSFFデータへアクセスします
			*
			* 　対象が存在しない場合はSFFConfig::SetThrowErrorの設定に準拠します
			*
			* @param int32_t GroupNo グループ番号
			* @param int32_t ImageNo イメージ番号
			* @retval 対象が存在する SpriteData
			* @retval 対象が存在しない SFFConfig::SetThrowError (false = ダミーデータの参照：true = 例外を投げる)
			*/
			const SpriteData GetSpriteData(int32_t GroupNo, int32_t ImageNo) {
				if (int32_t SpriteNumber = SpriteNumberUMap.find(GroupNo, ImageNo); SpriteNumber >= 0) { // SpriteExist(GroupNo, ImageNo)と同義
					return SpriteData(&SFFBinaryData, SpriteNumber);
				}
				if (!T_Config::Instance().ThrowError()) {
					return SpriteData(&SFFBinaryData, KSIZE_MAX);
				}
				T_ErrorHandle::Instance().ThrowError(ErrorMessage::SpriteNumberNotFound, GroupNo, ImageNo);
			}

			/**
			* @brief 指定インデックスデータの存在確認
			*
			* 　読み込んだSFFデータを検索し、指定インデックスのデータ存在するかを確認します
			*
			* @param int32_t index データ配列インデックス
			* @return bool 検索結果 (false = 存在なし : true = 存在あり)
			*/
			bool ExistSpriteDataIndex(int32_t SpriteDataIndex) const {
				return static_cast<ksize_t>(SpriteDataIndex) < SFFBinaryData.DataList().size();
			}

			/**
			* @brief 指定インデックスのデータへアクセス
			*
			* 　SFFデータへ指定したインデックスでアクセスします
			*
			* 　対象が存在しない場合はSFFConfig::SetThrowErrorの設定に準拠します
			*
			* @param int32_t index データ配列インデックス
			* @retval 対象が存在する SpriteData
			* @retval 対象が存在しない SFFConfig::SetThrowError (false = ダミーデータの参照：true = 例外を投げる)
			*/
			const SpriteData GetSpriteDataIndex(int32_t index) const {
				if (ExistSpriteDataIndex(index)) {
					return SpriteData(&SFFBinaryData, index);
				}
				if (!T_Config::Instance().ThrowError()) {
					return SpriteData(&SFFBinaryData, KSIZE_MAX);
				}
				T_ErrorHandle::Instance().ThrowError(ErrorMessage::SpriteIndexNotFound, index);
			}

			/**
			* @brief 指定番号の画像をBMP出力
			*
			* 　指定したグループ番号とイメージ番号のSFFデータをBMPファイルとして出力します
			*
			* 　出力先のファイルは SFFConfig::SetSAELibPath の設定に準拠します
			*
			* @param int32_t GroupNo グループ番号
			* @param int32_t ImageNo イメージ番号
			* @return bool 出力結果 (false = 失敗：true = 成功)
			*/
			bool ExportToBMP(int32_t GroupNo, int32_t ImageNo) {
				if (FileName().empty()) { return false; }
				if (ExistSpriteNumber(GroupNo, ImageNo)) {
					T_FilePathSystem SAELibFile(T_Config::Instance().SAELibFilePath() / (T_Config::Instance().CreateSAELibFile() ? ReadSffFileFormat::kSystemDirectoryName : ""));
					if (SAELibFile.ErrorCode()) {
						T_ErrorHandle::Instance().SetError(ErrorMessage::SAELibFolderInvalidPath);
						return false;
					}
					if (T_Config::Instance().CreateSAELibFile()) {
						SAELibFile.CreateDirectory(SAELibFile.Path());
						if (SAELibFile.ErrorCode()) {
							T_ErrorHandle::Instance().SetError(ErrorMessage::CreateSAELibFolderFailed);
							return false;
						}
					}

					const std::filesystem::path SaveFileName = "SFF_" + std::to_string(GroupNo) + "-" + std::to_string(ImageNo) + ".bmp";
					return WriteBMPFile(SpriteNumberUMap.find(GroupNo, ImageNo), SAELibFile.Path() / SaveFileName, true);
				}
				T_ErrorHandle::Instance().SetError(ErrorMessage::SpriteNumberNotFound, GroupNo, ImageNo);
				return false;
			}

			/**
			* @brief 全ての格納画像をBMP出力
			*
			* 　読み込んだSFFデータ全てをBMPファイルとして出力します
			*
			* 　出力先のファイルは SFFConfig::SetSAELibPath の設定に準拠します
			*
			* @param bool DuplicationSprite 重複した画像を出力するか(false = 含まない：true = 含む)
			* @return bool 出力結果 (false = 失敗：true = 成功)
			*/
			bool ExportToBMP(const bool DuplicationSprite = true) {
				if (FileName().empty()) { return false; }
				T_FilePathSystem SAELibFile(T_Config::Instance().SAELibFilePath() / (T_Config::Instance().CreateSAELibFile() ? ReadSffFileFormat::kSystemDirectoryName : ""));
				if (SAELibFile.ErrorCode()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::SAELibFolderInvalidPath);
					return false;
				}
				if (T_Config::Instance().CreateSAELibFile()) {
					SAELibFile.CreateDirectory(SAELibFile.Path());
					if (SAELibFile.ErrorCode()) {
						T_ErrorHandle::Instance().SetError(ErrorMessage::CreateSAELibFolderFailed);
						return false;
					}
				}

				const std::string DirectoryName = "ExportToBMP_" + FileName() + (DuplicationSprite ? "_DuplicationSprite" : "");
				SAELibFile.CreateDirectory(SAELibFile.Path() / DirectoryName);
				if (SAELibFile.ErrorCode()) {
					T_ErrorHandle::Instance().SetError(ErrorMessage::CreateExportBMPFolderFailed);
					return false;
				}

				// BMP出力
				std::string FileName = {};
				if (DuplicationSprite) {
					for (ksize_t SpriteListNumber = 0; SpriteListNumber < SFFBinaryData.DataList().size(); ++SpriteListNumber) {
						FileName = "SFF_" + std::to_string(SFFBinaryData.DataList(SpriteListNumber).GroupNo()) + "-" + std::to_string(SFFBinaryData.DataList(SpriteListNumber).ImageNo()) + ".bmp";
						if (!WriteBMPFile(SpriteListNumber, SAELibFile.Path() / DirectoryName / FileName, true)) {
							return false;
						}
					}
				}
				else {
					for (ksize_t IndexListNumber = 0; IndexListNumber < SFFBinaryData.IndexList().size(); ++IndexListNumber) {
						FileName = "SFF_No_" + std::to_string(IndexListNumber) + ".bmp";
						if (!WriteBMPFile(IndexListNumber, SAELibFile.Path() / DirectoryName / FileName)) {
							return false;
						}
					}
				}

				return true;
			}
		}; // struct T_SFFData
	} // namespace ReadSffFile_detail

	// 使用ユーザー向けの名前設定

	/**
	* @brief SFFファイルを扱うクラス
	*
	* 　- コンストラクタの引数を指定した場合、指定した引数でLoadSFF関数を実行します
	*
	* 　- 引数を指定しない場合、ファイル読み込みは行いません
	*
	* @param const std::string& FileName ファイル名 (拡張子 .sff は省略可)
	* @param const std::string& FilePath 対象のパス (省略時は実行ファイルの子階層を探索)
	*/
	using SFF = ReadSffFile_detail::T_SFFData;

	/**
	* @brief ReadSffFileのエラー情報空間
	*/
	namespace SFFError = ReadSffFile_detail::ErrorMessage;

	/**
	* @brief ReadSffFileのコンフィグ設定空間
	*/
	namespace SFFConfig {

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Setter /////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		* @brief エラー出力の切り替え設定
		*
		* 　このライブラリ関数で発生したエラーを例外として投げるかログとして記録するかを指定できます
		*
		* @param bool flag (false = ログとして記録する：true = 例外を投げる)
		*/
		inline void SetThrowError(bool flag) { ReadSffFile_detail::T_Config::Instance().ThrowError(flag); }

		/**
		* @brief エラーログファイルを作成設定
		*
		* 　このライブラリ関数で発生したエラーのログファイルを出力するかどうか指定できます
		*
		* @param bool flag (false = ログファイルを出力しない：true = ログファイルを出力する)
		*/
		inline void SetCreateLogFile(bool flag) { ReadSffFile_detail::T_Config::Instance().CreateLogFile(flag); }

		/**
		* @brief SAELibフォルダを作成設定
		*
		* 　ファイルの出力先としてSAELibファイルを使用するかを指定できます
		*
		* @param bool flag (false = SAELibファイルを使用しない：true = SAELibファイルを使用する)
		* @param const std::string& Path SAELibフォルダ作成先 (省略時はパスの設定なし)
		*/
		inline void SetCreateSAELibFile(bool flag, const std::string& Path = "") {
			ReadSffFile_detail::T_Config::Instance().CreateSAELibFile(flag);
			if (!Path.empty()) {
				ReadSffFile_detail::T_Config::Instance().SAELibFilePath(Path);
			}
		}

		/**
		* @brief SAELibフォルダのパス設定
		*
		* 　SAELibファイルの作成パスを指定できます
		*
		* @param const std::string& Path SAELibフォルダ作成先
		*/
		inline void SetSAELibFilePath(const std::string& Path = "") { ReadSffFile_detail::T_Config::Instance().SAELibFilePath(Path); }

		/**
		* @brief SFFファイルの検索パス設定
		*
		* 　SFFファイルの検索先のパスを指定できます
		*
		* 　SFFコンストラクタもしくはLoadSFF関数で検索先のパスを指定しない場合、この設定のパスで検索します
		*
		* @param const std::string& Path SFFファイルの検索先のパス
		*/
		inline void SetSFFSearchPath(const std::string& Path = "") { ReadSffFile_detail::T_Config::Instance().SFFSearchPath(Path); }
	
		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Getter /////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		* @brief エラー出力切り替え設定取得
		*
		* 　Config設定のエラー出力切り替え設定を取得します
		*
		* @return bool エラー設定出力切り替え設定(false = OFF：true = ON)
		*/
		inline bool GetThrowError() { return ReadSffFile_detail::T_Config::Instance().ThrowError(); }

		/**
		* @brief エラーログファイルを作成設定取得
		*
		* 　Config設定のエラーログファイルを作成設定を取得します
		*
		* @return bool エラーログファイルを作成設定(false = OFF：true = ON)
		*/
		inline bool GetCreateLogFile() { return ReadSffFile_detail::T_Config::Instance().CreateLogFile(); }

		/**
		* @brief SAELibフォルダを作成設定取得
		*
		* 　Config設定のSAELibフォルダを作成設定を取得します
		*
		* @return bool SAELibフォルダを作成設定(false = OFF：true = ON)
		*/
		inline bool GetCreateSAELibFile() { return ReadSffFile_detail::T_Config::Instance().CreateSAELibFile(); }

		/**
		* @brief Config設定取得
		*
		* 　Config設定のフラグをまとめて取得します
		*
		* @return int32_t Config設定
		*/
		inline int32_t GetConfigFlag() { return ReadSffFile_detail::T_Config::Instance().BitFlag(); }

		/**
		* @brief SAELibフォルダを作成パス取得
		*
		* 　Config設定のSAELibフォルダを作成パスを取得します
		*
		* @return const std::filesystem::path& SAELibフォルダを作成パス
		*/
		inline const std::filesystem::path& GetSAELibFilePath() { return ReadSffFile_detail::T_Config::Instance().SAELibFilePath(); }

		/**
		* @brief SFFファイルの検索パス取得
		*
		* 　Config設定のSFFファイルの検索パスを取得します
		*
		* @return const std::filesystem::path& SFFファイルの検索パス
		*/
		inline const std::filesystem::path& GetSFFSearchPath() { return ReadSffFile_detail::T_Config::Instance().SFFSearchPath(); }
	}
} // namespace SAELib

#endif