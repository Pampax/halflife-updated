/***
 * Black Mesa Relic Rush - projectile gluant vert (skill clic droit du porteur)
 ***/
#pragma once

class CBasePlayer;

constexpr float RELIC_GOO_COOLDOWN = 3.0f;
constexpr float RELIC_GOO_SPEED = 1500.0f;
constexpr float RELIC_GOO_AOE_RADIUS = 140.0f;
constexpr float RELIC_GOO_BLIND_HOLD = 3.0f;
constexpr float RELIC_GOO_BLIND_FADE = 1.0f;
constexpr int RELIC_GOO_BLIND_ALPHA = 220;

// Trail decals laisses par le porteur (sol/mur/plafond proches)
constexpr float RELIC_DECAL_MIN = 3.0f;
constexpr float RELIC_DECAL_MAX = 10.0f;
constexpr float RELIC_DECAL_RANGE = 96.0f;	  // distance max de trace vers une surface
constexpr int RELIC_DECAL_MIN_TILES = 1;	  // ~ 0.5 m^2
constexpr int RELIC_DECAL_MAX_TILES = 5;	  // ~ 2 m^2 (groupe de tiles)
constexpr float RELIC_DECAL_TILE_SPACING = 24.0f; // offset des tiles secondaires (units HL)

// Sons (vanilla HL, presents sur toutes les installs steam standard)
constexpr const char* RELIC_GOO_FIRE_SOUND = "weapons/glauncher.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_1 = "debris/flesh1.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_2 = "debris/flesh2.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_3 = "debris/flesh3.wav";

class CRelicGooProjectile : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT GooTouch(CBaseEntity* pOther);
	void EXPORT GooFlyThink();

	static CRelicGooProjectile* Shoot(CBasePlayer* pOwner, Vector vecOrigin, Vector vecAimDir);

private:
	void Explode(TraceResult* pTrace);
};

void RelicRush_PrecacheGooAssets();
void RelicRush_FireGoo(CBasePlayer* pCarrier);
void RelicRush_BlindPlayer(CBasePlayer* pVictim);
void RelicRush_TickPlayerBlindFade(CBasePlayer* pPlayer);
void RelicRush_TickCarrierTrailDecal(CBasePlayer* pCarrier);
void RelicRush_ResetCarrierGooState(CBasePlayer* pPlayer);
void RelicRush_SendGooCooldown(CBasePlayer* pPlayer, int iPct);
void RelicRush_TickCarrierGooSync(CBasePlayer* pPlayer);
