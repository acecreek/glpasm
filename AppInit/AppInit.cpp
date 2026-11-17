#include "../pch.h"
#include "AppInit.h"
#include <afxdlgs.h> // CFolderPickerDialog のため
#include <format>

// ソース内の文字リテラルをUTF-8で解釈させる（MSVC向け）
#pragma execution_character_set("utf-8")

// フォルダが存在するか確認するヘルパー関数
static bool FolderExists(const CString& path) {
    DWORD dwAttrib = ::GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// 実行中アプリケーション自身のフルパスから拡張子を .ini に変更したパスを取得
static CString GetIniPath() {
    TCHAR szPath[MAX_PATH];
    ::GetModuleFileName(NULL, szPath, MAX_PATH);
    CString iniPath(szPath);
    int pos = iniPath.ReverseFind('.');
    if (pos != -1) {
        iniPath = iniPath.Left(pos) + _T(".ini");
    } else {
        iniPath += _T(".ini");
    }
    return iniPath;
}

// 実行中アプリケーション自身のファイル名（拡張子なし）を取得
static CString GetAppName() {
    TCHAR szPath[MAX_PATH];
    ::GetModuleFileName(NULL, szPath, MAX_PATH);
    CString appName(szPath);
    int pos = appName.ReverseFind('\\');
    if (pos != -1) {
        appName = appName.Mid(pos + 1);
    }
    pos = appName.ReverseFind('.');
    if (pos != -1) {
        appName = appName.Left(pos);
    }
    return appName;
}

// デフォルト設定の .ini ファイルを新規作成（ユーザー指定の形式）
static void CreateDefaultIni(const CString& iniPath, const CString& appName) {
    CStdioFile file;
    if (file.Open(iniPath, CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
        // 新仕様: ini にはコメント行と DOGACGADir 行のみを出力する
        CStringArray lines;
        CString first;
        first.Format(_T("// %s.ini"), static_cast<LPCTSTR>(appName));
//        first.Format(_T("// %s.ini"), appName.GetString());
//        #include <format>
//        first = CString(std::format(L"// {}.ini", std::wstring(appName)));

        lines.Add(first);
        lines.Add(_T(""));
        lines.Add(_T("DOGACGADir=C:\\Program Files\\DOGACGA"));
        lines.Add(_T(""));

        for (INT_PTR i = 0; i < lines.GetCount(); ++i) {
            file.WriteString(lines[i] + _T("\n"));
        }
        file.Close();

        CString msg;
        msg.Format(_T("Created default ini:\r\n%s"), static_cast<LPCTSTR>(iniPath));
        ::MessageBox(nullptr, msg, _T("glsuf.ini created"), MB_OK | MB_ICONINFORMATION);
    }
    else {
        CString msg;
        msg.Format(_T("Failed to create ini file:\r\n%s\r\nCheck write permissions."), static_cast<LPCTSTR>(iniPath));
        ::MessageBox(nullptr, msg, _T("glsuf.ini error"), MB_OK | MB_ICONERROR);
    }
}

// .ini ファイルから設定を読み込み
static bool LoadIni(const CString& iniPath, AppConfig& config) {
    CStdioFile file;
    if (!file.Open(iniPath, CFile::modeRead | CFile::typeText)) {
        return false;
    }

    CString line;
    while (file.ReadString(line)) {
        line.Trim();
        // コメント行（//で始まる行）はスキップ
        if (line.Left(2) == _T("//")) {
            continue;
        }

        int eq = line.Find('=');
        if (eq != -1) {
            CString key = line.Left(eq);
            CString val = line.Mid(eq + 1);
            key.Trim();
            val.Trim();

            // 新設計: ini からは DOGACGADir のみを読み取る
            if (key.CompareNoCase(_T("DOGACGADir")) == 0) {
                config.dogaCgaDir = val;
            }
        }
    }
    file.Close();
    return true;
}

// .ini ファイルの DOGACGADir= 行のみを新しいパスに書き換える
static void UpdateDogaCgaDirInIni(const CString& iniPath, const CString& newDir) {
    CStdioFile file;
    if (!file.Open(iniPath, CFile::modeRead | CFile::typeText)) {
        return;
    }

    CStringArray lines;
    CString line;
    bool replaced = false;
    while (file.ReadString(line)) {
        CString trimmed = line;
        trimmed.Trim();
        // DOGACGADir= で始まる行を見つけたら置換
        if (trimmed.Left(11).CompareNoCase(_T("DOGACGADir=")) == 0) {
            line = _T("DOGACGADir=") + newDir;
            replaced = true;
        }
        lines.Add(line);
    }
    file.Close();

    // ファイルを再作成して内容を書き戻し
    if (file.Open(iniPath, CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
        for (INT_PTR i = 0; i < lines.GetCount(); ++i) {
            file.WriteString(lines[i] + _T("\n"));
        }
        file.Close();
    }

    // DOGACGADir 行が存在しなければ新規作成
    if (!replaced) {
    	CreateDefaultIni(iniPath, GetAppName());
    }

}

// フォルダ選択ダイアログを表示してインストール先を指定してもらう
static bool PromptForDogaCgaDir(CString& selectedDir) {
    ::AfxMessageBox(
        _T("DoGA-Lシリーズのフォルダがデフォルトのインストール先に見つかりません。\nインストール先を設定してください。"),
        MB_OK | MB_ICONWARNING );

    // CFolderPickerDialog (Vista以降標準のフォルダ選択ダイアログ)
    CFolderPickerDialog dlg(nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, nullptr);
    if (dlg.DoModal() == IDOK) {
        selectedDir = dlg.GetPathName();
        return true;
    }
    return false;
}

// アプリケーション初期化の主処理
AppConfig InitializeAppConfig() {
    AppConfig config;
    CString iniPath = GetIniPath();
    CString appName = GetAppName();

    // 1. iniファイルが存在しない場合はデフォルトの内容で作成
    if (::GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES) {
        CreateDefaultIni(iniPath, appName);
    }

    // 2. iniファイルを読み込む
    if (!LoadIni(iniPath, config)) {
        // 読み込み失敗時は DOGACGADir の既定値のみ設定
        config.dogaCgaDir = _T("C:\\Program Files\\DOGACGA");
    }

    // 固定文字列は ini に書き込まず、常に初期化時に設定する
    config.partsDir = _T("\\common\\parts");
    config.atrDir = _T("\\common\\atr");
    config.atrFile = _T("\\common\\atr\\genie.atr");

    // 3. インストール先フォルダが見つからない場合は見つかるかキャンセルされるまでループ
    while (!FolderExists(config.dogaCgaDir)) {
        CString newDir;
        if (PromptForDogaCgaDir(newDir)) {
            config.dogaCgaDir = newDir;
            // .ini ファイルの DOGACGADir= 行を書き換える
            UpdateDogaCgaDirInIni(iniPath, newDir);
        } else {
            // フォルダ選択がキャンセルされた場合はループを抜けて現在の設定（存在しないパス）のまま戻る
            break;
        }
    }

    return config;
}
