/***
 * Black Mesa Relic Rush - logique de clamp des cvars, isolee du moteur GoldSrc.
 ***/
#include "relicrush_clamp.h"

#include <cctype>
#include <cstring>

namespace
{
	// Equivalent local de stricmp (Platform.h tire steam/steamtypes.h, evite ici
	// pour garder ce fichier compilable sans le SDK Half-Life).
	bool NamesEqual(const char* a, const char* b)
	{
		while (*a && *b)
		{
			if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b)))
				return false;
			a++;
			b++;
		}
		return *a == *b;
	}
} // namespace

float RR_ClampValue(float value, float minVal, float maxVal)
{
	if (value < minVal)
		return minVal;
	if (value > maxVal)
		return maxVal;
	return value;
}

bool RRClampWarnTracker::AlreadyWarned(const char* name) const
{
	for (int i = 0; i < RR_CLAMP_WARN_SLOTS; i++)
	{
		if (names[i][0] == '\0')
			return false;
		if (NamesEqual(names[i], name))
			return true;
	}
	return false;
}

void RRClampWarnTracker::MarkWarned(const char* name)
{
	for (int i = 0; i < RR_CLAMP_WARN_SLOTS; i++)
	{
		if (names[i][0] == '\0' || NamesEqual(names[i], name))
		{
			strncpy(names[i], name, RR_CLAMP_WARN_NAME_LEN - 1);
			names[i][RR_CLAMP_WARN_NAME_LEN - 1] = '\0';
			return;
		}
	}
	// Tous les slots sont pris par des noms differents : on ne deduplique plus
	// (cas qui ne devrait pas arriver tant que RR_CLAMP_WARN_SLOTS >= nb. de cvars).
}

void RRClampWarnTracker::Clear()
{
	for (int i = 0; i < RR_CLAMP_WARN_SLOTS; i++)
		names[i][0] = '\0';
}
