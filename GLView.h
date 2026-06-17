#pragma once
#include "PartAssembly.h"

class CGLView : public CWnd
{
public:
    ~CGLView() override;

    BOOL CreateView(CWnd* parent, UINT id);
    void SetAssembly(const std::shared_ptr<PartAssembly>& assembly);
    void SetSelectedPart(int index);
    void FrameAll();
    void ResetCamera();
    void ToggleGrid();
    void ToggleAxes();
    void ToggleWireframe();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnMButtonDown(UINT flags, CPoint pt);
    afx_msg void OnMButtonUp(UINT flags, CPoint pt);
    afx_msg void OnMouseMove(UINT flags, CPoint pt);
    afx_msg BOOL OnMouseWheel(UINT flags, short delta, CPoint pt);
    afx_msg void OnLButtonDblClk(UINT flags, CPoint pt);
    afx_msg void OnKeyDown(UINT ch, UINT rep, UINT flags);
    DECLARE_MESSAGE_MAP()

private:
    enum class DragMode { None, Orbit, Pan };

    bool InitGL();
    void DestroyGL();
    void RenderScene();
    void ApplyCamera(float aspect);
    void DrawGrid(float half, float step);
    void DrawAxes(float length);
    void DrawPart(const PartInstance& part, int index);
    void DrawUnitBox(bool selected);
    void DrawOverlay(HDC hdc);

    HDC m_hDC = nullptr;
    HGLRC m_hRC = nullptr;
    bool m_ready = false;
    std::shared_ptr<PartAssembly> m_assembly;

    float m_yawDeg = -35.0f;
    float m_pitchDeg = 25.0f;
    float m_distance = 650.0f;
    float m_panX = 0.0f;
    float m_panY = 0.0f;
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_wireframe = false;
    int m_selectedPart = -1;

    DragMode m_dragMode = DragMode::None;
    CPoint m_lastPt{};
};
