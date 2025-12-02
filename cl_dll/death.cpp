/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// death notice
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#if USE_VGUI
#include "vgui_TeamFortressViewport.h"
#endif

DECLARE_MESSAGE( m_DeathNotice, DeathMsg )

struct DeathNoticeItem {
	char szKiller[MAX_PLAYER_NAME_LENGTH * 2];
	char szVictim[MAX_PLAYER_NAME_LENGTH * 2];
	int iId;	// the index number of the associated sprite
	int iSuicide;
	int iTeamKill;
	int iNonPlayerKill;
	float flDisplayTime;
	float flStartTime; // time when this notice was created (for slide animation)
	float flCurrentY;  // current vertical position for smooth stacking animation
	float *KillerColor;
	float *VictimColor;
};

#define MAX_DEATHNOTICES	4
static int DEATHNOTICE_DISPLAY_TIME = 6;

#define DEATHNOTICE_TOP		32

DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES + 1];

float g_ColorBlue[3]	= { 0.6, 0.8, 1.0 };
float g_ColorRed[3]	= { 1.0, 0.25, 0.25 };
float g_ColorGreen[3]	= { 0.6, 1.0, 0.6 };
float g_ColorYellow[3]	= { 1.0, 0.7, 0.0 };
float g_ColorGrey[3]	= { 0.8, 0.8, 0.8 };

float *GetClientColor( int clientIndex )
{
	switch( g_PlayerExtraInfo[clientIndex].teamnumber )
	{
	case 1:	return g_ColorBlue;
	case 2: return g_ColorRed;
	case 3: return g_ColorYellow;
	case 4: return g_ColorGreen;
	case 0: return g_ColorYellow;
	default: return g_ColorGrey;
	}

	return NULL;
}

int CHudDeathNotice::Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( DeathMsg );

	CVAR_CREATE( "hud_deathnotice_time", "6", FCVAR_ARCHIVE );

	return 1;
}

void CHudDeathNotice::InitHUDData( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}

int CHudDeathNotice::VidInit( void )
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );

	return 1;
}

