#include "pch.h"
#if 0

BEGIN_MESSAGE_MAP(CGLView, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
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

	return CreateEx(0, cls, _T("Perspective View"),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
		CRect(0, 0, 100, 100), parent, id);
}

void CGLView::SetAssembly(const std::shared_ptr<PartAssembly>& assembly)
{
	m_assembly = assembly;
	m_selectedPart = -1;
	FrameAll();
}

void CGLView::SetSelectedPart(int index)
{
	m_selectedPart = index;
	Invalidate(FALSE);
}

void CGLView::FrameAll()
{
	float maxExtent = 250.0f;
	if (m_assembly)
	{
		for (const auto& part : m_assembly->Parts())
		{
			maxExtent = (std::max)(maxExtent, std::fabs(part.move.x) + std::fabs(part.scale.x) * 40.0f);
			maxExtent = (std::max)(maxExtent, std::fabs(part.move.y) + std::fabs(part.scale.y) * 40.0f);
			maxExtent = (std::max)(maxExtent, std::fabs(part.move.z) + std::fabs(part.scale.z) * 40.0f);
		}
	}

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

void CGLView::ToggleWireframe()
{
	m_wireframe = !m_wireframe;
	Invalidate(FALSE);
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

void CGLView::OnMButtonDown(UINT flags, CPoint pt)
{
	SetCapture();
	SetFocus();
	m_lastPt = pt;
	m_dragMode = (flags & MK_SHIFT) ? DragMode::Pan : DragMode::Orbit;
}

void CGLView::OnMButtonUp(UINT, CPoint)
{
	if (GetCapture() == this) ReleaseCapture();
	m_dragMode = DragMode::None;
}

void CGLView::OnMouseMove(UINT, CPoint pt)
{
	if (m_dragMode == DragMode::None) return;

	const float dx = static_cast<float>(pt.x - m_lastPt.x);
	const float dy = static_cast<float>(pt.y - m_lastPt.y);
	m_lastPt = pt;

	if (m_dragMode == DragMode::Orbit)
	{
		m_yawDeg += dx * 0.3f;
		m_pitchDeg -= dy * 0.3f;
		m_pitchDeg = (std::max)(-89.0f, (std::min)(89.0f, m_pitchDeg));
	}
	else
	{
		const float scale = m_distance * 0.0025f;
		m_panX -= dx * scale;
		m_panY += dy * scale;
	}
	Invalidate(FALSE);
}

BOOL CGLView::OnMouseWheel(UINT, short delta, CPoint)
{
	m_distance *= (delta > 0) ? 0.90f : 1.10f;
	m_distance = (std::max)(20.0f, (std::min)(10000.0f, m_distance));
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
	case 'G': ToggleGrid(); break;
	case 'X': ToggleAxes(); break;
	case 'W': ToggleWireframe(); break;
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

	glClearColor(0.055f, 0.062f, 0.070f, 1.0f);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_COLOR_MATERIAL);
	m_ready = true;
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
}

void CGLView::RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CRect rc;
	GetClientRect(&rc);
	const float aspect = static_cast<float>((std::max)(1, rc.Width())) /
		static_cast<float>((std::max)(1, rc.Height()));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, aspect, 1.0, 20000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	ApplyCamera(aspect);

	const GLfloat lightPos[] = { -0.4f, -0.6f, 0.8f, 0.0f };
	const GLfloat lightAmb[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmb);

	if (m_showGrid) DrawGrid(500.0f, 25.0f);
	if (m_showAxes) DrawAxes(140.0f);

	glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);
	if (m_assembly && !m_assembly->Parts().empty())
	{
		int i = 0;
		for (const auto& part : m_assembly->Parts())
		{
			DrawPart(part, i++);
		}
	}
	else
	{
		PartInstance demo;
		demo.name = _T("Preview");
		demo.scale = { 2.0f, 1.25f, 0.9f };
		DrawPart(demo, -1);
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void CGLView::ApplyCamera(float)
{
	const float pi = 3.1415926535f;
	const float yaw = m_yawDeg * pi / 180.0f;
	const float pitch = m_pitchDeg * pi / 180.0f;
	const float cp = std::cos(pitch);
	const float eyeX = -std::cos(yaw) * cp * m_distance + m_panX;
	const float eyeY = -std::sin(yaw) * cp * m_distance + m_panY;
	const float eyeZ = -std::sin(pitch) * m_distance;
	gluLookAt(eyeX, eyeY, eyeZ, m_panX, m_panY, 0.0, 0.0, 0.0, 1.0);
}

void CGLView::DrawGrid(float half, float step)
{
	const GLboolean lighting = glIsEnabled(GL_LIGHTING);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	glBegin(GL_LINES);
	for (float x = -half; x <= half + 0.001f; x += step)
	{
		const bool major = std::fmod(std::fabs(x), step * 4.0f) < 0.001f;
		const float c = major ? 0.30f : 0.20f;
		glColor3f(c, c, c);
		glVertex3f(x, -half, 0.0f);
		glVertex3f(x, half, 0.0f);
	}
	for (float y = -half; y <= half + 0.001f; y += step)
	{
		const bool major = std::fmod(std::fabs(y), step * 4.0f) < 0.001f;
		const float c = major ? 0.30f : 0.20f;
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
	glColor3f(0.9f, 0.1f, 0.1f); glVertex3f(0, 0, 0); glVertex3f(length, 0, 0);
	glColor3f(0.1f, 0.8f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, length, 0);
	glColor3f(0.2f, 0.35f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, length);
	glEnd();
	glLineWidth(1.0f);
	if (lighting) glEnable(GL_LIGHTING);
}

void CGLView::DrawPart(const PartInstance& part, int index)
{
	glPushMatrix();
	glTranslatef(part.move.x, part.move.y, part.move.z);
	glRotatef(part.rotateDeg.z, 0, 0, 1);
	glRotatef(part.rotateDeg.y, 0, 1, 0);
	glRotatef(part.rotateDeg.x, 1, 0, 0);
	glScalef(part.scale.x * 35.0f, part.scale.y * 35.0f, part.scale.z * 35.0f);
	DrawUnitBox(index == m_selectedPart);
	glPopMatrix();
}

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

void CGLView::DrawOverlay(HDC hdc)
{
	CRect rc;
	GetClientRect(&rc);
	CString text;
	text.Format(_T("Perspective | MMB Orbit | Shift+MMB Pan | Wheel Zoom | G Grid | X Axes | W Wire | F Frame | Parts: %zu"),
		m_assembly ? m_assembly->Parts().size() : 0);

	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, RGB(230, 235, 240));
	CRect textRc(12, 10, rc.Width() - 12, 32);
	DrawText(hdc, text, -1, &textRc, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
}
