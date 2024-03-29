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
/// \brief Refresh module, data I/O, caching, retrieval of graphics by name

#ifndef __R_DATA__
#define __R_DATA__

#include "r_defs.h"
#include "r_state.h"

#ifdef __GNUG__
#pragma interface
#endif

// moved here for r_sky.c (texpatch_t is used)

// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
typedef struct
{
	// Block origin (always UL), which has already accounted for the internal origin of the patch.
	unsigned short originx, originy;
	lumpnum_t patch;
} texpatch_t;

// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.
typedef struct
{
	// Keep name for switch changing, etc.
	char name[8];
	short width, height;

	// All the patches[patchcount] are drawn back to front into the cached texture.
	short patchcount;
    texpatch_t patches[1];
} texture_t;

// all loaded and prepared textures from the start of the game
extern texture_t **textures;

extern short color8to16[256]; // remap color index to highcolor
extern short *hicolormaps; // remap high colors to high colors..

extern CV_PossibleValue_t Color_cons_t[];

// Load TEXTURE1/TEXTURE2/PNAMES definitions, create lookup tables
void R_LoadTextures(void);
void R_FlushTextureCache(void);

// Retrieve column data for span blitting.
byte *R_GetColumn(fixed_t tex, int col);

byte *R_GetFlat(lumpnum_t flatnum);

// I/O, setting up the stuff.
void R_InitData(void);
void R_PrecacheLevel(void);

// Retrieval.
// Floor/ceiling opaque texture tiles,
// lookup by name. For animation?
int R_GetFlatNumForName(const char *name);
#define R_FlatNumForName(x) R_GetFlatNumForName(x)

// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
long R_TextureNumForName(const char *name, unsigned short sidenum);
long R_CheckTextureNumForName(const char *name, unsigned short sidenum);

void R_ClearColormaps(void);
long R_ColormapNumForName(char *name);
long R_CreateColormap(char *p1, char *p2, char *p3);
void R_CreateColormap2(char *p1, char *p2, char *p3);
void R_MakeColormaps(void);
const char *R_ColormapNameForNum(int num);

extern size_t numtextures;

#endif
