/***
 * Black Mesa Relic Rush - halo vert peripherique + teinte verdatre (porteur)
 ***/
#include "hud.h"
#include "cl_util.h"
#include "relicrush_overlay.h"

extern bool g_bRelicCarrierHUD;
extern int g_iRelicCarrierGlowAlpha;
extern int g_iRelicGooCooldownPct;

static void RelicRush_FillVignetteH(int x, int y, int wide, int tall, int r, int g, int b, int alphaNear, int alphaFar)
{
	if (wide <= 0 || tall <= 0)
		return;

	for (int i = 0; i < tall; i++)
	{
		const int a = alphaFar + ((alphaNear - alphaFar) * i) / V_max(1, tall - 1);
		if (a > 0)
			FillRGBA(x, y + i, wide, 1, r, g, b, a);
	}
}

static void RelicRush_FillVignetteV(int x, int y, int wide, int tall, int r, int g, int b, int alphaNear, int alphaFar)
{
	if (wide <= 0 || tall <= 0)
		return;

	for (int i = 0; i < wide; i++)
	{
		const int a = alphaFar + ((alphaNear - alphaFar) * i) / V_max(1, wide - 1);
		if (a > 0)
			FillRGBA(x + i, y, 1, tall, r, g, b, a);
	}
}

void RelicRush_DrawCarrierVisionOverlay(float flTime)
{
	if (!g_bRelicCarrierHUD || g_iRelicCarrierGlowAlpha <= 0)
		return;
	if (0 != gEngfuncs.IsSpectateOnly())
		return;
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) != 0)
		return;

	const int w = ScreenWidth;
	const int h = ScreenHeight;
	if (w < 64 || h < 48)
		return;

	const int border = V_max(12, XRES(36));
	const int r = 0;
	const int g = 255;
	const int b = 0;

	// Synchronise avec le glow du skin (visible / fondu / invisible)
	const float glowScale = g_iRelicCarrierGlowAlpha / 255.0f;

	// Legere pulsation du glow (effet vivant)
	const float pulse = (0.85f + 0.15f * (0.5f + 0.5f * sinf(flTime * 2.2f))) * glowScale;
	const int edgeAlpha = (int)(110.0f * pulse);
	const int innerAlpha = 0;

	// Teinte verdatre sur toute la vue (tres legere)
	const int washAlpha = (int)(22.0f * pulse);
	FillRGBA(0, 0, w, h, 16, 72, 32, washAlpha);

	// Bords haut / bas (alpha fort au bord ecran)
	RelicRush_FillVignetteH(0, 0, w, border, r, g, b, edgeAlpha, innerAlpha);
	RelicRush_FillVignetteH(0, h - border, w, border, r, g, b, edgeAlpha, innerAlpha);

	// Bords gauche / droit
	RelicRush_FillVignetteV(0, 0, border, h, r, g, b, edgeAlpha, innerAlpha);
	RelicRush_FillVignetteV(w - border, 0, border, h, r, g, b, edgeAlpha, innerAlpha);

	// Coins renforces (glow autour de la fenetre)
	const int corner = V_max(8, XRES(20));
	const int cornerAlpha = (int)(140.0f * pulse);
	FillRGBA(0, 0, corner, corner, r, g, b, cornerAlpha);
	FillRGBA(w - corner, 0, corner, corner, r, g, b, cornerAlpha);
	FillRGBA(0, h - corner, corner, corner, r, g, b, cornerAlpha);
	FillRGBA(w - corner, h - corner, corner, corner, r, g, b, cornerAlpha);
}

// Barre de charge / cooldown du skill goo (clic droit). Visible uniquement pour le porteur.
void RelicRush_DrawGooCooldownBar(float flTime)
{
	if (!g_bRelicCarrierHUD)
		return;
	if (0 != gEngfuncs.IsSpectateOnly())
		return;
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) != 0)
		return;

	const int w = ScreenWidth;
	const int h = ScreenHeight;
	if (w < 64 || h < 48)
		return;

	const int barW = V_max(120, w / 4);
	const int barH = V_max(6, YRES(10));
	const int barX = (w - barW) / 2;
	const int barY = h - V_max(40, YRES(56));

	// Fond noir semi-transparent + liseré vert sombre.
	FillRGBA(barX - 2, barY - 2, barW + 4, barH + 4, 0, 48, 0, 180);
	FillRGBA(barX, barY, barW, barH, 0, 0, 0, 200);

	const int pct = (g_iRelicGooCooldownPct < 0) ? 0 : ((g_iRelicGooCooldownPct > 100) ? 100 : g_iRelicGooCooldownPct);
	const int fillW = (barW * pct) / 100;

	// Remplissage vert : sombre tant que ca charge, vif quand pret (100%).
	int rC, gC, bC, aC;
	if (pct >= 100)
	{
		// Pulsation legere quand pret.
		const float pulse = 0.85f + 0.15f * (0.5f + 0.5f * sinf(flTime * 6.0f));
		rC = 0;
		gC = (int)(255 * pulse);
		bC = 0;
		aC = 235;
	}
	else
	{
		rC = 0;
		gC = 200;
		bC = 0;
		aC = 220;
	}

	if (fillW > 0)
		FillRGBA(barX, barY, fillW, barH, rC, gC, bC, aC);

	// Marque centrale (subdivision visuelle, type "tick"). Optionnel mais lisible.
	const int tickH = barH / 2;
	const int tickY = barY + (barH - tickH) / 2;
	FillRGBA(barX + barW / 2 - 1, tickY, 2, tickH, 0, 96, 0, 120);
}

static bool s_bCarrierCrossOnTarget = false;

// Croix verte en X (diagonale) au centre de l'ecran.
void RelicRush_DrawCarrierCrosshair(float flTime)
{
	(void)flTime;
	if (!g_bRelicCarrierHUD)
		return;
	if (0 != gEngfuncs.IsSpectateOnly())
		return;
	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	const int w = ScreenWidth;
	const int h = ScreenHeight;
	if (w < 64 || h < 48)
		return;

	const int cx = w / 2;
	const int cy = h / 2;
	const int arm = V_max(6, XRES(10));
	const int thick = V_max(1, YRES(1));
	const int alpha = 50; // opacite

	for (int d = -arm; d <= arm; d++)
	{
		FillRGBA(cx + d - thick / 2, cy + d - thick / 2, thick, thick, 0, 255, 0, alpha);
		FillRGBA(cx + d - thick / 2, cy - d - thick / 2, thick, thick, 0, 255, 0, alpha);
	}
}

// Masque le reticule d'arme ; le X vert est dessine dans hud_redraw.
void RelicRush_ApplyCarrierCrosshair(bool bOnTarget)
{
	s_bCarrierCrossOnTarget = bOnTarget;

	if (!g_bRelicCarrierHUD)
		return;
	if (0 != gEngfuncs.IsSpectateOnly())
		return;
	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	static Rect nullrc;
	SetCrosshair(0, nullrc, 0, 0, 0);
}
