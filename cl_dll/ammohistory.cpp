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
//  ammohistory.cpp
//


#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"

HistoryResource gHR;

#define AMMO_PICKUP_GAP ( gHR.iHistoryGap + 5 )
#define AMMO_PICKUP_PICK_HEIGHT		( 32 + ( gHR.iHistoryGap * 2 ) )
#define AMMO_PICKUP_HEIGHT_MAX		( ScreenHeight - 100 )

#define MAX_ITEM_NAME	32
int HISTORY_DRAW_TIME = 5;

// keep a list of items
struct ITEM_INFO
{
	char szName[MAX_ITEM_NAME];
	HSPRITE spr;
	wrect_t rect;
};

void HistoryResource::AddToHistory( int iType, int iId, int iCount )
{
	// Do not show anything for pure ammo pickups anymore.
	// Only weapon pickups (HISTSLOT_WEAP) and items (HISTSLOT_ITEM) create HUD entries.
	if( iType == HISTSLOT_AMMO )
		return;

	if( ( ( ( AMMO_PICKUP_GAP * iCurrentHistorySlot ) + AMMO_PICKUP_PICK_HEIGHT ) > AMMO_PICKUP_HEIGHT_MAX ) || ( iCurrentHistorySlot >= MAX_HISTORY ) )
	{
		// the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}
	
	HIST_ITEM *freeslot = &rgAmmoHistory[iCurrentHistorySlot++];  // default to just writing to the first slot
	HISTORY_DRAW_TIME = CVAR_GET_FLOAT( "hud_drawhistory_time" );

	freeslot->type = iType;
	freeslot->iId = iId;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gHUD.m_flTime + HISTORY_DRAW_TIME;
}

void HistoryResource::AddToHistory( int iType, const char *szName, int iCount )
{
	if( iType != HISTSLOT_ITEM )
		return;

	if( ( ( ( AMMO_PICKUP_GAP * iCurrentHistorySlot ) + AMMO_PICKUP_PICK_HEIGHT ) > AMMO_PICKUP_HEIGHT_MAX ) || ( iCurrentHistorySlot >= MAX_HISTORY ) )
	{
		// the pic would have to be drawn too high
		// so start from the bottom
		iCurrentHistorySlot = 0;
	}

	HIST_ITEM *freeslot = &rgAmmoHistory[iCurrentHistorySlot++];  // default to just writing to the first slot

	// I am really unhappy with all the code in this file
	int i = gHUD.GetSpriteIndex( szName );
	if( i == -1 )
		return;  // unknown sprite name, don't add it to history

	freeslot->iId = i;
	freeslot->type = iType;
	freeslot->iCount = iCount;

	HISTORY_DRAW_TIME = CVAR_GET_FLOAT( "hud_drawhistory_time" );
	freeslot->DisplayTime = gHUD.m_flTime + HISTORY_DRAW_TIME;
}

void HistoryResource::CheckClearHistory( void )
{
	for( int i = 0; i < MAX_HISTORY; i++ )
	{
		if( rgAmmoHistory[i].type )
			return;
	}

	iCurrentHistorySlot = 0;
}

//
// Draw Ammo pickup history
//
int HistoryResource::DrawAmmoHistory( float flTime )
{
	SCREENINFO screenInfo;
	screenInfo.iSize = sizeof( SCREENINFO );
	gEngfuncs.pfnGetScreenInfo( &screenInfo );

	for( int i = 0; i < MAX_HISTORY; i++ )
	{
		if( rgAmmoHistory[i].type )
		{
			rgAmmoHistory[i].DisplayTime = Q_min( rgAmmoHistory[i].DisplayTime, gHUD.m_flTime + HISTORY_DRAW_TIME );

			if( rgAmmoHistory[i].DisplayTime <= flTime )
			{
				// pic drawing time has expired
				memset( &rgAmmoHistory[i], 0, sizeof(HIST_ITEM) );
				CheckClearHistory();
			}
			else if( rgAmmoHistory[i].type == HISTSLOT_WEAP )
			{
				WEAPON *weap = gWR.GetWeapon( rgAmmoHistory[i].iId );
				
				if( !weap )
					return 1;  // we don't know about the weapon yet, so don't draw anything

				// Show weapon pickup as text instead of an icon. Strip the "weapon_" prefix
				// so only the short name is shown (e.g. "glock", "mp5", "knife").
				char displayName[64];
				const char* srcName = weap->szName;
				const char* prefix = "weapon_";
				if( !strncmp( srcName, prefix, strlen( prefix ) ) )
					strlcpy( displayName, srcName + strlen( prefix ), sizeof( displayName ) );
				else
					strlcpy( displayName, srcName, sizeof( displayName ) );
				
				// If ammo has been picked up for this weapon (merged via HISTSLOT_AMMO),
				// append a "( + count )" suffix.
				char fullText[96];
				if( rgAmmoHistory[i].iCount > 0 )
				{
					_snprintf( fullText, sizeof( fullText ), "%s ( + %d )", displayName, rgAmmoHistory[i].iCount );
				}
				else
				{
					strlcpy( fullText, displayName, sizeof( fullText ) );
				}
				
				// Fade factor as before.
				float scale = ( rgAmmoHistory[i].DisplayTime - flTime ) * 80.0f;
				float clampedScale = Q_min( scale, 255.0f );
				if( clampedScale < 0.0f )
					clampedScale = 0.0f;
				
				int ypos = ScreenHeight - ( AMMO_PICKUP_PICK_HEIGHT + ( AMMO_PICKUP_GAP * i ) );
				
				// Measure text width using console font utilities.
				int textWidth = ConsoleStringLen( fullText );
				// Use some padding so there is visible empty space on the right side again.
				int paddingXLeft = XRES( 2 );
				int paddingXRight = XRES( 20 );
				int paddingY = YRES( 2 );
				int boxWidth = textWidth + paddingXLeft + paddingXRight;
				int boxHeight = gHUD.m_iFontHeight + paddingY * 2;
				
				// Position box anchored near the right side of the screen with a small margin.
				int rightMargin = XRES( 6 );
				int boxX = screenInfo.iWidth - boxWidth - rightMargin;
				int boxY = ypos;
				
				// Background: translucent black, alpha similar to death notices.
				int bgR = 0, bgG = 0, bgB = 0;
				int bgA = (int)( 100.0f * ( clampedScale / 255.0f ) );
				if( bgA > 0 )
				{
					gEngfuncs.pfnFillRGBABlend( boxX, boxY, boxWidth, boxHeight, bgR, bgG, bgB, bgA );
				}
				
				// Text position inside the box.
				int textX = boxX + paddingXLeft;
				int textY = boxY + paddingY;
				
				// Compute green->white flash for the text based on how new this entry is.
				float totalTime = CVAR_GET_FLOAT( "hud_drawhistory_time" );
				if( totalTime <= 0.0f )
					totalTime = 1.0f;
				float remaining = rgAmmoHistory[i].DisplayTime - flTime;
				float elapsed = totalTime - remaining;
				if( elapsed < 0.0f )
					elapsed = 0.0f;
				
				const float flashWindow = 0.4f;
				float flashT = 0.0f;
				if( flashWindow > 0.0f && elapsed < flashWindow )
				{
					flashT = 1.0f - ( elapsed / flashWindow );
					if( flashT < 0.0f ) flashT = 0.0f;
					if( flashT > 1.0f ) flashT = 1.0f;
				}
				
				// Text color: blend from green to white based on flashT, then apply alpha fade.
				int r = (int)( ( 1.0f - flashT ) * 255.0f );
				int g = 255;
				int b = (int)( ( 1.0f - flashT ) * 255.0f );
				ScaleColors( r, g, b, (int)clampedScale );
				DrawUtfString( textX, textY, ScreenWidth, fullText, r, g, b );
			}
			else if( rgAmmoHistory[i].type == HISTSLOT_ITEM )
			{
				int r, g, b;

				if( !rgAmmoHistory[i].iId )
					continue;  // sprite not loaded

				wrect_t rect = gHUD.GetSpriteRect( rgAmmoHistory[i].iId );

				UnpackRGB( r, g, b, RGB_YELLOWISH );
				float scale = ( rgAmmoHistory[i].DisplayTime - flTime ) * 80;
				ScaleColors( r, g, b, Q_min( scale, 255 ) );

				int ypos = ScreenHeight - ( AMMO_PICKUP_PICK_HEIGHT + ( AMMO_PICKUP_GAP * i ) );
				int xpos = ScreenWidth - ( rect.right - rect.left ) - 10;

				SPR_Set( gHUD.GetSprite( rgAmmoHistory[i].iId ), r, g, b );
				SPR_DrawAdditive( 0, xpos, ypos, &rect );
			}
		}
	}

	return 1;
}

const char* WeaponsResource::GetAmmoName( int iAmmoId )
{
	for( int i = 0; i < MAX_WEAPONS; ++i )
	{
		if( rgWeapons[i].iId == 0 )
			continue;

		if( rgWeapons[i].iAmmoType == iAmmoId || rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			return rgWeapons[i].szName;
		}
	}

	return NULL;
}
