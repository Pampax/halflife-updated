/***
 * Black Mesa Relic Rush - halo vert peripherique + teinte verdatre (porteur)
 ***/
#include "hud.h"
#include "cl_util.h"
#include "relicrush_overlay.h"

extern bool g_bRelicCarrierHUD;
extern int g_iRelicCarrierGlowAlpha;

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
	const int r = 24;
	const int g = 220;
	const int b = 72;

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
