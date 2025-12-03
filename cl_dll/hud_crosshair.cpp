#include "hud_crosshair.h"
#include "cl_util.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "triangleapi.h"
#include "camera.h"

extern "C" int CL_IsThirdPerson( void );
extern vec3_t v_origin;
extern vec3_t v_angles;

// Custom crosshair implementation inspired by csldr xhair

CHudCrosshair g_HudCrosshair;

int CHudCrosshair::Init(void)
{
	gHUD.AddHudElem(this);

	m_enable  = CVAR_CREATE("crosshair_enable", "1", FCVAR_ARCHIVE);
	m_gap     = CVAR_CREATE("crosshair_gap", "4", FCVAR_ARCHIVE);
	m_size    = CVAR_CREATE("crosshair_size", "5", FCVAR_ARCHIVE);
	m_thick   = CVAR_CREATE("crosshair_thick", "1", FCVAR_ARCHIVE);
	m_dot     = CVAR_CREATE("crosshair_dot", "0", FCVAR_ARCHIVE);
	m_colorR  = CVAR_CREATE("crosshair_color_r", "0", FCVAR_ARCHIVE);
	m_colorG  = CVAR_CREATE("crosshair_color_g", "1", FCVAR_ARCHIVE);
	m_colorB  = CVAR_CREATE("crosshair_color_b", "0", FCVAR_ARCHIVE);
	m_additive= CVAR_CREATE("crosshair_additive", "0", FCVAR_ARCHIVE);

	// init scope sprite cache
	m_hScope = 0;
	m_iScopeWidth = m_iScopeHeight = 0;

	m_iFlags = HUD_ACTIVE;
	return 1;
}

int CHudCrosshair::VidInit(void)
{
	// Load scope overlay sprite (used when zoomed)
	m_hScope = SPR_Load("sprites/awpscope.spr");
	if (m_hScope)
	{
		m_iScopeWidth = SPR_Width(m_hScope, 0);
		m_iScopeHeight = SPR_Height(m_hScope, 0);
	}
	else
	{
		m_iScopeWidth = m_iScopeHeight = 0;
	}

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

	// If we are zoomed (FOV < normal), draw scope overlay instead of the custom crosshair
	if (gHUD.m_iFOV > 0 && gHUD.m_iFOV < 90)
	{
		if (m_hScope && m_iScopeWidth > 0 && m_iScopeHeight > 0)
		{
			const int x = (ScreenWidth  - m_iScopeWidth)  / 2;
			const int y = (ScreenHeight - m_iScopeHeight) / 2;
			wrect_t rc;
			rc.left = 0;
			rc.top = 0;
			rc.right = m_iScopeWidth;
			rc.bottom = m_iScopeHeight;

			SPR_Set(m_hScope, 255, 255, 255);
			SPR_DrawHoles(0, x, y, &rc);
		}
		// When zoomed, suppress the normal crosshair
		return 1;
	}

	// position (default: screen center)
	int centerX = ScreenWidth  / 2;
	int centerY = ScreenHeight / 2;

	// In third person, move the crosshair to the actual aim hit point so that
	// it reflects where the camera is aiming, not just the screen center.
	if( CL_IsThirdPerson() )
	{
		vec3_t start, forward, right, up, end;
		AngleVectors( v_angles, forward, right, up );

		// v_origin is the current camera position (already offset back/right/up).
		// Reconstruct an approximate eye position by undoing the same offsets
		// we applied in V_CalcNormalRefdef: back along forward, minus right/up.
		const float backDist = cam_ofs[2];   // same value used for camera distance
		const float sideDist = 16.0f;       // must match view.cpp
		const float upDist   = 6.0f;        // must match view.cpp

		VectorCopy( v_origin, start );
		VectorMA( start,  backDist, forward, start );   // move forward to player
		VectorMA( start, -sideDist, right,   start );   // undo right-shoulder shift
		VectorMA( start, -upDist,   up,      start );   // undo vertical lift

		VectorMA( start, 4096.0f, forward, end );

		pmtrace_t *trace = gEngfuncs.PM_TraceLine( start, end, PM_TRACELINE_PHYSENTSONLY, 2, -1 );
		float world[3], screen[3];

		if( trace )
		{
			VectorCopy( trace->endpos, world );
		}
		else
		{
			VectorCopy( end, world );
		}

		// Project world-space hit position to screen-space.
		if( !gEngfuncs.pTriAPI->WorldToScreen( world, screen ) )
		{
			centerX = (int)XPROJECT( screen[0] );
			centerY = (int)YPROJECT( screen[1] );
		}
	}

	const float alpha = 1.0f;
	int r = 0, g = 255, b = 0;
	if (m_colorR) r = (int)(m_colorR->value * 255.0f);
	if (m_colorG) g = (int)(m_colorG->value * 255.0f);
	if (m_colorB) b = (int)(m_colorB->value * 255.0f);
	int a = (int)(alpha * 255.0f);

	int gap  = (int)(m_gap  ? m_gap->value  : 0.0f);
	int size = (int)(m_size ? m_size->value : 4.0f);
	if (gap  < 0) gap  = 0;
	if (size < 1) size = 1;

	// center dot (1x1)
	if (m_dot && m_dot->value > 0.0f)
	{
		FillRGBA(centerX, centerY, 1, 1, r, g, b, a);
	}

	// vertical arms: 1 pixel wide
	// gap is number of pixels between center and start of arm
	int topY    = centerY - gap - size;
	int bottomY = centerY + gap + 1;

	// top arm goes from (centerY - gap - size) .. (centerY - gap - 1)
	FillRGBA(centerX, topY, 1, size, r, g, b, a);
	// bottom arm goes from (centerY + gap + 1) .. (centerY + gap + size)
	FillRGBA(centerX, bottomY, 1, size, r, g, b, a);

	// horizontal arms: 1 pixel tall
	int leftX  = centerX - gap - size;
	int rightX = centerX + gap + 1;

	// left arm goes from (centerX - gap - size) .. (centerX - gap - 1)
	FillRGBA(leftX, centerY, size, 1, r, g, b, a);
	// right arm goes from (centerX + gap + 1) .. (centerX + gap + size)
	FillRGBA(rightX, centerY, size, 1, r, g, b, a);

	return 1;
}
