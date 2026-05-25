/***
 * Black Mesa Relic Rush - porteur de relique (helpers)
 ***/
#pragma once

#include "relicrush_config.h"

class CBasePlayer;

inline bool RelicRush_IsCarrier(CBasePlayer* pPlayer)
{
	return pPlayer != nullptr && pPlayer->m_bHasRelic;
}

// Cadence crowbar : bonus uniquement porteur relique (serveur). Autres joueurs = delai normal.
inline float RelicRush_CrowbarAttackDelay(float flNormalDelay, CBasePlayer* pPlayer)
{
#ifndef CLIENT_DLL
	if (pPlayer && pPlayer->m_bHasRelic && pPlayer->IsAlive() && g_RelicBalance.crowbarRate > 0.0f)
		return flNormalDelay / g_RelicBalance.crowbarRate;
#else
	(void)pPlayer;
#endif
	return flNormalDelay;
}

constexpr const char* RELIC_CARRIER_USERINFO_MODEL = "zombie";
constexpr const char* RELIC_CARRIER_MONSTER_MODEL = "models/zombie.mdl";
// Ambiance relique : pulsemachine (vanilla HL, pulsing1 absent sur Steam)
constexpr const char* RELIC_AMBIENT_SOUND = "ambience/pulsemachine.wav";
constexpr const char* RELIC_CARRIER_PICKUP_SOUND = "items/suitchargeok1.wav";

inline bool RelicRush_IsCarrierMoving(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return false;
	const float thresh = g_RelicBalance.moveThresh;
	const float vx = pPlayer->pev->velocity.x;
	const float vy = pPlayer->pev->velocity.y;
	return (vx * vx + vy * vy) >= (thresh * thresh);
}

void RelicRush_ResetCarrierVisuals(CBasePlayer* pPlayer);
void RelicRush_FinalizeCarrierLoss(CBasePlayer* pPlayer);
void RelicRush_RestorePlayMode(CBasePlayer* pPlayer);
void RelicRush_ApplyCarrierModel(CBasePlayer* pPlayer);
void RelicRush_ReapplyCarrierModel(CBasePlayer* pPlayer);
void RelicRush_RestoreCarrierModel(CBasePlayer* pPlayer);
void RelicRush_RefreshCarrierHUD(CBasePlayer* pPlayer, float flHealth);
void RelicRush_SendCarrierHUD(CBasePlayer* pPlayer, bool bCarrier);
void RelicRush_SyncCarrierClient(CBasePlayer* pPlayer);
void RelicRush_UpdateCarrierStealth(CBasePlayer* pPlayer);
int RelicRush_GetCarrierOverlayGlowAlpha(CBasePlayer* pPlayer);
void RelicRush_TickCarrierOverlaySync(CBasePlayer* pPlayer);
void RelicRush_OnCarrierCrowbarUsed(CBasePlayer* pPlayer);
void RelicRush_QueueCrowbarRush(CBasePlayer* pPlayer);
void RelicRush_ApplyPendingCrowbarRush(CBasePlayer* pPlayer);
void RelicRush_CrowbarRush(CBasePlayer* pPlayer);
void RelicRush_TickWallClimb(CBasePlayer* pPlayer);

void RelicRush_PrecacheModSounds();
void RelicRush_ScheduleNextCarrierScream(CBasePlayer* pCarrier);
void RelicRush_TickCarrierScreams(CBasePlayer* pCarrier);
void RelicRush_ApplySiphonHeal(CBasePlayer* pCarrier, float flDamageDealt);
