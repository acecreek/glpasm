#pragma once
#include "glsuf/GLView.hpp"
#include "PartAssembly.h"
#include "AssemblyHistory.hpp"
#include "NumericEdit.h"
#include "AppInit/AppInit.h"

class CMainFrame : public CFrameWnd
{
public:
    explicit CMainFrame(const AppConfig& config);
    BOOL PreTranslateMessage(MSG* message) override;

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
    afx_msg void OnSelectedValueChanged(UINT id);
    afx_msg void OnDuplicatePart();
    afx_msg void OnDeletePart();
    afx_msg void OnUndo();
    afx_msg void OnRedo();
    afx_msg void OnUpdateEditCommand(CCmdUI* command);
    afx_msg void OnNumericFocusLost(UINT id);
    afx_msg LRESULT OnNumericGestureFinished(WPARAM, LPARAM);
    DECLARE_MESSAGE_MAP()

private:
    void BuildMenu();
    void LayoutChildren(int cx, int cy);
    void RefreshPartList();
    void SetStatusText(const CString& text);

    void RecordEdit();
    void RestoreEdit(AssemblyHistory::State state);
    bool HasSelectedPart() const;
    AssemblyHistory m_history;
    bool m_numericEditing = false;
    AppConfig m_config;
    bool m_updatingSelection = false;
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
