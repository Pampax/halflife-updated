/***
 * Black Mesa Relic Rush - multiplayer game rules
 ***/
#pragma once

#include "gamerules.h"

class CRelicRushRelic;
class CBasePlayer;

class CRelicRushMultiplay : public CHalfLifeMultiplay
{
public:
	CRelicRushMultiplay();

	void Think() override;
	void RefreshSkillData() override;
	void PlayerThink(CBasePlayer* pPlayer) override;
	void PlayerPostThink(CBasePlayer* pPlayer) override;
	void PlayerSpawn(CBasePlayer* pPlayer) override;
	void PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor) override;
	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;
	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;
	bool CanHavePlayerItem(CBasePlayer* pPlayer, CBasePlayerItem* pItem) override;
	bool CanHaveItem(CBasePlayer* pPlayer, CItem* pItem) override;
	bool CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry) override;
	bool PlayFootstepSounds(CBasePlayer* pl, float fvol) override;
	bool ClientCommand(CBasePlayer* pPlayer, const char* pcmd) override;
	void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) override;
	void InitHUD(CBasePlayer* pl) override;
	void OnRelicPickedUp(CBasePlayer* pPlayer, class CRelicRushRelic* pRelic);
	void CompleteRelicPickup(CBasePlayer* pPlayer);
	void OnRelicDropped(const Vector& origin);

protected:
	CRelicRushRelic* m_pRelic = nullptr;
	CBasePlayer* m_pCarrier = nullptr;

	bool m_bRoundActive = false;
	float m_flEarliestRoundStart = 0.0f;
	float m_flRoundEndTime = 0.0f;
	float m_flRoundRestartTime = 0.0f;
	bool m_bWaitingRestart = false;

	void StartRound();
	void EndRound();

	Vector GetRelicSpawnOrigin() const;
	Vector GetRelicOriginNearPlayer(CBasePlayer* pPlayer) const;
	void SpawnRelic();
	void DebugPlaceRelicNear(CBasePlayer* pPlayer);
	void SetCarrier(CBasePlayer* pPlayer);
	void ClearCarrier(CBasePlayer* pPlayer, bool bAnnounce);
	void ApplyCarrierLoadout(CBasePlayer* pPlayer);
	void ApplyCarrierEffects(CBasePlayer* pPlayer, bool enable);
	void TickCarrier(CBasePlayer* pPlayer);
};
