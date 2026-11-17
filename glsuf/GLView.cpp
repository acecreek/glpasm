#include "pch.h"
#include "GLView.hpp"
#include "SufModel.h"
#include "../resource.h"

// Dear ImGui
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_opengl3.h"
#include "../third_party/imgui/backends/imgui_impl_win32.h"
// imgui_impl_win32.h intentionally leaves the WndProc handler declaration inside a disabled block.
// Forward-declare it here so we can call it from WindowProc without a missing symbol error.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include "SufModel.h"

// WM_IMGUI_SELECT は resource.h で定義済み

BEGIN_MESSAGE_MAP(CGLView, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
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

	return CreateEx(0, cls, _T("SUF OpenGL View"),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
		CRect(0, 0, 100, 100), parent, id);
}

void CGLView::SetScene(const std::shared_ptr<SufScene>& scene)
{
	m_scene = scene;
	FrameAll();
}

void CGLView::SetAssembly(const std::shared_ptr<PartAssembly>& assembly)
{
	m_assembly = assembly;
	m_selectedPart = -1;
	FrameAll();
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

void CGLView::FrameAll()
{
	m_center = { 0.0f, 0.0f, 0.0f };
	float radius = 400.0f;

	if (m_scene)
	{
		const auto bounds = m_scene->Bounds();
		if (bounds.valid)
		{
			m_center = {
				(bounds.min[0] + bounds.max[0]) * 0.5f,
				(bounds.min[1] + bounds.max[1]) * 0.5f,
				(bounds.min[2] + bounds.max[2]) * 0.5f
			};
			const float sx = bounds.max[0] - bounds.min[0];
			const float sy = bounds.max[1] - bounds.min[1];
			const float sz = bounds.max[2] - bounds.min[2];
			radius = (std::max)(100.0f, std::sqrt(sx * sx + sy * sy + sz * sz) * 0.75f);
		}
	}

	m_distance = -((std::max)(300.0f, radius * 2.2f));
	Invalidate(FALSE);
}

void CGLView::ResetCamera()
{
	m_rotX = -55.0f;
	m_rotY = -125.0f;
	m_rotZ = 0.0f;
	FrameAll();
}

int CGLView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) == -1) return -1;
	m_hDC = ::GetDC(m_hWnd);
	if (!m_hDC || !InitGL()) return -1;
	return 0;
}

void CGLView::OnDestroy()
{
	DestroyGL();
	CWnd::OnDestroy();
}

void CGLView::OnPaint()
{
	CPaintDC dc(this);
	if (!m_ready || !wglMakeCurrent(m_hDC, m_hRC)) return;

	RenderScene();
	// Draw ImGui UI into the current GL context
	DrawImGui();
	::SwapBuffers(m_hDC);
	DrawOverlay(dc.GetSafeHdc());
}

void CGLView::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (!m_ready || cx <= 0 || cy <= 0) return;
	wglMakeCurrent(m_hDC, m_hRC);
	glViewport(0, 0, cx, cy);
}

void CGLView::OnRButtonDown(UINT flags, CPoint pt)
{
	SetCapture();
	SetFocus();
	m_lastPt = pt;
	m_dragMode = (flags & MK_SHIFT) ? DragMode::Dolly : DragMode::Orbit;
}

void CGLView::OnRButtonUp(UINT, CPoint)
{
	if (GetCapture() == this) ReleaseCapture();
	m_dragMode = DragMode::None;
}

void CGLView::OnMouseMove(UINT, CPoint pt)
{
	if (m_dragMode == DragMode::None) return;

	const int dx = pt.x - m_lastPt.x;
	const int dy = pt.y - m_lastPt.y;
	m_lastPt = pt;

	if (m_dragMode == DragMode::Dolly)
	{
		m_distance -= dy * 10.0f;
	}
	else
	{
		m_rotX -= dy * 0.25f;
		m_rotY -= dx * 0.25f;
		m_rotX -= static_cast<float>(static_cast<int>(m_rotX / 360.0f)) * 360.0f;
		m_rotY -= static_cast<float>(static_cast<int>(m_rotY / 360.0f)) * 360.0f;
	}
	Invalidate(FALSE);
}

BOOL CGLView::OnMouseWheel(UINT, short delta, CPoint)
{
	m_distance += (delta > 0) ? -30.0f : 30.0f;
	if (m_distance > -20.0f) m_distance = -20.0f;
	if (m_distance < -20000.0f) m_distance = -20000.0f;
	Invalidate(FALSE);
	return TRUE;
}

