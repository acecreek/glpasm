//#pragma once
#include "pch.h"
#include "GLView.hpp"
// SufModel.hはApplyAttribute/PolygonNormalの実装で必要となるため、条件付きインクルードを外し常に含める
#include "SufModel.h"
#include "../PartAssembly.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_win32.h"
#include "../third_party/imgui/backends/imgui_impl_opengl3.h"
//#ifndef __GLPASM__
//#include "resource_2.h"
//#endif
#include <array> // PolygonNormalで使用
#include <cmath> // std::fabs, std::sqrtで使用
#include <limits> // std::numeric_limits<float>::epsilon()で使用
#include <format>
#include <string>
#pragma once
#include <Windows.h>

BEGIN_MESSAGE_MAP(CGLView, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE() // OnMouseMoveを追加
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

CGLView::~CGLView()
{
	DestroyGL();
}

BOOL CGLView::CreateView(CWnd* parent, UINT id)
{
	const LPCTSTR cls = AfxRegisterWndClass(
		CS_OWNDC | CS_DBLCLKS,
		::LoadCursor(nullptr, IDC_ARROW),
		reinterpret_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)),
		0);

	return CreateEx(0, cls, _T("Perspective View"),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
		CRect(0, 0, 100, 100), parent, id);
}

int CGLView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) == -1) return -1;
	m_hDC = ::GetDC(m_hWnd);
	if (!InitGL()) return -1;
	return 0;
}

void CGLView::OnDestroy()
{
	DestroyGL();
	CWnd::OnDestroy();
}

void CGLView::SetAssembly(const std::shared_ptr<PartAssembly>& assembly)
{
	m_assembly = assembly;
	m_selectedPart = -1;
	FrameAll();
}

void CGLView::SetScene(const std::shared_ptr<SufScene>& scene)
{
	m_scene = scene;
	FrameAll();
	Invalidate(FALSE);
}

void CGLView::SetSelectedPart(int index)
{
	m_selectedPart = index;
	Invalidate(FALSE);
}

void CGLView::SetDrawPolygon(bool enabled)
{
	m_drawPolygon = enabled;
	Invalidate(FALSE);
}

void CGLView::SetDrawLine(bool enabled)
{
	m_drawLine = enabled;
	Invalidate(FALSE);
}

void CGLView::SetDrawPoint(bool enabled)
{
	m_drawPoint = enabled;
	Invalidate(FALSE);
}

void CGLView::FrameAll()
{
	float maxExtent = 250.0f;
/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 ---
	if (m_assembly)
	{
		for (const auto& part : m_assembly->Parts())
		{
			maxExtent = (std::max)(maxExtent, std::fabs(part.move.x) + std::fabs(part.scale.x) * 40.0f);
			maxExtent = (std::max)(maxExtent, std::fabs(part.move.y) + std::fabs(part.scale.y) * 40.0f);
			maxExtent = (std::max)(maxExtent, std::fabs(part.move.z) + std::fabs(part.scale.z) * 40.0f);
		}
	}
#ifndef __GLPASM__
------------------------------------------------------------------------------- */

	if (m_scene)
	{
		for (const auto& object : m_scene->Objects())
		{
			for (const auto& prim : object.primitives)
			{
				for (const auto& point : prim.points)
				{
					if (point.values.size() >= 3)
					{
						maxExtent = (std::max)(maxExtent, std::fabs(point.values[0]) + 40.0f);
						maxExtent = (std::max)(maxExtent, std::fabs(point.values[1]) + 40.0f);
						maxExtent = (std::max)(maxExtent, std::fabs(point.values[2]) + 40.0f);
					}
				}
			}
		}
	}
/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 ---
	#endif
------------------------------------------------------------------------------ - */

	m_distance = (std::max)(300.0f, maxExtent * 2.4f);
	m_panX = 0.0f;
	m_panY = 0.0f;
	Invalidate(FALSE);
}

void CGLView::ResetCamera()
{
	m_yawDeg = -35.0f;
	m_pitchDeg = 25.0f;
	FrameAll();
}

