// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
/// \brief miscellaneous drawing (mainly 2d)

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "hw_glob.h"
#include "hw_drv.h"

#include "../m_misc.h" //FIL_WriteFile()
#include "../r_draw.h" //viewborderlump
#include "../r_main.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../st_stuff.h"

#ifndef UNIXLIKE // unix does not need this 19991024 by Kin
#ifdef __GNUC__
#include <sys/unistd.h>
#endif
#ifndef macintosh
#include <io.h>
#endif
#else
#endif // normalunix
#include <fcntl.h>
#include "../i_video.h"  // for rendermode != render_glide

#ifndef O_BINARY
#define O_BINARY 0
#endif

float gr_patch_scalex;
float gr_patch_scaley;

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct
{
	char id_field_length; // 1
	char color_map_type ; // 2
	char image_type     ; // 3
	char dummy[5]       ; // 4,  8
	short x_origin      ; // 9, 10
	short y_origin      ; //11, 12
	short width         ; //13, 14
	short height        ; //15, 16
	char image_pix_size ; //17
	char image_descriptor; //18
} ATTRPACK TGAHeader; // sizeof is 18
#if defined(_MSC_VER)
#pragma pack()
#endif
typedef unsigned char GLRGB[3];

#define BLENDMODE PF_Translucent

