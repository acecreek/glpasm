#include "pch.h"
#include "MainFrame.h"
#include "resource.h"
#include "WindowSettings.hpp"
#include <cerrno>
#include <limits>

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_COMMAND(ID_PART_DUPLICATE, &CMainFrame::OnDuplicatePart)
    ON_COMMAND(ID_PART_DELETE, &CMainFrame::OnDeletePart)
    ON_COMMAND(ID_ASSEMBLY_UNDO, &CMainFrame::OnUndo)
    ON_COMMAND(ID_ASSEMBLY_REDO, &CMainFrame::OnRedo)
    ON_UPDATE_COMMAND_UI(ID_PART_DUPLICATE, &CMainFrame::OnUpdateEditCommand)
    ON_UPDATE_COMMAND_UI(ID_PART_DELETE, &CMainFrame::OnUpdateEditCommand)
    ON_UPDATE_COMMAND_UI(ID_ASSEMBLY_UNDO, &CMainFrame::OnUpdateEditCommand)
    ON_UPDATE_COMMAND_UI(ID_ASSEMBLY_REDO, &CMainFrame::OnUpdateEditCommand)
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_PART_POS_X, IDC_PART_SCL_Z, OnNumericFocusLost)
    ON_MESSAGE(NumericGestureFinished, &CMainFrame::OnNumericGestureFinished)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_CLOSE()
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
END_MESSAGE_MAP()

CMainFrame::CMainFrame(const AppConfig& config)
    : m_config(config), m_assembly(std::make_shared<PartAssembly>())
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
    m_assembly->SetResourcePaths(m_config.GetPartsFullPath(), m_config.GetAtrFileFullPath());
    m_view.SetAssembly(m_assembly);

    OnPartSelectionChanged();
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
    loaded->SetResourcePaths(m_config.GetPartsFullPath(), m_config.GetAtrFileFullPath());
    if (!loaded->LoadLxP(dlg.GetPathName(), error))
    {
        AfxMessageBox(error, MB_ICONERROR);
        return;
    }

    m_history.Clear();
    m_numericEditing = false;
    m_assembly = loaded;
    m_view.SetAssembly(m_assembly);
    RefreshPartList();

    CString title;
    title.Format(_T("GL PartsAsm - %s"), dlg.GetPathName().GetString());
    SetWindowText(title);

    CString status;
    status.Format(_T("Loaded %zu part(s): %s"), m_assembly->Parts().size(), dlg.GetPathName().GetString());
    SetStatusText(status);

    OnPartSelectionChanged();
}

void CMainFrame::OnClose()
{
    constexpr auto saveError = L"ウィンドウのサイズと位置をiniファイルに保存できませんでした。書き込み権限を確認してください。";
    if (!WindowSettings::Save(*this)) AfxMessageBox(saveError, MB_ICONWARNING);
    CFrameWnd::OnClose();
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
    m_numericEditing = false;
    const int sel = m_partList.GetCurSel();
    m_view.SetSelectedPart(sel);
    m_updatingSelection = true;
    constexpr auto firstAxis = 0, axes = 3;
    for (int axis = firstAxis; axis < axes; ++axis)
    {
        m_posEdits[axis].EnableWindow(HasSelectedPart());
        m_rotEdits[axis].EnableWindow(HasSelectedPart());
        m_sclEdits[axis].EnableWindow(HasSelectedPart());
    }
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
        m_updatingSelection = false;
        return;
    }

    const auto& part = m_assembly->Parts()[sel];
    CString info;
    info.Format(_T("Pos: %.2f, %.2f, %.2f  Rot: %.1f, %.1f, %.1f  Scl: %.2f, %.2f, %.2f"),
        part.move.x, part.move.y, part.move.z,
        part.rotateDeg.x, part.rotateDeg.y, part.rotateDeg.z,
        part.scale.x, part.scale.y, part.scale.z);
    // 表示更新で発生する変更通知から、パーツの値を保護する。
    m_updatingSelection = true;
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
    m_updatingSelection = false;
}