void CGLView::ToggleGrid()
{
	m_showGrid = !m_showGrid;
	Invalidate(FALSE);
}

void CGLView::ToggleAxes()
{
	m_showAxes = !m_showAxes;
	Invalidate(FALSE);
}

void CGLView::OnPaint()
{
	CPaintDC dc(this);
	if (!m_ready || !wglMakeCurrent(m_hDC, m_hRC)) return;

	RenderScene();
	// Draw ImGui overlay (into current GL context)
	DrawImGui();
	::SwapBuffers(m_hDC);
	DrawOverlay(dc.GetSafeHdc());
}

bool CGLView::InitGL()
{
	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	const int format = ChoosePixelFormat(m_hDC, &pfd);
	if (format == 0 || !SetPixelFormat(m_hDC, format, &pfd)) return false;

	m_hRC = wglCreateContext(m_hDC);
	if (!m_hRC || !wglMakeCurrent(m_hDC, m_hRC)) return false;

	glClearColor(0.10f, 0.30f, 0.40f, 0.0f);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_NORMALIZE);
	m_ready = true;

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	// Initialize backends
	ImGui_ImplWin32_Init(m_hWnd);
	// Use GLSL 130 for compatibility
	ImGui_ImplOpenGL3_Init("#version 130");
	return true;
}

void CGLView::DestroyGL()
{
	if (m_hRC)
	{
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(m_hRC);
		m_hRC = nullptr;
	}
	if (m_hDC)
	{
		::ReleaseDC(m_hWnd, m_hDC);
		m_hDC = nullptr;
	}
	m_ready = false;

	// Shutdown ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// DrawGrid関数をGLView_Impl.cppから統合
void CGLView::DrawGrid(float half, float step)
{
	const GLboolean lighting = glIsEnabled(GL_LIGHTING);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	glBegin(GL_LINES);
	for (float x = -half; x <= half + 0.001f; x += step)
	{
		const float c = (std::fmod(std::fabs(x), step * 5.0f) < 0.001f) ? 0.28f : 0.20f;
		glColor3f(c, c, c);
		glVertex3f(x, -half, 0.0f);
		glVertex3f(x, half, 0.0f);
	}
	for (float y = -half; y <= half + 0.001f; y += step)
	{
		const float c = (std::fmod(std::fabs(y), step * 5.0f) < 0.001f) ? 0.28f : 0.20f;
		glColor3f(c, c, c);
		glVertex3f(-half, y, 0.0f);
		glVertex3f(half, y, 0.0f);
	}
	glEnd();
	if (lighting) glEnable(GL_LIGHTING);
}

// DrawAxes関数をGLView_Impl.cppから統合
void CGLView::DrawAxes(float length)
{
	const GLboolean lighting = glIsEnabled(GL_LIGHTING);
	glDisable(GL_LIGHTING);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(0, 0, 0); glVertex3f(length, 0, 0);
	glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0, 0, 0); glVertex3f(0, length, 0);
	glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, length);
	glEnd();
	glLineWidth(1.0f);
	if (lighting) glEnable(GL_LIGHTING);
}

// DrawUnitBox関数をGLView_Impl.cppから統合
void CGLView::DrawUnitBox(bool selected)
{
	const float r = selected ? 1.0f : 0.38f;
	const float g = selected ? 0.65f : 0.60f;
	const float b = selected ? 0.12f : 0.70f;
	glColor3f(r, g, b);

	glBegin(GL_QUADS);
	glNormal3f(1, 0, 0); glVertex3f(1, -1, -1); glVertex3f(1, 1, -1); glVertex3f(1, 1, 1); glVertex3f(1, -1, 1);
	glNormal3f(-1, 0, 0); glVertex3f(-1, 1, -1); glVertex3f(-1, -1, -1); glVertex3f(-1, -1, 1); glVertex3f(-1, 1, 1);
	glNormal3f(0, 1, 0); glVertex3f(-1, 1, -1); glVertex3f(1, 1, -1); glVertex3f(1, 1, 1); glVertex3f(-1, 1, 1);
	glNormal3f(0, -1, 0); glVertex3f(1, -1, -1); glVertex3f(-1, -1, -1); glVertex3f(-1, -1, 1); glVertex3f(1, -1, 1);
	glNormal3f(0, 0, 1); glVertex3f(-1, -1, 1); glVertex3f(1, -1, 1); glVertex3f(1, 1, 1); glVertex3f(-1, 1, 1);
	glNormal3f(0, 0, -1); glVertex3f(-1, 1, -1); glVertex3f(1, 1, -1); glVertex3f(1, -1, -1); glVertex3f(-1, -1, -1);
	glEnd();
}

