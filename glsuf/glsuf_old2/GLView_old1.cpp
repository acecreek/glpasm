#include "pch.h"
#include "GLView.hpp"
#ifndef __GLPASM__
#include "SufModel.h"
#endif
#include "../PartAssembly.h"
#ifndef __GLPASM__
#include "resource_2.h"
#endif

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

void CGLView::OnPaint()
{
	CPaintDC dc(this);
	if (!m_ready || !wglMakeCurrent(m_hDC, m_hRC)) return;

	RenderScene();
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
	const double aspect = static_cast<double>((std::max)(1, rc.Width())) /
		static_cast<double>((std::max)(1, rc.Height()));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0, aspect, 1.0, 20000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#ifdef __GLPASM__
	ApplyCamera(aspect);
#else
	ApplyCamera();
#endif

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

#ifndef __GLPASM__
	if (m_drawPolygon) DrawPolygons();
	if (m_drawLine) DrawLines();
	if (m_drawPoint) DrawPoints();
#endif
//	else
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

#ifndef __GLPASM__
void CGLView::ApplyCamera()
{
	glTranslated(0.0, 0.0, m_distance);
	glRotatef(m_rotX, 1.0f, 0.0f, 0.0f);
	glRotatef(m_rotY, 0.0f, 0.0f, 1.0f);
	glRotatef(m_rotZ, 0.0f, 1.0f, 0.0f);
	glTranslatef(-m_center[0], -m_center[1], -m_center[2]);
}
#endif

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

void CGLView::DrawOverlay(HDC hdc)
{
	CRect rc;
	GetClientRect(&rc);
	CString text;

#ifdef __GLPASM__
	size_t parts = 0;
	if (m_assembly)
	{
		parts = m_assembly->Parts().size();
	}
	text.Format(_T("GLPASM | RMB Orbit | MMB Pan | Wheel Zoom | Parts: %zu | Selected: %d"),
		parts, m_selectedPart);
#else
	size_t objects = 0;
	size_t prims = 0;
	if (m_scene)
	{
		objects = m_scene->Objects().size();
		for (const auto& object : m_scene->Objects()) prims += object.primitives.size();
	}
	text.Format(_T("SUF | RMB Rotate | Shift+RMB Dolly | Wheel Zoom | 1 Poly 2 Line 3 Point 4 All | Objects: %zu | Prims: %zu"),
		objects, prims);
#endif

	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, RGB(230, 235, 240));
	CRect textRc(12, 10, rc.Width() - 12, 32);
	DrawText(hdc, text, -1, &textRc, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
}

#ifndef __GLPASM__
void CGLView::ApplyAttribute(const SufPrimitive& prim)
{
	// TODO: Implement ApplyAttribute
}

void CGLView::DrawImGui()
{
	// TODO: Implement DrawImGui
}

std::array<float, 3> CGLView::PolygonNormal(const SufPrimitive& prim)
{
	// TODO: Implement PolygonNormal
	return { 0.0f, 0.0f, 0.0f };
}
#endif

void CGLView::DrawPolygons()
{
	if (!m_scene) return;

	CString lastAtr;
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

	CString lastAtr;
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

	CString lastAtr;
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
//#endif

void CGLView::OnSize(UINT nType, int width, int height)
{
	CWnd::OnSize(nType, width, height);
	if (width > 0 && height > 0)
	{
		glViewport(0, 0, (GLsizei)width, (GLsizei)height);
		Invalidate(FALSE);
	}
}

void CGLView::OnDestroy()
{
	DestroyGL();
	CWnd::OnDestroy();
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
#ifdef __GLPASM__
	m_lastPt = point;
	m_dragMode = DragMode::Orbit;
#else
	m_dragStart = point;
	m_dragLast = point;
	m_dragMode = DragMode::Orbit;
#endif
	CWnd::OnRButtonDown(UINT nFlags, CPoint point);
}

void CGLView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
}

void CGLView::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
#ifdef __GLPASM__
	m_lastPt = point;
	m_dragMode = DragMode::Pan;
#else
	m_dragStart = point;
	m_dragLast = point;
	m_dragMode = DragMode::Dolly;
#endif
	CWnd::OnMButtonDown(UINT nFlags, CPoint point);
}

void CGLView::OnMButtonUp(UINT nFlags, CPoint point)
#ifdef __GLPASM__
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
#else
	if (m_dragMode == DragMode::Orbit)
	{
		const float dx = static_cast<float>(point.x - m_dragLast.x);
		const float dy = static_cast<float>(point.y - m_dragLast.y);
		m_rotY += dx * 0.1f;
		m_rotX -= dy * 0.1f;
		Invalidate(FALSE);
	}
	if (m_dragMode == DragMode::Dolly)
	{
		const float dy = static_cast<float>(point.y - m_dragLast.y);
		m_distance -= dy * 0.5f;
		Invalidate(FALSE);
	}
	m_dragLast = point;
#endif
	CWnd::OnMouseMove(UINT nFlags, CPoint point)
		const float dy = static_cast<float>(point.y - m_dragLast.y);
		m_distance -= dy * 0.5f;
		Invalidate(FALSE);
	}

	m_dragLast = point;
}

BOOL CGLView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_distance -= zDelta * 0.1f;
	Invalidate(FALSE);
	return TRUE;
}
