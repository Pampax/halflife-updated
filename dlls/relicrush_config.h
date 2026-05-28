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
	// Poison (clic droit porteur) — voile vert victimes
	float poisonAoeRadius;
	float poisonVeilHold;
	float poisonVeilFade;
	float poisonVeilFadeIn;
	float poisonVeilAlpha;
	float poisonVeilR;
	float poisonVeilG;
	float poisonVeilB;
	float poisonVeilBlobScale; // multiplicateur rayon des taches (client)
	float poisonVeilBlobCount; // nombre de taches affichees (1..RELIC_POISON_VEIL_BLOBS_MAX)
	float poisonVictimGlowAmt;  // halo vert sur le modele victime (renderamt)
};

extern RelicRushBalance g_RelicBalance;

void RelicRush_RegisterBalanceCvars();
void RelicRush_RegisterServerCommands();
void RelicRush_RefreshBalance();
