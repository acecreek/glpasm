#include "pch.h"
#include "MainFrame.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_FILE_OPEN_LXP, &CMainFrame::OnFileOpen)
    ON_COMMAND(ID_FILE_EXIT, &CMainFrame::OnFileExit)
    ON_COMMAND(ID_VIEW_GRID, &CMainFrame::OnViewGrid)
    ON_COMMAND(ID_VIEW_AXES, &CMainFrame::OnViewAxes)
    ON_COMMAND(ID_VIEW_WIREFRAME, &CMainFrame::OnViewWireframe)
    ON_COMMAND(ID_VIEW_FRAME_ALL, &CMainFrame::OnViewFrameAll)
    ON_COMMAND(ID_VIEW_RESET_CAMERA, &CMainFrame::OnViewResetCamera)
    ON_LBN_SELCHANGE(IDC_GLPASM_PART_LIST, &CMainFrame::OnPartSelectionChanged)
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
    : m_assembly(std::make_shared<PartAssembly>())
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT cs)
{
    if (CFrameWnd::OnCreate(cs) == -1) return -1;

    BuildMenu();

    m_leftBar.Create(_T(" Select\n Move\n Rotate\n Scale\n Measure"),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CRect(0, 0, 1, 1), this, IDC_GLPASM_LEFT_BAR);

    m_partList.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        CRect(0, 0, 1, 1), this, IDC_GLPASM_PART_LIST);

    m_status.Create(_T("Ready"), WS_CHILD | WS_VISIBLE | SS_LEFT,
        CRect(0, 0, 1, 1), this, IDC_GLPASM_STATUS);

    if (!m_view.CreateView(this, IDC_GLPASM_GLVIEW)) return -1;
    m_view.SetAssembly(m_assembly);

    SetStatusText(_T("Open an L2P/L3P file to populate the assembly."));
    return 0;
}

void CMainFrame::OnSize(UINT type, int cx, int cy)
{
    CFrameWnd::OnSize(type, cx, cy);
    LayoutChildren(cx, cy);
}

void CMainFrame::OnFileOpen()
{
    CFileDialog dlg(TRUE, _T("l3p"), nullptr,
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("DoGA part assembly (*.l3p;*.l2p;*.fsc)|*.l3p;*.l2p;*.fsc|All files (*.*)|*.*||"),
        this);

    if (dlg.DoModal() != IDOK) return;

    CString error;
    auto loaded = std::make_shared<PartAssembly>();
    if (!loaded->LoadLxP(dlg.GetPathName(), error))
    {
        AfxMessageBox(error, MB_ICONERROR);
        return;
    }

    m_assembly = loaded;
    m_view.SetAssembly(m_assembly);
    RefreshPartList();

    CString title;
    title.Format(_T("GL PartsAsm - %s"), dlg.GetPathName().GetString());
    SetWindowText(title);

    CString status;
    status.Format(_T("Loaded %zu part(s): %s"), m_assembly->Parts().size(), dlg.GetPathName().GetString());
    SetStatusText(status);
}

void CMainFrame::OnFileExit()
{
    PostMessage(WM_CLOSE);
}

void CMainFrame::OnViewGrid()
{
    m_view.ToggleGrid();
}

void CMainFrame::OnViewAxes()
{
    m_view.ToggleAxes();
}

void CMainFrame::OnViewWireframe()
{
    m_view.ToggleWireframe();
}

void CMainFrame::OnViewFrameAll()
{
    m_view.FrameAll();
}

void CMainFrame::OnViewResetCamera()
{
    m_view.ResetCamera();
}

void CMainFrame::OnPartSelectionChanged()
{
    const int sel = m_partList.GetCurSel();
    m_view.SetSelectedPart(sel);
}

void CMainFrame::BuildMenu()
{
    auto* menu = new CMenu();
    auto* file = new CMenu();
    auto* view = new CMenu();
    menu->CreateMenu();
    file->CreatePopupMenu();
    view->CreatePopupMenu();

    file->AppendMenu(MF_STRING, ID_FILE_OPEN_LXP, _T("&Open L2P/L3P..."));
    file->AppendMenu(MF_SEPARATOR);
    file->AppendMenu(MF_STRING, ID_FILE_EXIT, _T("E&xit"));

    view->AppendMenu(MF_STRING, ID_VIEW_GRID, _T("Toggle &Grid"));
    view->AppendMenu(MF_STRING, ID_VIEW_AXES, _T("Toggle A&xes"));
    view->AppendMenu(MF_STRING, ID_VIEW_WIREFRAME, _T("Toggle &Wireframe"));
    view->AppendMenu(MF_SEPARATOR);
    view->AppendMenu(MF_STRING, ID_VIEW_FRAME_ALL, _T("&Frame All"));
    view->AppendMenu(MF_STRING, ID_VIEW_RESET_CAMERA, _T("&Reset Camera"));

    menu->AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(file->Detach()), _T("&File"));
    menu->AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(view->Detach()), _T("&View"));
    SetMenu(menu);
}

void CMainFrame::LayoutChildren(int cx, int cy)
{
    if (!::IsWindow(m_view.GetSafeHwnd())) return;

    const int statusH = 24;
    const int leftW = 70;
    const int rightW = 280;
    const int pad = 6;
    const int contentH = (std::max)(0, cy - statusH);

    m_leftBar.MoveWindow(0, 0, leftW, contentH);
    m_partList.MoveWindow(cx - rightW, 0, rightW, contentH);
    m_view.MoveWindow(leftW + pad, 0, (std::max)(0, cx - leftW - rightW - pad * 2), contentH);
    m_status.MoveWindow(0, contentH, cx, statusH);
}

void CMainFrame::RefreshPartList()
{
    m_partList.ResetContent();
    if (!m_assembly) return;

    int index = 0;
    for (const auto& part : m_assembly->Parts())
    {
        CString label;
        label.Format(_T("%03d  %s  [%s]"), ++index,
            part.name.GetString(),
            part.sufPath.IsEmpty() ? _T("no suf") : part.sufPath.GetString());
        m_partList.AddString(label);
    }
}

void CMainFrame::SetStatusText(const CString& text)
{
    if (::IsWindow(m_status.GetSafeHwnd()))
    {
        m_status.SetWindowText(text);
    }
}
