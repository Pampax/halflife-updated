/***
 * Black Mesa Relic Rush - cvars d'equilibrage
 ***/
#include "extdll.h"
#include "util.h"
#include "relicrush_config.h"
#include "relicrush_clamp.h"

// Valeurs initiales = relicrush_balance.cfg (gameplay lit g_RelicBalance apres RefreshBalance).
RelicRushBalance g_RelicBalance = {
	200.0f, 1.0f, 2.0f, 2.0f, 2.0f,
	25.0f, 50.0f, 50.0f, 255.0f, 50.0f,
	900.0f, 0.5f, 128.0f, 15.0f, 200.0f, -140.0f,
	3600.0f, 6.0f,
	1.0f, 3.0f, 10.0f, 1.0f,
	140.0f, 3.0f, 1.0f, 0.15f, 255.0f, 0.0f, 255.0f, 0.0f,
	1.5f, 24.0f, 50.0f, 6.0f,
};

static cvar_t rr_maxhealth = {"rr_maxhealth", "200", FCVAR_SERVER};
static cvar_t rr_regen_interval = {"rr_regen_interval", "1", FCVAR_SERVER};
static cvar_t rr_regen_amount = {"rr_regen_amount", "2", FCVAR_SERVER};
static cvar_t rr_crowbar_rate = {"rr_crowbar_rate", "2", FCVAR_SERVER};
static cvar_t rr_siphon_mult = {"rr_siphon_mult", "2", FCVAR_SERVER};
static cvar_t rr_stealth_renderamt = {"rr_stealth_renderamt", "25", FCVAR_SERVER};
static cvar_t rr_glow_renderamt = {"rr_glow_renderamt", "50", FCVAR_SERVER};
static cvar_t rr_glow_r = {"rr_glow_r", "50", FCVAR_SERVER};
static cvar_t rr_glow_g = {"rr_glow_g", "255", FCVAR_SERVER};
static cvar_t rr_glow_b = {"rr_glow_b", "50", FCVAR_SERVER};
static cvar_t rr_rush_speed = {"rr_rush_speed", "900", FCVAR_SERVER};
static cvar_t rr_rush_cooldown = {"rr_rush_cooldown", "0.5", FCVAR_SERVER};
static cvar_t rr_rush_trace = {"rr_rush_trace", "128", FCVAR_SERVER};
static cvar_t rr_rush_pitch = {"rr_rush_pitch", "15", FCVAR_SERVER};
static cvar_t rr_wall_up = {"rr_wall_up", "200", FCVAR_SERVER};
static cvar_t rr_wall_down = {"rr_wall_down", "-140", FCVAR_SERVER};
static cvar_t rr_round_length = {"rr_round_length", "3600", FCVAR_SERVER};
static cvar_t rr_round_restart = {"rr_round_restart", "6", FCVAR_SERVER};
static cvar_t rr_visible_time = {"rr_visible_time", "1", FCVAR_SERVER};
static cvar_t rr_sound_min = {"rr_sound_min", "3", FCVAR_SERVER};
static cvar_t rr_sound_max = {"rr_sound_max", "10", FCVAR_SERVER};
static cvar_t rr_stealth_fade = {"rr_stealth_fade", "1", FCVAR_SERVER};
static cvar_t rr_poison_aoe_radius = {"rr_poison_aoe_radius", "140", FCVAR_SERVER};
static cvar_t rr_poison_veil_hold = {"rr_poison_veil_hold", "3", FCVAR_SERVER};
static cvar_t rr_poison_veil_fade = {"rr_poison_veil_fade", "1", FCVAR_SERVER};
static cvar_t rr_poison_veil_fadein = {"rr_poison_veil_fadein", "0.15", FCVAR_SERVER};
static cvar_t rr_poison_veil_alpha = {"rr_poison_veil_alpha", "255", FCVAR_SERVER};
static cvar_t rr_poison_veil_r = {"rr_poison_veil_r", "0", FCVAR_SERVER};
static cvar_t rr_poison_veil_g = {"rr_poison_veil_g", "255", FCVAR_SERVER};
static cvar_t rr_poison_veil_b = {"rr_poison_veil_b", "0", FCVAR_SERVER};
static cvar_t rr_poison_veil_blob_scale = {"rr_poison_veil_blob_scale", "1.5", FCVAR_SERVER};
static cvar_t rr_poison_veil_blob_count = {"rr_poison_veil_blob_count", "24", FCVAR_SERVER};
static cvar_t rr_poison_victim_glow_amt = {"rr_poison_victim_glow_amt", "50", FCVAR_SERVER};
static cvar_t rr_poison_trail_life = {"rr_poison_trail_life", "6", FCVAR_SERVER};

// Clamp pur + dedup des avertissements : voir relicrush_clamp.h/.cpp (testable sans le moteur).
static RRClampWarnTracker s_rrClampWarned;

static float RR_ClampCvar(const char* name, float value, float minVal, float maxVal)
{
	float clamped = RR_ClampValue(value, minVal, maxVal);
	if (clamped != value && !s_rrClampWarned.AlreadyWarned(name))
	{
		ALERT(at_console, "Relic Rush: %s=%g %s, clamp %g\n", name, value, value < minVal ? "trop bas" : "trop haut", clamped);
		s_rrClampWarned.MarkWarned(name);
	}
	return clamped;
}

