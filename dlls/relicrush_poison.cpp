/***
 * Black Mesa Relic Rush - projectile gluant vert : entite + tir + aveuglement + decals
 *
 * Pattern projectile inspire de CRpgRocket (rpg.cpp) + CGrenade::Explode (ggrenade.cpp),
 * mais sans homing, sans degats, et avec effets verts (decals "spit", screen fade vert,
 * vapeur verte + sons liquide (pl_slosh / pl_wade), decals repandus en zone).
 ***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "decals.h"
#include "soundent.h"
#include "UserMessages.h"
#include "relicrush_carrier.h"
#include "relicrush_config.h"
#include "relicrush_poison.h"

LINK_ENTITY_TO_CLASS(relic_poison, CRelicPoisonProjectile);

// Couleur projectile / beam (fixe). Voile victime = rr_poison_veil_* dans relicrush_balance.cfg
static const Vector kRelicPoisonColor(0, 255, 0);

static void RelicRush_SendPoisonVeilClient(CBasePlayer* pPlayer, bool bStart)
{
	if (!pPlayer || !pPlayer->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgRelicPsnVl, NULL, pPlayer->edict());
	WRITE_BYTE(bStart ? 1 : 0);
	if (bStart)
	{
		const int fadeInTenths = (int)(g_RelicBalance.poisonVeilFadeIn * 10.0f + 0.5f);
		const int holdSec = (int)(g_RelicBalance.poisonVeilHold + 0.5f);
		const int fadeOutTenths = (int)(g_RelicBalance.poisonVeilFade * 10.0f + 0.5f);
		WRITE_BYTE(fadeInTenths < 1 ? 1 : (fadeInTenths > 255 ? 255 : fadeInTenths));
		WRITE_BYTE(holdSec < 0 ? 0 : (holdSec > 255 ? 255 : holdSec));
		WRITE_BYTE(fadeOutTenths < 1 ? 1 : (fadeOutTenths > 255 ? 255 : fadeOutTenths));
		WRITE_BYTE((int)g_RelicBalance.poisonVeilAlpha);
		WRITE_BYTE((int)g_RelicBalance.poisonVeilR);
		WRITE_BYTE((int)g_RelicBalance.poisonVeilG);
		WRITE_BYTE((int)g_RelicBalance.poisonVeilB);
		const int scalePct = (int)(g_RelicBalance.poisonVeilBlobScale * 100.0f + 0.5f);
		const int blobCount = (int)(g_RelicBalance.poisonVeilBlobCount + 0.5f);
		WRITE_BYTE(scalePct < 15 ? 15 : (scalePct > 150 ? 150 : scalePct));
		WRITE_BYTE(blobCount < 1 ? 1 : (blobCount > RELIC_POISON_VEIL_BLOBS_MAX ? RELIC_POISON_VEIL_BLOBS_MAX : blobCount));
	}
	else
	{
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
	}
	MESSAGE_END();
}

void RelicRush_ClearPoisonVeilClient(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;
	RelicRush_SendPoisonVeilClient(pPlayer, false);
	pPlayer->m_flRelicPoisonUntil = 0.0f;
	pPlayer->m_iRelicPoisonVeilFaded = 0;
	RelicRush_ClearPoisonVictimGlow(pPlayer);
}

bool RelicRush_IsPlayerPoisoned(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsAlive())
		return false;
	return pPlayer->m_flRelicPoisonUntil > gpGlobals->time;
}

static float RelicRush_GetPoisonVeilTotalDuration()
{
	return g_RelicBalance.poisonVeilFadeIn + g_RelicBalance.poisonVeilHold + g_RelicBalance.poisonVeilFade;
}

void RelicRush_ClearPoisonVictimGlow(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->m_bRelicPoisonVictimGlow)
		return;

	pPlayer->m_bRelicPoisonVictimGlow = false;

	// Le porteur a son propre rendu furtif ; TickCarrier le restaurera.
	if (RelicRush_IsCarrier(pPlayer))
		return;

	pPlayer->pev->rendermode = kRenderNormal;
	pPlayer->pev->renderfx = kRenderFxNone;
	pPlayer->pev->renderamt = 0;
	pPlayer->pev->rendercolor = Vector(255, 255, 255);
}

static void RelicRush_ApplyPoisonVictimGlow(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsAlive() || RelicRush_IsCarrier(pPlayer))
		return;

	pPlayer->m_bRelicPoisonVictimGlow = true;
	pPlayer->pev->rendermode = kRenderNormal;
	pPlayer->pev->renderfx = kRenderFxGlowShell;
	pPlayer->pev->renderamt = (int)g_RelicBalance.poisonVictimGlowAmt;
	pPlayer->pev->rendercolor = Vector(g_RelicBalance.poisonVeilR, g_RelicBalance.poisonVeilG, g_RelicBalance.poisonVeilB);
}

// Fumee xen verte (sprites additifs : TE_SPRITE, pas TE_SMOKE = carre noir).
static int g_iRelicPoisonSmokeSprite[4] = { 0, 0, 0, 0 };

static int RelicRush_PickPoisonDecal()
{
	return (RANDOM_LONG(0, 1) == 0) ? DECAL_SPIT1 : DECAL_SPIT2;
}

// Pose un decal vert si le trace touche une surface BSP.
static void RelicRush_TryPoisonDecalTrace(const Vector& vecStart, const Vector& vecEnd, edict_t* pSkip)
{
	TraceResult tr;
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, pSkip, &tr);
	if (tr.flFraction < 1.0f)
		UTIL_DecalTrace(&tr, RelicRush_PickPoisonDecal());
}

// Eclaboussures vertes reparties dans une sphere autour de l'impact (sol, murs, plafond).
static void RelicRush_SplashPoisonDecals(const Vector& vecOrigin, TraceResult* pCenterHit, edict_t* pSkip)
{
	const float flRadius = RELIC_POISON_SPLASH_DECAL_RADIUS;

	if (pCenterHit && pCenterHit->flFraction < 1.0f)
	{
		UTIL_DecalTrace(pCenterHit, RelicRush_PickPoisonDecal());

		const Vector wallN = pCenterHit->vecPlaneNormal;
		Vector right, up;
		if (fabs(wallN.z) > 0.7f)
		{
			right = Vector(1, 0, 0);
			up = Vector(0, 1, 0);
		}
		else
		{
			right = CrossProduct(wallN, Vector(0, 0, 1));
			if (right.Length() < 0.01f)
				right = Vector(1, 0, 0);
			else
				right = right.Normalize();
			up = CrossProduct(right, wallN).Normalize();
		}

		for (int c = 0; c < RANDOM_LONG(5, 8); c++)
		{
			const Vector vecOff = pCenterHit->vecEndPos
				+ right * RANDOM_FLOAT(-flRadius * 0.85f, flRadius * 0.85f)
				+ up * RANDOM_FLOAT(-flRadius * 0.85f, flRadius * 0.85f)
				+ wallN * 4.0f;
			RelicRush_TryPoisonDecalTrace(vecOff, vecOff - wallN * 32.0f, pSkip);
		}
	}

	const int nSplats = RANDOM_LONG(RELIC_POISON_SPLASH_DECAL_MIN, RELIC_POISON_SPLASH_DECAL_MAX);

	for (int i = 0; i < nSplats; i++)
	{
		// Point aleatoire dans une boule autour de l'explosion
		Vector dir(
			RANDOM_FLOAT(-1.0f, 1.0f),
			RANDOM_FLOAT(-1.0f, 1.0f),
			RANDOM_FLOAT(-1.0f, 1.0f));
		const float flLen = dir.Length();
		if (flLen < 0.01f)
			continue;
		dir = dir * (RANDOM_FLOAT(flRadius * 0.15f, flRadius) / flLen);

		const Vector vecProbe = vecOrigin + dir;
		RelicRush_TryPoisonDecalTrace(vecProbe, vecProbe - dir * 28.0f, pSkip);
	}

	// Anneaux supplementaires (sol + murs)
	for (int ring = 0; ring < 2; ring++)
	{
		const float flRing = flRadius * (0.4f + ring * 0.35f);
		const int nRingPts = 8 + ring * 2;
		for (int r = 0; r < nRingPts; r++)
		{
			const float ang = (6.2831853f * r) / nRingPts;
			const Vector vecProbe = vecOrigin + Vector(cosf(ang) * flRing, sinf(ang) * flRing, RANDOM_FLOAT(-12.0f, 18.0f));
			RelicRush_TryPoisonDecalTrace(vecProbe, vecProbe - Vector(0, 0, 48.0f), pSkip);
			RelicRush_TryPoisonDecalTrace(vecProbe, vecProbe + Vector(cosf(ang), sinf(ang), 0) * 32.0f, pSkip);
		}
	}

	// Renfort vers le sol sous l'impact (flaque au sol)
	for (int f = 0; f < 7; f++)
	{
		const Vector vecFloorStart = vecOrigin + Vector(
			RANDOM_FLOAT(-flRadius * 0.95f, flRadius * 0.95f),
			RANDOM_FLOAT(-flRadius * 0.95f, flRadius * 0.95f),
			12.0f);
		RelicRush_TryPoisonDecalTrace(vecFloorStart, vecFloorStart - Vector(0, 0, 96.0f), pSkip);
	}
}

static int RelicRush_PickPoisonSmokeSprite()
{
	int pool[4];
	int n = 0;
	for (int j = 0; j < 4; j++)
	{
		if (g_iRelicPoisonSmokeSprite[j] > 0)
			pool[n++] = g_iRelicPoisonSmokeSprite[j];
	}
	if (n > 0)
		return pool[RANDOM_LONG(0, n - 1)];
	return 0;
}

// Nuage vert xen : TE_SPRITE (additif). TE_SMOKE + xsmoke = carre noir (alphablend requis).
void RelicRush_PlayPoisonExplosionAt(const Vector& vecOrigin, edict_t* pOwner)
{
	for (int i = 0; i < 5; i++)
	{
		const int iSprite = RelicRush_PickPoisonSmokeSprite();
		if (iSprite <= 0)
			continue;

		const Vector vecPuff = vecOrigin + Vector(
			RANDOM_FLOAT(-20.0f, 20.0f),
			RANDOM_FLOAT(-20.0f, 20.0f),
			RANDOM_FLOAT(6.0f, 28.0f));

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecPuff);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(vecPuff.x);
		WRITE_COORD(vecPuff.y);
		WRITE_COORD(vecPuff.z);
		WRITE_SHORT(iSprite);
		WRITE_BYTE(RANDOM_LONG(16, 26));  // echelle (*0.1)
		WRITE_BYTE(RANDOM_LONG(150, 210)); // luminosite
		MESSAGE_END();
	}

	CSoundEnt::InsertSound(bits_SOUND_PLAYER, vecOrigin, 384, 2.5f);

	static const char* kSlosh[] =
	{
		RELIC_POISON_IMPACT_SOUND_1,
		RELIC_POISON_IMPACT_SOUND_2,
		RELIC_POISON_IMPACT_SOUND_3,
		RELIC_POISON_IMPACT_SOUND_4,
	};
	const char* pszSlosh = kSlosh[RANDOM_LONG(0, ARRAYSIZE(kSlosh) - 1)];

	edict_t* pEmit = (!FNullEnt(pOwner)) ? pOwner : INDEXENT(0);
	UTIL_EmitAmbientSound(pEmit, vecOrigin, pszSlosh, 1.0f, ATTN_NORM, 0, RANDOM_LONG(95, 108));
	UTIL_EmitAmbientSound(pEmit, vecOrigin, RELIC_POISON_IMPACT_SOUND_DEEP, 0.85f, ATTN_NORM, 0, RANDOM_LONG(88, 102));
}

void CRelicPoisonProjectile::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.0f;
	pev->classname = MAKE_STRING("relic_poison");

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector(-2, -2, -2), Vector(2, 2, 2));
	UTIL_SetOrigin(pev, pev->origin);

	// Projectile vert lumineux (glow fort, mesh discret)
	pev->rendermode = kRenderTransAdd;
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor = kRelicPoisonColor;
	pev->renderamt = 220;
	pev->effects |= EF_BRIGHTLIGHT;
	pev->scale = 0.45f;

	StartGreenTrail();

	SetTouch(&CRelicPoisonProjectile::PoisonTouch);
	SetThink(&CRelicPoisonProjectile::PoisonFlyThink);
	pev->nextthink = gpGlobals->time + 0.05f;

	// Auto-detruit apres 4s si rien touche
	pev->dmgtime = gpGlobals->time + 4.0f;
}

void CRelicPoisonProjectile::StartGreenTrail()
{
	// g_sModelIndexLaser : precache map (weapons.cpp), jamais PRECACHE_MODEL en jeu.
	if (g_sModelIndexLaser <= 0)
		return;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex());
	WRITE_SHORT(g_sModelIndexLaser);
	{
		const int iTrailLife = (int)(g_RelicBalance.poisonTrailLife + 0.5f);
		WRITE_BYTE(iTrailLife < 1 ? 1 : (iTrailLife > 255 ? 255 : iTrailLife)); // *0.1 s (6 = 0.6 s)
	}
	WRITE_BYTE(4);						// largeur
	WRITE_BYTE((int)kRelicPoisonColor.x);	// r, g, b
	WRITE_BYTE((int)kRelicPoisonColor.y);
	WRITE_BYTE((int)kRelicPoisonColor.z);
	WRITE_BYTE(140);					// luminosite (trainee semi-transparente)
	MESSAGE_END();
}

void CRelicPoisonProjectile::PoisonFlyThink()
{
	if (gpGlobals->time >= pev->dmgtime)
	{
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 32),
			ignore_monsters, ENT(pev), &tr);
		Explode(&tr);
		return;
	}
	pev->nextthink = gpGlobals->time + 0.05f;
}

void CRelicPoisonProjectile::Explode(TraceResult* pTrace)
{
	pev->model = iStringNull;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->effects &= ~EF_LIGHT;

	// Recule legerement de la surface pour eviter le z-fighting / decal hors-mur.
	if (pTrace && pTrace->flFraction != 1.0f)
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 2.0f);

	RelicRush_PlayPoisonExplosionAt(pev->origin, pev->owner);

	// Lumiere verte courte
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_BYTE(24);
	WRITE_BYTE((int)kRelicPoisonColor.x);
	WRITE_BYTE((int)kRelicPoisonColor.y);
	WRITE_BYTE((int)kRelicPoisonColor.z);
	WRITE_BYTE(6);
	WRITE_BYTE(90);
	MESSAGE_END();

	UTIL_Bubbles(
		pev->origin - Vector(48, 48, 24),
		pev->origin + Vector(48, 48, 48),
		RANDOM_LONG(8, 14));
	RelicRush_SplashPoisonDecals(pev->origin, pTrace, ENT(pev));

	// Aveuglement AoE : tous les joueurs dans le rayon (proprietaire epargne).
	CBaseEntity* pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityInSphere(pEnt, pev->origin, g_RelicBalance.poisonAoeRadius)) != nullptr)
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

void CRelicPoisonProjectile::PoisonTouch(CBaseEntity* pOther)
{
	// Si on touche le proprietaire au tout debut, ignorer.
	if (pOther && pOther->edict() == pev->owner)
	{
		pev->nextthink = gpGlobals->time + 0.05f;
		return;
	}

	TraceResult tr;
	Vector vecDir = pev->velocity;
	if (vecDir.Length() < 1.0f)
	{
		UTIL_MakeVectors(pev->angles);
		vecDir = gpGlobals->v_forward;
	}
	else
	{
		vecDir = vecDir.Normalize();
	}
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

CRelicPoisonProjectile* CRelicPoisonProjectile::Shoot(CBasePlayer* pOwner, Vector vecOrigin, Vector vecAimDir)
{
	CRelicPoisonProjectile* pPoison = GetClassPtr((CRelicPoisonProjectile*)nullptr);
	UTIL_SetOrigin(pPoison->pev, vecOrigin);
	pPoison->pev->angles = UTIL_VecToAngles(vecAimDir);
	pPoison->Spawn();
	pPoison->pev->velocity = vecAimDir * RELIC_POISON_SPEED;
	pPoison->pev->owner = pOwner->edict();
	return pPoison;
}

// -------------------- Tir cote porteur --------------------

void RelicRush_PrecachePoisonAssets()
{
	PRECACHE_MODEL("models/grenade.mdl");
	g_iRelicPoisonSmokeSprite[0] = PRECACHE_MODEL("sprites/xsmoke1.spr");
	g_iRelicPoisonSmokeSprite[1] = PRECACHE_MODEL("sprites/xsmoke3.spr");
	g_iRelicPoisonSmokeSprite[2] = PRECACHE_MODEL("sprites/xsmoke4.spr");
	g_iRelicPoisonSmokeSprite[3] = PRECACHE_MODEL("sprites/xssmke1.spr");
	PRECACHE_SOUND(RELIC_POISON_FIRE_SOUND);
	PRECACHE_SOUND(RELIC_POISON_IMPACT_SOUND_1);
	PRECACHE_SOUND(RELIC_POISON_IMPACT_SOUND_2);
	PRECACHE_SOUND(RELIC_POISON_IMPACT_SOUND_3);
	PRECACHE_SOUND(RELIC_POISON_IMPACT_SOUND_4);
	PRECACHE_SOUND(RELIC_POISON_IMPACT_SOUND_DEEP);
}

void RelicRush_FirePoison(CBasePlayer* pCarrier)
{
	if (!RelicRush_IsCarrier(pCarrier) || !pCarrier->IsAlive())
		return;
	if (gpGlobals->time < pCarrier->m_flNextRelicPoison)
		return;

	UTIL_MakeVectors(pCarrier->pev->v_angle);
	Vector vecAim = gpGlobals->v_forward;
	const float flLen = vecAim.Length();
	if (flLen < 0.01f)
		return;
	vecAim = vecAim * (1.0f / flLen);

	// Spawn devant les yeux du porteur.
	const Vector vecMuzzle = pCarrier->GetGunPosition() + vecAim * 14.0f;

	CRelicPoisonProjectile::Shoot(pCarrier, vecMuzzle, vecAim);

	// Son du tir (entendu globalement, ATTN_NORM pour proximite naturelle).
	EMIT_SOUND_DYN(pCarrier->edict(), CHAN_WEAPON, RELIC_POISON_FIRE_SOUND, 0.9f, ATTN_NORM, 0, 110);

	pCarrier->m_flNextRelicPoison = gpGlobals->time + RELIC_POISON_COOLDOWN;
	pCarrier->m_iRelicPoisonCooldownPct = -1; // force prochaine emission

	// Petit shake du porteur (effet de recul).
	UTIL_ScreenShake(pCarrier->pev->origin, 2.0f, 80.0f, 0.2f, 80.0f);
}

// -------------------- Aveuglement de la victime --------------------

void RelicRush_BlindPlayer(CBasePlayer* pVictim)
{
	if (!pVictim || !pVictim->IsAlive() || !pVictim->IsNetClient())
		return;

	pVictim->m_iRelicPoisonVeilFaded = 0; // permet un nouveau fadeout si re-touch pendant l'effet
	pVictim->m_flRelicPoisonUntil = gpGlobals->time + RelicRush_GetPoisonVeilTotalDuration();

	RelicRush_SendPoisonVeilClient(pVictim, true);
	RelicRush_ApplyPoisonVictimGlow(pVictim);
}

void RelicRush_TickPoisonVictimEffects(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	if (RelicRush_IsPlayerPoisoned(pPlayer))
	{
		if (pPlayer->IsNetClient())
		{
			const float flFadeStart = pPlayer->m_flRelicPoisonUntil - g_RelicBalance.poisonVeilFade;
			if (gpGlobals->time >= flFadeStart && pPlayer->m_iRelicPoisonVeilFaded == 0)
				pPlayer->m_iRelicPoisonVeilFaded = 1; // fadeout gere cote client (RelicPsnVl)
		}

		RelicRush_ApplyPoisonVictimGlow(pPlayer);
		return;
	}

	if (pPlayer->m_flRelicPoisonUntil > 0.0f || pPlayer->m_bRelicPoisonVictimGlow)
	{
		pPlayer->m_flRelicPoisonUntil = 0.0f;
		pPlayer->m_iRelicPoisonVeilFaded = 0;
		RelicRush_ClearPoisonVictimGlow(pPlayer);
	}
}

void RelicRush_TestPoisonOnSelf(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsAlive() || !pPlayer->IsNetClient())
		return;
	if (RelicRush_IsCarrier(pPlayer))
	{
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE,
			"rr_poison : reserve aux joueurs sans la relique (test voile poison).\n");
		return;
	}

	RelicRush_PlayPoisonExplosionAt(pPlayer->pev->origin, pPlayer->edict());
	RelicRush_BlindPlayer(pPlayer);
	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Test poison (debug)");
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

void RelicRush_SendPoisonCooldown(CBasePlayer* pPlayer, int iPct)
{
	if (gmsgRelicPoisn <= 0 || !pPlayer || !pPlayer->IsNetClient())
		return;

	if (iPct < 0) iPct = 0;
	if (iPct > 100) iPct = 100;

	MESSAGE_BEGIN(MSG_ONE, gmsgRelicPoisn, NULL, pPlayer->pev);
	WRITE_BYTE((byte)iPct);
	MESSAGE_END();

	pPlayer->m_iRelicPoisonCooldownPct = iPct;
}

void RelicRush_TickCarrierPoisonSync(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	int iPct;
	const float flRemain = pPlayer->m_flNextRelicPoison - gpGlobals->time;
	if (flRemain <= 0.0f)
		iPct = 100; // pret
	else
		iPct = (int)(100.0f * (1.0f - (flRemain / RELIC_POISON_COOLDOWN)));

	// Seulement si changement notable (eviter le spam reseau).
	const int iPrev = pPlayer->m_iRelicPoisonCooldownPct;
	if (iPrev < 0 || abs(iPct - iPrev) >= 4 || (iPct == 100 && iPrev != 100))
		RelicRush_SendPoisonCooldown(pPlayer, iPct);
}

void RelicRush_ResetCarrierPoisonState(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;
	pPlayer->m_flNextRelicPoison = 0.0f;
	pPlayer->m_flNextRelicDecal = 0.0f;
	if (pPlayer->m_iRelicPoisonCooldownPct != 0 && pPlayer->IsNetClient())
		RelicRush_SendPoisonCooldown(pPlayer, 0); // efface la barre cote client
	pPlayer->m_iRelicPoisonCooldownPct = -1;
}
