/***
*
*   Custom Opposing Force-style sniper rifle: AWP
*
***/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

// Simple animation enum for awp.mdl
enum awp_e
{
	AWP_IDLE = 0,
	AWP_SHOOT,
	AWP_RELOAD,
	AWP_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_awp, CAwp )

void CAwp::Spawn( void )
{
	pev->classname = MAKE_STRING( "weapon_awp" );
	Precache();
	m_iId = WEAPON_AWP;

	// World model
	SET_MODEL( ENT( pev ), "models/w_awp.mdl" );

	m_iDefaultAmmo = AWP_DEFAULT_GIVE;

	FallInit();
}

void CAwp::Precache( void )
{
	// View / world / player models
	PRECACHE_MODEL( "models/v_awp.mdl" );
	PRECACHE_MODEL( "models/w_awp.mdl" );
	PRECACHE_MODEL( "models/p_awp.mdl" );

	// Weapon Sounds
	PRECACHE_SOUND( "weapons/awp1.wav" );
}

int CAwp::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "awp";
	p->iMaxAmmo1 = AWP_MAX_CLIP * 3; // total carry ~3 clips
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AWP_MAX_CLIP;
	p->iSlot = 2;       // slot 3 (0-based)
	p->iPosition = 3;   // position 3 inside slot
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AWP;
	p->iWeight = AWP_WEIGHT;

	return 1;
}

int CAwp::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CAwp::Holster( int skiplocal /* = 0 */ )
{
	// Reset zoom/FOV when switching away from AWP
	if( m_pPlayer && m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
		m_fInZoom = 0;
	}

	m_fInReload = FALSE;

	// Call base weapon holster to clear view/world models
	CBasePlayerWeapon::Holster( skiplocal );
}

BOOL CAwp::Deploy( void )
{
	// Start weapon draw animation using the same hold type as the crossbow ('bow')
	if( DefaultDeploy( "models/v_awp.mdl", "models/p_awp.mdl", AWP_DRAW, "bow" ) )
	{
		// Ensure the draw animation plays for its full duration (90 frames @ 57fps = ~1.58s)
		// Using 1.7 seconds as a safe upper bound
		m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.7f;
		return TRUE;
	}
	return FALSE;
}

void CAwp::PrimaryAttack( void )
{
	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.75f );
		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	// viewmodel + player shoot animations
	SendWeaponAnim( AWP_SHOOT );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/awp1.wav", 1, ATTN_NORM );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	// Use tight auto-aim when not zoomed, very tight cone overall
	if( m_pPlayer->pev->fov != 0 )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );
	}
	else
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	// Apply recoil (view punch) – stronger when not scoped, slightly softer when scoped
	float flRecoil = ( m_pPlayer->pev->fov != 0 ) ? 4.0f : 6.0f;
	m_pPlayer->pev->punchangle.x -= flRecoil;
	m_pPlayer->pev->punchangle.y += RANDOM_FLOAT( -1.0f, 1.0f );

	// Leave a bullet decal on the surface we hit
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, m_pPlayer->edict(), &tr );
	if( tr.flFraction < 1.0f )
	{
		DecalGunshot( &tr, BULLET_PLAYER_357 );
	}

	// After firing, always reset zoom/FOV so the player must re-toggle scope manually
	if( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
		m_fInZoom = 0;
	}

	// Increase post-shot attack delay for slower fire rate (~0.5 shots per second)
	m_flNextPrimaryAttack = GetNextAttackDelay( 2.0f );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}

	// Idle animation delay after firing
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
}

void CAwp::SecondaryAttack( void )
{
	// Toggle zoom similar to crossbow/python
	if( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // reset to default
		m_fInZoom = 0;
	}
	else if( m_pPlayer->pev->fov != 20 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
	}

	pev->nextthink = UTIL_WeaponTimeBase() + 0.1f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
}

void CAwp::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == AWP_MAX_CLIP )
		return;

	if( m_pPlayer->pev->fov != 0 )
	{
		SecondaryAttack(); // leave zoom when reloading
	}

	// Explicitly send reload anim before starting the reload sequence
	SendWeaponAnim( AWP_RELOAD );
	if( DefaultReload( AWP_MAX_CLIP, AWP_RELOAD, 2.8f ) )
	{
		// reuse 357 reload sound
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/357_reload1.wav", RANDOM_FLOAT( 0.95f, 1.0f ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0xF ) );
	}
}

void CAwp::WeaponIdle( void )
{
	// Otomatik nişan alma
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );

	// Boş mermi sesini sıfırla
	ResetEmptySound();

	// Eğer idle zamanı gelmediyse çık
	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// Rastgele idle animasyonu seç
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );

	if( flRand <= 0.8f ) // %80 ihtimalle kısa idle
	{
		SendWeaponAnim( AWP_IDLE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 8, 12 );
	}
	else // %20 ihtimalle biraz daha uzun idle
	{
		SendWeaponAnim( AWP_IDLE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 4, 6 );
	}
}

// Simple ammo entity for AWP
class CAwpAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_awpclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_awpclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_AWP_GIVE, "awp", AWP_MAX_CLIP * 3 ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_awp, CAwpAmmo )