int CHudDeathNotice::Draw( float flTime )
{
	int x, y, r, g, b;

	int gap = 20;

	const wrect_t& sprite = gHUD.GetSpriteRect( m_HUD_d_skull );
	gap = sprite.bottom - sprite.top; // line height

	SCREENINFO screenInfo;

	screenInfo.iSize = sizeof( SCREENINFO );
	gEngfuncs.pfnGetScreenInfo( &screenInfo );
	gap = Q_max( gap, screenInfo.iCharHeight );

	// base Y and extra spacing between stacked messages
	float baseY = (float)YRES( DEATHNOTICE_TOP ) + 2.0f;
	int extraSpacing = YRES( 4 );

	for( int i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if( rgDeathNoticeList[i].iId == 0 )
			break;  // we've gone through them all

		if( rgDeathNoticeList[i].flDisplayTime < flTime )
		{
			// display time has expired
			// remove the current item from the list
			memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i + 1], sizeof(DeathNoticeItem) * ( MAX_DEATHNOTICES - i ) );
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = Q_min( rgDeathNoticeList[i].flDisplayTime, gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME );

		// Only draw if the viewport will let me
		// vgui dropped out
#if USE_VGUI
		if( gViewPort && gViewPort->AllowedToPrintText() )
#endif
		{
			// Target vertical position for this notice (with extra spacing)
			float targetY = baseY + (float)( ( gap + extraSpacing ) * i );

			// Initialize or smoothly move current Y towards target for stacking animation
			if( rgDeathNoticeList[i].flCurrentY == 0.0f )
			{
				rgDeathNoticeList[i].flCurrentY = targetY;
			}
			else
			{
				// simple critically damped-ish interpolation
				float lerp = 0.2f;
				rgDeathNoticeList[i].flCurrentY += ( targetY - rgDeathNoticeList[i].flCurrentY ) * lerp;
			}

			// Draw the death notice
			y = (int)rgDeathNoticeList[i].flCurrentY;

			// Always use a single generic kill icon instead of per-weapon sprites
			int id = m_HUD_d_skull;

			// Fade-out factor based on remaining lifetime (last 1 second fades)
			float fade = 1.0f;
			float fadeDuration = 1.0f;
			float timeLeft = rgDeathNoticeList[i].flDisplayTime - flTime;
			if( timeLeft < fadeDuration )
			{
				fade = timeLeft / fadeDuration;
				if( fade < 0.0f )
					fade = 0.0f;
			}
			// make fade steeper and skip drawing when almost gone
			fade = fade * fade; // quadratic falloff so it darkens faster
			if( fade <= 0.02f )
			{
				continue;
			}

			// Compute final target X (with some right margin)
			int rightMargin = XRES( 16 );
			const wrect_t& iconRect = gHUD.GetSpriteRect( id );
			int spriteWidth = iconRect.right - iconRect.left;
			int killerWidth = 0;
			if( !rgDeathNoticeList[i].iSuicide )
			{
				killerWidth = 5 + ConsoleStringLen( rgDeathNoticeList[i].szKiller );
			}
			int victimWidth = ( rgDeathNoticeList[i].iNonPlayerKill == FALSE ) ? ConsoleStringLen( rgDeathNoticeList[i].szVictim ) : 0;
			int lineWidth = killerWidth + spriteWidth + victimWidth;
			int targetX = ScreenWidth - rightMargin - lineWidth;

			// Slide-in animation from the right (smooth)
			float animDuration = 0.5f; // seconds
			float t = 1.0f;
			if( rgDeathNoticeList[i].flStartTime > 0.0f )
			{
				float life = flTime - rgDeathNoticeList[i].flStartTime;
				if( life < animDuration )
				{
					t = life / animDuration;
					if( t < 0.0f ) t = 0.0f;
				}
			}
			t = Q_min( t, 1.0f );
			// smoothstep easing for nicer motion
			t = t * t * ( 3.0f - 2.0f * t );

			int startOffset = XRES( 120 );
			int animatedX = targetX + (int)((1.0f - t) * startOffset);

			// Simple rectangular background box behind the whole line
			int boxX = animatedX - 6;
			int boxW = lineWidth + 12;
			if( boxW > 0 )
			{
				int bgAlpha = (int)( 100.0f * fade );
				if( bgAlpha > 0 )
				{
					gEngfuncs.pfnFillRGBABlend( boxX, y - 2, boxW, gap + 4, 0, 0, 0, bgAlpha );
				}
			}

			// Now draw text and sprite starting from animatedX
			x = animatedX;
			int textY = y + ( gap - screenInfo.iCharHeight ) / 2;

			if( !rgDeathNoticeList[i].iSuicide )
			{
				// Draw killer's name slightly higher inside the box, with strong fade
				float textFade = fade * fade; // text fades even faster than bg
				int textColor = (int)( 255.0f * textFade );
				if( textColor < 0 ) textColor = 0;
				if( textColor > 255 ) textColor = 255;
				x = DrawUtfString( x + 5, textY, ScreenWidth, rgDeathNoticeList[i].szKiller, textColor, textColor, textColor );
			}

			// Weapon icon: modulate brightness by fade so it also disappears
			int iconColor = (int)( 255.0f * fade );
			if( iconColor < 0 ) iconColor = 0;
			if( iconColor > 255 ) iconColor = 255;
			r = iconColor; g = iconColor; b = iconColor;

			// Draw death weapon, vertically centered in the line
			SPR_Set( gHUD.GetSprite(id), r, g, b );
			// pfnSPR_DrawAdditive doesn't take alpha directly, so rely on global fade in renderer
			int iconY = y + ( gap - ( iconRect.bottom - iconRect.top ) ) / 2;
			SPR_DrawAdditive( 0, x, iconY, &iconRect );

			x += ( iconRect.right - iconRect.left );

			// Draw victim's name (if it was a player that was killed)
			if( rgDeathNoticeList[i].iNonPlayerKill == FALSE )
			{
				float textFade = fade * fade;
				int textColor = (int)( 255.0f * textFade );
				if( textColor < 0 ) textColor = 0;
				if( textColor > 255 ) textColor = 255;
				x = DrawUtfString( x, textY, ScreenWidth, rgDeathNoticeList[i].szVictim, textColor, textColor, textColor );
			}
		}
	}

	return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice::MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	int i;
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );

	int killer = READ_BYTE();
	int victim = READ_BYTE();

	char killedwith[32];
	strcpy( killedwith, "d_" );
	strlcat( killedwith, READ_STRING(), sizeof( killedwith ));

