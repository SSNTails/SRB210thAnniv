// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief span / column drawer functions, for 8bpp and 16bpp
///
///	All drawing to the view buffer is accomplished in this file.
///	The other refresh files only know about ccordinates,
///	not the architecture of the frame buffer.
///	The frame buffer is a linear one, and we need only the base address.

#include "doomdef.h"
#include "doomstat.h"
#include "r_local.h"
#include "st_stuff.h" // need ST_HEIGHT
#include "i_video.h"
#include "v_video.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"
#include "console.h" // Until buffering gets finished

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// ==========================================================================
//                     COMMON DATA FOR 8bpp AND 16bpp
// ==========================================================================

/**	\brief view info
*/
int viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;

/**	\brief pointer to the start of each line of the screen,
*/
byte *ylookup[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view1 (splitscreen)
*/
byte *ylookup1[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view2 (splitscreen)
*/
byte *ylookup2[MAXVIDHEIGHT*4];

/**	\brief  x byte offset for columns inside the viewwindow,
	so the first column starts at (SCRWIDTH - VIEWWIDTH)/2
*/
int columnofs[MAXVIDWIDTH*4];

byte *topleft;

// =========================================================================
//                      COLUMN DRAWING CODE STUFF
// =========================================================================

lighttable_t *dc_colormap;
int dc_x = 0, dc_yl = 0, dc_yh = 0;

fixed_t dc_iscale, dc_texturemid;
byte dc_hires; // under MSVC boolean is a byte, while on other systems, it a bit,
               // soo lets make it a byte on all system for the ASM code
byte dc_vflip; // Flip image vertically
byte *dc_source;

// -----------------------
// translucency stuff here
// -----------------------
#define NUMTRANSTABLES 9 // how many translucency tables are used

byte *transtables; // translucency tables

/**	\brief R_DrawTransColumn uses this
*/
byte *dc_transmap; // one of the translucency tables

// ----------------------
// translation stuff here
// ----------------------

byte *translationtables[MAXSKINS];
byte *defaulttranslationtables;
byte *bosstranslationtables;

/**	\brief R_DrawTranslatedColumn uses this
*/
byte *dc_translation;

struct r_lightlist_s *dc_lightlist = NULL; /// \todo free leak
int dc_numlights = 0, dc_maxlights, dc_texheight;

// =========================================================================
//                      SPAN DRAWING CODE STUFF
// =========================================================================

int ds_y, ds_x1, ds_x2;
lighttable_t *ds_colormap;
fixed_t ds_xfrac, ds_yfrac, ds_xstep, ds_ystep;

byte *ds_source; // start of a 64*64 tile image
byte *ds_transmap; // one of the translucency tables

/**	\brief Variable flat sizes
*/

ULONG nflatxshift, nflatyshift, nflatshiftup, nflatmask;

// ==========================================================================
//                        OLD DOOM FUZZY EFFECT
// ==========================================================================

// =========================================================================
//                   TRANSLATION COLORMAP CODE
// =========================================================================

const char *Color_Names[MAXSKINCOLORS] =
{
	"None",
	"Grey",
	"Peach",
	"Lavender",
	"Silver",
	"Orange",
	"Red",
	"Blue",
	"Steel_Blue",
	"Pink",
	"Beige",
	"Purple", // By request of Matrixx Hedgehog
	"Green", // REAL green
	"White", // White (also used for fireflower)
	"Gold",
	"Yellow", // By insane popular demand
};

CV_PossibleValue_t Color_cons_t[] = {{0, NULL}, {1, NULL}, {2, NULL}, {3, NULL},
	{4, NULL}, {5, NULL}, {6, NULL}, {7, NULL}, {8, NULL}, {9, NULL}, {10, NULL},
	{11, NULL}, {12, NULL}, {13, NULL}, {14, NULL}, {15, NULL}, {0,NULL}};

/**	\brief the R_LoadSkinTable

	Creates the translation tables to map the green color ramp to
	another ramp (gray, brown, red, ...)

	This is precalculated for drawing the player sprites in the player's
	chosen color
*/

void R_LoadSkinTable(void)
{
	int i;

	for (i = 0; i < MAXSKINS; i++)
		translationtables[i] = Z_MallocAlign (256*(MAXSKINCOLORS-1), PU_STATIC, NULL, 16);
}

/**	\brief The R_InitTranslationTables

  load in translucency tables
*/
void R_InitTranslationTables(void)
{
	int i, j;
	byte bi;

	// Load here the transparency lookup tables 'TINTTAB'
	// NOTE: the TINTTAB resource MUST BE aligned on 64k for the asm
	// optimised code (in other words, transtables pointer low word is 0)
	transtables = Z_MallocAlign(NUMTRANSTABLES*0x10000, PU_STATIC,
		NULL, 16);

	W_ReadLump(W_GetNumForName("TRANS10"), transtables);
	W_ReadLump(W_GetNumForName("TRANS20"), transtables+0x10000);
	W_ReadLump(W_GetNumForName("TRANS30"), transtables+0x20000);
	W_ReadLump(W_GetNumForName("TRANS40"), transtables+0x30000);
	W_ReadLump(W_GetNumForName("TRANS50"), transtables+0x40000);
	W_ReadLump(W_GetNumForName("TRANS60"), transtables+0x50000);
	W_ReadLump(W_GetNumForName("TRANS70"), transtables+0x60000);
	W_ReadLump(W_GetNumForName("TRANS80"), transtables+0x70000);
	W_ReadLump(W_GetNumForName("TRANS90"), transtables+0x80000);

	// The old "default" transtable for thok mobjs and such
	defaulttranslationtables =
		Z_MallocAlign(256*MAXSKINCOLORS, PU_STATIC, NULL, 16);

	// Translate the colors specified by the skin information.
	for (i = 0; i < 256; i++)
	{
		if (i >= 160 && i <= 175)
		{
			bi = (byte)(i & 0xf);

			defaulttranslationtables[i      ] = (byte)(0x10 + bi); // Grey
			defaulttranslationtables[i+  256] = (byte)(0x40 + bi); // Peach // Tails 02-19-2000
			defaulttranslationtables[i+2*256] = (byte)(0xf8 + bi/2); // Lavender
			defaulttranslationtables[i+3*256] = (byte)(0x00 + bi); // silver // tails 02-19-2000
			defaulttranslationtables[i+4*256] = (byte)(0x50 + bi); // orange // tails 02-19-2000
			defaulttranslationtables[i+5*256] = (byte)(0x80 + bi); // light red
			defaulttranslationtables[i+6*256] = (byte)(0xe0 + bi); // light blue

			// Steel blue
			defaulttranslationtables[i+7*256] = (byte)(0xc8 + bi/2);

			defaulttranslationtables[i+8*256] = (byte)(0x90 + bi/2); // Pink
			defaulttranslationtables[i+9*256] = (byte)(0x20 + bi); // Beige

			// Purple
			defaulttranslationtables[i+10*256] = (byte)(0xc0 + bi/2);

			// Green
			defaulttranslationtables[i+11*256] = (byte)(0xa0 + bi);

			// White
			defaulttranslationtables[i+12*256] = (byte)(0x00 + bi/2);

			// Gold
			defaulttranslationtables[i+13*256] = (byte)(0x70 + bi/2);

			// Yellow
			switch (bi)
			{
				case 0:
				case 1:
					defaulttranslationtables[i+14*256] = 0x70;   // yellow
					break;
				case 2:
				case 3:
					defaulttranslationtables[i+14*256] = 0x71;   // yellow
					break;
				case 4:
				case 5:
					defaulttranslationtables[i+14*256] = 0x72;   // yellow
					break;
				case 6:
				case 7:
					defaulttranslationtables[i+14*256] = 0x73;   // yellow
					break;
				case 8:
				case 9:
					defaulttranslationtables[i+14*256] = 0x74;   // yellow
					break;
				case 10:
				case 11:
					defaulttranslationtables[i+14*256] = 0x75;   // yellow
					break;
				case 12:
				case 13:
					defaulttranslationtables[i+14*256] = 0x76;   // yellow
					break;
				default:
					defaulttranslationtables[i+14*256] = 0x77;   // yellow
					break;
			}
		}
		else // Keep other colors as is.
		{
			for (j = 0; j < MAXSKINCOLORS*256; j += 256)
				defaulttranslationtables[i+j] = (byte)i;
		}
	}

	bosstranslationtables = Z_MallocAlign(256, PU_STATIC, NULL, 16);

	for (i = 0; i < 256; i++)
		bosstranslationtables[i] = (byte)i;
	bosstranslationtables[31] = 0; // White!
}

/**	\brief	The R_InitSkinTranslationTables function

	Allow skins to choose which color is translated! Tails 06-07-2002

	\param	starttranscolor	starting color
	\param	endtranscolor	ending color
	\param	skinnum	number of skin

	\return	void


*/
void R_InitSkinTranslationTables(int starttranscolor, int endtranscolor, int skinnum)
{
	int i, j;
	byte bi;

	// Translate the colors specified by the skin information.
	for (i = 0; i < 256; i++)
	{
		if (i >= starttranscolor && i <= endtranscolor)
		{
			bi = (byte)((i - starttranscolor) & 0xf);

			translationtables[skinnum][i      ] = (byte)(0x10 + bi); // Grey
			translationtables[skinnum][i+  256] = (byte)(0x40 + bi); // Peach // Tails 02-19-2000
			translationtables[skinnum][i+2*256] = (byte)(0xf8 + bi/2); // Lavender
			translationtables[skinnum][i+3*256] = (byte)(0x00 + bi); // silver // tails 02-19-2000
			translationtables[skinnum][i+4*256] = (byte)(0x50 + bi); // orange // tails 02-19-2000
			translationtables[skinnum][i+5*256] = (byte)(0x80 + bi); // light red
			translationtables[skinnum][i+6*256] = (byte)(0xe0 + bi); // light blue

			// Steel blue
			translationtables[skinnum][i+7*256] = (byte)(0xc8 + bi/2);

			translationtables[skinnum][i+8*256] = (byte)(0x90 + bi/2); // Pink
			translationtables[skinnum][i+9*256] = (byte)(0x20 + bi); // Beige

			// Purple
			translationtables[skinnum][i+10*256] = (byte)(0xc0 + bi/2);

			// Green
			translationtables[skinnum][i+11*256] = (byte)(0xa0 + bi);

			// White
			translationtables[skinnum][i+12*256] = (byte)(0x00 + bi/2);

			// Gold
			translationtables[skinnum][i+13*256] = (byte)(0x70 + bi/2);

			// Yellow
			switch (bi)
			{
				case 0:
					translationtables[skinnum][i+14*256] = 97;
					break;
				case 1:
					translationtables[skinnum][i+14*256] = 98;   // yellow
					break;
				case 2:
					translationtables[skinnum][i+14*256] = 99; // yellow
					break;
				case 3:
					translationtables[skinnum][i+14*256] = 100;   // yellow
					break;
				case 4:
					translationtables[skinnum][i+14*256] = 101; // yellow
					break;
				case 5:
					translationtables[skinnum][i+14*256] = 102;   // yellow
					break;
				case 6:
					translationtables[skinnum][i+14*256] = 103; // yellow
					break;
				case 7:
					translationtables[skinnum][i+14*256] = 104;   // yellow
					break;
				case 8:
				case 9:
					translationtables[skinnum][i+14*256] = 113;   // yellow
					break;
				case 10:
					translationtables[skinnum][i+14*256] = 114;   // yellow
					break;
				case 11:
				case 12:
				case 13:
					translationtables[skinnum][i+14*256] = 115;   // yellow
					break;
				case 15:
					translationtables[skinnum][i+14*256] = 117; // yellow
					break;
				default:
					translationtables[skinnum][i+14*256] = 116;   // yellow
					break;
			}
		}
		else // Keep other colors as-is.
		{
			// NOTE: Why MAXSKINCOLORS-1 here, but MAXSKINCOLORS
			// in the above R_InitTranslationTables?
			for (j = 0; j < (MAXSKINCOLORS-1) * 256; j += 256)
				translationtables[skinnum][i+j] = (byte)i;
		}
	}
}

// ==========================================================================
//               COMMON DRAWER FOR 8 AND 16 BIT COLOR MODES
// ==========================================================================

// in a perfect world, all routines would be compatible for either mode,
// and optimised enough
//
// in reality, the few routines that can work for either mode, are
// put here

/**	\brief	The R_InitViewBuffer function

	Creates lookup tables for getting the framebuffer address
	of a pixel to draw.

	\param	width	witdh of buffer
	\param	height	hieght of buffer

	\return	void


*/

void R_InitViewBuffer(int width, int height)
{
	int i, bytesperpixel = vid.bpp;

	if (width > MAXVIDWIDTH)
		width = MAXVIDWIDTH;
	if (height > MAXVIDHEIGHT)
		height = MAXVIDHEIGHT;
	if (bytesperpixel < 1 || bytesperpixel > 4)
		I_Error("R_InitViewBuffer: wrong bytesperpixel value %d\n", bytesperpixel);

	// Handle resize, e.g. smaller view windows with border and/or status bar.
	viewwindowx = (vid.width - width) >> 1;

	// Column offset for those columns of the view window, but relative to the entire screen
	for (i = 0; i < width; i++)
		columnofs[i] = (viewwindowx + i) * bytesperpixel;

	// Same with base row offset.
	if (width == vid.width)
		viewwindowy = 0;
	else
		viewwindowy = (vid.height - height) >> 1;

	// Precalculate all row offsets.
	for (i = 0; i < height; i++)
	{
		ylookup[i] = ylookup1[i] = screens[0] + (i+viewwindowy)*vid.width*bytesperpixel;
		ylookup2[i] = screens[0] + (i+(vid.height>>1))*vid.width*bytesperpixel; // for splitscreen
	}
}

/**	\brief viewborder patches lump numbers
*/
lumpnum_t viewborderlump[8];

/**	\brief Store the lumpnumber of the viewborder patches
*/

void R_InitViewBorder(void)
{
	viewborderlump[BRDR_T] = W_GetNumForName("brdr_t");
	viewborderlump[BRDR_B] = W_GetNumForName("brdr_b");
	viewborderlump[BRDR_L] = W_GetNumForName("brdr_l");
	viewborderlump[BRDR_R] = W_GetNumForName("brdr_r");
	viewborderlump[BRDR_TL] = W_GetNumForName("brdr_tl");
	viewborderlump[BRDR_BL] = W_GetNumForName("brdr_bl");
	viewborderlump[BRDR_TR] = W_GetNumForName("brdr_tr");
	viewborderlump[BRDR_BR] = W_GetNumForName("brdr_br");
}

#if 0
/**	\brief R_FillBackScreen

	Fills the back screen with a pattern for variable screen sizes
	Also draws a beveled edge.
*/
void R_FillBackScreen(void)
{
	byte *src, *dest;
	patch_t *patch;
	int x, y, step, boff;

	// quickfix, don't cache lumps in both modes
	if (rendermode != render_soft)
		return;

	// draw pattern around the status bar too (when hires),
	// so return only when in full-screen without status bar.
	if (scaledviewwidth == vid.width && viewheight == vid.height)
		return;

	src = scr_borderpatch;
	dest = screens[1];

	for (y = 0; y < vid.height; y++)
	{
		for (x = 0; x < vid.width/128; x++)
		{
			M_Memcpy (dest, src+((y&127)<<7), 128);
			dest += 128;
		}

		if (vid.width&127)
		{
			M_Memcpy(dest, src+((y&127)<<7), vid.width&127);
			dest += (vid.width&127);
		}
	}

	// don't draw the borders when viewwidth is full vid.width.
	if (scaledviewwidth == vid.width)
		return;

	step = 8;
	boff = 8;

	patch = W_CacheLumpNum(viewborderlump[BRDR_T], PU_CACHE);
	for (x = 0; x < scaledviewwidth; x += step)
		V_DrawPatch(viewwindowx + x, viewwindowy - boff, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_B], PU_CACHE);
	for (x = 0; x < scaledviewwidth; x += step)
		V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_L], PU_CACHE);
	for (y = 0; y < viewheight; y += step)
		V_DrawPatch(viewwindowx - boff, viewwindowy + y, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_R],PU_CACHE);
	for (y = 0; y < viewheight; y += step)
		V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, 1,
			patch);

	// Draw beveled corners.
	V_DrawPatch(viewwindowx - boff, viewwindowy - boff, 1,
		W_CacheLumpNum(viewborderlump[BRDR_TL], PU_CACHE));
	V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - boff, 1,
		W_CacheLumpNum(viewborderlump[BRDR_TR], PU_CACHE));
	V_DrawPatch(viewwindowx - boff, viewwindowy + viewheight, 1,
		W_CacheLumpNum(viewborderlump[BRDR_BL], PU_CACHE));
	V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, 1,
		W_CacheLumpNum(viewborderlump[BRDR_BR], PU_CACHE));
}
#endif

