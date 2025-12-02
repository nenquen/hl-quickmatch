#include "hud_crosshair.h"
#include "cl_util.h"

// Custom crosshair implementation inspired by csldr xhair

CHudCrosshair g_HudCrosshair;

int CHudCrosshair::Init(void)
{
	gHUD.AddHudElem(this);

	m_enable  = CVAR_CREATE("crosshair_enable", "1", FCVAR_ARCHIVE);
	m_gap     = CVAR_CREATE("crosshair_gap", "0", FCVAR_ARCHIVE);
	m_size    = CVAR_CREATE("crosshair_size", "4", FCVAR_ARCHIVE);
	m_thick   = CVAR_CREATE("crosshair_thick", "1", FCVAR_ARCHIVE);
	m_dot     = CVAR_CREATE("crosshair_dot", "0", FCVAR_ARCHIVE);
	m_colorR  = CVAR_CREATE("crosshair_color_r", "0", FCVAR_ARCHIVE);
	m_colorG  = CVAR_CREATE("crosshair_color_g", "1", FCVAR_ARCHIVE);
	m_colorB  = CVAR_CREATE("crosshair_color_b", "0", FCVAR_ARCHIVE);
	m_alpha   = CVAR_CREATE("crosshair_alpha", "1", FCVAR_ARCHIVE);
	m_additive= CVAR_CREATE("crosshair_additive", "0", FCVAR_ARCHIVE);

	m_iFlags = HUD_ACTIVE;
	return 1;
}

int CHudCrosshair::VidInit(void)
{
	return 1;
}

void CHudCrosshair::Reset(void)
{
}

int CHudCrosshair::Draw(float flTime)
{
	if (!m_enable || m_enable->value <= 0.0f)
		return 0;

	if (!gHUD.m_pCvarDraw || gHUD.m_pCvarDraw->value == 0.0f)
		return 0;

	// basic checks similar to original code: no crosshair in intermission
	if (gHUD.m_iIntermission)
		return 0;

	// position
	const int centerX = ScreenWidth / 2;
	const int centerY = ScreenHeight / 2;

	const float alpha = m_alpha ? m_alpha->value : 1.0f;
	if (alpha <= 0.0f)
		return 0;

	int r = 0, g = 255, b = 0;
	if (m_colorR) r = (int)(m_colorR->value * 255.0f);
	if (m_colorG) g = (int)(m_colorG->value * 255.0f);
	if (m_colorB) b = (int)(m_colorB->value * 255.0f);
	int a = (int)(alpha * 255.0f);

	int gap     = (int)m_gap->value;
	int size    = (int)m_size->value;
	int thick   = (int)m_thick->value;
	if (thick < 1) thick = 1;

	// center dot
	if (m_dot && m_dot->value > 0.0f)
	{
		int x0 = centerX - thick / 2;
		int y0 = centerY - thick / 2;
		FillRGBA(x0, y0, thick, thick, r, g, b, a);
	}

	// vertical lines
	int x0 = centerX - thick / 2;
	int x1 = thick;

	// top
	FillRGBA(x0, centerY - gap - size, x1, size, r, g, b, a);
	// bottom
	FillRGBA(x0, centerY + gap, x1, size, r, g, b, a);

	// horizontal lines
	int y0 = centerY - thick / 2;
	int y1 = thick;

	// left
	FillRGBA(centerX - gap - size, y0, size, y1, r, g, b, a);
	// right
	FillRGBA(centerX + gap, y0, size, y1, r, g, b, a);

	return 1;
}
