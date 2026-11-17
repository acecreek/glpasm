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
    ON_COMMAND(ID_PARTS_ADD, &CMainFrame::OnPartsAdd)
    ON_COMMAND(ID_PARTS_ADD_FROM_FILE, &CMainFrame::OnPartsAddFromFile)
    ON_LBN_SELCHANGE(IDC_GLPASM_PART_LIST, &CMainFrame::OnPartSelectionChanged)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_PART_POS_X, IDC_PART_SCL_Z, OnSelectedValueChanged)
    ON_MESSAGE(WM_IMGUI_SELECT, &CMainFrame::OnImGuiSelect)
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
    : m_assembly(std::make_shared<PartAssembly>())
{
}

LRESULT CMainFrame::OnImGuiSelect(WPARAM wp, LPARAM lp)
{
    const int idx = static_cast<int>(wp);
    if (!m_assembly) return 0;
    if (idx < 0 || idx >= static_cast<int>(m_assembly->Parts().size())) return 0;
    m_partList.SetCurSel(idx);
    OnPartSelectionChanged();
    // ensure visible
    m_partList.SetTopIndex(idx);
    return 0;
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

    // Create numeric edits and labels (日本語ラベル)
    constexpr DWORD edtStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    m_selLabels[0].Create(_T("位置"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,1,1), this);
    m_posEdits[0].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_POS_X);
    m_posEdits[1].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_POS_Y);
    m_posEdits[2].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_POS_Z);
    m_selLabels[1].Create(_T("回転"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,1,1), this);
    m_rotEdits[0].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_ROT_X);
    m_rotEdits[1].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_ROT_Y);
    m_rotEdits[2].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_ROT_Z);
    m_selLabels[2].Create(_T("拡大"), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0,0,1,1), this);
    m_sclEdits[0].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_SCL_X);
    m_sclEdits[1].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_SCL_Y);
    m_sclEdits[2].Create(edtStyle, CRect(0,0,1,1), this, IDC_PART_SCL_Z);

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

    RefreshPartList();
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
    // Update selected info display
    if (!m_assembly) return;
    if (sel < 0 || sel >= static_cast<int>(m_assembly->Parts().size()))
    {
        // clear numeric edits
        for (int i = 0; i < 3; ++i)
        {
            m_posEdits[i].SetWindowText(_T(""));
            m_rotEdits[i].SetWindowText(_T(""));
            m_sclEdits[i].SetWindowText(_T(""));
        }
        return;
    }

    const auto& part = m_assembly->Parts()[sel];
    CString info;
    info.Format(_T("Pos: %.2f, %.2f, %.2f  Rot: %.1f, %.1f, %.1f  Scl: %.2f, %.2f, %.2f"),
        part.move.x, part.move.y, part.move.z,
        part.rotateDeg.x, part.rotateDeg.y, part.rotateDeg.z,
        part.scale.x, part.scale.y, part.scale.z);
    // Populate numeric edits
    m_posEdits[0].SetValue(part.move.x);
    m_posEdits[1].SetValue(part.move.y);
    m_posEdits[2].SetValue(part.move.z);
    m_rotEdits[0].SetValue(part.rotateDeg.x);
    m_rotEdits[1].SetValue(part.rotateDeg.y);
    m_rotEdits[2].SetValue(part.rotateDeg.z);
    m_sclEdits[0].SetValue(part.scale.x);
    m_sclEdits[1].SetValue(part.scale.y);
    m_sclEdits[2].SetValue(part.scale.z);
}

void CMainFrame::OnSelectedValueChanged(UINT id)
{
    const int sel = m_partList.GetCurSel();
    if (!m_assembly || sel < 0 || sel >= static_cast<int>(m_assembly->Parts().size())) return;

    PartInstance& part = m_assembly->Parts()[sel];

    CString txt;
    auto parse = [&](int cid, float& out)
    {
        GetDlgItem(cid)->GetWindowText(txt);
        out = static_cast<float>(_wtof(txt));
    };

    parse(IDC_PART_POS_X, part.move.x);
    parse(IDC_PART_POS_Y, part.move.y);
    parse(IDC_PART_POS_Z, part.move.z);
    parse(IDC_PART_ROT_X, part.rotateDeg.x);
    parse(IDC_PART_ROT_Y, part.rotateDeg.y);
    parse(IDC_PART_ROT_Z, part.rotateDeg.z);
    parse(IDC_PART_SCL_X, part.scale.x);
    parse(IDC_PART_SCL_Y, part.scale.y);
    parse(IDC_PART_SCL_Z, part.scale.z);

    // refresh view and status
    m_view.Invalidate(FALSE);
    CString status;
    status.Format(_T("Edited part %d"), sel + 1);
    SetStatusText(status);
    RefreshPartList();
    m_partList.SetCurSel(sel); // keep selection
}

