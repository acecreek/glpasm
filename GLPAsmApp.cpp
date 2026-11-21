#include "pch.h"
#include "GLPAsmApp.h"
#include "MainFrame.h"
#include "WindowSettings.hpp"
#include "AppInit/AppInit.h"

CGLPAsmApp theApp;

BOOL CGLPAsmApp::InitInstance()
{
    CWinApp::InitInstance();
    InitCommonControls();

    // 起動時に初期設定ルーチンを呼び出し、設定情報を取得
    AppConfig config = InitializeAppConfig();

    // ※ 取得した config.GetPartsFullPath() や config.GetAtrFullPath() を
    //    モデルの読み込み先フォルダの初期値としてお使いいただけます。

    auto* frame = new CMainFrame(config);
    if (!frame->Create(nullptr, _T("GL PartsAsm"), WS_OVERLAPPEDWINDOW,
        CRect(80, 80, 1280, 820)))
    {
        delete frame;
        return FALSE;
    }

    m_pMainWnd = frame;
    frame->ShowWindow(WindowSettings::Restore(*frame));
    frame->UpdateWindow();
    return TRUE;
}
