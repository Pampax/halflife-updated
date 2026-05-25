/***
 * Black Mesa Relic Rush - relic entity
 ***/
#pragma once

class CBasePlayer;

class CRelicRushRelic : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RelicTouch(CBaseEntity* pOther);
	void EXPORT DeferredPickup();
	void EXPORT RelicAmbientThink();

	static CRelicRushRelic* CreateAt(const Vector& origin);

	void HideForPickup();
	void StopAmbient();

private:
	bool m_bCarried = false;
	EHANDLE m_hPickupPlayer;
};

// Helper : stoppe le son ambiant puis libere l'entite (UTIL_Remove ne tue pas le son seul).
inline void RelicRush_RemoveRelic(CRelicRushRelic*& pRelic)
{
	if (!pRelic)
		return;
	pRelic->StopAmbient();
	UTIL_Remove(pRelic);
	pRelic = nullptr;
}
