/***
 * Black Mesa Relic Rush - multiplayer game rules
 ***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "game.h"
#include "skill.h"
#include "trains.h"
#include "items.h"
#include "shake.h"
#include "relicrush_relic.h"
#include "relicrush_carrier.h"
#include "relicrush_goo.h"
#include "relicrush_gamerules.h"
#include "relicrush_config.h"
#include "UserMessages.h"
struct RelicMapSpawn
{
	const char* pszMap;
	float x, y, z;
};
static const RelicMapSpawn g_RelicMapSpawns[] =
{
	{"crossfire", 64.0f, -704.0f, -112.0f},
	{"boot_camp", 128.0f, 320.0f, -64.0f},
	{"bounce", 0.0f, 0.0f, 256.0f},
	{"datacore", -256.0f, 512.0f, -128.0f},
	{"stalkyard", 0.0f, 0.0f, 64.0f},
	{"lambda_bunker", 256.0f, 0.0f, 128.0f},
};
CRelicRushMultiplay::CRelicRushMultiplay()
{
	CVAR_SET_STRING("motdfile", "motd.txt");
	CVAR_SET_STRING("hostname", "Black Mesa Relic Rush");

	m_bRoundActive = false;
	m_bWaitingRestart = false;
	m_pRelic = nullptr;
	m_pCarrier = nullptr;
	m_flEarliestRoundStart = 0.0f;

	// Precache pendant InstallGameRules (phase precache map), pas en jeu.
	PRECACHE_MODEL("models/w_antidote.mdl");
	PRECACHE_MODEL(RELIC_CARRIER_MONSTER_MODEL);
	// Viewmodel porteur : couteau OpFor (slash melee, meme seq que crowbar). Fichier
	// relicrush/models/v_knife.mdl — pas besoin d'OpFor installe chez les joueurs.
	PRECACHE_MODEL("models/v_knife.mdl");
	RelicRush_PrecacheModSounds();
	RelicRush_PrecacheGooAssets();
}

void CRelicRushMultiplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	if (RelicRush_IsCarrier(pPlayer))
	{
		g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(), infobuffer, "topcolor", "0");
		g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(), infobuffer, "bottomcolor", "0");
	}

	CHalfLifeMultiplay::ClientUserInfoChanged(pPlayer, infobuffer);

	if (RelicRush_IsCarrier(pPlayer))
		RelicRush_ReapplyCarrierModel(pPlayer);
}

void CRelicRushMultiplay::InitHUD(CBasePlayer* pl)
{
	CVAR_SET_STRING("motdfile", "motd.txt");
	CHalfLifeMultiplay::InitHUD(pl);

	if (pl && gmsgShowGameTitle > 0)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgShowGameTitle, NULL, pl->edict());
		WRITE_BYTE(0);
		MESSAGE_END();
	}
}
void CRelicRushMultiplay::RefreshSkillData()
{
	RelicRush_RefreshBalance();
	CHalfLifeMultiplay::RefreshSkillData();
	gSkillData.plrDmgCrowbar = 40;
}
void CRelicRushMultiplay::Think()
{
	if (g_fGameOver)
	{
		CHalfLifeMultiplay::Think();
		return;
	}
	if (m_bWaitingRestart)
	{
		if (gpGlobals->time >= m_flRoundRestartTime)
		{
			m_bWaitingRestart = false;
			ChangeLevel();
		}
		return;
	}
	if (!m_bRoundActive)
	{
		if (m_flEarliestRoundStart <= 0.0f)
			m_flEarliestRoundStart = gpGlobals->time + 2.0f;

		if (gpGlobals->time < m_flEarliestRoundStart)
		{
			CHalfLifeMultiplay::Think();
			return;
		}

		StartRound();
		return;
	}
	if (gpGlobals->time >= m_flRoundEndTime)
	{
		EndRound();
		return;
	}
	CHalfLifeMultiplay::Think();
}
void CRelicRushMultiplay::StartRound()
{
	m_bRoundActive = true;
	m_bWaitingRestart = false;
	m_pCarrier = nullptr;
	m_flRoundEndTime = gpGlobals->time + g_RelicBalance.roundLengthSec;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));
		if (pPlayer)
			ClearCarrier(pPlayer, false);
	}
	SpawnRelic();
	UTIL_ClientPrintAll(HUD_PRINTTALK, "Relic Rush : attrapez la relique Xen !\n");
}
void CRelicRushMultiplay::EndRound()
{
	m_bRoundActive = false;
	CBasePlayer* pLastCarrier = m_pCarrier;
	if (m_pCarrier)
		ClearCarrier(m_pCarrier, false);
	RelicRush_RemoveRelic(m_pRelic);
	if (pLastCarrier && pLastCarrier->IsAlive())
		UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("Fin de manche : %s tenait la relique", STRING(pLastCarrier->pev->netname)));
	else
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "Fin de manche");
	m_bWaitingRestart = true;
	m_flRoundRestartTime = gpGlobals->time + g_RelicBalance.roundRestartDelay;
}
void CRelicRushMultiplay::SpawnRelic()
{
	RelicRush_RemoveRelic(m_pRelic);
	m_pRelic = CRelicRushRelic::CreateAt(GetRelicSpawnOrigin());
}
Vector CRelicRushMultiplay::GetRelicOriginNearPlayer(CBasePlayer* pPlayer) const
{
	if (!pPlayer)
		return g_vecZero;
	UTIL_MakeVectors(pPlayer->pev->angles);
	Vector origin = pPlayer->pev->origin + gpGlobals->v_forward * 64.0f;
	origin.z += 16.0f;
	return origin;
}
void CRelicRushMultiplay::DebugPlaceRelicNear(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;
	if (!pPlayer->IsAlive())
	{
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "rr_here : vous devez etre en vie.\n");
		return;
	}
	if (m_pCarrier)
		ClearCarrier(m_pCarrier, false);
	RelicRush_RemoveRelic(m_pRelic);
	const Vector origin = GetRelicOriginNearPlayer(pPlayer);
	m_pRelic = CRelicRushRelic::CreateAt(origin);
	if (!m_bRoundActive)
	{
		m_bRoundActive = true;
		m_flRoundEndTime = gpGlobals->time + g_RelicBalance.roundLengthSec;
	}
	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Relique placee devant vous (debug)");
}
bool CRelicRushMultiplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (FStrEq(pcmd, "rr_here"))
	{
		DebugPlaceRelicNear(pPlayer);
		return true;
	}
	if (FStrEq(pcmd, "rr_cam"))
	{
		pPlayer->m_bRelicDebugCam = !pPlayer->m_bRelicDebugCam;
		if (pPlayer->m_bRelicDebugCam)
		{
			g_engfuncs.pfnClientCommand(pPlayer->edict(), "rr_allowthirdperson 1\n");
			g_engfuncs.pfnClientCommand(pPlayer->edict(), "thirdperson\n");
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Vue 3e personne (debug F6 / rr_cam)");
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE,
				"Relic Rush : vue 3PP active. Necessite client.dll du mod dans relicrush/cl_dlls.\n");
		}
		else
		{
			g_engfuncs.pfnClientCommand(pPlayer->edict(), "firstperson\n");
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Vue 1ere personne");
		}
		return true;
	}
	if (FStrEq(pcmd, "rr_status"))
	{
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE,
			UTIL_VarArgs("Relic Rush v0.2-stealth | carrier=%d | visible=%.1fs | stealth=%d glow=%d\n",
				RelicRush_IsCarrier(pPlayer) ? 1 : 0,
				V_max(0.0f, pPlayer->m_flRelicVisibleUntil - gpGlobals->time),
				(int)g_RelicBalance.stealthRenderAmt,
				(int)g_RelicBalance.glowRenderAmt));
		return true;
	}
	if (FStrEq(pcmd, "rr_poison"))
	{
		RelicRush_TestPoisonOnSelf(pPlayer);
		return true;
	}
	return CHalfLifeMultiplay::ClientCommand(pPlayer, pcmd);
}
Vector CRelicRushMultiplay::GetRelicSpawnOrigin() const
{
	const char* pszMap = STRING(gpGlobals->mapname);
	for (const auto& entry : g_RelicMapSpawns)
	{
		if (0 == stricmp(pszMap, entry.pszMap))
			return Vector(entry.x, entry.y, entry.z);
	}
	CBaseEntity* pSpot = nullptr;
	Vector avg = g_vecZero;
	int count = 0;
	while ((pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch")) != nullptr)
	{
		avg = avg + pSpot->pev->origin;
		++count;
	}
	if (count > 0)
	{
		avg = avg / static_cast<float>(count);
		avg.z += 32.0f;
		return avg;
	}
	return Vector(0, 0, 256);
}
void CRelicRushMultiplay::ApplyCarrierLoadout(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	// Retirer armes ; redonner suit (HUD vie) + crowbar (avant m_bHasRelic)
	pPlayer->RemoveAllItems(true);
	pPlayer->GiveNamedItem("item_suit");
	pPlayer->GiveNamedItem("weapon_crowbar");

	CBasePlayerItem* pCrowbar = nullptr;
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		for (CBasePlayerItem* pItem = pPlayer->m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext)
		{
			if (pItem->m_iId == WEAPON_CROWBAR)
			{
				pCrowbar = pItem;
				break;
			}
		}
		if (pCrowbar)
			break;
	}

	if (pCrowbar)
		pPlayer->SwitchWeapon(pCrowbar);
	else
		ALERT(at_console, "Relic Rush: crowbar manquante pour %s\n", STRING(pPlayer->pev->netname));
}

void CRelicRushMultiplay::SetCarrier(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	m_pCarrier = pPlayer;
	pPlayer->m_bPendingRelicCarrier = false;
	pPlayer->m_flNextRelicRegen = gpGlobals->time + g_RelicBalance.regenInterval;

	// Loadout AVANT m_bHasRelic sinon CanHavePlayerItem bloque la crowbar
	ApplyCarrierLoadout(pPlayer);
	pPlayer->m_bHasRelic = true;
	ApplyCarrierEffects(pPlayer, true);
	RelicRush_RestorePlayMode(pPlayer);
	pPlayer->m_flNextRelicClientSync = 0.0f;
	pPlayer->m_bRelicLastSyncWallCling = false;

	// Swap viewmodel couteau : ApplyCarrierLoadout a deploye v_crowbar avant m_bHasRelic.
	if (pPlayer->m_pActiveItem && pPlayer->m_pActiveItem->m_iId == WEAPON_CROWBAR)
	{
		pPlayer->pev->viewmodel = MAKE_STRING("models/v_knife.mdl");
		pPlayer->pev->weaponmodel = iStringNull;
	}

	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Vous portez la relique !");
	EMIT_SOUND_DYN(pPlayer->edict(), CHAN_ITEM, RELIC_CARRIER_PICKUP_SOUND, 1.0f, ATTN_NORM, 0, PITCH_NORM);
}

void CRelicRushMultiplay::CompleteRelicPickup(CBasePlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsAlive() || m_pCarrier)
		return;

	SetCarrier(pPlayer);
}
void CRelicRushMultiplay::ClearCarrier(CBasePlayer* pPlayer, bool bAnnounce)
{
	if (!pPlayer || !pPlayer->m_bHasRelic)
		return;
	ApplyCarrierEffects(pPlayer, false);
	pPlayer->m_bHasRelic = false;
	RelicRush_FinalizeCarrierLoss(pPlayer);
	RelicRush_ResetCarrierGooState(pPlayer);
	pPlayer->UpdateClientData();
	if (m_pCarrier == pPlayer)
		m_pCarrier = nullptr;
	if (bAnnounce)
		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("%s a perdu la relique.\n", STRING(pPlayer->pev->netname)));
}
void CRelicRushMultiplay::ApplyCarrierEffects(CBasePlayer* pPlayer, bool enable)
{
	if (!pPlayer)
		return;
	if (enable)
	{
		if (pPlayer->m_flBaseMaxHealth <= 0.0f)
			pPlayer->m_flBaseMaxHealth = pPlayer->pev->max_health;
		// Pas de long jump pour le porteur
		pPlayer->m_fLongJump = false;
		g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "slj", "0");
		// iuser1 = mode spectateur (OBS_*) : ne jamais le modifier
		pPlayer->pev->iuser1 = 0;
		// Pas : ne pas toucher flTimeStepSound (sync clientdata, overflow SZ_GetSpace)
		pPlayer->pev->max_health = g_RelicBalance.maxHealth;
		pPlayer->pev->health = g_RelicBalance.maxHealth;
		RelicRush_ApplyCarrierModel(pPlayer);
		pPlayer->m_flRelicVisibleUntil = 0.0f;
		pPlayer->m_flNextRelicScream = 0.0f;
		RelicRush_UpdateCarrierStealth(pPlayer);
		RelicRush_ScheduleNextCarrierScream(pPlayer);
		RelicRush_RefreshCarrierHUD(pPlayer, pPlayer->pev->health);
	}
	else
	{
		if (pPlayer->m_flBaseMaxHealth > 0.0f)
			pPlayer->pev->max_health = pPlayer->m_flBaseMaxHealth;
		pPlayer->m_fLongJump = false;
		g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "slj", "0");
		RelicRush_ResetCarrierVisuals(pPlayer);
		pPlayer->m_bRelicWallClinging = false;
		pPlayer->m_vecRelicWallNormal = g_vecZero;
		if (pPlayer->pev->movetype == MOVETYPE_FLY)
		{
			pPlayer->pev->movetype = MOVETYPE_WALK;
			pPlayer->pev->gravity = 1.0f;
		}
		if (pPlayer->IsAlive())
		{
			RelicRush_RestoreCarrierModel(pPlayer);
			RelicRush_RestorePlayMode(pPlayer);
		}
	}
}
void CRelicRushMultiplay::OnRelicPickedUp(CBasePlayer* pPlayer, CRelicRushRelic* pRelic)
{
	if (!pPlayer || !pPlayer->IsAlive() || m_pCarrier)
		return;

	if (pRelic && pRelic == m_pRelic)
	{
		pRelic->HideForPickup();
		m_pRelic = nullptr;
	}

	pPlayer->m_bPendingRelicCarrier = true;
}
void CRelicRushMultiplay::OnRelicDropped(const Vector& origin)
{
	RelicRush_RemoveRelic(m_pRelic);
	m_pRelic = CRelicRushRelic::CreateAt(origin);
	if (!m_pRelic)
		ALERT(at_warning, "Relic Rush: relique non creee apres drop\n");
}
void CRelicRushMultiplay::PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	const bool bWasCarrier = pVictim && pVictim->m_bHasRelic;
	const Vector dropOrigin = pVictim ? pVictim->pev->origin + Vector(0, 0, 8) : g_vecZero;

	// Retirer la relique tant que le joueur est encore vivant (evite SET_MODEL sur cadavre = crash).
	if (bWasCarrier)
		ClearCarrier(pVictim, false);

	CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);

	if (!bWasCarrier)
		return;

	OnRelicDropped(dropOrigin);
	UTIL_ClientPrintAll(HUD_PRINTCENTER, "La relique est tombee !");
}
int CRelicRushMultiplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	return 0;
}
void CRelicRushMultiplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	CHalfLifeMultiplay::PlayerSpawn(pPlayer);

	// Reset voile vert si la victime respawn pendant l'effet (sinon le ScreenFade
	// reste pose cote client jusqu'au prochain fade naturel).
	if (pPlayer && pPlayer->m_flRelicGooBlindUntil > 0.0f)
	{
		pPlayer->m_flRelicGooBlindUntil = 0.0f;
		pPlayer->m_iRelicGooBlindFaded = 0;
		if (pPlayer->IsNetClient())
			RelicRush_ClearGooBlindClient(pPlayer);
	}

	if (!RelicRush_IsCarrier(pPlayer))
		RelicRush_FinalizeCarrierLoss(pPlayer);
	else
		RelicRush_RestorePlayMode(pPlayer);

	if (!pPlayer->m_bRelicHelpShown)
	{
		pPlayer->m_bRelicHelpShown = true;
		ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY,
			"Relic Rush : console F12/F7/HOME | relique V | vue 3PP F6");
		// Fade out + stop de la musique : gere client-side dans HUD_Frame
		// sur transition GetMaxClients() 0 -> >0 (entree en jeu).
	}
	if (RelicRush_IsCarrier(pPlayer))
	{
		pPlayer->m_bHasRelic = false;
		ApplyCarrierLoadout(pPlayer);
		pPlayer->m_bHasRelic = true;
		ApplyCarrierEffects(pPlayer, true);
		if (pPlayer->m_pActiveItem && pPlayer->m_pActiveItem->m_iId == WEAPON_CROWBAR)
		{
			pPlayer->pev->viewmodel = MAKE_STRING("models/v_knife.mdl");
			pPlayer->pev->weaponmodel = iStringNull;
		}
	}
}
void CRelicRushMultiplay::PlayerThink(CBasePlayer* pPlayer)
{
	CHalfLifeMultiplay::PlayerThink(pPlayer);

	if (pPlayer->m_bPendingRelicCarrier)
		CompleteRelicPickup(pPlayer);

	// Fadeout du voile vert (victimes touchees par la goo) : a calculer pour TOUS les joueurs.
	RelicRush_TickPlayerBlindFade(pPlayer);

	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;

	if (!m_bRoundActive)
		return;

	TickCarrier(pPlayer);
}

void CRelicRushMultiplay::PlayerPostThink(CBasePlayer* pPlayer)
{
	if (!RelicRush_IsCarrier(pPlayer) || !pPlayer->IsAlive())
		return;

	pPlayer->m_fLongJump = false;
	g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "slj", "0");
	RelicRush_UpdateCarrierStealth(pPlayer);
	RelicRush_TickCarrierOverlaySync(pPlayer);
	RelicRush_ApplyPendingCrowbarRush(pPlayer);
	RelicRush_TickWallClimb(pPlayer);

	if (pPlayer->m_bRelicWallClinging != pPlayer->m_bRelicLastSyncWallCling)
		RelicRush_SyncCarrierClient(pPlayer);
}
void CRelicRushMultiplay::TickCarrier(CBasePlayer* pPlayer)
{
	pPlayer->pev->max_health = g_RelicBalance.maxHealth;

	const char* pszModel = g_engfuncs.pfnInfoKeyValue(
		g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model");
	if (!pszModel || stricmp(pszModel, RELIC_CARRIER_USERINFO_MODEL) != 0)
		RelicRush_ReapplyCarrierModel(pPlayer);
	else
		RelicRush_EnsureCarrierNeutralColors(pPlayer);

	RelicRush_UpdateCarrierStealth(pPlayer);
	RelicRush_TickCarrierScreams(pPlayer);

	// Perte de vie passive (rr_regen_interval + rr_regen_amount, ex. 1 HP/s).
	// Seul le siphon sur coups (crowbar) restaure — voir RelicRush_ApplySiphonHeal.
	if (gpGlobals->time >= pPlayer->m_flNextRelicRegen)
	{
		pPlayer->m_flNextRelicRegen = gpGlobals->time + g_RelicBalance.regenInterval;

		const float flDrain = g_RelicBalance.regenAmount;
		if (flDrain > 0.0f && pPlayer->pev->health > 0.0f)
		{
			pPlayer->pev->health = V_max(0.0f, pPlayer->pev->health - flDrain);
			pPlayer->m_iClientHealth = -1;
			pPlayer->UpdateClientData();
			RelicRush_SyncCarrierClient(pPlayer);

			if (pPlayer->pev->health <= 0.0f)
				pPlayer->Killed(pPlayer->pev, 0);
		}
	}

	if (gpGlobals->time >= pPlayer->m_flNextRelicClientSync)
	{
		pPlayer->m_flNextRelicClientSync = gpGlobals->time + 0.75f;
		RelicRush_SyncCarrierClient(pPlayer);
	}

	// Skill clic droit (front montant uniquement).
	if ((pPlayer->m_afButtonPressed & IN_ATTACK2) != 0)
		RelicRush_FireGoo(pPlayer);

	RelicRush_TickCarrierGooSync(pPlayer);
	RelicRush_TickCarrierTrailDecal(pPlayer);
}
bool CRelicRushMultiplay::CanHavePlayerItem(CBasePlayer* pPlayer, CBasePlayerItem* pItem)
{
	if (RelicRush_IsCarrier(pPlayer))
		return false;
	return CHalfLifeMultiplay::CanHavePlayerItem(pPlayer, pItem);
}

bool CRelicRushMultiplay::CanHaveItem(CBasePlayer* pPlayer, CItem* pItem)
{
	if (RelicRush_IsCarrier(pPlayer))
		return false;
	return CHalfLifeMultiplay::CanHaveItem(pPlayer, pItem);
}

bool CRelicRushMultiplay::CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry)
{
	if (RelicRush_IsCarrier(pPlayer))
		return false;
	return CGameRules::CanHaveAmmo(pPlayer, pszAmmoName, iMaxCarry);
}
bool CRelicRushMultiplay::PlayFootstepSounds(CBasePlayer* pl, float fvol)
{
	if (RelicRush_IsCarrier(pl))
		return false;
	return CHalfLifeMultiplay::PlayFootstepSounds(pl, fvol);
}