void CMainFrame::OnSelectedValueChanged(UINT id)
{
    if (m_updatingSelection) return;
    const int sel = m_partList.GetCurSel();
    if (!m_assembly || sel < 0 || sel >= static_cast<int>(m_assembly->Parts().size())) return;

    constexpr auto empty = L'\0';
    constexpr auto noError = 0;
    CString text;
    GetDlgItem(id)->GetWindowText(text);
    text.Trim();
    if (text.IsEmpty()) return;
    // 入力途中の「-」や不正な値では、配置と履歴を変更しない。
    errno = noError;
    auto number = std::wstring(text.GetString());
    auto end = number.data();
    const auto value = std::wcstod(number.data(), &end);
    if (end == number.data() || *end != empty || errno == ERANGE || !std::isfinite(value)
        || std::abs(value) > (std::numeric_limits<float>::max)()) return;
    auto& part = m_assembly->Parts()[sel];
    const std::array fields = {std::ref(part.move.x), std::ref(part.move.y), std::ref(part.move.z),
        std::ref(part.rotateDeg.x), std::ref(part.rotateDeg.y), std::ref(part.rotateDeg.z),
        std::ref(part.scale.x), std::ref(part.scale.y), std::ref(part.scale.z)};
    const auto updated = static_cast<float>(value);
    auto& target = fields.at(id - IDC_PART_POS_X).get();
    if (target == updated) return;
    if (!m_numericEditing)
    {
        RecordEdit();
        m_numericEditing = true;
    }
    target = updated;
    // refresh view and status
    m_view.Invalidate(FALSE);
    constexpr auto edited = L"選択パーツの配置を変更しました。";
    SetStatusText(edited);
    // keep selection

}

void CMainFrame::BuildMenu()
{
    constexpr auto openAssemblyLabel = L"L2P/L3Pを開く(&O)...";
    constexpr auto exitLabel = L"終了(&X)";
    constexpr auto gridLabel = L"グリッドの表示切替(&G)";
    constexpr auto axesLabel = L"座標軸の表示切替(&A)";
    constexpr auto wireframeLabel = L"ワイヤーフレームの表示切替(&W)";
    constexpr auto frameAllLabel = L"全パーツを表示範囲に収める(&F)";
    constexpr auto resetCameraLabel = L"カメラを初期化(&R)";
    constexpr auto fileLabel = L"ファイル(&F)";
    constexpr auto viewLabel = L"表示(&V)";
    constexpr auto addPartLabel = L"パーツを追加";
    constexpr auto addFromFileLabel = L"ファイルからパーツを追加...";
    constexpr auto partsLabel = L"パーツ";
    constexpr auto editLabel = L"編集(&E)";
    constexpr auto undoLabel = L"元に戻す(&U)\tCtrl+Z";
    constexpr auto redoLabel = L"やり直す(&R)\tCtrl+Shift+Z";
    constexpr auto duplicateLabel = L"パーツを複製(&D)\tShift+D";
    constexpr auto deleteLabel = L"パーツを削除(&X)\tX / Delete";
    CMenu edit;
    edit.CreatePopupMenu();
    edit.AppendMenu(MF_STRING, ID_ASSEMBLY_UNDO, undoLabel);
    edit.AppendMenu(MF_STRING, ID_ASSEMBLY_REDO, redoLabel);
    edit.AppendMenu(MF_SEPARATOR);
    edit.AppendMenu(MF_STRING, ID_PART_DELETE, deleteLabel);
    edit.AppendMenu(MF_STRING, ID_PART_DUPLICATE, duplicateLabel);
    CMenu menu;
    CMenu file;
    CMenu view;
    CMenu parts;
    menu.CreateMenu();
    file.CreatePopupMenu();
    view.CreatePopupMenu();
    parts.CreatePopupMenu();

    file.AppendMenu(MF_STRING, ID_FILE_OPEN_LXP, openAssemblyLabel);
    file.AppendMenu(MF_SEPARATOR);
    file.AppendMenu(MF_STRING, ID_FILE_EXIT, exitLabel);

    view.AppendMenu(MF_STRING, ID_VIEW_GRID, gridLabel);
    view.AppendMenu(MF_STRING, ID_VIEW_AXES, axesLabel);
    view.AppendMenu(MF_STRING, ID_VIEW_WIREFRAME, wireframeLabel);
    view.AppendMenu(MF_SEPARATOR);
    view.AppendMenu(MF_STRING, ID_VIEW_FRAME_ALL, frameAllLabel);
    view.AppendMenu(MF_STRING, ID_VIEW_RESET_CAMERA, resetCameraLabel);

    menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(file.Detach()), fileLabel);
    menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(edit.Detach()), editLabel);
    menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(view.Detach()), viewLabel);
    parts.AppendMenu(MF_STRING, ID_PARTS_ADD, addPartLabel);
    parts.AppendMenu(MF_STRING, ID_PARTS_ADD_FROM_FILE, addFromFileLabel);
    menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(parts.Detach()), partsLabel);
    SetMenu(&menu);
    menu.Detach();
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

    m_assembly->SetResourcePaths(m_config.GetPartsFullPath(), m_config.GetAtrFileFullPath());
    CString error;
    if (!m_assembly->LoadPart(inst, CString{}, error))
    {
        AfxMessageBox(error, MB_ICONERROR);
        return;
    }
    RecordEdit();
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

