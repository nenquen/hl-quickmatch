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

class CCrossbow : public CBasePlayerWeapon
{
public:
	void Spawn( void )
	{
		// Weapon disabled: immediately remove the entity so it cannot be used.
		UTIL_Remove( this );
	}

	void Precache( void ) { }
	int GetItemInfo( ItemInfo * ) { return 0; }
	int AddToPlayer( CBasePlayer * ) { return 0; }
	BOOL Deploy( void ) { return FALSE; }
	void Holster( int ) { }
	void PrimaryAttack( void ) { }
	void SecondaryAttack( void ) { }
	void Reload( void ) { }
	void WeaponIdle( void ) { }
};

LINK_ENTITY_TO_CLASS( weapon_crossbow, CCrossbow )

// Crossbow ammo entity is also disabled completely.
class CCrossbowAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		UTIL_Remove( this );
	}

	void Precache( void ) { }
	BOOL AddAmmo( CBaseEntity * ) { return FALSE; }
};

LINK_ENTITY_TO_CLASS( ammo_crossbow, CCrossbowAmmo )
#endif
