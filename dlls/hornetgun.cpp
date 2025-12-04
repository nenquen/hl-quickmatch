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
#if !OEM_BUILD && !HLDEMO_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"

enum hgun_e
{
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};

LINK_ENTITY_TO_CLASS( weapon_hornetgun, CHgun )

void CHgun::Spawn()
{
	// Weapon disabled: immediately remove the entity so it cannot be used.
	UTIL_Remove( this );
}

void CHgun::Precache( void )
{
	// No-op: hornet gun is fully disabled as a player weapon.
}

int CHgun::GetItemInfo( ItemInfo *p )
{
	// Hornet gun is disabled; do not register any usable item info.
	return 0;
}

int CHgun::AddToPlayer( CBasePlayer *pPlayer )
{
	// Never actually added to the player's inventory.
	return 0;
}

BOOL CHgun::Deploy( void )
{
	// Never deploy; weapon is removed on spawn.
	return FALSE;
}

BOOL CHgun::IsUseable( void )
{
	// Not useable by players.
	return FALSE;
}

void CHgun::Holster( int /*skiplocal*/ )
{
	// No-op; hornet gun should never be holstered in normal play.
}

void CHgun::PrimaryAttack( void )
{
	// No-op; weapon is disabled.
}

void CHgun::SecondaryAttack( void )
{
	// No-op; weapon is disabled.
}

void CHgun::Reload( void )
{
	// No-op; weapon is disabled.
}

void CHgun::WeaponIdle( void )
{
	// No-op; weapon is disabled.
}
#endif
