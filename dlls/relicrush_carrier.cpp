/***
 * Black Mesa Relic Rush - porteur de relique (helpers)
 ***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "relicrush_carrier.h"
#include "cdll_dll.h"
#include "UserMessages.h"

// Sons vanilla HL verifies (pas bullchicken/controller : absents sur certaines installs)
static const char* g_RelicCarrierScreams[] =
{
	"garg/gar_alert1.wav",
	"garg/gar_alert2.wav",
	"garg/gar_alert3.wav",
	"garg/gar_idle1.wav",
	"garg/gar_idle2.wav",
	"houndeye/he_alert1.wav",
	"houndeye/he_alert2.wav",
	"houndeye/he_attack1.wav",
	"tentacle/te_alert1.wav",
	"tentacle/te_alert2.wav",
	"tentacle/te_roar1.wav",
	"agrunt/ag_alert1.wav",
	"agrunt/ag_alert2.wav",
	"agrunt/ag_alert3.wav",
	"weapons/ric1.wav",
	"weapons/ric2.wav",
	"weapons/ric3.wav",
};

static void RelicRush_SetUserinfoModel(CBasePlayer* pPlayer, const char* pszModel);

void RelicRush_ResetCarrierVisuals(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	pPlayer->pev->rendermode = kRenderNormal;
	pPlayer->pev->renderfx = kRenderFxNone;
	pPlayer->pev->renderamt = 0;
	pPlayer->pev->rendercolor = g_vecZero;
}

static void RelicRush_EnsureNormalUserinfoModel(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	const char* pszModel = g_engfuncs.pfnInfoKeyValue(
		g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model");

	if (pPlayer->m_szRelicSavedUserModel[0])
	{
		RelicRush_SetUserinfoModel(pPlayer, pPlayer->m_szRelicSavedUserModel);
		pPlayer->m_szRelicSavedUserModel[0] = '\0';
		pPlayer->m_iRelicSavedBody = 0;
		pPlayer->m_iRelicSavedSkin = 0;
		return;
	}

	if (pszModel && stricmp(pszModel, RELIC_CARRIER_USERINFO_MODEL) == 0)
		RelicRush_SetUserinfoModel(pPlayer, "gina");
}

void RelicRush_FinalizeCarrierLoss(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	RelicRush_ResetCarrierVisuals(pPlayer);

	pPlayer->m_bPendingRelicCarrier = false;
	pPlayer->m_bRelicAllowHeal = false;
	pPlayer->m_flRelicVisibleUntil = 0.0f;
	pPlayer->m_flNextRelicClientSync = 0.0f;
	pPlayer->m_bRelicLastSyncWallCling = false;
	pPlayer->m_bRelicPendingCrowbarRush = false;
	pPlayer->m_flRelicSuppressWallUntil = 0.0f;
	pPlayer->m_iRelicLastSyncGlow = -1;
	pPlayer->m_bRelicWallClinging = false;
	pPlayer->m_vecRelicWallNormal = g_vecZero;

	if (pPlayer->pev->movetype == MOVETYPE_FLY)
	{
		pPlayer->pev->movetype = MOVETYPE_WALK;
		pPlayer->pev->gravity = 1.0f;
	}

	if (pPlayer->IsAlive())
	{
		if (pPlayer->m_szRelicSavedUserModel[0])
			RelicRush_RestoreCarrierModel(pPlayer);
		else
		{
			RelicRush_EnsureNormalUserinfoModel(pPlayer);
			const char* pszModel = g_engfuncs.pfnInfoKeyValue(
				g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model");
			if (pszModel && pszModel[0] && stricmp(pszModel, RELIC_CARRIER_USERINFO_MODEL) != 0)
			{
				char szModelPath[72];
				snprintf(szModelPath, sizeof(szModelPath), "models/%s.mdl", pszModel);
				SET_MODEL(ENT(pPlayer->pev), szModelPath);
			}
		}
		RelicRush_RestorePlayMode(pPlayer);
	}
	else
		RelicRush_EnsureNormalUserinfoModel(pPlayer);

	RelicRush_SyncCarrierClient(pPlayer);
}

void RelicRush_RestorePlayMode(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsAlive())
		return;

	RelicRush_ResetCarrierVisuals(pPlayer);

	pPlayer->pev->iuser1 = 0;
	pPlayer->pev->iuser2 = 0;
	pPlayer->pev->iuser3 = 0;
	pPlayer->m_iHideHUD = 0;
	pPlayer->m_iClientHideHUD = -1;
	pPlayer->m_fInitHUD = true;

	pPlayer->m_afPhysicsFlags &= ~PFLAG_OBSERVER;
	pPlayer->pev->effects &= ~EF_NODRAW;

	if (pPlayer->pev->view_ofs == g_vecZero)
		pPlayer->pev->view_ofs = VEC_VIEW;
	if (pPlayer->pev->solid == SOLID_NOT)
		pPlayer->pev->solid = SOLID_SLIDEBOX;
	if (pPlayer->pev->movetype == MOVETYPE_NONE)
		pPlayer->pev->movetype = MOVETYPE_WALK;

	pPlayer->pev->takedamage = DAMAGE_AIM;
	pPlayer->pev->fixangle = 0;
	if (pPlayer->pev->deadflag != DEAD_NO)
		pPlayer->pev->deadflag = DEAD_NO;

	if (pPlayer->m_bRelicWallClinging)
	{
		pPlayer->m_bRelicWallClinging = false;
		pPlayer->m_vecRelicWallNormal = g_vecZero;
		if (pPlayer->pev->movetype == MOVETYPE_FLY)
			pPlayer->pev->movetype = MOVETYPE_WALK;
		pPlayer->pev->gravity = 1.0f;
	}
}

static void RelicRush_ReleaseWallCling(CBasePlayer* pPlayer)
{
	pPlayer->m_bRelicWallClinging = false;
	pPlayer->m_vecRelicWallNormal = g_vecZero;
	if (pPlayer->pev->movetype == MOVETYPE_FLY)
		pPlayer->pev->movetype = MOVETYPE_WALK;
	pPlayer->pev->gravity = 1.0f;
}

static bool RelicRush_IsValidWallHit(const TraceResult* ptr)
{
	if (!ptr || ptr->flFraction >= 1.0f)
		return false;
	if (fabs(ptr->vecPlaneNormal.z) > 0.35f)
		return false;
	return true;
}

static bool RelicRush_TryAcquireWall(CBasePlayer* pPlayer, TraceResult* ptrBest, Vector* pWallNormal)
{
	const float flTraceLen = 48.0f;
	const Vector vecStart = pPlayer->pev->origin + Vector(0, 0, 20);

	Vector dirs[6];
	int nDirs = 0;

	UTIL_MakeVectors(pPlayer->pev->angles);
	Vector vForward = gpGlobals->v_forward;
	vForward.z = 0.0f;
	float flLen = vForward.Length();
	if (flLen > 0.1f)
	{
		vForward = vForward * (1.0f / flLen);
		dirs[nDirs++] = vForward;
		dirs[nDirs++] = -vForward;
	}

	Vector vRight = gpGlobals->v_right;
	vRight.z = 0.0f;
	flLen = vRight.Length();
	if (flLen > 0.1f)
	{
		vRight = vRight * (1.0f / flLen);
		dirs[nDirs++] = vRight;
		dirs[nDirs++] = -vRight;
	}

	Vector vel = pPlayer->pev->velocity;
	vel.z = 0.0f;
	flLen = vel.Length();
	if (flLen > 48.0f)
		dirs[nDirs++] = vel * (1.0f / flLen);

	float flBestFrac = 2.0f;
	bool bFound = false;
	TraceResult tr;

	for (int i = 0; i < nDirs; i++)
	{
		UTIL_TraceLine(vecStart, vecStart + dirs[i] * flTraceLen, dont_ignore_monsters, ENT(pPlayer->pev), &tr);
		if (!RelicRush_IsValidWallHit(&tr))
			continue;
		if (tr.flFraction < flBestFrac)
		{
			flBestFrac = tr.flFraction;
			*ptrBest = tr;
			bFound = true;
		}
	}

	if (!bFound)
		return false;

	if (pWallNormal)
		*pWallNormal = ptrBest->vecPlaneNormal;
	return true;
}

static bool RelicRush_StillOnStoredWall(CBasePlayer* pPlayer, TraceResult* ptr)
{
	Vector wallNormal = pPlayer->m_vecRelicWallNormal;
	if (wallNormal.Length() < 0.1f)
		return false;

	const float flTraceLen = 56.0f;
	const Vector vecStart = pPlayer->pev->origin + Vector(0, 0, 20);
	const Vector vecEnd = vecStart - wallNormal * flTraceLen;

	UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, ENT(pPlayer->pev), ptr);
	if (!RelicRush_IsValidWallHit(ptr))
		return false;
	if (DotProduct(ptr->vecPlaneNormal, wallNormal) < 0.5f)
		return false;
	if (ptr->flFraction * flTraceLen > 50.0f)
		return false;

	return true;
}

static void RelicRush_SetPlayerHull(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	if (FBitSet(pPlayer->pev->flags, FL_DUCKING))
		UTIL_SetSize(pPlayer->pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pPlayer->pev, VEC_HULL_MIN, VEC_HULL_MAX);
}

static void RelicRush_SetUserinfoModel(CBasePlayer* pPlayer, const char* pszModel)
{
	if (!pPlayer || !pszModel || !pszModel[0])
		return;

	char* infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict());
	g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(), infobuffer, "model", pszModel);
}

static void RelicRush_SaveCarrierAppearance(CBasePlayer* pPlayer)
{
	if (!pPlayer || pPlayer->m_szRelicSavedUserModel[0])
		return;

	const char* pszCurrent = g_engfuncs.pfnInfoKeyValue(
		g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model");

	if (!pszCurrent || !pszCurrent[0])
		pszCurrent = "gina";

	strncpy(pPlayer->m_szRelicSavedUserModel, pszCurrent, sizeof(pPlayer->m_szRelicSavedUserModel) - 1);
	pPlayer->m_szRelicSavedUserModel[sizeof(pPlayer->m_szRelicSavedUserModel) - 1] = '\0';
	pPlayer->m_iRelicSavedBody = pPlayer->pev->body;
	pPlayer->m_iRelicSavedSkin = pPlayer->pev->skin;
}

void RelicRush_ApplyCarrierModel(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	RelicRush_SaveCarrierAppearance(pPlayer);
	RelicRush_SetUserinfoModel(pPlayer, RELIC_CARRIER_USERINFO_MODEL);
	SET_MODEL(ENT(pPlayer->pev), RELIC_CARRIER_MONSTER_MODEL);
	pPlayer->pev->body = 0;
	pPlayer->pev->skin = 0;
	pPlayer->pev->sequence = pPlayer->LookupActivity(ACT_IDLE);
	RelicRush_SetPlayerHull(pPlayer);
}

void RelicRush_ReapplyCarrierModel(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer))
		return;

	RelicRush_SetUserinfoModel(pPlayer, RELIC_CARRIER_USERINFO_MODEL);
	SET_MODEL(ENT(pPlayer->pev), RELIC_CARRIER_MONSTER_MODEL);
	pPlayer->pev->body = 0;
	pPlayer->pev->skin = 0;
	RelicRush_SetPlayerHull(pPlayer);
}

void RelicRush_RestoreCarrierModel(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsAlive() || !pPlayer->m_szRelicSavedUserModel[0])
		return;

	char szModelPath[72];
	snprintf(szModelPath, sizeof(szModelPath), "models/%s.mdl", pPlayer->m_szRelicSavedUserModel);

	RelicRush_SetUserinfoModel(pPlayer, pPlayer->m_szRelicSavedUserModel);
	SET_MODEL(ENT(pPlayer->pev), szModelPath);
	pPlayer->pev->body = pPlayer->m_iRelicSavedBody;
	pPlayer->pev->skin = pPlayer->m_iRelicSavedSkin;
	pPlayer->pev->sequence = pPlayer->LookupActivity(ACT_IDLE);
	pPlayer->m_szRelicSavedUserModel[0] = '\0';
	pPlayer->m_iRelicSavedBody = 0;
	pPlayer->m_iRelicSavedSkin = 0;
	RelicRush_SetPlayerHull(pPlayer);
}

// Cri d'attaque crowbar : entendu par toute la map (alerte) + léger tremblement (résonance).
static void RelicRush_PlayBroadcastAttackScream(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	const int iSound = RANDOM_LONG(0, ARRAYSIZE(g_RelicCarrierScreams) - 1);
	const char* pszSound = g_RelicCarrierScreams[iSound];
	const int iMainPitch = RANDOM_LONG(90, 105);
	const int iEchoPitch = RANDOM_LONG(72, 85);

	EMIT_SOUND_DYN(pPlayer->edict(), CHAN_VOICE, pszSound, 1.0f, ATTN_NONE, 0, iMainPitch);
	EMIT_SOUND_DYN(pPlayer->edict(), CHAN_STATIC, pszSound, 0.5f, ATTN_NONE, 0, iEchoPitch);

	UTIL_ScreenShakeExcept(pPlayer->edict(), pPlayer->pev->origin, 5.0f, 45.0f, 0.5f);
}

int RelicRush_GetCarrierOverlayGlowAlpha(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return 0;

	const float flNow = gpGlobals->time;
	const float flVisibleEnd = pPlayer->m_flRelicVisibleUntil;
	const float flFadeEnd = flVisibleEnd + g_RelicBalance.stealthFadeDuration;

	if (flNow < flVisibleEnd)
		return 255;

	if (flNow < flFadeEnd && g_RelicBalance.stealthFadeDuration > 0.0f)
	{
		const float t = (flNow - flVisibleEnd) / g_RelicBalance.stealthFadeDuration;
		return (int)(255.0f * (1.0f - t));
	}

	return 0;
}

void RelicRush_TickCarrierOverlaySync(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;

	const int iGlow = RelicRush_GetCarrierOverlayGlowAlpha(pPlayer);
	if (iGlow == pPlayer->m_iRelicLastSyncGlow)
		return;

	pPlayer->m_iRelicLastSyncGlow = iGlow;
	RelicRush_SyncCarrierClient(pPlayer);
}

void RelicRush_UpdateCarrierStealth(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;

	RelicRush_RefreshBalance();

	const float flNow = gpGlobals->time;
	const float flVisibleEnd = pPlayer->m_flRelicVisibleUntil;
	const float flFadeEnd = flVisibleEnd + g_RelicBalance.stealthFadeDuration;
	const int iStealthAmt = (int)g_RelicBalance.stealthRenderAmt;
	const int iGlowAmt = (int)g_RelicBalance.glowRenderAmt;
	const Vector glowColor(g_RelicBalance.glowR, g_RelicBalance.glowG, g_RelicBalance.glowB);

	if (flNow < flVisibleEnd)
	{
		pPlayer->pev->rendermode = kRenderNormal;
		pPlayer->pev->renderfx = kRenderFxGlowShell;
		pPlayer->pev->renderamt = iGlowAmt;
		pPlayer->pev->rendercolor = glowColor;
		return;
	}

	if (flNow < flFadeEnd && g_RelicBalance.stealthFadeDuration > 0.0f)
	{
		const float t = (flNow - flVisibleEnd) / g_RelicBalance.stealthFadeDuration;
		const float flInv = 1.0f - t;

		pPlayer->pev->rendermode = kRenderTransTexture;
		pPlayer->pev->renderamt = (int)(iStealthAmt + (iGlowAmt - iStealthAmt) * flInv);
		pPlayer->pev->rendercolor = glowColor * flInv;

		if (t < 0.5f)
			pPlayer->pev->renderfx = kRenderFxGlowShell;
		else
			pPlayer->pev->renderfx = kRenderFxNone;

		return;
	}

	pPlayer->pev->rendermode = kRenderTransTexture;
	pPlayer->pev->renderfx = kRenderFxNone;
	pPlayer->pev->renderamt = iStealthAmt;
	pPlayer->pev->rendercolor = g_vecZero;
}

void RelicRush_OnCarrierCrowbarUsed(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;

	pPlayer->m_flRelicVisibleUntil = gpGlobals->time + g_RelicBalance.visibleDuration;
	pPlayer->m_iRelicLastSyncGlow = -1;

	RelicRush_PlayBroadcastAttackScream(pPlayer);
	RelicRush_ScheduleNextCarrierScream(pPlayer);
	RelicRush_SyncCarrierClient(pPlayer);
}

void RelicRush_ScheduleNextCarrierScream(CBasePlayer* pCarrier)
{
	if (!RelicRush_IsCarrier(pCarrier))
		return;

	const float flMin = g_RelicBalance.soundIntervalMin;
	const float flMax = g_RelicBalance.soundIntervalMax;
	pCarrier->m_flNextRelicScream = gpGlobals->time + RANDOM_FLOAT(flMin, flMax);
}

void RelicRush_TickCarrierScreams(CBasePlayer* pCarrier)
{
	if (!RelicRush_IsCarrier(pCarrier) || !pCarrier->IsAlive())
		return;

	if (gpGlobals->time < pCarrier->m_flNextRelicScream)
		return;

	// Cris ambiants : portée normale (proximité), pas d'alerte globale.
	const int iSound = RANDOM_LONG(0, ARRAYSIZE(g_RelicCarrierScreams) - 1);
	EMIT_SOUND_DYN(pCarrier->edict(), CHAN_VOICE, g_RelicCarrierScreams[iSound], 0.85f, ATTN_NORM, 0,
		RANDOM_LONG(88, 108));

	RelicRush_ScheduleNextCarrierScream(pCarrier);
}

void RelicRush_RefreshCarrierHUD(CBasePlayer* pPlayer, float flHealth)
{
	if (!pPlayer)
		return;

	if (!pPlayer->HasSuit())
		pPlayer->SetHasSuit(true);

	pPlayer->m_iHideHUD = 0;
	pPlayer->m_iClientHideHUD = -1;
	pPlayer->m_fInitHUD = true;
	pPlayer->m_ClientWeaponBits = 0;
	pPlayer->m_iClientBattery = -1;

	const int iHealth = (int)V_max(1.0f, V_min(flHealth, g_RelicBalance.maxHealth));
	pPlayer->m_iClientHealth = -1;

	RelicRush_SyncCarrierClient(pPlayer);

	if (gmsgHideWeapon > 0)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pPlayer->pev);
		WRITE_BYTE(0);
		MESSAGE_END();
	}

	if (gmsgHealth > 0)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pPlayer->pev);
		WRITE_SHORT(iHealth);
		MESSAGE_END();
		pPlayer->m_iClientHealth = iHealth;
	}

	if (gmsgBattery > 0)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgBattery, NULL, pPlayer->pev);
		WRITE_SHORT(0);
		MESSAGE_END();
		pPlayer->m_iClientBattery = 0;
	}

	if (gmsgWeapons > 0)
	{
		const int lowerBits = (int)(pPlayer->m_WeaponBits & 0xFFFFFFFF);
		const int upperBits = (int)((pPlayer->m_WeaponBits >> 32) & 0xFFFFFFFF);
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapons, NULL, pPlayer->pev);
		WRITE_LONG(lowerBits);
		WRITE_LONG(upperBits);
		MESSAGE_END();
		pPlayer->m_ClientWeaponBits = pPlayer->m_WeaponBits;
	}

	EMIT_SOUND_DYN(pPlayer->edict(), CHAN_ITEM, RELIC_CARRIER_PICKUP_SOUND, 0.6f, ATTN_NORM, 0, PITCH_NORM);
	pPlayer->UpdateClientData();
}

// RelicSyn (3 octets) : flags = porteur(1) | mur(2) | glow 6 bits(4-252 via *4)
static byte RelicRush_PackSyncFlags(bool bCarrier, bool bWall, int iGlow)
{
	byte flags = 0;
	if (bCarrier)
		flags |= 1;
	if (bWall)
		flags |= 2;
	const int glow6 = V_min(63, (iGlow * 63) / 255);
	flags |= (byte)((glow6 & 0x3F) << 2);
	return flags;
}

void RelicRush_SendCarrierHUD(CBasePlayer* pPlayer, bool bCarrier)
{
	if (!pPlayer || gmsgRelicCarrier <= 0)
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgRelicCarrier, NULL, pPlayer->pev);
	WRITE_BYTE(bCarrier ? 1 : 0);
	MESSAGE_END();
}

void RelicRush_SyncCarrierClient(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	const bool bCarrier = RelicRush_IsCarrier(pPlayer) && pPlayer->IsAlive();
	const int iHealth = (int)V_max(0.0f, V_min(pPlayer->pev->health, g_RelicBalance.maxHealth));
	const int iGlow = bCarrier ? RelicRush_GetCarrierOverlayGlowAlpha(pPlayer) : 0;
	const byte flags = RelicRush_PackSyncFlags(bCarrier, pPlayer->m_bRelicWallClinging, iGlow);

	RelicRush_SendCarrierHUD(pPlayer, bCarrier);

	if (gmsgRelicSync > 0)
	{
		if (bCarrier)
			pPlayer->m_iRelicLastSyncGlow = iGlow;

		MESSAGE_BEGIN(MSG_ONE, gmsgRelicSync, NULL, pPlayer->pev);
		WRITE_BYTE(flags);
		WRITE_SHORT(iHealth);
		MESSAGE_END();
	}

	if (bCarrier && gmsgHealth > 0 && pPlayer->m_iClientHealth != iHealth)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pPlayer->pev);
		WRITE_SHORT(iHealth);
		MESSAGE_END();
		pPlayer->m_iClientHealth = iHealth;
	}

	pPlayer->m_bRelicLastSyncWallCling = pPlayer->m_bRelicWallClinging;
}

void RelicRush_QueueCrowbarRush(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;
	pPlayer->m_bRelicPendingCrowbarRush = true;
}

void RelicRush_ApplyPendingCrowbarRush(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->m_bRelicPendingCrowbarRush)
		return;
	pPlayer->m_bRelicPendingCrowbarRush = false;
	RelicRush_CrowbarRush(pPlayer);
}

static bool RelicRush_IsCarrierOnWall(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return false;
	if (pPlayer->m_bRelicWallClinging)
		return true;
	// Mur actif : FLY + accroupi (meme si le flag n'est pas encore pose ce frame)
	return pPlayer->pev->movetype == MOVETYPE_FLY && (pPlayer->pev->button & IN_DUCK) != 0;
}

void RelicRush_CrowbarRush(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;
	if (gpGlobals->time < pPlayer->m_flNextRelicRush)
		return;

	const bool bFromWall = RelicRush_IsCarrierOnWall(pPlayer);

	if (!bFromWall)
	{
		if (pPlayer->pev->movetype != MOVETYPE_WALK)
			return;
		if (!FBitSet(pPlayer->pev->flags, FL_ONGROUND))
			return;
	}

	UTIL_MakeVectors(pPlayer->pev->v_angle);
	Vector aim = gpGlobals->v_forward;
	const float flLen = aim.Length();
	if (flLen < 0.01f)
		return;
	aim = aim * (1.0f / flLen);

	TraceResult tr;
	const Vector vecStart = pPlayer->pev->origin + Vector(0, 0, 8);
	const Vector vecEnd = vecStart + aim * g_RelicBalance.rushTraceDist;
	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, pPlayer->edict(), &tr);

	float flScale = 1.0f;
	if (tr.flFraction < 1.0f)
		flScale = V_max(0.25f, tr.flFraction - 0.05f);

	const Vector velRush = aim * (g_RelicBalance.rushSpeed * flScale);

	if (bFromWall)
	{
		RelicRush_ReleaseWallCling(pPlayer);
		pPlayer->pev->flags &= ~FL_ONGROUND;
		pPlayer->pev->velocity = velRush;
		pPlayer->m_flRelicSuppressWallUntil = gpGlobals->time + 0.4f;
	}
	else
	{
		pPlayer->pev->velocity = pPlayer->pev->velocity + velRush;
	}

	pPlayer->m_flNextRelicRush = gpGlobals->time + g_RelicBalance.rushCooldown;
}

void RelicRush_TickWallClimb(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;
	if (pPlayer->pev->movetype != MOVETYPE_WALK && pPlayer->pev->movetype != MOVETYPE_FLY)
		return;
	if (gpGlobals->time < pPlayer->m_flRelicSuppressWallUntil)
		return;

	const bool bDuckHeld = (pPlayer->pev->button & IN_DUCK) != 0;

	if (!bDuckHeld)
	{
		if (pPlayer->m_bRelicWallClinging)
			RelicRush_ReleaseWallCling(pPlayer);
		return;
	}

	TraceResult tr;
	Vector wallNormal;

	if (pPlayer->m_bRelicWallClinging)
	{
		if (!RelicRush_StillOnStoredWall(pPlayer, &tr))
		{
			RelicRush_ReleaseWallCling(pPlayer);
			return;
		}
		wallNormal = pPlayer->m_vecRelicWallNormal;
	}
	else
	{
		if (!RelicRush_TryAcquireWall(pPlayer, &tr, &wallNormal))
			return;

		pPlayer->m_bRelicWallClinging = true;
		pPlayer->m_vecRelicWallNormal = wallNormal;
	}

	pPlayer->pev->movetype = MOVETYPE_FLY;
	pPlayer->pev->gravity = 0.0f;
	pPlayer->pev->flags &= ~FL_ONGROUND;

	const int iMoveButtons = IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_LEFT | IN_RIGHT | IN_JUMP;
	const bool bMove = (pPlayer->pev->button & iMoveButtons) != 0;

	Vector vel;

	if (!bMove)
	{
		vel = g_vecZero;
	}
	else
	{
		vel = g_vecZero;

		if ((pPlayer->pev->button & IN_FORWARD) != 0 || (pPlayer->pev->button & IN_JUMP) != 0)
			vel.z = g_RelicBalance.wallClimbUp;
		else if ((pPlayer->pev->button & IN_BACK) != 0)
			vel.z = g_RelicBalance.wallClimbDown;

		const float flDist = tr.flFraction * 56.0f;
		const float flIdeal = 32.0f;
		if (flDist > flIdeal + 2.0f)
			vel = vel - wallNormal * ((flDist - flIdeal) * 12.0f);
		else if (flDist < flIdeal - 4.0f)
			vel = vel + wallNormal * 40.0f;
	}

	pPlayer->pev->velocity = vel;
}

static bool g_bRelicModSoundsPrecached = false;

void RelicRush_PrecacheModSounds()
{
	if (g_bRelicModSoundsPrecached)
		return;

	PRECACHE_SOUND(RELIC_AMBIENT_SOUND);
	PRECACHE_SOUND(RELIC_CARRIER_PICKUP_SOUND);

	for (int i = 0; i < ARRAYSIZE(g_RelicCarrierScreams); i++)
		PRECACHE_SOUND(g_RelicCarrierScreams[i]);

	g_bRelicModSoundsPrecached = true;
}

void RelicRush_ApplySiphonHeal(CBasePlayer* pCarrier, float flDamageDealt)
{
	// Siphon de vie : uniquement le porteur actif de la relique
	if (!pCarrier || !pCarrier->m_bHasRelic || !pCarrier->IsAlive() || flDamageDealt <= 0.0f)
		return;

	pCarrier->m_bRelicAllowHeal = true;
	pCarrier->TakeHealth(flDamageDealt, DMG_GENERIC);
	pCarrier->m_bRelicAllowHeal = false;
	pCarrier->m_iClientHealth = -1;
	RelicRush_SyncCarrierClient(pCarrier);
}
