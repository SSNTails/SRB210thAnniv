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
/// \brief All the global variables that store the internal state.
///
///	Theoretically speaking, the internal state of the engine
///	should be found by looking at the variables collected
///	here, and every relevant module will have to include
///	this header file.
///	In practice, things are a bit messy.

#ifndef __DOOMSTAT__
#define __DOOMSTAT__

// We need globally shared data structures, for defining the global state variables.
#include "doomdata.h"

// We need the player data structure as well.
#include "d_player.h"

// =============================
// Selected skill type, map etc.
// =============================

// Selected by user.
extern skill_t gameskill;
extern short gamemap;
extern musicenum_t mapmusic;
extern short maptol;
extern int globalweather;
extern int curWeather;
extern int cursaveslot;
extern short lastmapsaved;

extern boolean itsSpazzo;

#define PRECIP_NONE  0
#define PRECIP_STORM 1
#define PRECIP_SNOW  2
#define PRECIP_RAIN  3
#define PRECIP_BLANK 4
#define PRECIP_STORM_NORAIN 5

// Set if homebrew PWAD stuff has been added.
extern boolean modifiedgame;
extern USHORT mainwads;
extern boolean savemoddata; // This mod saves time/emblem data.
extern boolean timeattacking;
extern boolean disableSpeedAdjust; // Don't alter the duration of player states if true
extern boolean imcontinuing; // Temporary flag while continuing to ignore "noreload" mapheaders

// Netgame? only true in a netgame
extern boolean netgame;
extern boolean addedtogame; // true after the server has added you
// Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
extern boolean multiplayer;

extern int gametype;
extern boolean splitscreen;
extern boolean circuitmap; // Does this level have 'circuit mode'?
extern int fromlevelselect;
extern int cv_debug;

// ========================================
// Internal parameters for sound rendering.
// ========================================

extern boolean nomusic; // defined in d_main.c
extern boolean nosound;
extern boolean nofmod;
extern boolean music_disabled;
extern boolean sound_disabled;
extern boolean digital_disabled;

// =========================
// Status flags for refresh.
// =========================
//

extern boolean menuactive; // Menu overlaid?
extern boolean paused; // Game paused?

extern boolean nodrawers;
extern boolean noblit;
extern boolean lastdraw;
extern postimg_t postimgtype;
extern int postimgparam;

extern int viewwindowx, viewwindowy;
extern int viewwidth, scaledviewwidth;

extern boolean gamedataloaded;

// Player taking events, and displaying.
extern int consoleplayer;
extern int displayplayer;
extern int secondarydisplayplayer; // for splitscreen

// Maps of special importance
extern int spstage_start;
extern int sstage_start;
extern int sstage_end;
extern int racestage_start;

extern boolean looptitle;

extern tic_t countdowntimer;
extern byte countdowntimeup;

typedef struct
{
	byte numpics;
	char picname[8][8];
	boolean pichires[8];
	char *text;
	unsigned short xcoord[8];
	unsigned short ycoord[8];
	unsigned short picduration[8];
	musicenum_t musicslot;
	boolean musicloop;
	unsigned short textxpos;
	unsigned short textypos;
} scene_t;

typedef struct
{
	scene_t scene[128]; // 128 scenes per cutscene.
	int numscenes; // Number of scenes in this cutscene
} cutscene_t;

extern cutscene_t cutscenes[128];

// For the Custom Exit linedef.
extern int nextmapoverride, nextmapgametype;
extern boolean skipstats;

extern ULONG totalrings; //  Total # of rings in a level

// Fun extra stuff
extern ULONG lastmap; // Last level you were at (returning from special stages).
extern mapthing_t *rflagpoint, *bflagpoint; // Original flag spawn locations
#define MF_REDFLAG 1
#define MF_BLUEFLAG 2

#define LEVELARRAYSIZE 1035+2
extern char lvltable[LEVELARRAYSIZE+3][64];

extern boolean twodlevel;

/** Map header information.
  */
typedef struct
{
	// The original eight.
	char lvlttl[33];      ///< Level name without "Zone".
	char subttl[33];      ///< Subtitle for level
	byte actnum;          ///< Act number or 0 for none.
	short typeoflevel;    ///< Combination of typeoflevel flags.
	short nextlevel;      ///< Map number of next level, or 1100-1102 to end.
	musicenum_t musicslot;///< Music slot number to play. 0 for no music.
	byte forcecharacter;  ///< Skin number to switch to or 255 to disable.
	byte weather;         ///< 0 = sunny day, 1 = storm, 2 = snow, 3 = rain, 4 = blank, 5 = thunder w/o rain.
	short skynum;         ///< Sky number to use.

	// Extra information.
	char interscreen[8];  ///< 320x200 patch to display at intermission.
	char scriptname[192]; ///< Script to use when the map is switched to.
	boolean scriptislump; ///< True if the script is a lump, not a file.
	byte precutscenenum;  ///< Cutscene number to play BEFORE a level starts.
	byte cutscenenum;     ///< Cutscene number to use, 0 for none.
	short countdown;      ///< Countdown until level end?
	boolean nozone;       ///< True to hide "Zone" in level name.
	boolean hideinmenu;   ///< True to hide in the multiplayer menu.
	boolean nossmusic;    ///< True to disable Super Sonic music in this level.
	boolean speedmusic;   ///< Speed up the music for super sneakers?
	boolean noreload;     ///< True to retain level state when you die in single player
	boolean timeattack;   ///< Count in time attack calculations
	boolean levelselect;  ///< Does it appear in the level select?
	char runsoc[64];      ///< SOC to execute at start of level
} mapheader_t;

