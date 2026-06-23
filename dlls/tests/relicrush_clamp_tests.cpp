/***
 * Black Mesa Relic Rush - tests de RR_ClampValue / RRClampWarnTracker (relicrush_clamp.h).
 *
 * Executable autonome, sans dependance au moteur Half-Life : compile et tourne
 * sur l'hote (pas besoin de hl.exe/HLDS). Code de sortie != 0 si un test echoue.
 ***/
#include "../relicrush_clamp.h"

#include <cstdio>

static int g_failures = 0;

static void Check(bool condition, const char* description)
{
	if (condition)
	{
		std::printf("[ OK ] %s\n", description);
		return;
	}
	std::printf("[FAIL] %s\n", description);
	g_failures++;
}

static void TestClampValue()
{
	Check(RR_ClampValue(100.0f, 50.0f, 500.0f) == 100.0f, "valeur dans les bornes inchangee");
	Check(RR_ClampValue(0.0f, 50.0f, 500.0f) == 50.0f, "valeur trop basse clampee au minimum");
	Check(RR_ClampValue(-1000.0f, 50.0f, 500.0f) == 50.0f, "valeur tres negative clampee au minimum");
	Check(RR_ClampValue(99999.0f, 50.0f, 500.0f) == 500.0f, "valeur trop haute clampee au maximum");
	Check(RR_ClampValue(50.0f, 50.0f, 500.0f) == 50.0f, "borne min incluse (pas clampee a tort)");
	Check(RR_ClampValue(500.0f, 50.0f, 500.0f) == 500.0f, "borne max incluse (pas clampee a tort)");

	// Cas reel : rr_sound_max < rr_sound_min (cf. relicrush_config.cpp) n'est pas gere par
	// RR_ClampValue lui-meme (chaque cvar est clampee independamment) ; le reglage croise
	// est corrige ensuite dans RelicRush_RefreshBalance(). On verifie juste le clamp simple ici.
	Check(RR_ClampValue(3.0f, 0.5f, 120.0f) == 3.0f, "rr_sound_min valide reste inchangee");
}

static void TestWarnTrackerDedup()
{
	RRClampWarnTracker tracker;

	Check(!tracker.AlreadyWarned("rr_maxhealth"), "pas encore averti au depart");

	tracker.MarkWarned("rr_maxhealth");
	Check(tracker.AlreadyWarned("rr_maxhealth"), "averti apres MarkWarned");
	Check(tracker.AlreadyWarned("RR_MAXHEALTH"), "dedup insensible a la casse");
	Check(!tracker.AlreadyWarned("rr_regen_amount"), "un autre nom n'est pas affecte");

	tracker.Clear();
	Check(!tracker.AlreadyWarned("rr_maxhealth"), "Clear() reinitialise la dedup");
}

static void TestWarnTrackerCapacity()
{
	// Toutes les cvars rr_* actuelles (34 au 2026-06-23) doivent pouvoir etre
	// deduppliquees simultanement : RR_CLAMP_WARN_SLOTS doit rester au-dessus de ce compte
	// (regression testee ici : avec 12 slots seulement, ce test echouait).
	RRClampWarnTracker tracker;
	const int kCvarCount = 34;
	char name[32];

	for (int i = 0; i < kCvarCount; i++)
	{
		std::snprintf(name, sizeof(name), "rr_fake_cvar_%02d", i);
		tracker.MarkWarned(name);
	}

	bool allDistinctNamesTracked = true;
	for (int i = 0; i < kCvarCount; i++)
	{
		std::snprintf(name, sizeof(name), "rr_fake_cvar_%02d", i);
		if (!tracker.AlreadyWarned(name))
			allDistinctNamesTracked = false;
	}
	Check(allDistinctNamesTracked, "RR_CLAMP_WARN_SLOTS couvre toutes les cvars rr_* sans perdre la dedup");
}

int main()
{
	TestClampValue();
	TestWarnTrackerDedup();
	TestWarnTrackerCapacity();

	if (g_failures == 0)
	{
		std::printf("\nrelicrush_clamp_tests: OK\n");
		return 0;
	}
	std::printf("\nrelicrush_clamp_tests: %d echec(s)\n", g_failures);
	return 1;
}
