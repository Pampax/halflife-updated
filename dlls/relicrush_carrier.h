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

// Modele 3PP porteur : models/robo/robo.mdl (rgrunt + textures RG_*.bmp dans le meme dossier)
// "robo" en userinfo = cle factice (evite que l'engine remette gina/barney).
constexpr const char* RELIC_CARRIER_USERINFO_MODEL = "robo";
constexpr const char* RELIC_CARRIER_MONSTER_MODEL = "models/robo/robo.mdl";
constexpr int RELIC_CARRIER_SKIN = 0;
// topcolor/bottomcolor a 0 cote serveur ; le client ignore StudioSetRemapColors pour robo
// (sinon une partie du mesh suit les couleurs MP du menu, l'autre reste rouge LUT moteur).
constexpr int RELIC_CARRIER_TOPCOLOR = 0;
constexpr int RELIC_CARRIER_BOTTOMCOLOR = 0;
// Ambiance relique : son Xen distinctif (les ambient_generic des maps DM peuvent
// utiliser pulsemachine/etc., on prend un son electrique-portail clairement identifiable).
constexpr const char* RELIC_AMBIENT_SOUND = "debris/beamstart8.wav";
constexpr const char* RELIC_CARRIER_PICKUP_SOUND = "items/suitchargeok1.wav";
// v_knife.mdl : sequence "attack3" = CROWBAR_ATTACK3MISS (slash griffes porteur).
constexpr int RELIC_CARRIER_CLAW_ATTACK_SEQ = 7; // CROWBAR_ATTACK3MISS
constexpr float RELIC_CARRIER_CLAW_ANIM_FRAMERATE = 2.0f;

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
void RelicRush_ApplyCarrierDefaultSkin(CBasePlayer* pPlayer);
void RelicRush_EnsureCarrierNeutralColors(CBasePlayer* pPlayer);
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
void RelicRush_ApplySiphonHeal(CBasePlayer* pCarrier, CBasePlayer* pVictim, float flDamageDealt);