//
// -----------------+
// HWR_DrawPatch    : Draw a 'tile' graphic
// Notes            : x,y : positions relative to the original Doom resolution
//                  : textes(console+score) + menus + status bar
// -----------------+
void HWR_DrawPatch(GlidePatch_t *gpatch, int x, int y, int option)
{
	FOutVector v[4];
	int flags;

//  3--2
//  | /|
//  |/ |
//  0--1
	float sdupx = vid.fdupx*2;
	float sdupy = vid.fdupy*2;
	float pdupx = vid.fdupx*2;
	float pdupy = vid.fdupy*2;

	// make patch ready in hardware cache
	HWR_GetPatch(gpatch);

	if (option & V_NOSCALEPATCH)
		pdupx = pdupy = 2.0f;
	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x = (x*sdupx-gpatch->leftoffset*pdupx)/vid.width - 1;
	v[2].x = v[1].x = (x*sdupx+(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
	v[0].y = v[1].y = 1-(y*sdupy-gpatch->topoffset*pdupy)/vid.height;
	v[2].y = v[3].y = 1-(y*sdupy+(gpatch->height-gpatch->topoffset)*pdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = gpatch->max_s;
	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = gpatch->max_t;

	flags = BLENDMODE|PF_Clip|PF_NoZClip|PF_NoDepthTest;

	if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;

	// clip it since it is used for bunny scroll in doom I
	if (option & V_TRANSLUCENT)
	{
		FSurfaceInfo Surf;
		Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
		Surf.FlatColor.s.alpha = (byte)cv_grtranslucenthud.value;
		flags |= PF_Modulated;
		HWD.pfnDrawPolygon(&Surf, v, 4, flags);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

void HWR_DrawClippedPatch (GlidePatch_t *gpatch, int x, int y, int option)
{
	// hardware clips the patch quite nicely anyway :)
	HWR_DrawPatch(gpatch, x, y, option); /// \todo do real cliping
}

// Only supports one kind of translucent for now. Tails 06-12-2003
// Boked
// Alam_GBC: Why? you could not get a FSurfaceInfo to set the alpha channel?
void HWR_DrawTranslucentPatch (GlidePatch_t *gpatch, int x, int y, int option)
{
	FOutVector      v[4];
	int flags;

//  3--2
//  | /|
//  |/ |
//  0--1
	float sdupx = vid.fdupx*2;
	float sdupy = vid.fdupy*2;
	float pdupx = vid.fdupx*2;
	float pdupy = vid.fdupy*2;
	FSurfaceInfo Surf;

	// make patch ready in hardware cache
	HWR_GetPatch (gpatch);

	if (option & V_NOSCALEPATCH)
		pdupx = pdupy = 2.0f;
	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x = (x*sdupx-gpatch->leftoffset*pdupx)/vid.width - 1;
	v[2].x = v[1].x = (x*sdupx+(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
	v[0].y = v[1].y = 1-(y*sdupy-gpatch->topoffset*pdupy)/vid.height;
	v[2].y = v[3].y = 1-(y*sdupy+(gpatch->height-gpatch->topoffset)*pdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = gpatch->max_s;
	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = gpatch->max_t;

	flags = PF_Modulated | BLENDMODE | PF_Clip | PF_NoZClip | PF_NoDepthTest;

	if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;

	Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
	// Alam_GBC: There, you have translucent HW Draw, OK?
	if ((option & V_TRANSLUCENT) && cv_grtranslucenthud.value != 255)
	{
		Surf.FlatColor.s.alpha = (byte)(cv_grtranslucenthud.value/2);
	}
	else
		Surf.FlatColor.s.alpha = 127;

	HWD.pfnDrawPolygon(&Surf, v, 4, flags);
}

// Draws a patch 2x as small SSNTails 06-10-2003
void HWR_DrawSmallPatch (GlidePatch_t *gpatch, int x, int y, int option, const byte *colormap)
{
	FOutVector      v[4];
	int flags;

	float sdupx = vid.fdupx;
	float sdupy = vid.fdupy;
	float pdupx = vid.fdupx;
	float pdupy = vid.fdupy;

	// make patch ready in hardware cache
	HWR_GetMappedPatch (gpatch, colormap);

	if (option & V_NOSCALEPATCH)
		pdupx = pdupy = 2.0f;
	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x = (x*sdupx-gpatch->leftoffset*pdupx)/vid.width - 1;
	v[2].x = v[1].x = (x*sdupx+(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
	v[0].y = v[1].y = 1-(y*sdupy-gpatch->topoffset*pdupy)/vid.height;
	v[2].y = v[3].y = 1-(y*sdupy+(gpatch->height-gpatch->topoffset)*pdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = gpatch->max_s;
	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = gpatch->max_t;

	flags = BLENDMODE | PF_Clip | PF_NoZClip | PF_NoDepthTest;

	if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;

	// clip it since it is used for bunny scroll in doom I
	if (option & V_TRANSLUCENT)
	{
		FSurfaceInfo Surf;
		Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
		Surf.FlatColor.s.alpha = (byte)cv_grtranslucenthud.value;
		flags |= PF_Modulated;
		HWD.pfnDrawPolygon(&Surf, v, 4, flags);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

//
// HWR_DrawMappedPatch(): Like HWR_DrawPatch but with translated color
//
void HWR_DrawMappedPatch (GlidePatch_t *gpatch, int x, int y, int option, const byte *colormap)
{
	FOutVector      v[4];
	int flags;

	float sdupx = vid.fdupx*2;
	float sdupy = vid.fdupy*2;
	float pdupx = vid.fdupx*2;
	float pdupy = vid.fdupy*2;

	// make patch ready in hardware cache
	HWR_GetMappedPatch (gpatch, colormap);

	if (option & V_NOSCALEPATCH)
		pdupx = pdupy = 2.0f;
	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x = (x*sdupx-gpatch->leftoffset*pdupx)/vid.width - 1;
	v[2].x = v[1].x = (x*sdupx+(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
	v[0].y = v[1].y = 1-(y*sdupy-gpatch->topoffset*pdupy)/vid.height;
	v[2].y = v[3].y = 1-(y*sdupy+(gpatch->height-gpatch->topoffset)*pdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = gpatch->max_s;
	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = gpatch->max_t;

	flags = BLENDMODE | PF_Clip | PF_NoZClip | PF_NoDepthTest;

	if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;

	// clip it since it is used for bunny scroll in doom I
	if (option & V_TRANSLUCENT)
	{
		FSurfaceInfo Surf;
		Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
		Surf.FlatColor.s.alpha = (byte)cv_grtranslucenthud.value;
		flags |= PF_Modulated;
		HWD.pfnDrawPolygon(&Surf, v, 4, flags);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

void HWR_DrawPic(int x, int y, lumpnum_t lumpnum)
{
	FOutVector      v[4];
	const GlidePatch_t    *patch;

	// make pic ready in hardware cache
	patch = HWR_GetPic(lumpnum);

//  3--2
//  | /|
//  |/ |
//  0--1

	v[0].x = v[3].x = (float)2.0 * (float)x/vid.width - 1;
	v[2].x = v[1].x = (float)2.0 * (float)(x + patch->width*vid.fdupx)/vid.width - 1;
	v[0].y = v[1].y = 1 - (float)2.0 * (float)y/vid.height;
	v[2].y = v[3].y = 1 - (float)2.0 * (float)(y + patch->height*vid.fdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow =  0;
	v[2].sow = v[1].sow =  patch->max_s;
	v[0].tow = v[1].tow =  0;
	v[2].tow = v[3].tow =  patch->max_t;


	//Hurdler: Boris, the same comment as above... but maybe for pics
	// it not a problem since they don't have any transparent pixel
	// if I'm right !?
	// But then, the question is: why not 0 instead of PF_Masked ?
	// or maybe PF_Environment ??? (like what I said above)
	// BP: PF_Environment don't change anything ! and 0 is undifined
	if (cv_grtranslucenthud.value != 255)
	{
		FSurfaceInfo Surf;
		Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
		Surf.FlatColor.s.alpha = (byte)cv_grtranslucenthud.value;
		HWD.pfnDrawPolygon(&Surf, v, 4, PF_Modulated | BLENDMODE | PF_NoDepthTest | PF_Clip | PF_NoZClip);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, BLENDMODE | PF_NoDepthTest | PF_Clip | PF_NoZClip);
}

// ==========================================================================
//                                                            V_VIDEO.C STUFF
// ==========================================================================


// --------------------------------------------------------------------------
// Fills a box of pixels using a flat texture as a pattern
// --------------------------------------------------------------------------
void HWR_DrawFlatFill (int x, int y, int w, int h, lumpnum_t flatlumpnum)
{
	FOutVector  v[4];
	double dflatsize;
	int flatflag;
	const size_t len = W_LumpLength(flatlumpnum);

	switch (len)
	{
		case 4194304: // 2048x2048 lump
			dflatsize = 2048.0f;
			flatflag = 2047;
			break;
		case 1048576: // 1024x1024 lump
			dflatsize = 1024.0f;
			flatflag = 1023;
			break;
		case 262144:// 512x512 lump
			dflatsize = 512.0f;
			flatflag = 511;
			break;
		case 65536: // 256x256 lump
			dflatsize = 256.0f;
			flatflag = 255;
			break;
		case 16384: // 128x128 lump
			dflatsize = 128.0f;
			flatflag = 127;
			break;
		case 1024: // 32x32 lump
			dflatsize = 32.0f;
			flatflag = 31;
			break;
		default: // 64x64 lump
			dflatsize = 64.0f;
			flatflag = 63;
			break;
	}

//  3--2
//  | /|
//  |/ |
//  0--1

	v[0].x = v[3].x = (x - 160.0f)/160.0f;
	v[2].x = v[1].x = ((x+w) - 160.0f)/160.0f;
	v[0].y = v[1].y = -(y - 100.0f)/100.0f;
	v[2].y = v[3].y = -((y+h) - 100.0f)/100.0f;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	// flat is 64x64 lod and texture offsets are [0.0, 1.0]
	v[0].sow = v[3].sow = (float)((x & flatflag)/dflatsize);
	v[2].sow = v[1].sow = (float)(v[0].sow + w/dflatsize);
	v[0].tow = v[1].tow = (float)((y & flatflag)/dflatsize);
	v[2].tow = v[3].tow = (float)(v[0].tow + h/dflatsize);

	HWR_GetFlat(flatlumpnum);

	//Hurdler: Boris, the same comment as above... but maybe for pics
	// it not a problem since they don't have any transparent pixel
	// if I'm right !?
	// BTW, I see we put 0 for PFs, and If I'm right, that
	// means we take the previous PFs as default
	// how can we be sure they are ok?
	HWD.pfnDrawPolygon(NULL, v, 4, PF_NoDepthTest); //PF_Translucent);
}


// --------------------------------------------------------------------------
// Fade down the screen so that the menu drawn on top of it looks brighter
// --------------------------------------------------------------------------
//  3--2
//  | /|
//  |/ |
//  0--1
void HWR_FadeScreenMenuBack(unsigned long color, int height)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;

	// setup some neat-o translucency effect
	if (!height) //cool hack 0 height is full height
		height = vid.height;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y =  1.0f-((height<<1)/(float)vid.height);
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = 1.0f;
	v[0].tow = v[1].tow = 1.0f;
	v[2].tow = v[3].tow = 0.0f;

	Surf.FlatColor.rgba = UINT2RGBA(color);
	Surf.FlatColor.s.alpha = (byte)((0xff/2) * ((float)height / vid.height)); //calum: varies console alpha
	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}


// ==========================================================================
//                                                             R_DRAW.C STUFF
// ==========================================================================

// ------------------
// HWR_DrawViewBorder
// Fill the space around the view window with a Doom flat texture, draw the
// beveled edges.
// 'clearlines' is useful to clear the heads up messages, when the view
// window is reduced, it doesn't refresh all the view borders.
// ------------------
void HWR_DrawViewBorder(int clearlines)
{
	int x, y;
	int top, side;
	int baseviewwidth, baseviewheight;
	int basewindowx, basewindowy;
	GlidePatch_t *patch;

//    if (gr_viewwidth == vid.width)
//        return;

	if (!clearlines)
		clearlines = BASEVIDHEIGHT; // refresh all

	// calc view size based on original game resolution
	baseviewwidth = (int)(gr_viewwidth/vid.fdupx); //(cv_viewsize.value * BASEVIDWIDTH/10)&~7;
	baseviewheight = (int)(gr_viewheight/vid.fdupy);
	top = (int)(gr_baseviewwindowy/vid.fdupy);
	side = (int)(gr_viewwindowx/vid.fdupx);

	// top
	HWR_DrawFlatFill(0, 0,
		BASEVIDWIDTH, (top < clearlines ? top : clearlines),
		st_borderpatchnum);

	// left
	if (top < clearlines)
		HWR_DrawFlatFill(0, top, side,
			(clearlines-top < baseviewheight ? clearlines-top : baseviewheight),
			st_borderpatchnum);

	// right
	if (top < clearlines)
		HWR_DrawFlatFill(side + baseviewwidth, top, side,
			(clearlines-top < baseviewheight ? clearlines-top : baseviewheight),
			st_borderpatchnum);

	// bottom
	if (top + baseviewheight < clearlines)
		HWR_DrawFlatFill(0, top + baseviewheight,
			BASEVIDWIDTH, BASEVIDHEIGHT, st_borderpatchnum);

	//
	// draw the view borders
	//

	basewindowx = (BASEVIDWIDTH - baseviewwidth)>>1;
	if (baseviewwidth == BASEVIDWIDTH)
		basewindowy = 0;
	else
		basewindowy = top;

	// top edge
	if (clearlines > basewindowy - 8)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_T], PU_CACHE);
		for (x = 0; x < baseviewwidth; x += 8)
			HWR_DrawPatch(patch, basewindowx + x, basewindowy - 8,
				0);
	}

	// bottom edge
	if (clearlines > basewindowy + baseviewheight)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_B], PU_CACHE);
		for (x = 0; x < baseviewwidth; x += 8)
			HWR_DrawPatch(patch, basewindowx + x,
				basewindowy + baseviewheight, 0);
	}

	// left edge
	if (clearlines > basewindowy)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_L], PU_CACHE);
		for (y = 0; y < baseviewheight && basewindowy + y < clearlines;
			y += 8)
		{
			HWR_DrawPatch(patch, basewindowx - 8, basewindowy + y,
				0);
		}
	}

	// right edge
	if (clearlines > basewindowy)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_R], PU_CACHE);
		for (y = 0; y < baseviewheight && basewindowy+y < clearlines;
			y += 8)
		{
			HWR_DrawPatch(patch, basewindowx + baseviewwidth,
				basewindowy + y, 0);
		}
	}

	// Draw beveled corners.
	if (clearlines > basewindowy - 8)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_TL],
				PU_CACHE),
			basewindowx - 8, basewindowy - 8, 0);

	if (clearlines > basewindowy - 8)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_TR],
				PU_CACHE),
			basewindowx + baseviewwidth, basewindowy - 8, 0);

	if (clearlines > basewindowy+baseviewheight)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_BL],
				PU_CACHE),
			basewindowx - 8, basewindowy + baseviewheight, 0);

	if (clearlines > basewindowy + baseviewheight)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_BR],
				PU_CACHE),
			basewindowx + baseviewwidth,
			basewindowy + baseviewheight, 0);
}


