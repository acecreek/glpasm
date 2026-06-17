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
