/***
 * Black Mesa Relic Rush - relic entity
 ***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "relicrush_relic.h"
#include "relicrush_gamerules.h"
#include "relicrush_carrier.h"

LINK_ENTITY_TO_CLASS(item_relic, CRelicRushRelic);

void CRelicRushRelic::Precache()
{
	RelicRush_PrecacheModSounds();
}

void CRelicRushRelic::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	pev->effects = 0;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxGlowShell;
	pev->renderamt = (int)g_RelicBalance.glowRenderAmt;
	pev->rendercolor = Vector(g_RelicBalance.glowR, g_RelicBalance.glowG, g_RelicBalance.glowB);

	SET_MODEL(ENT(pev), "models/w_antidote.mdl");
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 24));

	SetTouch(&CRelicRushRelic::RelicTouch);
	SetThink(&CRelicRushRelic::RelicAmbientThink);
	pev->nextthink = gpGlobals->time + 0.1f;

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_warning, "Relic Rush: relique hors carte a %f,%f,%f\n",
			pev->origin.x, pev->origin.y, pev->origin.z);
	}
}

void CRelicRushRelic::RelicAmbientThink()
{
	if (m_bCarried)
		return;

	// EMIT_SOUND sur CHAN_STATIC (canal stoppable), pas UTIL_EmitAmbientSound (SND_STOP non fiable)
	EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, RELIC_AMBIENT_SOUND, 0.65f, ATTN_IDLE, 0, 100);
	pev->nextthink = gpGlobals->time + 2.0f;
}

void CRelicRushRelic::StopAmbient()
{
	// Double stop : le son est emis via EMIT_SOUND_DYN (canal) mais certaines builds
	// GoldSrc routent les sons longs via le pipe ambient -> on couvre les deux APIs.
	STOP_SOUND(ENT(pev), CHAN_STATIC, RELIC_AMBIENT_SOUND);
	UTIL_EmitAmbientSound(ENT(pev), pev->origin, RELIC_AMBIENT_SOUND, 0, 0, SND_STOP, 0);
}

void CRelicRushRelic::HideForPickup()
{
	m_bCarried = true;
	SetTouch(nullptr);
	SetThink(nullptr);
	StopAmbient();
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.05f;
}

void CRelicRushRelic::DeferredPickup()
{
	CBaseEntity* pActivator = m_hPickupPlayer;
	if (!pActivator || !pActivator->IsPlayer())
	{
		UTIL_Remove(this);
		return;
	}

	CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pActivator);

	if (g_pGameRules && g_pGameRules->IsMultiplayer())
		static_cast<CRelicRushMultiplay*>(g_pGameRules)->OnRelicPickedUp(pPlayer, this);
}

void CRelicRushRelic::RelicTouch(CBaseEntity* pOther)
{
	if (m_bCarried)
		return;

	if (!pOther || !pOther->IsPlayer())
		return;

	CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pOther);
	if (!pPlayer->IsAlive() || pPlayer->m_bHasRelic || pPlayer->m_bPendingRelicCarrier)
		return;

	SetTouch(nullptr);
	m_hPickupPlayer = pPlayer;
	SetThink(&CRelicRushRelic::DeferredPickup);
	pev->nextthink = gpGlobals->time + 0.05f;
}

CRelicRushRelic* CRelicRushRelic::CreateAt(const Vector& origin)
{
	CBaseEntity* pEntity = CBaseEntity::Create("item_relic", origin, g_vecZero);
	if (!pEntity)
	{
		ALERT(at_warning, "Relic Rush: echec spawn relique a %g %g %g\n", origin.x, origin.y, origin.z);
		return nullptr;
	}
	return static_cast<CRelicRushRelic*>(pEntity);
}