// MFCのメッセージ引数は借用するだけで、所有権は持たない。
BOOL CMainFrame::PreTranslateMessage(MSG* message)
{
    constexpr auto keyZ = 'Z', keyD = 'D', keyX = 'X';
    constexpr auto keyDownMask = 0x8000;
    constexpr auto repeatMask = 0x40000000;
    constexpr auto zero = 0;
    if (message->message == WM_KEYDOWN && (GetKeyState(VK_MENU) & keyDownMask) == zero)
    {
        const bool control = (GetKeyState(VK_CONTROL) & keyDownMask) != zero;
        const bool shift = (GetKeyState(VK_SHIFT) & keyDownMask) != zero;
        const auto focus = GetFocus();
        const bool numeric = focus && focus->GetDlgCtrlID() >= IDC_PART_POS_X
            && focus->GetDlgCtrlID() <= IDC_PART_SCL_Z;
        if (control && message->wParam == keyZ)
        {
            if ((message->lParam & repeatMask) == zero) shift ? OnRedo() : OnUndo();
            return TRUE;
        }
        if (numeric && message->wParam == VK_RETURN)
        {
            m_numericEditing = false;
            m_view.SetFocus();
            return TRUE;
        }
        if (!numeric && !control)
        {
            if (shift && message->wParam == keyD)
            {
                if ((message->lParam & repeatMask) == zero) OnDuplicatePart();
                return TRUE;
            }
            if (!shift && (message->wParam == keyX || message->wParam == VK_DELETE))
            {
                if ((message->lParam & repeatMask) == zero) OnDeletePart();
                return TRUE;
            }
        }
    }
    return CFrameWnd::PreTranslateMessage(message);
}

bool CMainFrame::HasSelectedPart() const
{
    constexpr auto first = 0;
    const auto selection = m_partList.GetCurSel();
    return m_assembly && selection >= first && static_cast<size_t>(selection) < m_assembly->Parts().size();
}

void CMainFrame::RecordEdit()
{
    m_numericEditing = false;
    m_history.Record(m_assembly->Parts(), m_partList.GetCurSel());
}

void CMainFrame::RestoreEdit(AssemblyHistory::State state)
{
    m_numericEditing = false;
    m_assembly->Parts() = std::move(state.parts);
    RefreshPartList();
    m_partList.SetCurSel(state.selection);
    OnPartSelectionChanged();
}

void CMainFrame::OnDuplicatePart()
{
    if (!HasSelectedPart()) return;
    constexpr auto copied = L"パーツを同じ位置に複製しました。位置欄で移動できます。";
    constexpr auto lastOffset = 1;
    const auto part = m_assembly->Parts().at(m_partList.GetCurSel());
    RecordEdit();
    m_assembly->Parts().push_back(part);
    RefreshPartList();
    const auto selected = static_cast<int>(m_assembly->Parts().size()) - lastOffset;
    m_partList.SetCurSel(selected);
    m_partList.SetTopIndex(selected);
    OnPartSelectionChanged();
    SetStatusText(copied);
}

void CMainFrame::OnDeletePart()
{
    if (!HasSelectedPart()) return;
    constexpr auto deleted = L"パーツを削除しました。Ctrl+Zで元に戻せます。";
    constexpr auto lastOffset = 1;
    const auto selected = m_partList.GetCurSel();
    RecordEdit();
    auto& parts = m_assembly->Parts();
    parts.erase(parts.begin() + selected);
    RefreshPartList();
    m_partList.SetCurSel(parts.empty() ? AssemblyHistory::noSelection
        : (std::min)(selected, static_cast<int>(parts.size()) - lastOffset));
    OnPartSelectionChanged();
    SetStatusText(deleted);
}

void CMainFrame::OnUndo()
{
    if (auto state = m_history.Undo(m_assembly->Parts(), m_partList.GetCurSel()))
    {
        RestoreEdit(std::move(*state));
        constexpr auto undone = L"編集を元に戻しました。";
        SetStatusText(undone);
    }
}

void CMainFrame::OnRedo()
{
    if (auto state = m_history.Redo(m_assembly->Parts(), m_partList.GetCurSel()))
    {
        RestoreEdit(std::move(*state));
        constexpr auto redone = L"編集をやり直しました。";
        SetStatusText(redone);
    }
}

void CMainFrame::OnUpdateEditCommand(CCmdUI* command)
{
    if (command->m_nID == ID_ASSEMBLY_UNDO) command->Enable(m_history.CanUndo());
    else if (command->m_nID == ID_ASSEMBLY_REDO) command->Enable(m_history.CanRedo());
    else command->Enable(HasSelectedPart());
}

void CMainFrame::OnNumericFocusLost(UINT)
{
    m_numericEditing = false;
    // 入力途中の文字列も、欄を離れると確定済みの値へ戻す。
    if (!m_updatingSelection) OnPartSelectionChanged();
}

LRESULT CMainFrame::OnNumericGestureFinished(WPARAM, LPARAM)
{
    m_numericEditing = false;
    constexpr LRESULT handled = 0;
    return handled;
}