// DrawPart関数をGLView_Impl.cppから統合
void CGLView::DrawPart(const PartInstance& part, int index)
{
	// 最低限のスタブ実装: 必要に応じて詳細実装を追加する
	(void)part;
	(void)index;
}

void CGLView::RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CRect rc;
	GetClientRect(&rc);
	const double aspect = static_cast<double>((std::max)(1, rc.Width())) /
		static_cast<double>((std::max)(1, rc.Height()));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0, aspect, 1.0, 20000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
//#ifdef __GLPASM__
//	ApplyCamera(aspect);
//#else
	ApplyCamera();
//#endif

	const GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	const GLfloat lightDiffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	const GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const GLfloat lightPosition[] = { -1.0f, 1.0f, 1.0f, 0.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 0.0f);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0.0f);

	if (m_showGrid) DrawGrid(1000.0f, 100.0f);
	if (m_showAxes) DrawAxes(1800.0f);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glShadeModel(GL_SMOOTH);

//#ifndef __GLPASM__
	if (m_drawPolygon) DrawPolygons();
	if (m_drawLine) DrawLines();
	if (m_drawPoint) DrawPoints();
//#endif
	// else
	{
		if (m_assembly)
		{
			for (size_t i = 0; i < m_assembly->Parts().size(); ++i)
			{
				DrawPart(m_assembly->Parts()[i], i);
			}
		}
	}

	glDisable(GL_LIGHTING);
	glFinish();
}

//#ifndef __GLPASM__
void CGLView::ApplyCamera()
{
    if (m_useOldCameraMode)
    {
		// 旧 ApplyCamera() のロジック
		// m_distance は FrameAll で正の値に設定されるため、
		// カメラ位置として扱うには負方向へ移動する必要がある。
		// 以前の実装では符号が逆になっておりシーンがカメラの後ろに
		// 置かれて表示されない問題があったため -m_distance を使用する。
		glTranslated(0.0, 0.0, -m_distance);
        glRotatef(m_rotX, 1.0f, 0.0f, 0.0f);
        glRotatef(m_rotY, 0.0f, 0.0f, 1.0f);
        glRotatef(m_rotZ, 0.0f, 1.0f, 0.0f);
        glTranslatef(-m_center[0], -m_center[1], -m_center[2]);
    }
    else
    {
        // 新 ApplyCamera(float) のロジック (引数は不要)
        constexpr float pi = 3.1415926535f; // C++20 の constexpr を使用
        const float yaw = m_yawDeg * pi / 180.0f;
        const float pitch = m_pitchDeg * pi / 180.0f;
        const float cp = std::cos(pitch);
        const float eyeX = -std::cos(yaw) * cp * m_distance + m_panX;
        const float eyeY = -std::sin(yaw) * cp * m_distance + m_panY;
        const float eyeZ = -std::sin(pitch) * m_distance;
        gluLookAt(eyeX, eyeY, eyeZ, m_panX, m_panY, 0.0, 0.0, 0.0, 1.0);
    }
}

