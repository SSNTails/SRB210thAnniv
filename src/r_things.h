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
/// \brief Rendering of moving objects, sprites

#ifndef __R_THINGS__
#define __R_THINGS__

#include "sounds.h"

// number of sprite lumps for spritewidth,offset,topoffset lookup tables
// Fab: this is a hack : should allocate the lookup tables per sprite
#define	MAXSPRITELUMPS 8192 // Increase maxspritelumps Graue 11-06-2003

#define MAXVISSPRITES 2048 // added 2-2-98 was 128

// Constant arrays used for psprite clipping
//  and initializing clipping.
extern short negonearray[MAXVIDWIDTH];
extern short screenheightarray[MAXVIDWIDTH];

// vars for R_DrawMaskedColumn
extern short *mfloorclip;
extern short *mceilingclip;
extern fixed_t spryscale;
extern fixed_t sprtopscreen;
extern fixed_t sprbotscreen;
extern fixed_t windowtop;
extern fixed_t windowbottom;

void R_DrawMaskedColumn(column_t *column);
void R_SortVisSprites(void);

//faB: find sprites in wadfile, replace existing, add new ones
//     (only sprites from namelist are added or replaced)
void R_AddSpriteDefs(USHORT wadnum);
void R_DelSpriteDefs(USHORT wadnum);
//SoM: 6/5/2000: Light sprites correctly!
void R_AddSprites(sector_t *sec, int lightlevel);
void R_InitSprites(void);
void R_ClearSprites(void);
void R_DrawMasked(void);

// -----------
// SKINS STUFF
// -----------
#define SKINNAMESIZE 16
#define DEFAULTSKIN "sonic"

typedef struct
{
	char name[SKINNAMESIZE+1]; // short descriptive name of the skin
	spritedef_t spritedef;
	USHORT wadnum;
	char sprite[9];
	char faceprefix[9]; // 8 chars+'\0', default is "SBOSLIFE"
	char nameprefix[9]; // 8 chars+'\0', default is "STSONIC"
	char ability[2]; // ability definition
	char spin[2];
	char runonwater[2];
	char nojumpspin[2];
	char multiability[2];
	char lightdash[2];
	char homing[2];
	char ringslinger[2];
	char slingitem[8];
	char thokitem[8];
	char spinitem[8];
	char waterskip[2];
	char actionspd[4];
	char mindash[3];
	char maxdash[3];

	// Lots of super definitions...
	char super[2];
	char superanims[2];
	char supercolor[3];
	char superspin[2];

	char normalspeed[3]; // Normal ground

	char runspeed[3]; // Speed that you break into your run animation

	char accelstart[4]; // Acceleration if speed = 0
	char acceleration[3]; // Acceleration
	char thrustfactor[2]; // Thrust = thrustfactor * acceleration

	char jumpfactor[4]; // % of standard jump height

	// Definable color translation table
	char starttranscolor[4];
	char endtranscolor[4];

	char prefcolor[3];

	// Draw the sprite 2x as small?
	char highres[2];

	// specific sounds per skin
	sfxenum_t soundsid[NUMSKINSOUNDS]; // sound # in S_sfx table
} skin_t;

extern int numskins;
extern skin_t skins[MAXSKINS + 1];

void SetPlayerSkin(int playernum,const char *skinname);
void SetPlayerSkinByNum(int playernum,int skinnum); // Tails 03-16-2002
int R_SkinAvailable(const char *name);
void R_AddSkins(USHORT wadnum);
void R_DelSkins(USHORT wadnum);
void R_InitDrawNodes(void);
void SetSavedSkin(int playernum, int skinnum, int skincolor);

char *GetPlayerFacePic(int skinnum);

#endif //__R_THINGS__
