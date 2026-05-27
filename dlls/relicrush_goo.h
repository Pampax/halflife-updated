/***
 * Black Mesa Relic Rush - projectile gluant vert (skill clic droit du porteur)
 ***/
#pragma once

class CBasePlayer;

constexpr float RELIC_GOO_COOLDOWN = 3.0f;
constexpr float RELIC_GOO_SPEED = 1500.0f;

// Trail decals laisses par le porteur (sol/mur/plafond proches)
constexpr float RELIC_DECAL_MIN = 3.0f;
constexpr float RELIC_DECAL_MAX = 10.0f;
constexpr float RELIC_DECAL_RANGE = 96.0f;	  // distance max de trace vers une surface
constexpr int RELIC_DECAL_MIN_TILES = 1;	  // ~ 0.5 m^2
constexpr int RELIC_DECAL_MAX_TILES = 5;	  // ~ 2 m^2 (groupe de tiles)
constexpr float RELIC_DECAL_TILE_SPACING = 24.0f; // offset des tiles secondaires (units HL)

// Eclaboussure a l'impact (decals verts repartis autour du point d'explosion)
constexpr float RELIC_GOO_SPLASH_DECAL_RADIUS = 140.0f;
constexpr int RELIC_GOO_SPLASH_DECAL_MIN = 16;
constexpr int RELIC_GOO_SPLASH_DECAL_MAX = 26;

// Trainee projectile (TE_BEAMFOLLOW : duree segment = valeur * 0.1 s)
constexpr int RELIC_GOO_TRAIL_LIFE = 12;

// Voile client (taches vertes) : nombre max envoye par RelicBlnd
#define RELIC_GOO_BLIND_BLOBS_MAX 24

// Sons (vanilla HL) — impact = eau / liquide uniquement (pas weapons/explode*.wav)
constexpr const char* RELIC_GOO_FIRE_SOUND = "weapons/glauncher.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_1 = "player/pl_slosh1.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_2 = "player/pl_slosh2.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_3 = "player/pl_slosh3.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_4 = "player/pl_slosh4.wav";
constexpr const char* RELIC_GOO_IMPACT_SOUND_DEEP = "player/pl_wade2.wav";

class CRelicGooProjectile : public CBaseEntity
{
public:
	void Spawn() override;
	void EXPORT GooTouch(CBaseEntity* pOther);
	void EXPORT GooFlyThink();

	static CRelicGooProjectile* Shoot(CBasePlayer* pOwner, Vector vecOrigin, Vector vecAimDir);

private:
	void Explode(TraceResult* pTrace);
	void StartGreenTrail();
};

void RelicRush_PrecacheGooAssets();
void RelicRush_FireGoo(CBasePlayer* pCarrier);
void RelicRush_BlindPlayer(CBasePlayer* pVictim);
void RelicRush_ClearGooBlindClient(CBasePlayer* pPlayer);
bool RelicRush_IsPlayerGooAffected(CBasePlayer* pPlayer);
void RelicRush_ClearGooVictimGlow(CBasePlayer* pPlayer);
void RelicRush_PlayGooExplosionAt(const Vector& vecOrigin, edict_t* pOwner);
void RelicRush_TickGooVictimEffects(CBasePlayer* pPlayer);
void RelicRush_TickCarrierTrailDecal(CBasePlayer* pCarrier);
void RelicRush_ResetCarrierGooState(CBasePlayer* pPlayer);
void RelicRush_SendGooCooldown(CBasePlayer* pPlayer, int iPct);
void RelicRush_TickCarrierGooSync(CBasePlayer* pPlayer);
// Debug / test : non-porteur uniquement — voile + FX goo a la position du joueur
void RelicRush_TestPoisonOnSelf(CBasePlayer* pPlayer);
