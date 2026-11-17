#pragma once
#include <afxwin.h>

// アプリケーションの設定情報を保持する構造体
struct AppConfig {
    CString dogaCgaDir;
    CString partsDir;
    CString atrDir;
    CString atrFile;

    // 結合した最終パスを取得するヘルパー
    CString GetPartsFullPath() const { return dogaCgaDir + partsDir; }
    CString GetAtrFullPath() const { return dogaCgaDir + atrDir; }
    CString GetAtrFileFullPath() const { return dogaCgaDir + atrFile; }
};

// アプリ初期設定ルーチン
// 必要に応じてiniファイルを生成・読込し、フォルダがない場合は選択ダイアログを表示してiniを書き換えます
AppConfig InitializeAppConfig();
// フォルダが存在するか確認するヘルパー関数
static bool FolderExists(const CString& path);
// 実行中アプリケーション自身のフルパスから拡張子を .ini に変更したパスを取得
static CString GetIniPath();
// 実行中アプリケーション自身のファイル名（拡張子なし）を取得
static CString GetAppName();
// デフォルト設定の .ini ファイルを新規作成（ユーザー指定の形式）
static void CreateDefaultIni(const CString& iniPath, const CString& appName);
// .ini ファイルから設定を読み込み
static bool LoadIni(const CString& iniPath, AppConfig& config);
// .ini ファイルの DOGACGADir= 行のみを新しいパスに書き換える
static void UpdateDogaCgaDirInIni(const CString& iniPath, const CString& newDir);
// フォルダ選択ダイアログを表示してインストール先を指定してもらう
static bool PromptForDogaCgaDir(CString& selectedDir);