// ==========================================================================
//                                                     AM_MAP.C DRAWING STUFF
// ==========================================================================

// Clear the automap part of the screen
void HWR_clearAutomap(void)
{
	FRGBAFloat fColor = {0, 0, 0, 1};

	/// \note faB - optimize by clearing only colors ?
	//HWD.pfnSetBlend(PF_NoOcclude);

	// minx,miny,maxx,maxy
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
	HWD.pfnClearBuffer(true, true, &fColor);
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}


// -----------------+
// HWR_drawAMline   : draw a line of the automap (the clipping is already done in automap code)
// Arg              : color is a RGB 888 value
// -----------------+
void HWR_drawAMline(const fline_t *fl, int color)
{
	F2DCoord v1, v2;
	RGBA_t color_rgba;

	color_rgba = V_GetColor(color);

	v1.x = ((float)fl->a.x-(vid.width/2.0f))*(2.0f/vid.width);
	v1.y = ((float)fl->a.y-(vid.height/2.0f))*(2.0f/vid.height);

	v2.x = ((float)fl->b.x-(vid.width/2.0f))*(2.0f/vid.width);
	v2.y = ((float)fl->b.y-(vid.height/2.0f))*(2.0f/vid.height);

	HWD.pfnDraw2DLine(&v1, &v2, color_rgba);
}


