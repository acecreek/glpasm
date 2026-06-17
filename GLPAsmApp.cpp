#include "pch.h"
#include "GLPAsmApp.h"
#include "MainFrame.h"

CGLPAsmApp theApp;

BOOL CGLPAsmApp::InitInstance()
{
    CWinApp::InitInstance();
    InitCommonControls();

    auto* frame = new CMainFrame();
    if (!frame->Create(nullptr, _T("GL PartsAsm"), WS_OVERLAPPEDWINDOW,
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
