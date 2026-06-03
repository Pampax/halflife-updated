/***
 * Black Mesa Relic Rush - projectile gluant vert (skill clic droit du porteur)
 ***/
#pragma once

class CBasePlayer;

constexpr float RELIC_POISON_COOLDOWN = 3.0f;
constexpr float RELIC_POISON_SPEED = 1500.0f;

// Trail decals laisses par le porteur (sol/mur/plafond proches)
constexpr float RELIC_DECAL_MIN = 3.0f;
constexpr float RELIC_DECAL_MAX = 10.0f;
constexpr float RELIC_DECAL_RANGE = 96.0f;	  // distance max de trace vers une surface
constexpr int RELIC_DECAL_MIN_TILES = 1;	  // ~ 0.5 m^2
constexpr int RELIC_DECAL_MAX_TILES = 5;	  // ~ 2 m^2 (groupe de tiles)
constexpr float RELIC_DECAL_TILE_SPACING = 24.0f; // offset des tiles secondaires (units HL)

// Eclaboussure a l'impact (decals verts repartis autour du point d'explosion)
constexpr float RELIC_POISON_SPLASH_DECAL_RADIUS = 140.0f;
constexpr int RELIC_POISON_SPLASH_DECAL_MIN = 16;
constexpr int RELIC_POISON_SPLASH_DECAL_MAX = 26;

// Trainee projectile (TE_BEAMFOLLOW : duree = rr_poison_trail_life * 0.1 s, defaut 6 = 0.6 s)

// Voile client (taches vertes) : nombre max envoye par RelicPsnVl
#define RELIC_POISON_VEIL_BLOBS_MAX 24

// Sons (vanilla HL) — impact = eau / liquide uniquement (pas weapons/explode*.wav)
constexpr const char* RELIC_POISON_FIRE_SOUND = "weapons/glauncher.wav";
constexpr const char* RELIC_POISON_IMPACT_SOUND_1 = "player/pl_slosh1.wav";
constexpr const char* RELIC_POISON_IMPACT_SOUND_2 = "player/pl_slosh2.wav";
constexpr const char* RELIC_POISON_IMPACT_SOUND_3 = "player/pl_slosh3.wav";
constexpr const char* RELIC_POISON_IMPACT_SOUND_4 = "player/pl_slosh4.wav";
constexpr const char* RELIC_POISON_IMPACT_SOUND_DEEP = "player/pl_wade2.wav";

class CRelicPoisonProjectile : public CBaseEntity
{
public:
	void Spawn() override;
	void EXPORT PoisonTouch(CBaseEntity* pOther);
	void EXPORT PoisonFlyThink();

	static CRelicPoisonProjectile* Shoot(CBasePlayer* pOwner, Vector vecOrigin, Vector vecAimDir);

private:
	void Explode(TraceResult* pTrace);
	void StartGreenTrail();
};

void RelicRush_PrecachePoisonAssets();
void RelicRush_FirePoison(CBasePlayer* pCarrier);
void RelicRush_BlindPlayer(CBasePlayer* pVictim);
void RelicRush_ClearPoisonVeilClient(CBasePlayer* pPlayer);
bool RelicRush_IsPlayerPoisoned(CBasePlayer* pPlayer);
void RelicRush_ClearPoisonVictimGlow(CBasePlayer* pPlayer);
void RelicRush_PlayPoisonExplosionAt(const Vector& vecOrigin, edict_t* pOwner);
void RelicRush_TickPoisonVictimEffects(CBasePlayer* pPlayer);
void RelicRush_TickCarrierTrailDecal(CBasePlayer* pCarrier);
void RelicRush_ResetCarrierPoisonState(CBasePlayer* pPlayer);
void RelicRush_SendPoisonCooldown(CBasePlayer* pPlayer, int iPct);
void RelicRush_TickCarrierPoisonSync(CBasePlayer* pPlayer);
// Debug / test : non-porteur uniquement — voile + FX poison a la position du joueur
void RelicRush_TestPoisonOnSelf(CBasePlayer* pPlayer);