extern mapheader_t mapheaderinfo[NUMMAPS];

#define TOL_COOP        1 ///< Cooperative
#define TOL_RACE        2 ///< Race
#define TOL_MATCH       4 ///< Match
#define TOL_TAG         8 ///< Tag
#define TOL_CTF        16 ///< Capture the Flag
#ifdef CHAOSISNOTDEADYET
#define TOL_CHAOS      32 ///< Chaos
#endif
#define TOL_NIGHTS     64 ///< NiGHTS
#define TOL_ADVENTURE 128 ///< Adventure
#define TOL_MARIO     256 ///< Mario
#define TOL_2D        512 ///< 2D
#define TOL_XMAS     1024 ///< Xmas
#define TOL_SP       4096 ///< Single Player

// Gametypes
#define GT_COOP  0 // also used in single player
#define GT_MATCH 1
#define GT_RACE  2
#define GT_TAG   3
#define GT_CTF   4 // capture the flag
#ifndef CHAOSISNOTDEADYET
#define NUMGAMETYPES 5
#else
#define GT_CHAOS 5
#define NUMGAMETYPES 6
#endif
// If you alter this list, update gametype_cons_t in m_menu.c

// Fake gametypes
#define GTF_TEAMMATCH 42
#define GTF_TIMEONLYRACE 43

// Emeralds stored as bits to throw savegame hackers off.
extern USHORT emeralds;
extern tic_t totalplaytime;
#define EMERALD1 1
#define EMERALD2 2
#define EMERALD3 4
#define EMERALD4 8
#define EMERALD5 16
#define EMERALD6 32
#define EMERALD7 64
#define ALL7EMERALDS(v) ((v & (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7)) == (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7))

#define MAXEMBLEMS 512 // If you have more emblems than this in your game, you seriously need to get a life.
extern int numemblems;

#define NUMEGGS 12
extern ULONG foundeggs;

/** Hidden emblem/egg structure.
  */
typedef struct
{
	signed short x; ///< X coordinate.
	signed short y; ///< Y coordinate.
	signed short z; ///< Z coordinate.
	byte player;    ///< Player who can access this emblem.
	signed short level;     ///< Level on which this emblem/egg can be found.
	boolean collected; ///< Do you have this emblem?
} emblem_t;

extern emblem_t emblemlocations[MAXEMBLEMS];

extern emblem_t egglocations[NUMEGGS]; // Easter eggs... literally!

/** Time attack information, currently a very small structure.
  */
typedef struct
{
	tic_t time; ///< Time in which the level was finished.
} timeattack_t;

extern timeattack_t timedata[NUMMAPS];
extern boolean mapvisited[NUMMAPS];

extern ULONG token; ///< Number of tokens collected in a level
extern ULONG tokenlist; ///< List of tokens collected
extern int tokenbits; ///< Used for setting token bits
extern long sstimer; ///< Time allotted in the special stage
extern ULONG bluescore; ///< Blue Team Scores
extern ULONG redscore;  ///< Red Team Scores

// Eliminates unnecessary searching.
extern boolean CheckForBustableBlocks;
extern boolean CheckForBouncySector;
extern boolean CheckForQuicksand;
extern boolean CheckForMarioBlocks;
extern boolean CheckForFloatBob;
extern boolean CheckForReverseGravity;

// Powerup durations
extern tic_t invulntics;
extern tic_t sneakertics;
extern int flashingtics;
extern tic_t tailsflytics;
extern int underwatertics;
extern tic_t spacetimetics;
extern tic_t extralifetics;
extern tic_t gravbootstics;
// NiGHTS Powerups
extern tic_t paralooptics;
extern tic_t helpertics;

extern byte introtoplay;
extern byte creditscutscene;

extern mobj_t *hunt1, *hunt2, *hunt3; // Emerald hunt locations

// For racing
extern ULONG countdown;
extern ULONG countdown2;

extern fixed_t gravity;

// Grading
// 0 = No grade
// 1 = F
// 2 = E
// 3 = D
// 4 = C
// 5 = B
// 6 = A
// 7 = A+
extern ULONG grade;

extern ULONG timesbeaten; // # of times the game has been beaten.

// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern tic_t gametic;
#define localgametic leveltime

// Player spawn spots.
extern mapthing_t *playerstarts[MAXPLAYERS]; // Cooperative
extern mapthing_t *bluectfstarts[MAXPLAYERS]; // CTF
extern mapthing_t *redctfstarts[MAXPLAYERS]; // CTF

// =====================================
// Internal parameters, used for engine.
// =====================================

#if defined (macintosh)
#define DEBFILE(msg) I_OutputMsg(msg)
extern FILE *debugfile;
#else
#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if (debugfile) { fputs(msg, debugfile); fflush(debugfile); } }
extern FILE *debugfile;
#else
#define DEBFILE(msg) {}
extern FILE *debugfile;
#endif
#endif

#ifdef DEBUGFILE
extern int debugload;
#endif

// if true, load all graphics at level load
extern boolean precache;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;

// debug flag to cancel adaptiveness
extern boolean singletics;

// =============
// Netgame stuff
// =============

#include "d_clisrv.h"

extern consvar_t cv_timetic; // display high resolution timer
extern consvar_t cv_forceskin; // force clients to use the server's skin
extern consvar_t cv_downloading; // allow clients to downloading WADs.
extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
extern int adminplayer, serverplayer;

/// \note put these in d_clisrv outright?

#endif //__DOOMSTAT__
