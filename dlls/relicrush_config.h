/***
 * Black Mesa Relic Rush - cvars d'equilibrage (relicrush_balance.cfg)
 ***/
#pragma once

struct RelicRushBalance
{
	float maxHealth;
	float regenInterval;
	float regenAmount;
	float crowbarRate;
	float moveThresh;
	float stealthRenderAmt;
	float glowRenderAmt;
	float glowR;
	float glowG;
	float glowB;
	float rushSpeed;
	float rushCooldown;
	float rushTraceDist;
	float wallClimbUp;
	float wallClimbDown;
	float roundLengthSec;
	float roundRestartDelay;
	float visibleDuration;
	float soundIntervalMin;
	float soundIntervalMax;
	float stealthFadeDuration;
	// Goo (clic droit porteur) — voile vert victimes
	float gooAoeRadius;
	float gooBlindHold;
	float gooBlindFade;
	float gooBlindFadeIn;
	float gooBlindAlpha;
	float gooBlindR;
	float gooBlindG;
	float gooBlindB;
	float gooBlindBlobScale; // multiplicateur rayon des taches (client)
	float gooBlindBlobCount; // nombre de taches affichees (1..RELIC_GOO_BLIND_BLOBS_MAX)
	float gooVictimGlowAmt;  // halo vert sur le modele victime (renderamt)
};

extern RelicRushBalance g_RelicBalance;

void RelicRush_RegisterBalanceCvars();
void RelicRush_RegisterServerCommands();
void RelicRush_RefreshBalance();
