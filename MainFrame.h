#pragma once
#include "glsuf/GLView.hpp"
#include "PartAssembly.h"
#include "NumericEdit.h"

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
    afx_msg void OnViewWireframe()
    {
        m_view.ToggleWireframe();
    }
    afx_msg void OnViewFrameAll();
    afx_msg void OnViewResetCamera();
    afx_msg void OnPartSelectionChanged();
    afx_msg void OnPartsAdd();
    afx_msg void OnPartsAddFromFile();
    afx_msg LRESULT OnImGuiSelect(WPARAM wp, LPARAM lp);
    afx_msg void OnSelectedValueChanged(UINT id);
    DECLARE_MESSAGE_MAP()

private:
    void BuildMenu();
    void LayoutChildren(int cx, int cy);
    void RefreshPartList();
    void SetStatusText(const CString& text);

    CGLView m_view;
    CStatic m_leftBar;
    CListBox m_partList;
    // Selected part editable numeric fields
    CNumericEdit m_posEdits[3];
    CNumericEdit m_rotEdits[3];
    CNumericEdit m_sclEdits[3];
    CStatic m_selLabels[3]; // labels for Pos/Rot/Scl rows
    CStatic m_status;
    std::shared_ptr<PartAssembly> m_assembly;
    std::shared_ptr<SufScene> scene;
};