/**	\brief	The R_VideoErase function

	Copy a screen buffer.

	\param	ofs	offest from buffer
	\param	count	bytes to erase

	\return	void


*/
void R_VideoErase(unsigned ofs, int count)
{
	// LFB copy.
	// This might not be a good idea if memcpy
	//  is not optimal, e.g. byte by byte on
	//  a 32bit CPU, as GNU GCC/Linux libc did
	//  at one point.
	M_Memcpy(screens[0] + ofs, screens[1] + ofs, count);
}

#if 0
/**	\brief The R_DrawViewBorder

  Draws the border around the view
	for different size windows?
*/
void R_DrawViewBorder(void)
{
	int top, side, ofs;

	if (rendermode == render_none)
		return;
#ifdef HWRENDER
	if (rendermode != render_soft)
	{
		HWR_DrawViewBorder(0);
		return;
	}
	else
#endif

#ifdef DEBUG
	fprintf(stderr,"RDVB: vidwidth %d vidheight %d scaledviewwidth %d viewheight %d\n",
		vid.width, vid.height, scaledviewwidth, viewheight);
#endif

	if (scaledviewwidth == vid.width)
		return;

	top = (vid.height - viewheight)>>1;
	side = (vid.width - scaledviewwidth)>>1;

	// copy top and one line of left side
	R_VideoErase(0, top*vid.width+side);

	// copy one line of right side and bottom
	ofs = (viewheight+top)*vid.width - side;
	R_VideoErase(ofs, top*vid.width + side);

	// copy sides using wraparound
	ofs = top*vid.width + vid.width-side;
	side <<= 1;

    // simpler using our VID_Blit routine
	VID_BlitLinearScreen(screens[1] + ofs, screens[0] + ofs, side, viewheight - 1,
		vid.width, vid.width);
}
#endif

// ==========================================================================
//                   INCLUDE 8bpp DRAWING CODE HERE
// ==========================================================================

#include "r_draw8.c"

// ==========================================================================
//                   INCLUDE 16bpp DRAWING CODE HERE
// ==========================================================================

#include "r_draw16.c"