void CGLView::DrawOverlay(HDC hdc)
{
	CRect rc;
	GetClientRect(&rc);
	CString text;

/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 --- 

//#ifdef __GLPASM__
	size_t parts = 0;
	if (m_assembly)
	{
		parts = m_assembly->Parts().size();
	}
	text.Format(_T("GLPASM | RMB Orbit | MMB Pan | Wheel Zoom | Parts: %zu | Selected: %d"),
		parts, m_selectedPart);
//#else
------------------------------------------------------------------------------- */
	size_t objects = 0;
	size_t prims = 0;
	if (m_scene)
	{
		objects = m_scene->Objects().size();
		for (const auto& object : m_scene->Objects()) prims += object.primitives.size();
	}
	text.Format(_T("SUF | RMB Rotate | Shift+RMB Dolly | Wheel Zoom | 1 Poly 2 Line 3 Point 4 All | Objects: %zu | Prims: %zu"),
		objects, prims);
/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 ---
#endif
------------------------------------------------------------------------------ */
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, RGB(230, 235, 240));
	CRect textRc(12, 10, rc.Width() - 12, 32);
	DrawText(hdc, text, -1, &textRc, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
}

// ApplyAttribute関数をGLView_Impl.cppから統合
void CGLView::ApplyAttribute(const SufPrimitive& prim)
{
	const SufAttribute* atr = m_scene ? m_scene->FindAttribute(prim.atrName) : nullptr;
	if (!atr)
	{
		const GLfloat color[] = { 0.95f, 0.95f, 0.95f, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64.0f);
		return;
	}

	const float alpha = 1.0f - atr->tra;
	GLfloat amb[] = { atr->amb * atr->col[0], atr->amb * atr->col[1], atr->amb * atr->col[2], alpha };
	GLfloat dif[] = { atr->dif * atr->col[0], atr->dif * atr->col[1], atr->dif * atr->col[2], alpha };
	GLfloat spc[] = { atr->spc[0], atr->spc[0], atr->spc[0], 1.0f };
	const float emissionScale = (atr->amb == 1.0f) ? 1.0f : 0.0f;
	GLfloat ems[] = { emissionScale * atr->col[0], emissionScale * atr->col[1], emissionScale * atr->col[2], alpha };

	glAlphaFunc(GL_LEQUAL, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spc);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ems);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 128.0f * atr->spc[1]);
}

// PolygonNormal関数をGLView_Impl.cppから統合
std::array<float, 3> CGLView::PolygonNormal(const SufPrimitive& prim)
{
	if (prim.points.size() < 3 ||
		prim.points[0].values.size() < 3 ||
		prim.points[1].values.size() < 3 ||
		prim.points[2].values.size() < 3)
	{
		return { 0.0f, 0.0f, 1.0f };
	}

	const auto& p0 = prim.points[0].values;
	const auto& p1 = prim.points[1].values;
	const auto& p2 = prim.points[2].values;

	const float ax = p1[0] - p0[0];
	const float ay = p1[1] - p0[1];
	const float az = p1[2] - p0[2];
	const float bx = p2[0] - p0[0];
	const float by = p2[1] - p0[1];
	const float bz = p2[2] - p0[2];

	float nx = ay * bz - az * by;
	float ny = az * bx - ax * bz;
	float nz = ax * by - ay * bx;
	const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
	if (len <= std::numeric_limits<float>::epsilon()) return { 0.0f, 0.0f, 1.0f };
	nx /= len;
	ny /= len;
	nz /= len;
	return { nx, ny, nz };
}

