#//#pragma once
#include "../pch.h"
#include "GLSufApp.h"
#include "MainFrame_2.h"
#include "resource_2.h"
#include "../AppInit\AppInit.h"

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_OPEN_SUF, &CMainFrame::OnFileOpenSuf)
	ON_COMMAND(ID_FILE_OPEN_ATR, &CMainFrame::OnFileOpenAtr)
	ON_COMMAND(ID_FILE_EXIT, &CMainFrame::OnFileExit)
	ON_COMMAND(ID_VIEW_POLYGON, &CMainFrame::OnViewPolygon)
	ON_COMMAND(ID_VIEW_LINE, &CMainFrame::OnViewLine)
	ON_COMMAND(ID_VIEW_POINT, &CMainFrame::OnViewPoint)
	ON_COMMAND(ID_VIEW_ALL, &CMainFrame::OnViewAll)
	ON_COMMAND(ID_VIEW_GRID, &CMainFrame::OnViewGrid)
	ON_COMMAND(ID_VIEW_AXES, &CMainFrame::OnViewAxes)
	ON_COMMAND(ID_VIEW_FRAME_ALL, &CMainFrame::OnViewFrameAll)
	ON_COMMAND(ID_VIEW_RESET_CAMERA, &CMainFrame::OnViewResetCamera)
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
	: m_scene(std::make_shared<SufScene>())
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT cs)
{
	if (CFrameWnd::OnCreate(cs) == -1) return -1;

	BuildMenu();

	m_objectList.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
		CRect(0, 0, 1, 1), this, IDC_GLSUF_OBJECT_LIST);

	m_status.Create(_T("Ready"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(0, 0, 1, 1), this, IDC_GLSUF_STATUS);

	if (!m_view.CreateView(this, IDC_GLSUF_GLVIEW)) return -1;
	m_view.SetScene(m_scene);

	// デフォルトの ATR ファイルを自動ロード
	const auto& config = theApp.GetConfig();
	CString atrPath = config.GetAtrFileFullPath();
	if (!atrPath.IsEmpty())
	{
		CString error;
		if (m_scene->LoadAtr(atrPath, error))
		{
			CString status;
			status.Format(_T("Loaded default attribute(s): %s"), atrPath.GetString());
			SetStatusText(status);
		}
		else
		{
			SetStatusText(_T("Open a SUF file. Optional: open GENIE.ATR before or after loading the model."));
		}
	}
	else
	{
		SetStatusText(_T("Open a SUF file. Optional: open GENIE.ATR before or after loading the model."));
	}
	return 0;
}

void CMainFrame::OnSize(UINT type, int cx, int cy)
{
	CFrameWnd::OnSize(type, cx, cy);
	LayoutChildren(cx, cy);
}

void CMainFrame::OnFileOpenSuf()
{
	CFileDialog dlg(TRUE, _T("suf"), nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("DoGA SUF (*.suf)|*.suf|All files (*.*)|*.*||"),
		this);

	if (dlg.DoModal() != IDOK) return;

	CString error;
	if (!m_scene->LoadSuf(dlg.GetPathName(), error))
	{
		AfxMessageBox(error, MB_ICONERROR);
		return;
	}

	m_view.SetScene(m_scene);
	RefreshObjectList();

	CString title;
	title.Format(_T("GL SUF Viewer - %s"), dlg.GetPathName().GetString());
	SetWindowText(title);

	size_t primCount = 0;
	for (const auto& object : m_scene->Objects()) primCount += object.primitives.size();

	CString status;
	status.Format(_T("Loaded %zu object(s), %zu primitive(s): %s"),
		m_scene->Objects().size(), primCount, dlg.GetPathName().GetString());
	SetStatusText(status);
}

void CMainFrame::OnFileOpenAtr()
{
	CFileDialog dlg(TRUE, _T("atr"), nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("DoGA ATR (*.atr)|*.atr|All files (*.*)|*.*||"),
		this);

	if (dlg.DoModal() != IDOK) return;

	CString error;
	if (!m_scene->LoadAtr(dlg.GetPathName(), error))
	{
		AfxMessageBox(error, MB_ICONERROR);
		return;
	}

	CString status;
	status.Format(_T("Loaded %zu attribute(s): %s"),
		m_scene->Attributes().size(), dlg.GetPathName().GetString());
	SetStatusText(status);
	m_view.Invalidate(FALSE);
}

