#pragma once
//#include "../pch.h"
//#include <memory>
//#include <array>
#include "../PartAssembly.h" // glpasm プロジェクトで必要
#include "SufModel.h"

// forward declarations to avoid including SUF headers in this header
//#ifndef __GLPASM__ // glpasm プロジェクト以外でのみ SufScene 関連を前方宣言
//struct SufScene;
struct SufPrimitive;
struct SufAttribute;
struct PartInstance;
//#endif

class CGLView : public CWnd
{
public:
	~CGLView() override;

	BOOL CreateView(CWnd* parent, UINT id);
	void SetScene(const std::shared_ptr<SufScene>& scene);

	void SetAssembly(const std::shared_ptr<PartAssembly>& assembly);
	void SetDrawPolygon(bool enabled);
	void SetDrawLine(bool enabled);
	void SetDrawPoint(bool enabled);
//	void DrawPolygons();
//	void DrawLines();
//	void DrawPoints();
	void ToggleGrid();
	void ToggleAxes();
	void ToggleWireframe();
//	BOOL OnEraseBkgnd(CDC*);
	void FrameAll();
	void ResetCamera();
	void SetSelectedPart(int index);
	void AddPart(const PartInstance& part);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT cs);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
	afx_msg void OnRButtonDown(UINT flags, CPoint pt);
	afx_msg void OnRButtonUp(UINT flags, CPoint pt);
	afx_msg void OnMButtonDown(UINT flags, CPoint pt);
	afx_msg void OnMButtonUp(UINT flags, CPoint pt);
	afx_msg void OnMouseMove(UINT flags, CPoint pt);
	afx_msg BOOL OnMouseWheel(UINT flags, short delta, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT flags, CPoint pt);
	afx_msg void OnKeyDown(UINT ch, UINT rep, UINT flags);
	DECLARE_MESSAGE_MAP()

private:
	enum class DragMode { None, Orbit, Pan, Dolly };

	bool InitGL();
	void DestroyGL();
	void RenderScene();
#ifndef __GLPASM__
	void ApplyCamera();
#endif
	void ApplyCamera(float aspect);
//#ifndef __GLPASM__
	void ApplyAttribute(const SufPrimitive& prim);
	void DrawPolygons();
	void DrawLines();
	void DrawPoints();
//#endif
	void DrawGrid(float half, float step);
	void DrawAxes(float length);
	void DrawPart(const PartInstance& part, int index);
	void DrawUnitBox(bool selected);
	void DrawOverlay(HDC hdc);
//#ifndef __GLPASM__
	void DrawImGui();
	static std::array<float, 3> PolygonNormal(const SufPrimitive& prim);
//#endif

	HDC m_hDC = nullptr;
	HGLRC m_hRC = nullptr;
	bool m_ready = false;
//#ifndef __GLPASM__
	std::shared_ptr<SufScene> m_scene;
//#endif
	std::shared_ptr<PartAssembly> m_assembly;

	// camera / view
	float m_rotX = -55.0f;
	float m_rotY = -125.0f;
	float m_rotZ = 0.0f;
	float m_yawDeg = -35.0f;
	float m_pitchDeg = 25.0f;
	float m_distance = -1000.0f;
	std::array<float, 3> m_center = { 0.0f, 0.0f, 0.0f };
	float m_panX = 0.0f;
	float m_panY = 0.0f;

//#ifndef __GLPASM__
	bool m_drawPolygon = true;
	bool m_drawLine = false;
	bool m_drawPoint = false;
//#endif
	bool m_showGrid = true;
	bool m_showAxes = true;
	bool m_wireframe = false;

	int m_selectedPart = -1;

	DragMode m_dragMode = DragMode::None;
	CPoint m_dragStart{};
	CPoint m_dragLast{};
	CPoint m_lastPt{};
};