// -----------------+
// HWR_DrawFill     : draw flat coloured rectangle, with no texture
// -----------------+
void HWR_DrawFill(int x, int y, int w, int h, int color)
{
	FOutVector v[4];
	FSurfaceInfo Surf;

//  3--2
//  | /|
//  |/ |
//  0--1
	v[0].x = v[3].x = (x - 160.0f)/160.0f;
	v[2].x = v[1].x = ((x+w) - 160.0f)/160.0f;
	v[0].y = v[1].y = -(y - 100.0f)/100.0f;
	v[2].y = v[3].y = -((y+h) - 100.0f)/100.0f;

	//Hurdler: do we still use this argb color? if not, we should remove it
	v[0].argb = v[1].argb = v[2].argb = v[3].argb = 0xff00ff00; //;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = 1.0f;
	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = 1.0f;

	Surf.FlatColor = V_GetColor(color);

	HWD.pfnDrawPolygon(&Surf, v, 4,
		PF_Modulated|PF_NoTexture|PF_NoDepthTest);
}

#ifdef HAVE_PNG
 #include "png.h"
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // PNG is only used if write is supported (see ../m_misc.c)
 #endif
#endif

#ifndef USE_PNG
// --------------------------------------------------------------------------
// save screenshots with TGA format
// --------------------------------------------------------------------------
static inline boolean saveTGA(const char *file_name, const void *buffer,
	int width, int height)
{
	int fd;
	TGAHeader tga_hdr;

	fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
	if (fd < 0)
		return false;

	memset(&tga_hdr, 0, sizeof (tga_hdr));
	tga_hdr.width = SHORT(width);
	tga_hdr.height = SHORT(height);
	tga_hdr.image_pix_size = 24;
	tga_hdr.image_type = 2;
	tga_hdr.image_descriptor = 32;

	write(fd, &tga_hdr, sizeof (TGAHeader));
	write(fd, buffer, width * height * 3);
	close(fd);
	return true;
}
#endif

