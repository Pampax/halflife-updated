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

private:
	bool m_bCarried = false;
	EHANDLE m_hPickupPlayer;
};
