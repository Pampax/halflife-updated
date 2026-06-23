/***
 * Black Mesa Relic Rush - logique de clamp des cvars, isolee du moteur GoldSrc.
 *
 * Volontairement libre de toute dependance engine (pas d'ALERT, pas de cvar_t)
 * pour pouvoir etre compilee et testee a part (voir dlls/tests/relicrush_clamp_tests.cpp),
 * sans avoir a linker hl.dll ni a lancer le moteur Half-Life.
 ***/
#pragma once

// Nombre de cvars distinctes dont on peut deduper l'avertissement de clamp en simultane.
// Doit rester >= au nombre de cvars enregistrees dans relicrush_config.cpp (34 au 2026-06-23) ;
// sinon les cvars excedentaires perdent la dedup et spamment la console a chaque RefreshBalance.
#define RR_CLAMP_WARN_SLOTS 48
#define RR_CLAMP_WARN_NAME_LEN 32

// Ramene value dans [minVal, maxVal]. Pure, sans effet de bord.
float RR_ClampValue(float value, float minVal, float maxVal);

// Dedup des avertissements de clamp par nom de cvar ("avertir une seule fois par nom").
struct RRClampWarnTracker
{
	char names[RR_CLAMP_WARN_SLOTS][RR_CLAMP_WARN_NAME_LEN] = {};

	bool AlreadyWarned(const char* name) const;
	void MarkWarned(const char* name);
	void Clear();
};
