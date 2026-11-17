#pragma once

#include <afxwin.h>

class CNumericEdit : public CEdit
{
public:
	CNumericEdit();
	virtual ~CNumericEdit();

	void SetValue(double v);
	double GetValue() const { return m_value; }
	void SetSensitivity(double s) { m_sensitivity = s; }

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

private:
	double m_value = 0.0;
	double m_sensitivity = 0.1; // units per pixel
	bool m_dragging = false;
	int m_startX = 0;
	double m_startValue = 0.0;

	// key repeat
	int m_keyDir = 0; // -1 or +1 or 0
	UINT_PTR m_keyTimer = 0;
	void StartKeyRepeat(int dir);
	void StopKeyRepeat();
};
