#pragma once
#include "GLView.h"
#include "PartAssembly.h"

class CMainFrame : public CFrameWnd
{
public:
    CMainFrame();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnFileOpen();
    afx_msg void OnFileExit();
    afx_msg void OnViewGrid();
    afx_msg void OnViewAxes();
    afx_msg void OnViewWireframe();
    afx_msg void OnViewFrameAll();
    afx_msg void OnViewResetCamera();
    afx_msg void OnPartSelectionChanged();
    DECLARE_MESSAGE_MAP()

private:
    void BuildMenu();
    void LayoutChildren(int cx, int cy);
    void RefreshPartList();
    void SetStatusText(const CString& text);

    CGLView m_view;
    CStatic m_leftBar;
    CListBox m_partList;
    CStatic m_status;
    std::shared_ptr<PartAssembly> m_assembly;
};
