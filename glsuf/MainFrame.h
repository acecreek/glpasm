#pragma once
#include "../GLView.h"
#include "../SufModel.h"

class CMainFrame : public CFrameWnd
{
public:
    CMainFrame();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnFileOpenSuf();
    afx_msg void OnFileOpenAtr();
    afx_msg void OnFileExit();
    afx_msg void OnViewPolygon();
    afx_msg void OnViewLine();
    afx_msg void OnViewPoint();
    afx_msg void OnViewAll();
    afx_msg void OnViewGrid();
    afx_msg void OnViewAxes();
    afx_msg void OnViewFrameAll();
    afx_msg void OnViewResetCamera();
    DECLARE_MESSAGE_MAP()

private:
    void BuildMenu();
    void LayoutChildren(int cx, int cy);
    void RefreshObjectList();
    void SetStatusText(const CString& text);

    CGLView m_view;
    CListBox m_objectList;
    CStatic m_status;
    std::shared_ptr<SufScene> m_scene;
};