void CGLView::DrawImGui()
{
	if (!m_ready) return;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Build a top-right child window showing parts
	CRect rc;
	GetClientRect(&rc);
	const float padding = 8.0f;
	const float winW = 280.0f;
	const float winH = (rc.Height() / 2) - padding * 2;
	ImGui::SetNextWindowPos(ImVec2((float)rc.right - winW - padding, padding), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	if (ImGui::Begin("PartsPanel", nullptr, flags))
	{
		ImGui::Text("パーツ一覧");
		ImGui::Separator();
		ImGui::BeginChild("Parts", ImVec2(0, 0), true);
		if (m_assembly)
		{
			for (int i = 0; i < static_cast<int>(m_assembly->Parts().size()); ++i)
			{
				const auto& p = m_assembly->Parts()[i];
				bool selected = (i == m_selectedPart);
				if (ImGui::Selectable(CT2A(p.name.GetString()), selected))
				{
					m_selectedPart = i;
					// notify mainframe to sync listbox
					if (::IsWindow(GetParent()->GetSafeHwnd()))
					{
						GetParent()->PostMessage(WM_IMGUI_SELECT, static_cast<WPARAM>(i), 0);
					}
				}
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void CGLView::DrawPolygons()
{
	if (!m_scene) return;

	// prim.atrName が空文字列の場合でも最初のプリミティブで
	// ApplyAttribute を必ず呼び出すため、初期値を空でない値にする
	CString lastAtr = _T("\x01");
	glEnable(GL_NORMALIZE);
	glPolygonMode(GL_FRONT, GL_FILL);

	for (const auto& object : m_scene->Objects())
	{
		for (const auto& prim : object.primitives)
		{
			if (prim.points.size() < 3) continue;
			if (prim.atrName.Compare(lastAtr) != 0)
			{
				ApplyAttribute(prim);
				lastAtr = prim.atrName;
			}

			const auto normal = PolygonNormal(prim);
			glBegin(GL_POLYGON);
			for (const auto& point : prim.points)
			{
				if (point.values.size() < 3) continue;
				if ((prim.type == SufPrimType::Shade || prim.type == SufPrimType::UvShade) && point.values.size() >= 6)
				{
					glNormal3f(point.values[3], point.values[4], point.values[5]);
				}
				else
				{
					glNormal3f(normal[0], normal[1], normal[2]);
				}
				glVertex3f(point.values[0], point.values[1], point.values[2]);
			}
			glEnd();
		}
	}
}

void CGLView::DrawLines()
{
	if (!m_scene) return;

	// prim.atrName が空文字列の場合でも最初のプリミティブで
	// ApplyAttribute を必ず呼び出すため、初期値を空でない値にする
	CString lastAtr = _T("\x01");
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (const auto& object : m_scene->Objects())
	{
		for (const auto& prim : object.primitives)
		{
			if (prim.points.size() < 3) continue;
			if (prim.atrName.Compare(lastAtr) != 0)
			{
				ApplyAttribute(prim);
				lastAtr = prim.atrName;
			}

			glBegin(GL_LINE_STRIP);
			for (const auto& point : prim.points)
			{
				if (point.values.size() < 3) continue;
				glVertex3f(point.values[0], point.values[1], point.values[2]);
			}
			glEnd();
		}
	}
}

void CGLView::DrawPoints()
{
	if (!m_scene) return;

	// prim.atrName が空文字列の場合でも最初のプリミティブで
	// ApplyAttribute を必ず呼び出すため、初期値を空でない値にする
	CString lastAtr = _T("\x01");
	glPointSize(4.0f);
	glBegin(GL_POINTS);

	for (const auto& object : m_scene->Objects())
	{
		for (const auto& prim : object.primitives)
		{
			if (prim.points.size() < 3) continue;
			if (prim.atrName.Compare(lastAtr) != 0)
			{
				ApplyAttribute(prim);
				lastAtr = prim.atrName;
			}

			for (const auto& point : prim.points)
			{
				if (point.values.size() < 3) continue;
				glVertex3f(point.values[0], point.values[1], point.values[2]);
			}
		}
	}

	glEnd();
}

void CGLView::OnSize(UINT nType, int width, int height)
{
	CWnd::OnSize(nType, width, height);
	if (width > 0 && height > 0)
	{
		glViewport(0, 0, (GLsizei)width, (GLsizei)height);
		Invalidate(FALSE);
	}
}

void CGLView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CGLView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
#ifndef __GLPASM__
	if (nChar == 'P')
	{
		m_drawPolygon = !m_drawPolygon;
		Invalidate(FALSE);
	}
	if (nChar == 'L')
	{
		m_drawLine = !m_drawLine;
		Invalidate(FALSE);
	}
	if (nChar == '1')
	{
		m_drawPolygon = true;
		m_drawLine = false;
		m_drawPoint = false;
		Invalidate(FALSE);
	}
	if (nChar == '2')
	{
		m_drawPolygon = false;
		m_drawLine = true;
		m_drawPoint = false;
		Invalidate(FALSE);
	}
	if (nChar == '3')
	{
		m_drawPolygon = false;
		m_drawLine = false;
		m_drawPoint = true;
		Invalidate(FALSE);
	}
	if (nChar == '4')
	{
		m_drawPolygon = true;
		m_drawLine = true;
		m_drawPoint = true;
		Invalidate(FALSE);
	}
#endif

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CGLView::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 ---
	#ifdef __GLPASM__
	m_lastPt = point;
	m_dragMode = DragMode::Orbit;
#else
	m_dragStart = point;
	m_dragLast = point;
	m_dragMode = DragMode::Orbit;
#endif
------------------------------------------------------------------------------- */
	m_dragStart = point;
	m_dragLast = point;
	m_dragMode = DragMode::Orbit;
	CWnd::OnRButtonDown(nFlags, point);
}

LRESULT CGLView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// Let ImGui handle Win32 messages first
	if (ImGui_ImplWin32_WndProcHandler(GetSafeHwnd(), message, wParam, lParam))
	{
		return 0;
	}
	return CWnd::WindowProc(message, wParam, lParam);
}

void CGLView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	CWnd::OnRButtonUp(nFlags, point);
}

void CGLView::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 ---

#ifdef __GLPASM__
	m_lastPt = point;
	m_dragMode = DragMode::Pan;
#else
    m_dragStart = point;
	m_dragLast = point;
	m_dragMode = DragMode::Dolly;
#endif
------------------------------------------------------------------------------- */
	m_dragStart = point;
	m_dragLast = point;
	m_dragMode = DragMode::Dolly;
CWnd::OnMButtonDown(nFlags, point);
}

void CGLView::OnMButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	CWnd::OnMButtonUp(nFlags, point);
}