#if USE_VGUI && !USE_NOVGUI_SCOREBOARD
	if (gViewPort)
		gViewPort->DeathMsg( killer, victim );
#else
	gHUD.m_Scoreboard.DeathMsg( killer, victim );
#endif

	gHUD.m_Spectator.DeathMessage( victim );

	for( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if( rgDeathNoticeList[i].iId == 0 )
			break;
	}
	if( i == MAX_DEATHNOTICES )
	{
		// move the rest of the list forward to make room for this item
		memmove( rgDeathNoticeList, rgDeathNoticeList + 1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	gHUD.GetAllPlayersInfo();

	// Get the Killer's name
	const char *killer_name = "";
	killer_name = g_PlayerInfoList[killer].name;
	if( !killer_name )
	{
		killer_name = "";
		rgDeathNoticeList[i].szKiller[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
		strlcpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
	}

	// Get the Victim's name
	const char *victim_name = "";
	// If victim is -1, the killer killed a specific, non-player object (like a sentrygun)
	if( ( (signed char)victim ) != -1 )
		victim_name = g_PlayerInfoList[victim].name;
	if( !victim_name )
	{
		victim_name = "";
		rgDeathNoticeList[i].szVictim[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
		strlcpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );
	}

	// Is it a non-player object kill?
	if( ( (signed char)victim ) == -1 )
	{
		rgDeathNoticeList[i].iNonPlayerKill = TRUE;

		// Store the object's name in the Victim slot (skip the d_ bit)
		strcpy( rgDeathNoticeList[i].szVictim, killedwith + 2 );
	}
	else
	{
		if( killer == victim || killer == 0 )
			rgDeathNoticeList[i].iSuicide = TRUE;

		if( !strcmp( killedwith, "d_teammate" ) )
			rgDeathNoticeList[i].iTeamKill = TRUE;
	}

	// Find the sprite in the list
	int spr = gHUD.GetSpriteIndex( killedwith );

	rgDeathNoticeList[i].iId = spr;
	rgDeathNoticeList[i].flStartTime = gHUD.m_flTime;

	DEATHNOTICE_DISPLAY_TIME = CVAR_GET_FLOAT( "hud_deathnotice_time" );
	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME;

	if( rgDeathNoticeList[i].iNonPlayerKill )
	{
		ConsolePrint( rgDeathNoticeList[i].szKiller );
		ConsolePrint( " killed a " );
		ConsolePrint( rgDeathNoticeList[i].szVictim );
		ConsolePrint( "\n" );
	}
	else
	{
		// record the death notice in the console
		if( rgDeathNoticeList[i].iSuicide )
		{
			ConsolePrint( rgDeathNoticeList[i].szVictim );

			if( !strcmp( killedwith, "d_world" ) )
			{
				ConsolePrint( " died" );
			}
			else
			{
				ConsolePrint( " killed self" );
			}
		}
		else if( rgDeathNoticeList[i].iTeamKill )
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed his teammate " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}
		else
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}

		if( *killedwith && (*killedwith > 13 ) && strcmp( killedwith, "d_world" ) && !rgDeathNoticeList[i].iTeamKill )
		{
			ConsolePrint( " with " );
			ConsolePrint( killedwith + 2 ); // skip over the "d_" part
		}

		ConsolePrint( "\n" );
	}

	return 1;
}
