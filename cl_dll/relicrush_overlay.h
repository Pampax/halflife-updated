/***
 * Black Mesa Relic Rush - overlay vision porteur (client)
 ***/
#pragma once

void RelicRush_DrawCarrierVisionOverlay(float flTime);
void RelicRush_DrawPoisonCooldownBar(float flTime);
void RelicRush_DrawCarrierCrosshair(float flTime);
void RelicRush_ApplyCarrierCrosshair(bool bOnTarget);

// Voile poison : grosses taches vertes (message RelicPsnVl)
void RelicRush_OnPoisonVeilMessage(int iSize, void* pbuf);
void RelicRush_DrawPoisonVeilOverlay(float flTime);
void RelicRush_ResetPoisonVeilOverlay();