void CGLView::OnMouseMove(UINT nFlags, CPoint point)
{
/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 ---
	#ifdef __GLPASM__
	if (GetCapture() == this)
	{
		if (m_dragMode == DragMode::Orbit)
		{
			const float dx = static_cast<float>(point.x - m_lastPt.x);
			const float dy = static_cast<float>(point.y - m_lastPt.y);
			m_yawDeg += dx * 0.2f;
			m_pitchDeg -= dy * 0.2f;
			Invalidate(FALSE);
		}
		else if (m_dragMode == DragMode::Pan)
		{
			const float dx = static_cast<float>(point.x - m_lastPt.x);
			const float dy = static_cast<float>(point.y - m_lastPt.y);
			m_panX += dx;
			m_panY -= dy;
			Invalidate(FALSE);
		}
		m_lastPt = point;
	}
#else
------------------------------------------------------------------------------- */
	if (GetCapture() == this)
	{
		if (m_dragMode == DragMode::Orbit)
		{
			const float dx = static_cast<float>(point.x - m_dragLast.x);
			const float dy = static_cast<float>(point.y - m_dragLast.y);
			m_rotY += dx * 0.1f;
			m_rotX -= dy * 0.1f;
			Invalidate(FALSE);
		}
		else if (m_dragMode == DragMode::Dolly)
		{
			const float dy = static_cast<float>(point.y - m_dragLast.y);
			m_distance -= dy * 0.5f;
			Invalidate(FALSE);
		}
		m_dragLast = point;
	}
/* --- 2026-06-10 AceCreek: GLPASMモードの表示を削除し、SUFモードの表示に統一 ---
#endif
------------------------------------------------------------------------------ */
		CWnd::OnMouseMove(nFlags, point);
}

BOOL CGLView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_distance -= zDelta * 0.1f;
	Invalidate(FALSE);
	return TRUE;
}
// ImGui -> アプリへの選択メッセージ
// WM_APP 系の空き領域を使います。必要なら値を調整してください。
//inline constexpr UINT WM_IMGUI_SELECT = WM_APP + 0x100;