static void RelicRush_ClearClampWarnings()
{
	s_rrClampWarned.Clear();
}

static void RelicRush_ReloadBalanceCmd()
{
	RelicRush_ClearClampWarnings();
	RelicRush_RefreshBalance();
	// Ne pas utiliser UTIL_ClientPrintAll ici : gmsgTextMsg peut etre 0 avant LinkUserMessages.
	ALERT(at_console, "Relic Rush: equilibrage recharge (cvars -> gameplay).\n");
}

void RelicRush_RefreshBalance()
{
	g_RelicBalance.maxHealth = RR_ClampCvar("rr_maxhealth", CVAR_GET_FLOAT("rr_maxhealth"), 50.0f, 500.0f);
	g_RelicBalance.regenInterval = RR_ClampCvar("rr_regen_interval", CVAR_GET_FLOAT("rr_regen_interval"), 0.25f, 30.0f);
	g_RelicBalance.regenAmount = RR_ClampCvar("rr_regen_amount", CVAR_GET_FLOAT("rr_regen_amount"), 0.0f, 100.0f);
	g_RelicBalance.crowbarRate = RR_ClampCvar("rr_crowbar_rate", CVAR_GET_FLOAT("rr_crowbar_rate"), 1.0f, 20.0f);
	g_RelicBalance.siphonMultiplier = RR_ClampCvar("rr_siphon_mult", CVAR_GET_FLOAT("rr_siphon_mult"), 0.0f, 10.0f);
	g_RelicBalance.stealthRenderAmt = RR_ClampCvar("rr_stealth_renderamt", CVAR_GET_FLOAT("rr_stealth_renderamt"), 0.0f, 255.0f);
	g_RelicBalance.glowRenderAmt = RR_ClampCvar("rr_glow_renderamt", CVAR_GET_FLOAT("rr_glow_renderamt"), 1.0f, 255.0f);
	g_RelicBalance.glowR = RR_ClampCvar("rr_glow_r", CVAR_GET_FLOAT("rr_glow_r"), 0.0f, 255.0f);
	g_RelicBalance.glowG = RR_ClampCvar("rr_glow_g", CVAR_GET_FLOAT("rr_glow_g"), 0.0f, 255.0f);
	g_RelicBalance.glowB = RR_ClampCvar("rr_glow_b", CVAR_GET_FLOAT("rr_glow_b"), 0.0f, 255.0f);
	g_RelicBalance.rushSpeed = RR_ClampCvar("rr_rush_speed", CVAR_GET_FLOAT("rr_rush_speed"), 100.0f, 2000.0f);
	g_RelicBalance.rushCooldown = RR_ClampCvar("rr_rush_cooldown", CVAR_GET_FLOAT("rr_rush_cooldown"), 0.05f, 5.0f);
	g_RelicBalance.rushTraceDist = RR_ClampCvar("rr_rush_trace", CVAR_GET_FLOAT("rr_rush_trace"), 32.0f, 512.0f);
	g_RelicBalance.rushPitchOffset = RR_ClampCvar("rr_rush_pitch", CVAR_GET_FLOAT("rr_rush_pitch"), -45.0f, 45.0f);
	g_RelicBalance.wallClimbUp = RR_ClampCvar("rr_wall_up", CVAR_GET_FLOAT("rr_wall_up"), 50.0f, 600.0f);
	g_RelicBalance.wallClimbDown = RR_ClampCvar("rr_wall_down", CVAR_GET_FLOAT("rr_wall_down"), -600.0f, -50.0f);
	g_RelicBalance.roundLengthSec = RR_ClampCvar("rr_round_length", CVAR_GET_FLOAT("rr_round_length"), 60.0f, 3600.0f);
	g_RelicBalance.roundRestartDelay = RR_ClampCvar("rr_round_restart", CVAR_GET_FLOAT("rr_round_restart"), 1.0f, 120.0f);
	g_RelicBalance.visibleDuration = RR_ClampCvar("rr_visible_time", CVAR_GET_FLOAT("rr_visible_time"), 0.5f, 60.0f);
	g_RelicBalance.soundIntervalMin = RR_ClampCvar("rr_sound_min", CVAR_GET_FLOAT("rr_sound_min"), 0.5f, 120.0f);
	g_RelicBalance.soundIntervalMax = RR_ClampCvar("rr_sound_max", CVAR_GET_FLOAT("rr_sound_max"), 0.5f, 120.0f);
	if (g_RelicBalance.soundIntervalMax < g_RelicBalance.soundIntervalMin)
		g_RelicBalance.soundIntervalMax = g_RelicBalance.soundIntervalMin;
	g_RelicBalance.stealthFadeDuration = RR_ClampCvar("rr_stealth_fade", CVAR_GET_FLOAT("rr_stealth_fade"), 0.0f, 5.0f);
	g_RelicBalance.poisonAoeRadius = RR_ClampCvar("rr_poison_aoe_radius", CVAR_GET_FLOAT("rr_poison_aoe_radius"), 32.0f, 512.0f);
	g_RelicBalance.poisonVeilHold = RR_ClampCvar("rr_poison_veil_hold", CVAR_GET_FLOAT("rr_poison_veil_hold"), 0.0f, 30.0f);
	g_RelicBalance.poisonVeilFade = RR_ClampCvar("rr_poison_veil_fade", CVAR_GET_FLOAT("rr_poison_veil_fade"), 0.0f, 10.0f);
	g_RelicBalance.poisonVeilFadeIn = RR_ClampCvar("rr_poison_veil_fadein", CVAR_GET_FLOAT("rr_poison_veil_fadein"), 0.0f, 3.0f);
	g_RelicBalance.poisonVeilAlpha = RR_ClampCvar("rr_poison_veil_alpha", CVAR_GET_FLOAT("rr_poison_veil_alpha"), 1.0f, 255.0f);
	g_RelicBalance.poisonVeilR = RR_ClampCvar("rr_poison_veil_r", CVAR_GET_FLOAT("rr_poison_veil_r"), 0.0f, 255.0f);
	g_RelicBalance.poisonVeilG = RR_ClampCvar("rr_poison_veil_g", CVAR_GET_FLOAT("rr_poison_veil_g"), 0.0f, 255.0f);
	g_RelicBalance.poisonVeilB = RR_ClampCvar("rr_poison_veil_b", CVAR_GET_FLOAT("rr_poison_veil_b"), 0.0f, 255.0f);
	g_RelicBalance.poisonVeilBlobScale = RR_ClampCvar("rr_poison_veil_blob_scale", CVAR_GET_FLOAT("rr_poison_veil_blob_scale"), 0.15f, 1.5f);
	g_RelicBalance.poisonVeilBlobCount = RR_ClampCvar("rr_poison_veil_blob_count", CVAR_GET_FLOAT("rr_poison_veil_blob_count"), 1.0f, 24.0f);
	g_RelicBalance.poisonVictimGlowAmt = RR_ClampCvar("rr_poison_victim_glow_amt", CVAR_GET_FLOAT("rr_poison_victim_glow_amt"), 1.0f, 255.0f);
	g_RelicBalance.poisonTrailLife = RR_ClampCvar("rr_poison_trail_life", CVAR_GET_FLOAT("rr_poison_trail_life"), 1.0f, 255.0f);
}

