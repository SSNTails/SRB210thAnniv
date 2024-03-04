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
/// \brief Heads up display

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"
#include "w_wad.h"
#include "r_defs.h"

//------------------------------------
//           heads up font
//------------------------------------
#define HU_FONTSTART '!' // the first font character
#define HU_REALFONTEND '_' // the last font character
#define HU_FONTEND '~'

#define HU_REALFONTSIZE (HU_REALFONTEND - HU_FONTSTART + 1)
#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1)

// Level title font
#define LT_FONTSTART '\'' // the first font characters
#define LT_REALFONTSTART 'A'
#define LT_FONTEND 'Z' // the last font characters
#define LT_FONTSIZE (LT_FONTEND - LT_FONTSTART + 1)
#define LT_REALFONTSIZE (LT_FONTEND - LT_REALFONTSTART + 1)

#define CRED_FONTSTART 'A' // the first font character
#define CRED_FONTEND 'Z' // the last font character
#define CRED_FONTSIZE (CRED_FONTEND - CRED_FONTSTART + 1)

#define HU_CROSSHAIRS 3 // maximum of 9 - see HU_Init();

extern char *shiftxform; // english translation shift table
extern char english_shiftxform[];

extern char cechotext[1024];
extern tic_t cechotimer;
extern tic_t cechoduration;
extern int cechoflags;

//------------------------------------
//        sorted player lines
//------------------------------------

typedef struct
{
	ULONG count;
	int num;
	int color;
	const char *name;
} playersort_t;

//------------------------------------
//           chat stuff
//------------------------------------
#define HU_MAXMSGLEN 224

extern patch_t *hu_font[HU_FONTSIZE];
extern patch_t *lt_font[LT_FONTSIZE];
extern patch_t *cred_font[CRED_FONTSIZE];
extern patch_t *emerald1, *emerald2, *emerald3, *emerald4,
	*emerald5, *emerald6, *emerald7;
extern patch_t *rflagico;
extern patch_t *bflagico;

// set true when entering a chat message
extern boolean chat_on;

// P_DeathThink sets this true to show scores while dead, in multiplayer
extern boolean playerdeadview;

// init heads up data at game startup.
void HU_Init(void);

// reset heads up when consoleplayer respawns.
void HU_Start(void);

boolean HU_Responder(event_t *ev);

void HU_Ticker(void);
void HU_Drawer(void);
char HU_dequeueChatChar(void);
void HU_Erase(void);
void HU_clearChatChars(void);
void HU_DrawTabRankings(int x, int y, playersort_t *tab, int scorelines, int whiteplayer);

int HU_CreateTeamScoresTbl(playersort_t *tab, ULONG dmtotals[]);
void TeamPlay_OnChange(void);

#endif
