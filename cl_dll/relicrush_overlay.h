/***
 * Black Mesa Relic Rush - overlay vision porteur (client)
 ***/
#pragma once

void RelicRush_DrawCarrierVisionOverlay(float flTime);
void RelicRush_DrawGooCooldownBar(float flTime);
void RelicRush_DrawCarrierCrosshair(float flTime);
void RelicRush_ApplyCarrierCrosshair(bool bOnTarget);

// Voile poison goo : grosses taches vertes (message RelicBlnd)
void RelicRush_OnGooBlindMessage(int iSize, void* pbuf);
void RelicRush_DrawGooPoisonOverlay(float flTime);
void RelicRush_ResetGooBlindOverlay();
