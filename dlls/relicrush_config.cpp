/***
 * Black Mesa Relic Rush - cvars d'equilibrage
 ***/
#include "extdll.h"
#include "util.h"
#include "relicrush_config.h"

RelicRushBalance g_RelicBalance = {
	250.0f, 1.0f, 1.0f, 5.0f, 8.0f,
	12.0f, 55.0f, 64.0f, 255.0f, 64.0f,
	900.0f, 0.35f, 128.0f, 200.0f, -140.0f,
	300.0f, 6.0f,
	3.0f, 3.0f, 10.0f, 1.0f,
	140.0f, 3.0f, 1.0f, 0.15f, 255.0f, 0.0f, 255.0f, 0.0f,
	0.45f, 18.0f, 50.0f,
};

static cvar_t rr_maxhealth = {"rr_maxhealth", "250", FCVAR_SERVER};
static cvar_t rr_regen_interval = {"rr_regen_interval", "1", FCVAR_SERVER};
static cvar_t rr_regen_amount = {"rr_regen_amount", "1", FCVAR_SERVER};
static cvar_t rr_crowbar_rate = {"rr_crowbar_rate", "5", FCVAR_SERVER};
static cvar_t rr_move_thresh = {"rr_move_thresh", "8", FCVAR_SERVER};
static cvar_t rr_stealth_renderamt = {"rr_stealth_renderamt", "12", FCVAR_SERVER};
static cvar_t rr_glow_renderamt = {"rr_glow_renderamt", "55", FCVAR_SERVER};
static cvar_t rr_glow_r = {"rr_glow_r", "64", FCVAR_SERVER};
static cvar_t rr_glow_g = {"rr_glow_g", "255", FCVAR_SERVER};
static cvar_t rr_glow_b = {"rr_glow_b", "64", FCVAR_SERVER};
static cvar_t rr_rush_speed = {"rr_rush_speed", "900", FCVAR_SERVER};
static cvar_t rr_rush_cooldown = {"rr_rush_cooldown", "0.35", FCVAR_SERVER};
static cvar_t rr_rush_trace = {"rr_rush_trace", "128", FCVAR_SERVER};
static cvar_t rr_wall_up = {"rr_wall_up", "200", FCVAR_SERVER};
static cvar_t rr_wall_down = {"rr_wall_down", "-140", FCVAR_SERVER};
static cvar_t rr_round_length = {"rr_round_length", "300", FCVAR_SERVER};
static cvar_t rr_round_restart = {"rr_round_restart", "6", FCVAR_SERVER};
static cvar_t rr_visible_time = {"rr_visible_time", "3", FCVAR_SERVER};
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
static cvar_t rr_poison_veil_blob_scale = {"rr_poison_veil_blob_scale", "0.45", FCVAR_SERVER};
static cvar_t rr_poison_veil_blob_count = {"rr_poison_veil_blob_count", "18", FCVAR_SERVER};
static cvar_t rr_poison_victim_glow_amt = {"rr_poison_victim_glow_amt", "50", FCVAR_SERVER};

#define RR_CLAMP_WARN_SLOTS 12

static struct
{
	char szName[32];
} s_rrClampWarned[RR_CLAMP_WARN_SLOTS];

static bool RR_ClampAlreadyWarned(const char* name)
{
	for (int i = 0; i < RR_CLAMP_WARN_SLOTS; i++)
	{
		if (s_rrClampWarned[i].szName[0] == '\0')
			return false;
		if (!stricmp(s_rrClampWarned[i].szName, name))
			return true;
	}
	return false;
}

static void RR_ClampMarkWarned(const char* name)
{
	for (int i = 0; i < RR_CLAMP_WARN_SLOTS; i++)
	{
		if (s_rrClampWarned[i].szName[0] == '\0' || !stricmp(s_rrClampWarned[i].szName, name))
		{
			strncpy(s_rrClampWarned[i].szName, name, sizeof(s_rrClampWarned[i].szName) - 1);
			s_rrClampWarned[i].szName[sizeof(s_rrClampWarned[i].szName) - 1] = '\0';
			return;
		}
	}
}

