#pragma once
#include "../SufModel.h"

class CGLView : public CWnd
{
public:
    ~CGLView() override;

    BOOL CreateView(CWnd* parent, UINT id);
    void SetScene(const std::shared_ptr<SufScene>& scene);
    void SetDrawPolygon(bool enabled);
    void SetDrawLine(bool enabled);
    void SetDrawPoint(bool enabled);
    void ToggleGrid();
    void ToggleAxes();
    void FrameAll();
    void ResetCamera();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnRButtonDown(UINT flags, CPoint pt);
    afx_msg void OnRButtonUp(UINT flags, CPoint pt);
    afx_msg void OnMouseMove(UINT flags, CPoint pt);
    afx_msg BOOL OnMouseWheel(UINT flags, short delta, CPoint pt);
    afx_msg void OnLButtonDblClk(UINT flags, CPoint pt);
    afx_msg void OnKeyDown(UINT ch, UINT rep, UINT flags);
    DECLARE_MESSAGE_MAP()

private:
    enum class DragMode { None, Orbit, Dolly };

    bool InitGL();
    void DestroyGL();
    void RenderScene();
    void ApplyCamera();
    void ApplyAttribute(const SufPrimitive& prim);
    void DrawPolygons();
    void DrawLines();
    void DrawPoints();
    void DrawGrid(float half, float step);
    void DrawAxes(float length);
    void DrawOverlay(HDC hdc);
    static std::array<float, 3> PolygonNormal(const SufPrimitive& prim);

    HDC m_hDC = nullptr;
    HGLRC m_hRC = nullptr;
    bool m_ready = false;
    std::shared_ptr<SufScene> m_scene;

    float m_rotX = -55.0f;
    float m_rotY = -125.0f;
    float m_rotZ = 0.0f;
    float m_distance = -1000.0f;
    std::array<float, 3> m_center = { 0.0f, 0.0f, 0.0f };

    bool m_drawPolygon = true;
    bool m_drawLine = false;
    bool m_drawPoint = false;
    bool m_showGrid = true;
    bool m_showAxes = true;

    DragMode m_dragMode = DragMode::None;
    CPoint m_lastPt{};
};

