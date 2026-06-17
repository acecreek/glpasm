#include "../pch.h"
#include "GLSufApp.h"
#include "../MainFrame.h"

CGLSufApp theApp;

BOOL CGLSufApp::InitInstance()
{
    CWinApp::InitInstance();
    InitCommonControls();

    // AppInitの初期設定処理を実行
    m_config = InitializeAppConfig();

    auto* frame = new CMainFrame();
    if (!frame->Create(nullptr, _T("GL SUF Viewer"), WS_OVERLAPPEDWINDOW,
        CRect(80, 80, 1280, 820)))
    {
        delete frame;
        return FALSE;
    }

    m_pMainWnd = frame;
    frame->ShowWindow(SW_SHOW);
    frame->UpdateWindow();
    return TRUE;
}