static float RR_ClampCvar(const char* name, float value, float minVal, float maxVal)
{
	if (value < minVal)
	{
		if (!RR_ClampAlreadyWarned(name))
		{
			ALERT(at_console, "Relic Rush: %s=%g trop bas, clamp %g\n", name, value, minVal);
			RR_ClampMarkWarned(name);
		}
		return minVal;
	}
	if (value > maxVal)
	{
		if (!RR_ClampAlreadyWarned(name))
		{
			ALERT(at_console, "Relic Rush: %s=%g trop haut, clamp %g\n", name, value, maxVal);
			RR_ClampMarkWarned(name);
		}
		return maxVal;
	}
	return value;
}

static void RelicRush_ClearClampWarnings()
{
	for (int i = 0; i < RR_CLAMP_WARN_SLOTS; i++)
		s_rrClampWarned[i].szName[0] = '\0';
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
	g_RelicBalance.moveThresh = RR_ClampCvar("rr_move_thresh", CVAR_GET_FLOAT("rr_move_thresh"), 1.0f, 64.0f);
	g_RelicBalance.stealthRenderAmt = RR_ClampCvar("rr_stealth_renderamt", CVAR_GET_FLOAT("rr_stealth_renderamt"), 0.0f, 255.0f);
	g_RelicBalance.glowRenderAmt = RR_ClampCvar("rr_glow_renderamt", CVAR_GET_FLOAT("rr_glow_renderamt"), 1.0f, 255.0f);
	g_RelicBalance.glowR = RR_ClampCvar("rr_glow_r", CVAR_GET_FLOAT("rr_glow_r"), 0.0f, 255.0f);
	g_RelicBalance.glowG = RR_ClampCvar("rr_glow_g", CVAR_GET_FLOAT("rr_glow_g"), 0.0f, 255.0f);
	g_RelicBalance.glowB = RR_ClampCvar("rr_glow_b", CVAR_GET_FLOAT("rr_glow_b"), 0.0f, 255.0f);
	g_RelicBalance.rushSpeed = RR_ClampCvar("rr_rush_speed", CVAR_GET_FLOAT("rr_rush_speed"), 100.0f, 2000.0f);
	g_RelicBalance.rushCooldown = RR_ClampCvar("rr_rush_cooldown", CVAR_GET_FLOAT("rr_rush_cooldown"), 0.05f, 5.0f);
	g_RelicBalance.rushTraceDist = RR_ClampCvar("rr_rush_trace", CVAR_GET_FLOAT("rr_rush_trace"), 32.0f, 512.0f);
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
}

void RelicRush_RegisterBalanceCvars()
{
	CVAR_REGISTER(&rr_maxhealth);
	CVAR_REGISTER(&rr_regen_interval);
	CVAR_REGISTER(&rr_regen_amount);
	CVAR_REGISTER(&rr_crowbar_rate);
	CVAR_REGISTER(&rr_move_thresh);
	CVAR_REGISTER(&rr_stealth_renderamt);
	CVAR_REGISTER(&rr_glow_renderamt);
	CVAR_REGISTER(&rr_glow_r);
	CVAR_REGISTER(&rr_glow_g);
	CVAR_REGISTER(&rr_glow_b);
	CVAR_REGISTER(&rr_rush_speed);
	CVAR_REGISTER(&rr_rush_cooldown);
	CVAR_REGISTER(&rr_rush_trace);
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

	SERVER_COMMAND("exec relicrush_balance.cfg\n");
	RelicRush_RefreshBalance();
}

void RelicRush_RegisterServerCommands()
{
	static bool s_bRegistered = false;
	if (s_bRegistered)
		return;
	s_bRegistered = true;
	g_engfuncs.pfnAddServerCommand("rr_reload_balance", &RelicRush_ReloadBalanceCmd);
}
