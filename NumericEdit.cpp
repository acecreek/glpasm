#include "pch.h"
#include "NumericEdit.h"

BEGIN_MESSAGE_MAP(CNumericEdit, CEdit)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_TIMER()
    ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

CNumericEdit::CNumericEdit() {}
CNumericEdit::~CNumericEdit() {}

void CNumericEdit::SetValue(double v)
{
	m_value = v;
	CString txt;
	txt.Format(_T("%.4f"), m_value);
	SetWindowText(txt);
}

void CNumericEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	m_dragging = true;
	m_startX = point.x;
    CString text;
    GetWindowText(text);
    m_value = _wtof(text);
	m_startValue = m_value;
	CEdit::OnLButtonDown(nFlags, point);
}

void CNumericEdit::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (GetCapture() == this) ReleaseCapture();
	m_dragging = false;
    GetParent()->SendMessage(NumericGestureFinished);
	CEdit::OnLButtonUp(nFlags, point);
}

void CNumericEdit::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_dragging)
	{
		const int dx = point.x - m_startX;
		double delta = dx * m_sensitivity;
		SetValue(m_startValue + delta);
		// notify parent of change
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), EN_CHANGE), reinterpret_cast<LPARAM>(m_hWnd));
	}
	CEdit::OnMouseMove(nFlags, point);
}

void CNumericEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_LEFT)
	{
		StartKeyRepeat(-1);
	}
	else if (nChar == VK_RIGHT)
	{
		StartKeyRepeat(1);
	}
	else
	{
		CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}
}

void CNumericEdit::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ((nChar == VK_LEFT && m_keyDir == -1) || (nChar == VK_RIGHT && m_keyDir == 1))
	{
		StopKeyRepeat();
        GetParent()->SendMessage(NumericGestureFinished);
	}
	else
	{
		CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
	}
}

void CNumericEdit::StartKeyRepeat(int dir)
{
    if (m_keyDir == dir) return;
    CString text;
    GetWindowText(text);
    m_value = _wtof(text);
	m_keyDir = dir;
	// immediate step
	m_value += m_keyDir * m_sensitivity;
	SetValue(m_value);
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), EN_CHANGE), reinterpret_cast<LPARAM>(m_hWnd));
	// start timer for repeat
	if (!m_keyTimer) m_keyTimer = SetTimer(1, 80, nullptr);
}

void CNumericEdit::StopKeyRepeat()
{
	if (m_keyTimer)
	{
		KillTimer(m_keyTimer);
		m_keyTimer = 0;
	}
	m_keyDir = 0;
}

void CNumericEdit::OnTimer(UINT_PTR nIDEvent)
{
	if (m_keyDir != 0)
	{
		m_value += m_keyDir * m_sensitivity;
		SetValue(m_value);
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), EN_CHANGE), reinterpret_cast<LPARAM>(m_hWnd));
	}
	CEdit::OnTimer(nIDEvent);
}

void CNumericEdit::OnKillFocus(CWnd* newWindow)
{
    // 別の欄やパーツへ移った後に、押しっぱなしの処理が残るのを防ぐ。
    StopKeyRepeat();
    m_dragging = false;
    if (GetCapture() == this) ReleaseCapture();
    GetParent()->SendMessage(NumericGestureFinished);
    CEdit::OnKillFocus(newWindow);
}