void CGLView::OnLButtonDblClk(UINT, CPoint)
{
	ResetCamera();
}

void CGLView::OnKeyDown(UINT ch, UINT, UINT)
{
	switch (ch)
	{
	case '1': SetDrawPolygon(true); SetDrawLine(false); SetDrawPoint(false); break;
	case '2': SetDrawPolygon(false); SetDrawLine(true); SetDrawPoint(false); break;
	case '3': SetDrawPolygon(false); SetDrawLine(false); SetDrawPoint(true); break;
	case '4': SetDrawPolygon(true); SetDrawLine(true); SetDrawPoint(true); break;
	case 'G': ToggleGrid(); break;
	case 'X': ToggleAxes(); break;
	case 'F': FrameAll(); break;
	default: CWnd::OnKeyDown(ch, 0, 0); break;
	}
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
	ImGui_ImplWin32_Init(m_hWnd);
	ImGui_ImplOpenGL3_Init("#version 130");

	return true;
}

void CGLView::DestroyGL()
{
	// Shutdown ImGui
	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

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
}

void CGLView::RenderScene()
{
	// Placeholder patch: no-op change to ensure context matches for patch application.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Ensure grid and axes are drawn immediately after clearing so overlay
	// behavior and hit-testing remain consistent even when later rendering
	// or lighting state changes.
	if (m_showGrid) DrawGrid(1000.0f, 100.0f);
	if (m_showAxes) DrawAxes(1800.0f);

	CRect rc;
	GetClientRect(&rc);
	const double aspect = static_cast<double>((std::max)(1, rc.Width())) /
		static_cast<double>((std::max)(1, rc.Height()));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0, aspect, 1.0, 20000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	ApplyCamera();

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

	if (m_drawPolygon) DrawPolygons();
	if (m_drawLine) DrawLines();
	if (m_drawPoint) DrawPoints();

	glDisable(GL_LIGHTING);
	glFinish();
}

void CGLView::ApplyCamera()
{
	// Use legacy camera that matches other restored functions
	glTranslated(0.0, 0.0, m_distance);
	glRotatef(m_rotX, 1.0f, 0.0f, 0.0f);
	glRotatef(m_rotY, 0.0f, 0.0f, 1.0f);
	glRotatef(m_rotZ, 0.0f, 1.0f, 0.0f);
	glTranslatef(-m_center[0], -m_center[1], -m_center[2]);
}

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

	// DoGACGA の tra 値（0.0=不透明、1.0=透明）を OpenGL のアルファ値（0.0=透明、1.0=不透明）に変換
	const float alpha = 1.0f - atr->tra;
	GLfloat amb[] = {
		atr->amb * atr->col[0],
		atr->amb * atr->col[1],
		atr->amb * atr->col[2],
		alpha
	};
	GLfloat dif[] = {
		atr->dif * atr->col[0],
		atr->dif * atr->col[1],
		atr->dif * atr->col[2],
		alpha
	};
	GLfloat spc[] = {
		atr->spc[0],
		atr->spc[0],
		atr->spc[0],
		1.0f
	};
	const float emissionScale = (atr->amb == 1.0f) ? 1.0f : 0.0f;
	GLfloat ems[] = {
		emissionScale * atr->col[0],
		emissionScale * atr->col[1],
		emissionScale * atr->col[2],
		alpha
	};

	glAlphaFunc(GL_LEQUAL, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spc);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ems);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 128.0f * atr->spc[1]);
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

LRESULT CGLView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui にフォワード。ImGui が処理したらそこで終了する。
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler((HWND)m_hWnd, message, wParam, lParam))
        return 0;

    return CWnd::WindowProc(message, wParam, lParam);
}

void CGLView::DrawOverlay(HDC hdc)
{
	CRect rc;
	GetClientRect(&rc);
	CString text;

	size_t objects = 0;
	size_t prims = 0;
	if (m_scene)
	{
		objects = m_scene->Objects().size();
		for (const auto& object : m_scene->Objects()) prims += object.primitives.size();
	}
	text.Format(_T("SUF | RMB Rotate | Shift+RMB Dolly | Wheel Zoom | 1 Poly 2 Line 3 Point 4 All | Objects: %zu | Prims: %zu"),
		objects, prims);
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, RGB(230, 235, 240));
	CRect textRc(12, 10, rc.Width() - 12, 32);
	DrawText(hdc, text, -1, &textRc, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
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

void CGLView::SetSelectedPart(int index)
{
	m_selectedPart = index;
	Invalidate(FALSE);
}