void CMainFrame::BuildMenu()
{
    auto* menu = new CMenu();
    auto* file = new CMenu();
    auto* view = new CMenu();
    auto* parts = new CMenu();
    menu->CreateMenu();
    file->CreatePopupMenu();
    view->CreatePopupMenu();
    parts->CreatePopupMenu();

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
    parts->AppendMenu(MF_STRING, ID_PARTS_ADD, _T("パーツ追加"));
    parts->AppendMenu(MF_STRING, ID_PARTS_ADD_FROM_FILE, _T("ファイルからパーツ追加..."));
    menu->AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(parts->Detach()), _T("パーツ"));
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
    // right column: split equally top/bottom
    const int partListH = contentH / 2;
    m_partList.MoveWindow(cx - rightW, 0, rightW, partListH);
    m_view.MoveWindow(leftW + pad, 0, (std::max)(0, cx - leftW - rightW - pad * 2), contentH);
    m_status.MoveWindow(0, contentH, cx - rightW, statusH);
    // Selected info area (bottom half)
    const int sx = cx - rightW;
    const int sy = partListH;
    const int sw = rightW;
    const int sh = contentH - partListH;
    // labels and three columns of edits
    const int labelW = 36;
    const int colW = (sw - labelW - pad * 4) / 3;
    const int rowH = 24;
    for (int row = 0; row < 3; ++row)
    {
        const int y = sy + pad + row * (rowH + pad);
        m_selLabels[row].MoveWindow(sx + pad, y, labelW, rowH);
        // three columns for X,Y,Z
        for (int col = 0; col < 3; ++col)
        {
            const int x = sx + pad + labelW + pad + col * (colW + pad);
            if (row == 0) m_posEdits[col].MoveWindow(x, y, colW, rowH);
            else if (row == 1) m_rotEdits[col].MoveWindow(x, y, colW, rowH);
            else if (row == 2) m_sclEdits[col].MoveWindow(x, y, colW, rowH);
        }
    }
    // no separate info label
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

void CMainFrame::OnPartsAdd()
{
    AfxMessageBox(_T("Visual Filer によるパーツ追加は未実装です。"), MB_ICONINFORMATION);
}

void CMainFrame::OnPartsAddFromFile()
{
    CFileDialog dlg(TRUE, _T("suf"), nullptr,
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("SUF files (*.suf)|*.suf|All files (*.*)|*.*||"), this);

    if (dlg.DoModal() != IDOK) return;

    CString path = dlg.GetPathName();
    // Create a new PartInstance and append
    if (!m_assembly) m_assembly = std::make_shared<PartAssembly>();
    PartInstance inst;
    // name from filename
    const CString fname = dlg.GetFileName();
    inst.name = fname;
    inst.sufPath = path;
    inst.move = { 0.0f, 0.0f, 0.0f };
    inst.rotateDeg = { 0.0f, 0.0f, 0.0f };
    inst.scale = { 1.0f, 1.0f, 1.0f };

    m_assembly->Parts().push_back(inst);
    m_view.SetAssembly(m_assembly);
    RefreshPartList();

    const int idx = static_cast<int>(m_assembly->Parts().size()) - 1;
    if (idx >= 0)
    {
        m_partList.SetCurSel(idx);
        m_partList.SetTopIndex(idx);
        OnPartSelectionChanged();
    }

    CString status;
    status.Format(_T("Added part: %s"), path.GetString());
    SetStatusText(status);
}

void CMainFrame::SetStatusText(const CString& text)
{
    if (::IsWindow(m_status.GetSafeHwnd()))
    {
        m_status.SetWindowText(text);
    }
}

// ワイヤーフレーム表示のON/OFFを切り替える
void CGLView::ToggleWireframe()
{
    m_wireframe = !m_wireframe;
    Invalidate(); // 再描画要求
}

// end of MainFrame.cpp
