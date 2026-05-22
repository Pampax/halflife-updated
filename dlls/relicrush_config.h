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
};

extern RelicRushBalance g_RelicBalance;

void RelicRush_RegisterBalanceCvars();
void RelicRush_RegisterServerCommands();
void RelicRush_RefreshBalance();
