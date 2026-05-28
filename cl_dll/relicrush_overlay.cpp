/***
 * Black Mesa Relic Rush - halo vert peripherique + teinte verdatre (porteur)
 * + voile poison poison (grosses taches vertes pleines)
 ***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "relicrush_overlay.h"
#include <math.h>

extern bool g_bRelicCarrierHUD;
extern int g_iRelicCarrierGlowAlpha;
extern int g_iRelicPoisonCooldownPct;

// --- Voile poison poison (taches vertes opaques, pas de ScreenFade plein ecran) ---

#define RELIC_POISON_VEIL_BLOBS_MAX 24
#define RELIC_POISON_VEIL_MIN_PX 3

struct RelicPoisonVeilBlob
{
	float cx, cy; // centre normalise 0..1
	float rx, ry; // demi-axes normalises
};

// Gabarit des taches (rayons normalises) ; rr_poison_veil_blob_scale les redimensionne.
static const RelicPoisonVeilBlob kRelicPoisonVeilBlobs[RELIC_POISON_VEIL_BLOBS_MAX] =
{
	{0.12f, 0.10f, 0.07f, 0.06f},
	{0.38f, 0.06f, 0.08f, 0.06f},
	{0.72f, 0.14f, 0.07f, 0.06f},
	{0.90f, 0.32f, 0.06f, 0.07f},
	{0.06f, 0.42f, 0.07f, 0.07f},
	{0.30f, 0.38f, 0.08f, 0.08f},
	{0.58f, 0.48f, 0.09f, 0.08f},
	{0.82f, 0.55f, 0.07f, 0.07f},
	{0.18f, 0.68f, 0.08f, 0.06f},
	{0.48f, 0.72f, 0.09f, 0.07f},
	{0.74f, 0.78f, 0.07f, 0.06f},
	{0.04f, 0.82f, 0.06f, 0.05f},
	{0.92f, 0.88f, 0.07f, 0.05f},
	{0.50f, 0.22f, 0.09f, 0.08f},
	{0.22f, 0.52f, 0.06f, 0.05f},
	{0.65f, 0.28f, 0.07f, 0.06f},
	{0.42f, 0.58f, 0.06f, 0.05f},
	{0.86f, 0.72f, 0.06f, 0.05f},
	{0.28f, 0.18f, 0.06f, 0.05f},
	{0.56f, 0.12f, 0.07f, 0.06f},
	{0.08f, 0.58f, 0.06f, 0.05f},
	{0.94f, 0.48f, 0.05f, 0.05f},
	{0.36f, 0.86f, 0.07f, 0.06f},
	{0.62f, 0.64f, 0.06f, 0.05f},
};

static bool s_bPoisonVeilActive = false;
static float s_flPoisonVeilStart = 0.0f;
static float s_flPoisonVeilFadeIn = 0.3f;
static float s_flPoisonVeilHold = 3.0f;
static float s_flPoisonVeilFadeOut = 1.0f;
static int s_iPoisonVeilPeakAlpha = 255;
static int s_iPoisonVeilR = 0;
static int s_iPoisonVeilG = 255;
static int s_iPoisonVeilB = 0;
static float s_flPoisonVeilBlobScale = 0.55f;
static int s_iPoisonVeilBlobCount = 18;

static void RelicRush_FillBlobEllipse(int cx, int cy, int rx, int ry, int r, int g, int b, int a)
{
	if (a <= 0 || rx <= 0 || ry <= 0)
		return;

	const float invRy2 = 1.0f / (float)(ry * ry);
	for (int dy = -ry; dy <= ry; dy++)
	{
		const float t = (float)(dy * dy) * invRy2;
		if (t > 1.0f)
			continue;
		const int dx = (int)((float)rx * sqrtf(1.0f - t));
		if (dx > 0)
			FillRGBA(cx - dx, cy + dy, dx * 2, 1, r, g, b, a);
	}
}

void RelicRush_ResetPoisonVeilOverlay()
{
	s_bPoisonVeilActive = false;
	s_flPoisonVeilStart = 0.0f;
}

void RelicRush_OnPoisonVeilMessage(int iSize, void* pbuf)
{
	if (iSize < 1)
		return;

	BEGIN_READ(pbuf, iSize);
	const int event = READ_BYTE();
	if (event == 0)
	{
		RelicRush_ResetPoisonVeilOverlay();
		return;
	}

	if (iSize < 8)
		return;

	const int fadeInTenths = READ_BYTE();
	const int holdSec = READ_BYTE();
	const int fadeOutTenths = READ_BYTE();
	s_iPoisonVeilPeakAlpha = READ_BYTE();
	s_iPoisonVeilR = READ_BYTE();
	s_iPoisonVeilG = READ_BYTE();
	s_iPoisonVeilB = READ_BYTE();

	if (iSize >= 10)
	{
		const int scalePct = READ_BYTE();
		const int blobCount = READ_BYTE();
		s_flPoisonVeilBlobScale = V_max(0.15f, scalePct * 0.01f);
		s_iPoisonVeilBlobCount = blobCount < 1 ? 1 : (blobCount > RELIC_POISON_VEIL_BLOBS_MAX ? RELIC_POISON_VEIL_BLOBS_MAX : blobCount);
	}
	else
	{
		s_flPoisonVeilBlobScale = 0.55f;
		s_iPoisonVeilBlobCount = 18;
	}

	s_flPoisonVeilFadeIn = V_max(0.05f, fadeInTenths * 0.1f);
	s_flPoisonVeilHold = V_max(0.0f, (float)holdSec);
	s_flPoisonVeilFadeOut = V_max(0.05f, fadeOutTenths * 0.1f);
	if (s_iPoisonVeilPeakAlpha < 1)
		s_iPoisonVeilPeakAlpha = 1;
	if (s_iPoisonVeilPeakAlpha > 255)
		s_iPoisonVeilPeakAlpha = 255;

	s_flPoisonVeilStart = gEngfuncs.GetClientTime();
	s_bPoisonVeilActive = true;
}

void RelicRush_DrawPoisonVeilOverlay(float flTime)
{
	if (!s_bPoisonVeilActive)
		return;
	if (0 != gEngfuncs.IsSpectateOnly())
		return;
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) != 0)
		return;

	const int w = ScreenWidth;
	const int h = ScreenHeight;
	if (w < 64 || h < 48)
		return;

	const float elapsed = flTime - s_flPoisonVeilStart;
	const float total = s_flPoisonVeilFadeIn + s_flPoisonVeilHold + s_flPoisonVeilFadeOut;

	float coverage = 0.0f;
	if (elapsed < s_flPoisonVeilFadeIn)
		coverage = elapsed / s_flPoisonVeilFadeIn;
	else if (elapsed < s_flPoisonVeilFadeIn + s_flPoisonVeilHold)
		coverage = 1.0f;
	else if (elapsed < total)
		coverage = 1.0f - (elapsed - s_flPoisonVeilFadeIn - s_flPoisonVeilHold) / s_flPoisonVeilFadeOut;
	else
	{
		RelicRush_ResetPoisonVeilOverlay();
		return;
	}

	int alpha = (int)(s_iPoisonVeilPeakAlpha * coverage);
	// Pleine opacité dès que le voile est établi (taches bien pleines, pas voilées).
	if (coverage >= 1.0f)
		alpha = s_iPoisonVeilPeakAlpha;
	if (alpha <= 0)
		return;

	const float scale = s_flPoisonVeilBlobScale;
	const int nBlobs = s_iPoisonVeilBlobCount;

	for (int i = 0; i < nBlobs; i++)
	{
		const RelicPoisonVeilBlob& b = kRelicPoisonVeilBlobs[i];
		const int cx = (int)(b.cx * (float)w);
		const int cy = (int)(b.cy * (float)h);
		const int rx = V_max(RELIC_POISON_VEIL_MIN_PX, (int)(b.rx * (float)w * scale));
		const int ry = V_max(RELIC_POISON_VEIL_MIN_PX, (int)(b.ry * (float)h * scale));
		RelicRush_FillBlobEllipse(cx, cy, rx, ry, s_iPoisonVeilR, s_iPoisonVeilG, s_iPoisonVeilB, alpha);
	}
}

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

// Barre de charge / cooldown du skill poison (clic droit). Visible uniquement pour le porteur.
void RelicRush_DrawPoisonCooldownBar(float flTime)
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

	const int pct = (g_iRelicPoisonCooldownPct < 0) ? 0 : ((g_iRelicPoisonCooldownPct > 100) ? 100 : g_iRelicPoisonCooldownPct);
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