void CMainFrame::OnFileExit()
{
	PostMessage(WM_CLOSE);
}

void CMainFrame::OnViewPolygon()
{
	m_view.SetDrawPolygon(true);
	m_view.SetDrawLine(false);
	m_view.SetDrawPoint(false);
}

void CMainFrame::OnViewLine()
{
	m_view.SetDrawPolygon(false);
	m_view.SetDrawLine(true);
	m_view.SetDrawPoint(false);
}

void CMainFrame::OnViewPoint()
{
	m_view.SetDrawPolygon(false);
	m_view.SetDrawLine(false);
	m_view.SetDrawPoint(true);
}

void CMainFrame::OnViewAll()
{
	m_view.SetDrawPolygon(true);
	m_view.SetDrawLine(true);
	m_view.SetDrawPoint(true);
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

void CMainFrame::BuildMenu()
{
	auto* menu = new CMenu();
	auto* file = new CMenu();
	auto* view = new CMenu();
	menu->CreateMenu();
	file->CreatePopupMenu();
	view->CreatePopupMenu();

	file->AppendMenu(MF_STRING, ID_FILE_OPEN_SUF, _T("&Open SUF..."));
	file->AppendMenu(MF_STRING, ID_FILE_OPEN_ATR, _T("Open &ATR..."));
	file->AppendMenu(MF_SEPARATOR);
	file->AppendMenu(MF_STRING, ID_FILE_EXIT, _T("E&xit"));

	view->AppendMenu(MF_STRING, ID_VIEW_POLYGON, _T("&Polygon"));
	view->AppendMenu(MF_STRING, ID_VIEW_LINE, _T("&Line"));
	view->AppendMenu(MF_STRING, ID_VIEW_POINT, _T("P&oint"));
	view->AppendMenu(MF_STRING, ID_VIEW_ALL, _T("&All"));
	view->AppendMenu(MF_SEPARATOR);
	view->AppendMenu(MF_STRING, ID_VIEW_GRID, _T("Toggle &Grid"));
	view->AppendMenu(MF_STRING, ID_VIEW_AXES, _T("Toggle A&xes"));
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
	const int rightW = 320;
	const int pad = 6;
	const int contentH = (std::max)(0, cy - statusH);

	m_view.MoveWindow(0, 0, (std::max)(0, cx - rightW - pad), contentH);
	m_objectList.MoveWindow(cx - rightW, 0, rightW, contentH);
	m_status.MoveWindow(0, contentH, cx, statusH);
}

void CMainFrame::RefreshObjectList()
{
	m_objectList.ResetContent();
	if (!m_scene) return;

	int index = 0;
	for (const auto& object : m_scene->Objects())
	{
		CString label;
		label.Format(_T("%03d  %s  prims:%zu"), ++index,
			object.name.GetString(), object.primitives.size());
		m_objectList.AddString(label);
		// 下の行にそのオブジェクトで使用しているアトリビュート名を表示
		std::vector<CString> used;
		for (const auto& prim : object.primitives)
		{
			if (prim.atrName.IsEmpty()) continue;
			bool found = false;
			for (const auto& u : used) { if (u.CompareNoCase(prim.atrName) == 0) { found = true; break; } }
			if (!found) used.push_back(prim.atrName);
		}
		if (used.empty())
		{
			CString atrLine = _T("    atr: (none)");
			m_objectList.AddString(atrLine);
		}
		else
		{
			for (const auto& u : used)
			{
				CString atrLine;
				atrLine.Format(_T("    atr: %s"), u.GetString());
				m_objectList.AddString(atrLine);
			}
		}
	}
}

void CMainFrame::SetStatusText(const CString& text)
{
	if (::IsWindow(m_status.GetSafeHwnd()))
	{
		m_status.SetWindowText(text);
	}
}