void RelicRush_RegisterBalanceCvars()
{
	CVAR_REGISTER(&rr_maxhealth);
	CVAR_REGISTER(&rr_regen_interval);
	CVAR_REGISTER(&rr_regen_amount);
	CVAR_REGISTER(&rr_crowbar_rate);
	CVAR_REGISTER(&rr_siphon_mult);
	CVAR_REGISTER(&rr_stealth_renderamt);
	CVAR_REGISTER(&rr_glow_renderamt);
	CVAR_REGISTER(&rr_glow_r);
	CVAR_REGISTER(&rr_glow_g);
	CVAR_REGISTER(&rr_glow_b);
	CVAR_REGISTER(&rr_rush_speed);
	CVAR_REGISTER(&rr_rush_cooldown);
	CVAR_REGISTER(&rr_rush_trace);
	CVAR_REGISTER(&rr_rush_pitch);
	CVAR_REGISTER(&rr_wall_up);
	CVAR_REGISTER(&rr_wall_down);
	CVAR_REGISTER(&rr_round_length);
	CVAR_REGISTER(&rr_round_restart);
	CVAR_REGISTER(&rr_visible_time);
	CVAR_REGISTER(&rr_sound_min);
	CVAR_REGISTER(&rr_sound_max);
	CVAR_REGISTER(&rr_stealth_fade);
	CVAR_REGISTER(&rr_poison_aoe_radius);
	CVAR_REGISTER(&rr_poison_veil_hold);
	CVAR_REGISTER(&rr_poison_veil_fade);
	CVAR_REGISTER(&rr_poison_veil_fadein);
	CVAR_REGISTER(&rr_poison_veil_alpha);
	CVAR_REGISTER(&rr_poison_veil_r);
	CVAR_REGISTER(&rr_poison_veil_g);
	CVAR_REGISTER(&rr_poison_veil_b);
	CVAR_REGISTER(&rr_poison_veil_blob_scale);
	CVAR_REGISTER(&rr_poison_veil_blob_count);
	CVAR_REGISTER(&rr_poison_victim_glow_amt);
	CVAR_REGISTER(&rr_poison_trail_life);

	// relicrush_balance.cfg est execute par game.cfg / listenserver.cfg / server.cfg.
	// Ne pas RefreshBalance ici : trop tot (avant exec cfg) et RefreshSkillData du mod
	// n'est pas appele depuis le constructeur CHalfLifeMultiplay.
}

void RelicRush_RegisterServerCommands()
{
	static bool s_bRegistered = false;
	if (s_bRegistered)
		return;
	s_bRegistered = true;
	g_engfuncs.pfnAddServerCommand("rr_reload_balance", &RelicRush_ReloadBalanceCmd);
}
