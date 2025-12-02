#pragma once

#include "hud.h"

// Simple custom crosshair HUD element based on csldr xhair idea
class CHudCrosshair : public CHudBase
{
public:
	int Init(void) override;
	int VidInit(void) override;
	int Draw(float flTime) override;
	void Reset(void) override;

private:
	// cached cvars
	cvar_t* m_enable;
	cvar_t* m_gap;
	cvar_t* m_size;
	cvar_t* m_thick;
	cvar_t* m_dot;
	cvar_t* m_colorR;
	cvar_t* m_colorG;
	cvar_t* m_colorB;
	cvar_t* m_additive;
};

extern CHudCrosshair g_HudCrosshair;
