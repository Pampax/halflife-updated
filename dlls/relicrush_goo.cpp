/***
 * Black Mesa Relic Rush - projectile gluant vert : entite + tir + aveuglement + decals
 *
 * Pattern projectile inspire de CRpgRocket (rpg.cpp) + CGrenade::Explode (ggrenade.cpp),
 * mais sans homing, sans degats, et avec effets verts (decals "spit", screen fade vert,
 * son d'impact "fleshy").
 ***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "decals.h"
#include "shake.h"
#include "soundent.h"
#include "UserMessages.h"
#include "relicrush_carrier.h"
#include "relicrush_goo.h"

LINK_ENTITY_TO_CLASS(relic_goo, CRelicGooProjectile);

// Couleur verte commune (decal/fade/glow)
static const Vector kRelicGooColor(32, 220, 72);

void CRelicGooProjectile::Precache()
{
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_SOUND(RELIC_GOO_FIRE_SOUND);
	PRECACHE_SOUND(RELIC_GOO_IMPACT_SOUND_1);
	PRECACHE_SOUND(RELIC_GOO_IMPACT_SOUND_2);
	PRECACHE_SOUND(RELIC_GOO_IMPACT_SOUND_3);
}

void CRelicGooProjectile::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.0f;
	pev->classname = MAKE_STRING("relic_goo");

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector(-2, -2, -2), Vector(2, 2, 2));
	UTIL_SetOrigin(pev, pev->origin);

	// Boule verte glow
	pev->rendermode = kRenderTransAdd;
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor = kRelicGooColor;
	pev->renderamt = 255;
	pev->effects |= EF_LIGHT;
	pev->scale = 1.4f;

	SetTouch(&CRelicGooProjectile::GooTouch);
	SetThink(&CRelicGooProjectile::GooFlyThink);
	pev->nextthink = gpGlobals->time + 0.1f;

	// Auto-detruit apres 4s si rien touche
	pev->dmgtime = gpGlobals->time + 4.0f;
}

void CRelicGooProjectile::GooFlyThink()
{
	if (gpGlobals->time >= pev->dmgtime)
	{
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 32),
			ignore_monsters, ENT(pev), &tr);
		Explode(&tr);
		return;
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRelicGooProjectile::Explode(TraceResult* pTrace)
{
	pev->model = iStringNull;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->effects &= ~EF_LIGHT;

	// Recule legerement de la surface pour eviter le z-fighting / decal hors-mur.
	if (pTrace && pTrace->flFraction != 1.0f)
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 2.0f);

	// Splash visuel : lumiere dynamique verte (TE_DLIGHT) + sprite explosion energy.
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_BYTE(20);				   // radius (*10)
	WRITE_BYTE((int)kRelicGooColor.x);
	WRITE_BYTE((int)kRelicGooColor.y);
	WRITE_BYTE((int)kRelicGooColor.z);
	WRITE_BYTE(8);				   // life (*10 -> 0.8s)
	WRITE_BYTE(80);				   // decay
	MESSAGE_END();

	// Petit nuage de bulles vert vif (TE_BUBBLES) : effet de splatter sans dependance sprite.
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPARKS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	MESSAGE_END();

	// Son d'impact aleatoire (3 variantes liquides).
	const char* pszImpact = RELIC_GOO_IMPACT_SOUND_1;
	switch (RANDOM_LONG(0, 2))
	{
	case 0: pszImpact = RELIC_GOO_IMPACT_SOUND_1; break;
	case 1: pszImpact = RELIC_GOO_IMPACT_SOUND_2; break;
	case 2: pszImpact = RELIC_GOO_IMPACT_SOUND_3; break;
	}
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pszImpact, 0.95f, ATTN_NORM, 0, RANDOM_LONG(85, 100));
	// Surcouche basse plus lente pour effet "blob".
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, pszImpact, 0.6f, ATTN_NORM, 0, 60);

	// Decal vert (crachat alien) sur la surface impactee, taille moyenne (2 tiles).
	if (pTrace && pTrace->flFraction < 1.0f)
	{
		const int iDecal = (RANDOM_LONG(0, 1) == 0) ? DECAL_SPIT1 : DECAL_SPIT2;
		UTIL_DecalTrace(pTrace, iDecal);

		// Quelques eclaboussures secondaires autour pour grossir la tache.
		const Vector wallN = pTrace->vecPlaneNormal;
		Vector right(1, 0, 0), up(0, 0, 1);
		if (fabs(wallN.z) > 0.7f)
		{
			right = Vector(1, 0, 0);
			up = Vector(0, 1, 0);
		}
		else
		{
			right = CrossProduct(wallN, Vector(0, 0, 1)).Normalize();
			up = CrossProduct(right, wallN).Normalize();
		}
		const int nExtra = RANDOM_LONG(1, 3);
		for (int i = 0; i < nExtra; i++)
		{
			const float dx = RANDOM_FLOAT(-32.0f, 32.0f);
			const float dy = RANDOM_FLOAT(-32.0f, 32.0f);
			const Vector vecOff = pTrace->vecEndPos + right * dx + up * dy + wallN * 4.0f;
			TraceResult tr2;
			UTIL_TraceLine(vecOff, vecOff - wallN * 16.0f, ignore_monsters, ENT(pev), &tr2);
			if (tr2.flFraction < 1.0f)
				UTIL_DecalTrace(&tr2, (RANDOM_LONG(0, 1) == 0) ? DECAL_SPIT1 : DECAL_SPIT2);
		}
	}

	// Aveuglement AoE : tous les joueurs dans le rayon (proprietaire epargne).
	CBaseEntity* pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityInSphere(pEnt, pev->origin, RELIC_GOO_AOE_RADIUS)) != nullptr)
	{
		if (!pEnt->IsPlayer() || !pEnt->IsAlive())
			continue;
		if (pEnt->edict() == pev->owner)
			continue; // pas le tireur

		// Trace pour confirmer la ligne de vue (eviter aveuglement a travers un mur).
		TraceResult trLOS;
		UTIL_TraceLine(pev->origin, pEnt->Center(), ignore_monsters, ENT(pev), &trLOS);
		if (trLOS.flFraction < 0.97f && trLOS.pHit != pEnt->edict())
			continue;

		RelicRush_BlindPlayer((CBasePlayer*)pEnt);
	}

	UTIL_Remove(this);
}

void CRelicGooProjectile::GooTouch(CBaseEntity* pOther)
{
	// Si on touche le proprietaire au tout debut, ignorer.
	if (pOther && pOther->edict() == pev->owner)
	{
		pev->nextthink = gpGlobals->time + 0.05f;
		return;
	}

	TraceResult tr;
	const Vector vecDir = pev->velocity.Normalize();
	const Vector vecStart = pev->origin - vecDir * 8.0f;
	UTIL_TraceLine(vecStart, vecStart + vecDir * 32.0f, ignore_monsters, ENT(pev), &tr);

	// Si le trace ne touche rien (impact joueur sans collision world), on cible directement.
	if (tr.flFraction >= 1.0f && pOther)
	{
		tr.vecEndPos = pev->origin;
		tr.flFraction = 0.99f;
		tr.pHit = pOther->edict();
		tr.vecPlaneNormal = -vecDir;
	}

	// Aveuglement direct du joueur touche (en plus de l'AoE).
	if (pOther && pOther->IsPlayer() && pOther->IsAlive() && pOther->edict() != pev->owner)
		RelicRush_BlindPlayer((CBasePlayer*)pOther);

	Explode(&tr);
}

CRelicGooProjectile* CRelicGooProjectile::Shoot(CBasePlayer* pOwner, Vector vecOrigin, Vector vecAimDir)
{
	CRelicGooProjectile* pGoo = GetClassPtr((CRelicGooProjectile*)nullptr);
	UTIL_SetOrigin(pGoo->pev, vecOrigin);
	pGoo->pev->angles = UTIL_VecToAngles(vecAimDir);
	pGoo->Spawn();
	pGoo->pev->velocity = vecAimDir * RELIC_GOO_SPEED;
	pGoo->pev->owner = pOwner->edict();
	return pGoo;
}

// -------------------- Tir cote porteur --------------------

void RelicRush_PrecacheGooAssets()
{
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_SOUND(RELIC_GOO_FIRE_SOUND);
	PRECACHE_SOUND(RELIC_GOO_IMPACT_SOUND_1);
	PRECACHE_SOUND(RELIC_GOO_IMPACT_SOUND_2);
	PRECACHE_SOUND(RELIC_GOO_IMPACT_SOUND_3);
}

void RelicRush_FireGoo(CBasePlayer* pCarrier)
{
	if (!RelicRush_IsCarrier(pCarrier) || !pCarrier->IsAlive())
		return;
	if (gpGlobals->time < pCarrier->m_flNextRelicGoo)
		return;

	UTIL_MakeVectors(pCarrier->pev->v_angle);
	Vector vecAim = gpGlobals->v_forward;
	const float flLen = vecAim.Length();
	if (flLen < 0.01f)
		return;
	vecAim = vecAim * (1.0f / flLen);

	// Spawn devant les yeux du porteur.
	const Vector vecMuzzle = pCarrier->GetGunPosition() + vecAim * 14.0f;

	CRelicGooProjectile::Shoot(pCarrier, vecMuzzle, vecAim);

	// Son du tir (entendu globalement, ATTN_NORM pour proximite naturelle).
	EMIT_SOUND_DYN(pCarrier->edict(), CHAN_WEAPON, RELIC_GOO_FIRE_SOUND, 0.9f, ATTN_NORM, 0, 110);

	pCarrier->m_flNextRelicGoo = gpGlobals->time + RELIC_GOO_COOLDOWN;
	pCarrier->m_iRelicGooCooldownPct = -1; // force prochaine emission

	// Petit shake du porteur (effet de recul).
	UTIL_ScreenShake(pCarrier->pev->origin, 2.0f, 80.0f, 0.2f, 80.0f);
}

// -------------------- Aveuglement de la victime --------------------

void RelicRush_BlindPlayer(CBasePlayer* pVictim)
{
	if (!pVictim || !pVictim->IsAlive() || !pVictim->IsNetClient())
		return;

	pVictim->m_flRelicGooBlindUntil = gpGlobals->time + RELIC_GOO_BLIND_HOLD + RELIC_GOO_BLIND_FADE;

	// Etape 1 : montee rapide + maintien (3s). FFADE_IN draine vers la couleur,
	// FFADE_STAYOUT laisse l'effet jusqu'au prochain ScreenFade.
	UTIL_ScreenFade(pVictim, kRelicGooColor,
		0.15f, RELIC_GOO_BLIND_HOLD,
		RELIC_GOO_BLIND_ALPHA, FFADE_IN | FFADE_STAYOUT);

	// Etape 2 : fadeout 1s programme par think. On utilise pev->dmgtime du joueur
	// pas dispo -> on stocke dans m_flRelicGooBlindUntil et on declenche le fadeout
	// dans le tick joueur (RelicRush_TickPlayerBlindFade) appele depuis PlayerThink.
}

void RelicRush_TickPlayerBlindFade(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsNetClient())
		return;
	if (pPlayer->m_flRelicGooBlindUntil <= 0.0f)
		return;

	const float flFadeStart = pPlayer->m_flRelicGooBlindUntil - RELIC_GOO_BLIND_FADE;
	if (gpGlobals->time >= flFadeStart && pPlayer->m_iRelicGooBlindFaded == 0)
	{
		// Declenche un FFADE_OUT (dissipe l'overlay pose par FFADE_STAYOUT).
		UTIL_ScreenFade(pPlayer, kRelicGooColor,
			RELIC_GOO_BLIND_FADE, 0.0f,
			RELIC_GOO_BLIND_ALPHA, FFADE_OUT);
		pPlayer->m_iRelicGooBlindFaded = 1;
	}

	if (gpGlobals->time >= pPlayer->m_flRelicGooBlindUntil)
	{
		pPlayer->m_flRelicGooBlindUntil = 0.0f;
		pPlayer->m_iRelicGooBlindFaded = 0;
	}
}

// -------------------- Trail decals (pister le porteur) --------------------

static void RelicRush_PlaceTileGroup(const Vector& vecOrigin, const Vector& wallN, int nTiles)
{
	// Pose une tache : un decal central + jusqu'a (nTiles-1) decals secondaires autour.
	TraceResult tr;
	UTIL_TraceLine(vecOrigin, vecOrigin - wallN * 4.0f, ignore_monsters, NULL, &tr);
	if (tr.flFraction < 1.0f)
		UTIL_DecalTrace(&tr, (RANDOM_LONG(0, 1) == 0) ? DECAL_SPIT1 : DECAL_SPIT2);

	if (nTiles <= 1)
		return;

	// Base orthonormee pour repartir les tiles dans le plan de la surface.
	Vector right, up;
	if (fabs(wallN.z) > 0.7f)
	{
		right = Vector(1, 0, 0);
		up = Vector(0, 1, 0);
	}
	else
	{
		right = CrossProduct(wallN, Vector(0, 0, 1)).Normalize();
		up = CrossProduct(right, wallN).Normalize();
	}

	for (int i = 0; i < nTiles - 1; i++)
	{
		const float ang = RANDOM_FLOAT(0.0f, 6.2832f);
		const float dist = RANDOM_FLOAT(RELIC_DECAL_TILE_SPACING * 0.5f, RELIC_DECAL_TILE_SPACING * 1.5f);
		const Vector vecOff = vecOrigin + right * (cosf(ang) * dist) + up * (sinf(ang) * dist) + wallN * 2.0f;
		UTIL_TraceLine(vecOff, vecOff - wallN * 16.0f, ignore_monsters, NULL, &tr);
		if (tr.flFraction < 1.0f)
			UTIL_DecalTrace(&tr, (RANDOM_LONG(0, 1) == 0) ? DECAL_SPIT1 : DECAL_SPIT2);
	}
}

static bool RelicRush_TryStampSurface(CBasePlayer* pCarrier)
{
	// Cherche une surface proche : 6 directions cardinales (sol, plafond, 4 murs).
	static const Vector kProbes[6] = {
		Vector(0, 0, -1),  // sol
		Vector(0, 0, 1),   // plafond
		Vector(1, 0, 0),
		Vector(-1, 0, 0),
		Vector(0, 1, 0),
		Vector(0, -1, 0),
	};

	const Vector vecCenter = pCarrier->Center();

	// Brouille l'ordre pour avoir des taches non systematiquement sous les pieds.
	int order[6] = {0, 1, 2, 3, 4, 5};
	for (int i = 5; i > 0; i--)
	{
		const int j = RANDOM_LONG(0, i);
		const int tmp = order[i];
		order[i] = order[j];
		order[j] = tmp;
	}

	TraceResult tr;
	for (int k = 0; k < 6; k++)
	{
		const Vector& dir = kProbes[order[k]];
		UTIL_TraceLine(vecCenter, vecCenter + dir * RELIC_DECAL_RANGE,
			ignore_monsters, pCarrier->edict(), &tr);
		if (tr.flFraction < 1.0f)
		{
			const int nTiles = RANDOM_LONG(RELIC_DECAL_MIN_TILES, RELIC_DECAL_MAX_TILES);
			RelicRush_PlaceTileGroup(tr.vecEndPos, tr.vecPlaneNormal, nTiles);
			return true;
		}
	}
	return false;
}

void RelicRush_TickCarrierTrailDecal(CBasePlayer* pCarrier)
{
	if (!RelicRush_IsCarrier(pCarrier) || !pCarrier->IsAlive())
		return;

	if (pCarrier->m_flNextRelicDecal <= 0.0f)
		pCarrier->m_flNextRelicDecal = gpGlobals->time + RANDOM_FLOAT(RELIC_DECAL_MIN, RELIC_DECAL_MAX);

	if (gpGlobals->time < pCarrier->m_flNextRelicDecal)
		return;

	RelicRush_TryStampSurface(pCarrier);
	pCarrier->m_flNextRelicDecal = gpGlobals->time + RANDOM_FLOAT(RELIC_DECAL_MIN, RELIC_DECAL_MAX);
}

// -------------------- Sync cooldown -> client HUD --------------------

void RelicRush_SendGooCooldown(CBasePlayer* pPlayer, int iPct)
{
	if (gmsgRelicGoo <= 0 || !pPlayer || !pPlayer->IsNetClient())
		return;

	if (iPct < 0) iPct = 0;
	if (iPct > 100) iPct = 100;

	MESSAGE_BEGIN(MSG_ONE, gmsgRelicGoo, NULL, pPlayer->pev);
	WRITE_BYTE((byte)iPct);
	MESSAGE_END();

	pPlayer->m_iRelicGooCooldownPct = iPct;
}

void RelicRush_TickCarrierGooSync(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	int iPct;
	const float flRemain = pPlayer->m_flNextRelicGoo - gpGlobals->time;
	if (flRemain <= 0.0f)
		iPct = 100; // pret
	else
		iPct = (int)(100.0f * (1.0f - (flRemain / RELIC_GOO_COOLDOWN)));

	// Seulement si changement notable (eviter le spam reseau).
	const int iPrev = pPlayer->m_iRelicGooCooldownPct;
	if (iPrev < 0 || abs(iPct - iPrev) >= 4 || (iPct == 100 && iPrev != 100))
		RelicRush_SendGooCooldown(pPlayer, iPct);
}

void RelicRush_ResetCarrierGooState(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;
	pPlayer->m_flNextRelicGoo = 0.0f;
	pPlayer->m_flNextRelicDecal = 0.0f;
	if (pPlayer->m_iRelicGooCooldownPct != 0 && pPlayer->IsNetClient())
		RelicRush_SendGooCooldown(pPlayer, 0); // efface la barre cote client
	pPlayer->m_iRelicGooCooldownPct = -1;
}