// --------------------------------------------------------------------------
// screen shot
// --------------------------------------------------------------------------

boolean HWR_Screenshot(const char *lbmname)
{
	boolean ret;
	byte *bufw = malloc(vid.width * vid.height * 3 * sizeof (*bufw));
	int i;

	if (!bufw)
		return false;;

	if (rendermode == render_glide)
	{
		byte *dest = bufw;
		unsigned short *bufr = malloc(vid.width * vid.height * sizeof (*bufr));
		if (!bufr)
		{
			free(bufw);
			return false;
		}
		// glide can only returns 16bit 565 RGB
		HWD.pfnReadRect(0, 0, vid.width, vid.height, vid.width * 2, bufr);

		// format to 888 BGR
		for (i = 0; i < vid.width * vid.height; i++)
		{
			const unsigned short rgb565 = bufr[i];
			*(dest++) = (byte)((rgb565 & 31) <<3);
			*(dest++) = (byte)(((rgb565 >> 5) & 63) <<2);
			*(dest++) = (byte)(((rgb565 >> 11) & 31) <<3);
		}
		free(bufr);
	}
	else
	{
		// returns 24bit 888 RGB
		HWD.pfnReadRect(0, 0, vid.width, vid.height, vid.width * 3, (void *)bufw);

#ifndef USE_PNG
		// format to 888 BGR
		for (i = 0; i < vid.width * vid.height * 3; i+=3)
		{
			const byte temp = bufw[i];
			bufw[i] = bufw[i+2];
			bufw[i+2] = temp;
		}
#endif
	}

#ifdef USE_PNG
	ret = M_SavePNG(lbmname, bufw, vid.width, vid.height, NULL);
#else
	ret = saveTGA(lbmname, bufw, vid.width, vid.height);
#endif
	free(bufw);
	return ret;
}
