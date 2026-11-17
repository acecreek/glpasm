#include "pch.h"
#include "GLView.hpp"
#include "SufModel.h"
#include "../PartAssembly.h"

// Implementations moved here to ensure GLView.cpp compiles and link symbols present

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
	if (len <= FLT_EPSILON) return { 0.0f, 0.0f, 1.0f };
	nx /= len;
	ny /= len;
	nz /= len;
	return { nx, ny, nz };
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

void CGLView::DrawPart(const PartInstance& part, int index)
{
	// 最低限のスタブ実装: 必要に応じて詳細実装を追加する
	(void)part;
	(void)index;
}

// 以下は不足していたメソッドの最小実装（リンクエラー回避用）。
// 必要に応じて元の実装を移植してください。

int CGLView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) == -1) return -1;
	m_hDC = ::GetDC(m_hWnd);
	if (!InitGL()) return -1;
	return 0;
}

void CGLView::SetScene(const std::shared_ptr<SufScene>& scene)
{
	m_scene = scene;
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

void CGLView::ResetCamera()
{
	m_yawDeg = -35.0f;
	m_pitchDeg = 25.0f;
	m_distance = -1000.0f;
	m_panX = 0.0f;
	m_panY = 0.0f;
	Invalidate(FALSE);
}
