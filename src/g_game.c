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
/// \brief game loop functions, events handling

#include "doomdef.h"
#include "console.h"
#include "dstrings.h"
#include "d_main.h"
#include "f_finale.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "i_system.h"
#include "am_map.h"
#include "m_random.h"
#include "p_local.h"
#include "r_draw.h"
#include "r_main.h"
#include "s_sound.h"
#include "g_game.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_argv.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "z_zone.h"
#include "i_video.h"
#include "byteptr.h"
#include "i_joy.h"
#include "r_local.h"
#include "r_things.h"
#include "y_inter.h"
#include "v_video.h"

gameaction_t gameaction;
gamestate_t gamestate = GS_NULL;
boolean oncontinuescreen = false;
skill_t gameskill  = sk_medium;

boolean itsSpazzo = false;

JoyType_t Joystick;
JoyType_t Joystick2;

// 1024 bytes is plenty for a savegame
#define SAVEGAMESIZE (1024)

char gamedatafilename[64] = "gamedata.dat";
char timeattackfolder[64] = "main";

static void G_ReadDemoTiccmd(ticcmd_t *cmd, int playernum);
static void G_WriteDemoTiccmd(ticcmd_t *cmd, int playernum);
static void G_DoWorldDone(void);

short gamemap = 1;
musicenum_t mapmusic;
short maptol;
int globalweather = 0;
int curWeather = PRECIP_NONE;
int cursaveslot = -1; // Auto-save 1p savegame slot
short lastmapsaved = 0; // Last map we auto-saved at

USHORT mainwads = 0;
boolean modifiedgame; // Set if homebrew PWAD stuff has been added.
boolean savemoddata = false;
boolean paused;
boolean timeattacking = false;
boolean disableSpeedAdjust = false;
boolean imcontinuing = false;
boolean runemeraldmanager = false;

boolean timingdemo; // if true, exit with report on completion
boolean nodrawers; // for comparative timing purposes
boolean noblit; // for comparative timing purposes
static tic_t demostarttime; // for comparative timing purposes

boolean netgame; // only true if packets are broadcast
boolean multiplayer;
boolean playeringame[MAXPLAYERS];
boolean addedtogame;
player_t players[MAXPLAYERS];

int consoleplayer; // player taking events and displaying
int displayplayer; // view being displayed
int secondarydisplayplayer; // for splitscreen

tic_t gametic;
tic_t levelstarttic; // gametic at level start
ULONG totalrings; // for intermission
ULONG lastmap; // last level you were at (returning from special stages)
tic_t timeinmap; // Ticker for time spent in level (used for levelcard display)

int spstage_start;
int sstage_start;
int sstage_end;
int racestage_start;

boolean looptitle = false;

tic_t countdowntimer = 0;
byte countdowntimeup = false;

cutscene_t cutscenes[128];

int nextmapoverride, nextmapgametype;
boolean skipstats;

// Original flag spawn locations
mapthing_t *rflagpoint;
mapthing_t *bflagpoint;

boolean twodlevel;

// Map Header Information
mapheader_t mapheaderinfo[NUMMAPS];

static boolean exitgame = false;

USHORT emeralds;
ULONG token; // Number of tokens collected in a level
ULONG tokenlist; // List of tokens collected
int tokenbits; // Used for setting token bits
long sstimer; // Time allotted in the special stage

char lvltable[LEVELARRAYSIZE+3][64];

tic_t totalplaytime;
ULONG foundeggs = 0;
int numemblems = 20;
boolean gamedataloaded = 0;

// Mystic's funky SA ripoff emblem idea
emblem_t emblemlocations[MAXEMBLEMS] =
{
	{  1566,  4352,  608, 0, 1, 0}, // GFZ1 Sonic
	{  2079, -1284,  768, 1, 1, 0}, // GFZ1 Tails
	{  1270,  5597,  960, 2, 1, 0}, // GFZ1 Knuckles
	{ -5560, -2865, 2816, 0, 2, 0}, // GFZ2 Sonic
	{    29,  -589, 3840, 1, 2, 0}, // GFZ2 Tails
	{ -3872,  4203, 3072, 2, 2, 0}, // GFZ2 Knuckles
	{ -985, -12193, 2172, 0, 4, 0}, // THZ1 Sonic
	{  4272, -5720, 3700, 1, 4, 0}, // THZ1 Tails
	{ 14318, -9281, 4040, 2, 4, 0}, // THZ1 Knuckles
	{-13402,  3822, 1312, 0, 5, 0}, // THZ2 Sonic
	{  4674,  3917, 2300, 1, 5, 0}, // THZ2 Tails
	{ -1330, -3505, 2432, 2, 5, 0}, // THZ2 Knuckles
	{  2848, -4094, 3800, 0, 7, 0}, // CEZ1 Sonic
	{ -5936,   322, 2900, 1, 7, 0}, // CEZ1 Tails
	{   -17,  1169,  912, 2, 7, 0}, // CEZ1 Knuckles
	{ -3264,  -544, 1080, 0, 8, 0}, // CEZ2 Sonic
	{  4777,  4725,  460, 1, 8, 0}, // CEZ2 Tails
	{  2186, -2447,  212, 2, 8, 0} // CEZ2 Knuckles
};

// Easter Eggs - Literally!
emblem_t egglocations[NUMEGGS] =
{
	{  -472,   2041,  103, 0, 1, 0},
	{  3201,   6794,  103, 0, 1, 0},
	{  6300,   4544, 1536, 0, 2, 0},
	{  2803,  -6232, 1300, 0, 2, 0},
	{ -1072, -10618, 2172, 0, 4, 0},
	{  2158,  -1378, 2468, 0, 4, 0},
	{-10057,   5274, 1280, 0, 5, 0},
	{   374,  -1373, 1024, 0, 5, 0},
	{  3322,   5459,  624, 0, 7, 0},
	{ -5441,   3907, 1088, 0, 7, 0},
	{  4784,  -2816,  408, 0, 8, 0},
	{ -3877,  -2077,  601, 0, 8, 0}
};

// Time attack data for levels
timeattack_t timedata[NUMMAPS];
boolean mapvisited[NUMMAPS];

ULONG bluescore, redscore; // CTF team scores

// Elminates unnecessary searching.
boolean CheckForBustableBlocks;
boolean CheckForBouncySector;
boolean CheckForQuicksand;
boolean CheckForMarioBlocks;
boolean CheckForFloatBob;
boolean CheckForReverseGravity;

// Powerup durations
tic_t invulntics = 20*TICRATE;
tic_t sneakertics = 20*TICRATE;
int flashingtics = 3*TICRATE;
tic_t tailsflytics = 8*TICRATE;
int underwatertics = 30*TICRATE;
tic_t spacetimetics = 11*TICRATE;
tic_t extralifetics = 4*TICRATE;
tic_t gravbootstics = 20*TICRATE;
tic_t paralooptics = 20*TICRATE;
tic_t helpertics = 20*TICRATE;

int gameovertics = 45*TICRATE;

byte introtoplay;
byte creditscutscene;

// Emerald locations
mobj_t *hunt1;
mobj_t *hunt2;
mobj_t *hunt3;

ULONG countdown, countdown2; // for racing

fixed_t gravity;

// Grading
ULONG grade;
ULONG timesbeaten;

static char demoname[32];
boolean demorecording;
boolean demoplayback;
static byte *demobuffer = NULL;
static byte *demo_p;
static byte *demoend;
boolean singledemo; // quit after playing a demo from cmdline

boolean precache = true; // if true, load all graphics at start

int prevmap, nextmap;

static byte *savebuffer;

// Analog Control
static void UserAnalog_OnChange(void);
static void UserAnalog2_OnChange(void);
static void Analog_OnChange(void);
static void Analog2_OnChange(void);

static CV_PossibleValue_t crosshair_cons_t[] = {{0, "Off"}, {1, "Cross"}, {2, "Angle"}, {3, "Point"}, {0, NULL}};
static CV_PossibleValue_t joyaxis_cons_t[] = { {0, "None"}, {1, "X-Axis"}, {2, "Y-Axis"}, {-1, "X-Axis-"}, {-2, "Y-Axis-"},
#ifdef _arch_dreamcast
{3, "R-Trig"}, {4, "L-Trig"}, {-3, "R-Trig-"}, {-4, "L-Trig-"},
{5, "Alt X-Axis"}, {6, "Alt Y-Axis"}, {-5, "Alt X-Axis-"}, {-6, "Alt Y-Axis-"},
{7, "Triggers"}, {-7,"Triggers-"},
#elif defined (_XBOX)
{3, "Alt X-Axis"}, {4, "Alt Y-Axis"}, {-3, "Alt X-Axis-"}, {-4, "Alt Y-Axis-"},
#else
#if JOYAXISSET > 1
 {3, "Z-Axis"}, {4, "X-Rudder"}, {-3, "Z-Axis-"}, {-4, "X-Rudder-"},
#endif
#if JOYAXISSET > 2
 {5, "Y-Rudder"}, {6, "Z-Rudder"}, {-5, "Y-Rudder-"}, {-6, "Z-Rudder-"},
#endif
#if JOYAXISSET > 3
 {7, "U-Axis"}, {8, "V-Axis"}, {-7, "U-Axis-"}, {-8, "V-Axis-"},
#endif
#endif
 {0, NULL}};
#if JOYAXISSET > 4
"More Axis Sets"
#endif

consvar_t cv_crosshair = {"crosshair", "Cross", CV_SAVE, crosshair_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_crosshair2 = {"crosshair2", "Cross", CV_SAVE, crosshair_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_invertmouse = {"invertmouse", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_alwaysfreelook = {"alwaysmlook", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_invertmouse2 = {"invertmouse2", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_alwaysfreelook2 = {"alwaysmlook2", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mousemove = {"mousemove", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mousemove2 = {"mousemove2", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_analog = {"analog", "Off", CV_CALL, CV_OnOff, Analog_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_analog2 = {"analog2", "Off", CV_CALL, CV_OnOff, Analog2_OnChange, 0, NULL, NULL, 0, 0, NULL};
#ifdef DC
consvar_t cv_useranalog = {"useranalog", "On", CV_SAVE|CV_CALL, CV_OnOff, UserAnalog_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_useranalog2 = {"useranalog2", "On", CV_SAVE|CV_CALL, CV_OnOff, UserAnalog2_OnChange, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_useranalog = {"useranalog", "Off", CV_SAVE|CV_CALL, CV_OnOff, UserAnalog_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_useranalog2 = {"useranalog2", "Off", CV_SAVE|CV_CALL, CV_OnOff, UserAnalog2_OnChange, 0, NULL, NULL, 0, 0, NULL};
#endif

typedef enum
{
	AXISNONE = 0,
	AXISTURN,
	AXISMOVE,
	AXISLOOK,
	AXISSTRAFE,
	AXISDEAD, //Axises that don't want deadzones
	AXISFIRE,
	AXISFIRENORMAL,
} axis_input_e;

consvar_t cv_turnaxis = {"joyaxis_turn", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#ifdef _PSP_
consvar_t cv_moveaxis = {"joyaxis_move", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_moveaxis = {"joyaxis_move", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
#ifdef _arch_dreamcast
consvar_t cv_sideaxis = {"joyaxis_side", "Triggers", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#elif defined (_XBOX)
consvar_t cv_sideaxis = {"joyaxis_side", "Alt X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_lookaxis = {"joyaxis_look", "Alt Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#elif defined (_PSP_)
consvar_t cv_sideaxis = {"joyaxis_side", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_sideaxis = {"joyaxis_side", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
#ifndef _XBOX
#ifdef _PSP_
consvar_t cv_lookaxis = {"joyaxis_look", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_lookaxis = {"joyaxis_look", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
#endif
consvar_t cv_fireaxis = {"joyaxis_fire", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_firenaxis = {"joyaxis_firenormal", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_turnaxis2 = {"joyaxis2_turn", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_moveaxis2 = {"joyaxis2_move", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#ifdef _arch_dreamcast
consvar_t cv_sideaxis2 = {"joyaxis2_side", "Triggers", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#elif defined (_XBOX)
consvar_t cv_sideaxis2 = {"joyaxis2_side", "Alt X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_lookaxis2 = {"joyaxis2_look", "Alt Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#elif defined (_PSP)
consvar_t cv_sideaxis2 = {"joyaxis2_side", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_sideaxis2 = {"joyaxis2_side", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
#ifndef _XBOX
consvar_t cv_lookaxis2 = {"joyaxis2_look", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
consvar_t cv_fireaxis2 = {"joyaxis2_fire", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_firenaxis2 = {"joyaxis2_firenormal", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};


#if MAXPLAYERS > 32
#error "please update player_name table using the new value for MAXPLAYERS"
#endif

char player_names[MAXPLAYERS][MAXPLAYERNAME+1] =
{
	"Player 1",
	"Player 2",
	"Player 3",
	"Player 4",
	"Player 5",
	"Player 6",
	"Player 7",
	"Player 8",
	"Player 9",
	"Player 10",
	"Player 11",
	"Player 12",
	"Player 13",
	"Player 14",
	"Player 15",
	"Player 16",
	"Player 17",
	"Player 18",
	"Player 19",
	"Player 20",
	"Player 21",
	"Player 22",
	"Player 23",
	"Player 24",
	"Player 25",
	"Player 26",
	"Player 27",
	"Player 28",
	"Player 29",
	"Player 30",
	"Player 31",
	"Player 32"
};

char team_names[MAXPLAYERS][MAXPLAYERNAME+1] =
{
	"Team 1",
	"Team 2",
	"Team 3",
	"Team 4",
	"Team 5",
	"Team 6",
	"Team 7",
	"Team 8",
	"Team 9",
	"Team 10",
	"Team 11",
	"Team 12",
	"Team 13",
	"Team 14",
	"Team 15",
	"Team 16",
	"Team 17", // teams from here on cannot be used for teams by color,
	"Team 18", // but theoretically you can have 32 teams by skin
	"Team 19",
	"Team 20",
	"Team 21",
	"Team 22",
	"Team 23",
	"Team 24",
	"Team 25",
	"Team 26",
	"Team 27",
	"Team 28",
	"Team 29",
	"Team 30",
	"Team 31",
	"Team 32",
};

/** Builds an original game map name from a map number.
  * The complexity is due to MAPA0-MAPZZ.
  *
  * \param map Map number.
  * \return Pointer to a static buffer containing the desired map name.
  * \sa M_MapNumber
  */
const char *G_BuildMapName(int map)
{
	static char mapname[9] = "MAPXX"; // internal map name (wad resource name)

	I_Assert(map > 0);
	I_Assert(map <= NUMMAPS);

	if (map < 100)
		sprintf(&mapname[3], "%.2d", map);
	else
	{
		mapname[3] = (char)('A' + (char)((map - 100) / 36));
		if ((map - 100) % 36 < 10)
			mapname[4] = (char)('0' + (char)((map - 100) % 36));
		else
			mapname[4] = (char)('A' + (char)((map - 100) % 36) - 10);
		mapname[5] = '\0';
	}

	return mapname;
}

/** Clips the console player's mouse aiming to the current view.
  * Used whenever the player view is changed manually.
  *
  * \param aiming Pointer to the vertical angle to clip.
  * \return Short version of the clipped angle for building a ticcmd.
  */
short G_ClipAimingPitch(int *aiming)
{
	int limitangle;

	// note: the current software mode implementation doesn't have true perspective
	if (rendermode == render_soft)
		limitangle = ANG45 + (ANG45/2) - ((ANG45/45)*3); // Some viewing fun, but not too far down...
	else
		limitangle = ANG90 - 1;

	if (*aiming > limitangle)
		*aiming = limitangle;
	else if (*aiming < -limitangle)
		*aiming = -limitangle;

	return (short)((*aiming)>>16);
}

static int JoyAxis(axis_input_e axissel)
{
	int retaxis;
	int axisval;
	boolean flp = false;

	//find what axis to get
	switch (axissel)
	{
		case AXISTURN:
			axisval = cv_turnaxis.value;
			break;
		case AXISMOVE:
			axisval = cv_moveaxis.value;
			break;
		case AXISLOOK:
			axisval = cv_lookaxis.value;
			break;
		case AXISSTRAFE:
			axisval = cv_sideaxis.value;
			break;
		case AXISFIRE:
			axisval = cv_fireaxis.value;
			break;
		case AXISFIRENORMAL:
			axisval = cv_firenaxis.value;
			break;
		default:
			return 0;
	}

	if (axisval < 0) //odd -axises
	{
		axisval = -axisval;
		flp = true;
	}
#ifdef _arch_dreamcast
	if (axisval == 7) // special case
	{
		retaxis = joyxmove[1] - joyymove[1];
		goto skipDC;
	}
	else
#endif
	if (axisval > JOYAXISSET*2 || axisval == 0) //not there in array or None
		return 0;

	if (axisval%2)
	{
		axisval /= 2;
		retaxis = joyxmove[axisval];
	}
	else
	{
		axisval--;
		axisval /= 2;
		retaxis = joyymove[axisval];
	}

#ifdef _arch_dreamcast
	skipDC:
#endif

	if (retaxis < (-JOYAXISRANGE))
		retaxis = -JOYAXISRANGE;
	if (retaxis > (+JOYAXISRANGE))
		retaxis = +JOYAXISRANGE;
	if (!Joystick.bGamepadStyle && axissel < AXISDEAD)
	{
		const int jdeadzone = JOYAXISRANGE/4;
		if (-jdeadzone < retaxis && retaxis < jdeadzone)
			return 0;
	}
	if (flp) retaxis = -retaxis; //flip it around
	return retaxis;
}

static int Joy2Axis(axis_input_e axissel)
{
	int retaxis;
	int axisval;
	boolean flp = false;

	//find what axis to get
	switch (axissel)
	{
		case AXISTURN:
			axisval = cv_turnaxis2.value;
			break;
		case AXISMOVE:
			axisval = cv_moveaxis2.value;
			break;
		case AXISLOOK:
			axisval = cv_lookaxis2.value;
			break;
		case AXISSTRAFE:
			axisval = cv_sideaxis2.value;
			break;
		case AXISFIRE:
			axisval = cv_fireaxis2.value;
			break;
		case AXISFIRENORMAL:
			axisval = cv_firenaxis2.value;
			break;
		default:
			return 0;
	}


	if (axisval < 0) //odd -axises
	{
		axisval = -axisval;
		flp = true;
	}
#ifdef _arch_dreamcast
	if (axisval == 7) // special case
	{
		retaxis = joy2xmove[1] - joy2ymove[1];
		goto skipDC;
	}
	else
#endif
	if (axisval > JOYAXISSET*2 || axisval == 0) //not there in array or None
		return 0;

	if (axisval%2)
	{
		axisval /= 2;
		retaxis = joy2xmove[axisval];
	}
	else
	{
		axisval--;
		axisval /= 2;
		retaxis = joy2ymove[axisval];
	}

#ifdef _arch_dreamcast
	skipDC:
#endif

	if (retaxis < (-JOYAXISRANGE))
		retaxis = -JOYAXISRANGE;
	if (retaxis > (+JOYAXISRANGE))
		retaxis = +JOYAXISRANGE;
	if (!Joystick2.bGamepadStyle && axissel < AXISDEAD)
	{
		const int jdeadzone = JOYAXISRANGE/4;
		if (-jdeadzone < retaxis && retaxis < jdeadzone)
			return 0;
	}
	if (flp) retaxis = -retaxis; //flip it around
	return retaxis;
}


//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
// set secondaryplayer true to build player 2's ticcmd in splitscreen mode
//
int localaiming, localaiming2;
angle_t localangle, localangle2;

// mouseaiming (looking up/down with the mouse or keyboard)
#define KB_LOOKSPEED (1<<25)
#define MAXPLMOVE (forwardmove[1])
#define SLOWTURNTICS (6*NEWTICRATERATIO)

static fixed_t forwardmove[2] = {25/NEWTICRATERATIO, 50/NEWTICRATERATIO};
static fixed_t sidemove[2] = {25/NEWTICRATERATIO, 50/NEWTICRATERATIO}; // faster!
static fixed_t angleturn[3] = {640/NEWTICRATERATIO, 1280/NEWTICRATERATIO, 320/NEWTICRATERATIO}; // + slow turn

void G_BuildTiccmd(ticcmd_t *cmd, int realtics)
{
	boolean strafe;
	int tspeed, forward, side, axis;
	const int speed = 1;
	ticcmd_t *base;
	// these ones used for multiple conditions
	boolean turnleft, turnright, mouseaiming, analogjoystickmove, gamepadjoystickmove;

	static int turnheld; // for accelerative turning
	static boolean keyboard_look; // true if lookup/down using keyboard

	base = I_BaseTiccmd(); // empty, or external driver
	M_Memcpy(cmd, base, sizeof (*cmd));

	// a little clumsy, but then the g_input.c became a lot simpler!
	strafe = gamekeydown[gamecontrol[gc_strafe][0]] ||
		gamekeydown[gamecontrol[gc_strafe][1]];

	turnright = gamekeydown[gamecontrol[gc_turnright][0]]
		|| gamekeydown[gamecontrol[gc_turnright][1]];
	turnleft = gamekeydown[gamecontrol[gc_turnleft][0]]
		|| gamekeydown[gamecontrol[gc_turnleft][1]];
	mouseaiming = (gamekeydown[gamecontrol[gc_mouseaiming][0]]
		|| gamekeydown[gamecontrol[gc_mouseaiming][1]]) ^ cv_alwaysfreelook.value;
	analogjoystickmove = cv_usejoystick.value && !Joystick.bGamepadStyle;
	gamepadjoystickmove = cv_usejoystick.value && Joystick.bGamepadStyle;

	axis = JoyAxis(AXISTURN);
	if (gamepadjoystickmove && axis != 0)
	{
		turnright = turnright || (axis > 0);
		turnleft = turnleft || (axis < 0);
	}
	forward = side = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (turnleft || turnright)
		turnheld += realtics;
	else
		turnheld = 0;

	if (turnheld < SLOWTURNTICS)
		tspeed = 2; // slow turn
	else
		tspeed = speed;

	// let movement keys cancel each other out
	if (cv_analog.value) // Analog
	{
		if (turnright)
			cmd->angleturn = (short)(cmd->angleturn - angleturn[tspeed]);
		if (turnleft)
			cmd->angleturn = (short)(cmd->angleturn + angleturn[tspeed]);
	}
	if (strafe || cv_analog.value || twodlevel
		|| (players[consoleplayer].mo && (players[consoleplayer].mo->flags2 & MF2_TWOD))
		|| players[consoleplayer].climbing
		|| (players[consoleplayer].pflags & PF_NIGHTSMODE)
		|| (players[consoleplayer].pflags & PF_SLIDING)) // Analog
	{
		if (turnright)
			side += sidemove[speed];
		if (turnleft)
			side -= sidemove[speed];

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			side += ((axis * sidemove[1]) >> 10);
		}
	}
	else
	{
		if (turnright)
			cmd->angleturn = (short)(cmd->angleturn - angleturn[tspeed]);
		else if (turnleft)
			cmd->angleturn = (short)(cmd->angleturn + angleturn[tspeed]);

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE should be 1023 (divide by 1024)
			cmd->angleturn = (short)(cmd->angleturn - ((axis * angleturn[1]) >> 10)); // ANALOG!
		}
	}

	axis = JoyAxis(AXISSTRAFE);
	if (gamepadjoystickmove && axis != 0)
	{
		if (axis < 0)
			side += sidemove[speed];
		else if (axis > 0)
			side -= sidemove[speed];
	}
	else if (analogjoystickmove && axis != 0)
	{
		// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			side += ((axis * sidemove[1]) >> 10);
	}

	// forward with key or button
	axis = JoyAxis(AXISMOVE);
	if (gamekeydown[gamecontrol[gc_forward][0]] ||
		gamekeydown[gamecontrol[gc_forward][1]] ||
		(gamepadjoystickmove && axis < 0))
	{
		forward += forwardmove[speed];
	}
	if (gamekeydown[gamecontrol[gc_backward][0]] ||
		gamekeydown[gamecontrol[gc_backward][1]] ||
		(gamepadjoystickmove && axis > 0))
	{
		forward -= forwardmove[speed];
	}

	if (analogjoystickmove && axis != 0)
		forward -= ((axis * forwardmove[1]) >> 10); // ANALOG!

	// some people strafe left & right with mouse buttons
	if (gamekeydown[gamecontrol[gc_straferight][0]] ||
		gamekeydown[gamecontrol[gc_straferight][1]])
	{
		side += sidemove[speed];
	}
	if (gamekeydown[gamecontrol[gc_strafeleft][0]] ||
		gamekeydown[gamecontrol[gc_strafeleft][1]])
	{
		side -= sidemove[speed];
	}

	// Next Weapon
	if (gamekeydown[gamecontrol[gc_weaponnext][0]] ||
		gamekeydown[gamecontrol[gc_weaponnext][1]])
		cmd->buttons |= BT_WEAPONNEXT;

	// Previous Weapon
	if (gamekeydown[gamecontrol[gc_weaponprev][0]] ||
		gamekeydown[gamecontrol[gc_weaponprev][1]])
		cmd->buttons |= BT_WEAPONPREV;

	// fire with any button/key
	axis = JoyAxis(AXISFIRE);
	if (gamekeydown[gamecontrol[gc_fire][0]] ||
		gamekeydown[gamecontrol[gc_fire][1]] ||
		axis > 0)
		cmd->buttons |= BT_ATTACK;

	// fire normal with any button/key
	axis = JoyAxis(AXISFIRENORMAL);
	if (gamekeydown[gamecontrol[gc_firenormal][0]] ||
		gamekeydown[gamecontrol[gc_firenormal][1]] ||
		axis > 0)
		cmd->buttons |= BT_FIRENORMAL;

	if (gamekeydown[gamecontrol[gc_lightdash][0]] ||
		gamekeydown[gamecontrol[gc_lightdash][1]])
		cmd->buttons |= BT_LIGHTDASH;

	// use with any button/key
	if (gamekeydown[gamecontrol[gc_use][0]] ||
		gamekeydown[gamecontrol[gc_use][1]])
		cmd->buttons |= BT_USE;

	// Taunts
	if (gamekeydown[gamecontrol[gc_taunt][0]] ||
		gamekeydown[gamecontrol[gc_taunt][1]])
		cmd->buttons |= BT_TAUNT;

	// Camera Controls
	if (gamekeydown[gamecontrol[gc_camleft][0]] ||
		gamekeydown[gamecontrol[gc_camleft][1]])
		cmd->buttons |= BT_CAMLEFT;

	if (gamekeydown[gamecontrol[gc_camright][0]] ||
		gamekeydown[gamecontrol[gc_camright][1]])
		cmd->buttons |= BT_CAMRIGHT;

	if (gamekeydown[gamecontrol[gc_camreset][0]] ||
		gamekeydown[gamecontrol[gc_camreset][1]])
	{
		if (cv_chasecam.value)
			P_ResetCamera(&players[displayplayer], &camera);
	}

	// jump button
	if (gamekeydown[gamecontrol[gc_jump][0]] ||
		gamekeydown[gamecontrol[gc_jump][1]])
		cmd->buttons |= BT_JUMP;

	// mouse look stuff (mouse look is not the same as mouse aim)
	if (mouseaiming)
	{
		keyboard_look = false;

		// looking up/down
		if (cv_invertmouse.value)
			localaiming -= mlooky<<19;
		else
			localaiming += mlooky<<19;
	}

	axis = JoyAxis(AXISLOOK);
	if (analogjoystickmove && axis != 0 && cv_lookaxis.value != 0)
		localaiming += axis<<16;

	// spring back if not using keyboard neither mouselookin'
	if (!keyboard_look && cv_lookaxis.value == 0 && !mouseaiming)
		localaiming = 0;

	if (gamekeydown[gamecontrol[gc_lookup][0]] ||
		gamekeydown[gamecontrol[gc_lookup][1]] ||
		(gamepadjoystickmove && axis < 0))
	{	
		if (players[consoleplayer].mo &&
			players[consoleplayer].mo->eflags & MF_VERTICALFLIP)
			localaiming -= KB_LOOKSPEED;
		else
			localaiming += KB_LOOKSPEED;
		keyboard_look = true;
	}
	else if (gamekeydown[gamecontrol[gc_lookdown][0]] ||
		gamekeydown[gamecontrol[gc_lookdown][1]] ||
		(gamepadjoystickmove && axis > 0))
	{
		if (players[consoleplayer].mo &&
			players[consoleplayer].mo->eflags & MF_VERTICALFLIP)
			localaiming += KB_LOOKSPEED;
		else
			localaiming -= KB_LOOKSPEED;
		keyboard_look = true;
	}
	else if (gamekeydown[gamecontrol[gc_centerview][0]] ||
		gamekeydown[gamecontrol[gc_centerview][1]])
		localaiming = 0;

	// accept no mlook for network games
	if (!cv_allowmlook.value)
		localaiming = 0;

	cmd->aiming = G_ClipAimingPitch(&localaiming);

	if (!mouseaiming && cv_mousemove.value)
		forward += mousey;

	if (strafe || cv_analog.value
		|| players[consoleplayer].climbing
		|| (players[consoleplayer].pflags & PF_SLIDING)) // Analog for mouse
		side += mousex*2;
	else
		cmd->angleturn = (short)(cmd->angleturn - (mousex*8));

	mousex = mousey = mlooky = 0;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;
	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	cmd->forwardmove = (signed char)(cmd->forwardmove + forward);
	cmd->sidemove = (signed char)(cmd->sidemove+ side);

	localangle += (cmd->angleturn<<16);
	cmd->angleturn = (short)(localangle >> 16);
}

// like the g_buildticcmd 1 but using mouse2, gamcontrolbis, ...
void G_BuildTiccmd2(ticcmd_t *cmd, int realtics)
{
	boolean strafe;
	int tspeed, forward, side, axis;
	const int speed = 1;
	ticcmd_t *base;
	// these ones used for multiple conditions
	boolean turnleft, turnright, mouseaiming, analogjoystickmove, gamepadjoystickmove;

	static int turnheld; // for accelerative turning
	static boolean keyboard_look; // true if lookup/down using keyboard

	base = I_BaseTiccmd2(); // empty, or external driver
	M_Memcpy(cmd, base, sizeof (*cmd));

	// a little clumsy, but then the g_input.c became a lot simpler!
	strafe = gamekeydown[gamecontrolbis[gc_strafe][0]] ||
		gamekeydown[gamecontrolbis[gc_strafe][1]];

	turnright = gamekeydown[gamecontrolbis[gc_turnright][0]]
		|| gamekeydown[gamecontrolbis[gc_turnright][1]];
	turnleft = gamekeydown[gamecontrolbis[gc_turnleft][0]]
		|| gamekeydown[gamecontrolbis[gc_turnleft][1]];

	mouseaiming = (gamekeydown[gamecontrolbis[gc_mouseaiming][0]]
		|| gamekeydown[gamecontrolbis[gc_mouseaiming][1]]) ^ cv_alwaysfreelook2.value;
	analogjoystickmove = cv_usejoystick2.value && !Joystick2.bGamepadStyle;
	gamepadjoystickmove = cv_usejoystick2.value && Joystick2.bGamepadStyle;

	axis = Joy2Axis(AXISTURN);
	if (gamepadjoystickmove && axis != 0)
	{
		turnright = turnright || (axis > 0);
		turnleft = turnleft || (axis < 0);
	}

	forward = side = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (turnleft || turnright)
		turnheld += realtics;
	else
		turnheld = 0;

	if (turnheld < SLOWTURNTICS)
		tspeed = 2; // slow turn
	else
		tspeed = speed;

	// let movement keys cancel each other out
	if (cv_analog2.value) // Analog
	{
		if (turnright)
			cmd->angleturn = (short)(cmd->angleturn - angleturn[tspeed]);
		if (turnleft)
			cmd->angleturn = (short)(cmd->angleturn + angleturn[tspeed]);
	}

	if (strafe || cv_analog2.value || twodlevel
		|| (players[secondarydisplayplayer].mo && (players[secondarydisplayplayer].mo->flags2 & MF2_TWOD))
		|| players[secondarydisplayplayer].climbing
		|| (players[secondarydisplayplayer].pflags & PF_NIGHTSMODE)
		|| (players[secondarydisplayplayer].pflags & PF_SLIDING)) // Analog
	{
		if (turnright)
			side += sidemove[speed];
		if (turnleft)
			side -= sidemove[speed];

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			side += ((axis * sidemove[1]) >> 10);
		}
	}
	else
	{
		if (turnright)
			cmd->angleturn = (short)(cmd->angleturn - angleturn[tspeed]);
		else if (turnleft)
			cmd->angleturn = (short)(cmd->angleturn + angleturn[tspeed]);

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE should be 1023 (divide by 1024)
			cmd->angleturn = (short)(cmd->angleturn - ((axis * angleturn[1]) >> 10)); // ANALOG!
		}
	}

	axis = Joy2Axis(AXISSTRAFE);
	if (gamepadjoystickmove && axis != 0)
	{
		if (axis < 0)
			side += sidemove[speed];
		else if (axis > 0)
			side -= sidemove[speed];
	}
	else if (analogjoystickmove && axis != 0)
	{
		// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			side += ((axis * sidemove[1]) >> 10);
	}

	// forward with key or button
	axis = Joy2Axis(AXISMOVE);
	if (gamekeydown[gamecontrolbis[gc_forward][0]] ||
		gamekeydown[gamecontrolbis[gc_forward][1]] ||
		(gamepadjoystickmove && axis < 0))
	{
		forward += forwardmove[speed];
	}

	if (gamekeydown[gamecontrolbis[gc_backward][0]] ||
		gamekeydown[gamecontrolbis[gc_backward][1]] ||
		(gamepadjoystickmove && axis > 0))
	{
		forward -= forwardmove[speed];
	}

	if (analogjoystickmove && axis != 0)
		forward -= ((axis * forwardmove[1]) >> 10); // ANALOG!

	// some people strafe left & right with mouse buttons
	if (gamekeydown[gamecontrolbis[gc_straferight][0]] ||
		gamekeydown[gamecontrolbis[gc_straferight][1]])
		side += sidemove[speed];
	if (gamekeydown[gamecontrolbis[gc_strafeleft][0]] ||
		gamekeydown[gamecontrolbis[gc_strafeleft][1]])
		side -= sidemove[speed];

	// Next Weapon
	if (gamekeydown[gamecontrolbis[gc_weaponnext][0]] ||
		gamekeydown[gamecontrolbis[gc_weaponnext][1]])
		cmd->buttons |= BT_WEAPONNEXT;

	// Previous Weapon
	if (gamekeydown[gamecontrolbis[gc_weaponprev][0]] ||
		gamekeydown[gamecontrolbis[gc_weaponprev][1]])
		cmd->buttons |= BT_WEAPONPREV;

	// fire with any button/key
	axis = Joy2Axis(AXISFIRE);
	if (gamekeydown[gamecontrolbis[gc_fire][0]] ||
		gamekeydown[gamecontrolbis[gc_fire][1]] ||
		axis > 0)
		cmd->buttons |= BT_ATTACK;

	// fire normal with any button/key
	axis = Joy2Axis(AXISFIRENORMAL);
	if (gamekeydown[gamecontrolbis[gc_firenormal][0]] ||
		gamekeydown[gamecontrolbis[gc_firenormal][1]] ||
		axis > 0)
		cmd->buttons |= BT_FIRENORMAL;

	if (gamekeydown[gamecontrolbis[gc_lightdash][0]] ||
		gamekeydown[gamecontrolbis[gc_lightdash][1]])
		cmd->buttons |= BT_LIGHTDASH;

	// use with any button/key
	if (gamekeydown[gamecontrolbis[gc_use][0]] ||
		gamekeydown[gamecontrolbis[gc_use][1]])
		cmd->buttons |= BT_USE;

	// Taunts
	if (gamekeydown[gamecontrolbis[gc_taunt][0]] ||
		gamekeydown[gamecontrolbis[gc_taunt][1]])
		cmd->buttons |= BT_TAUNT;

	// Camera Controls
	if (gamekeydown[gamecontrolbis[gc_camleft][0]] ||
		gamekeydown[gamecontrolbis[gc_camleft][1]])
		cmd->buttons |= BT_CAMLEFT;

	if (gamekeydown[gamecontrolbis[gc_camright][0]] ||
		gamekeydown[gamecontrolbis[gc_camright][1]])
		cmd->buttons |= BT_CAMRIGHT;

	if (gamekeydown[gamecontrolbis[gc_camreset][0]] ||
		gamekeydown[gamecontrolbis[gc_camreset][1]])
	{
		if (cv_chasecam2.value)
			P_ResetCamera(&players[secondarydisplayplayer], &camera2);
	}

	// jump button
	if (gamekeydown[gamecontrolbis[gc_jump][0]] ||
		gamekeydown[gamecontrolbis[gc_jump][1]])
		cmd->buttons |= BT_JUMP;

	// mouse look stuff (mouse look is not the same as mouse aim)
	if (mouseaiming)
	{
		keyboard_look = false;

		// looking up/down
		if (cv_invertmouse2.value)
			localaiming2 -= mlook2y<<19;
		else
			localaiming2 += mlook2y<<19;
	}

	axis = Joy2Axis(AXISLOOK);

	if (analogjoystickmove && cv_lookaxis2.value != 0 && axis != 0)
		localaiming2 += axis<<16;

	// spring back if not using keyboard neither mouselookin'
	if (!keyboard_look && cv_lookaxis2.value == 0 && !mouseaiming)
		localaiming2 = 0;

	if (gamekeydown[gamecontrolbis[gc_lookup][0]] ||
		gamekeydown[gamecontrolbis[gc_lookup][1]] ||
		(gamepadjoystickmove && axis < 0))
	{
		localaiming2 += KB_LOOKSPEED;
		keyboard_look = true;
	}
	else if (gamekeydown[gamecontrolbis[gc_lookdown][0]] ||
		gamekeydown[gamecontrolbis[gc_lookdown][1]] ||
		(gamepadjoystickmove && axis > 0))
	{
		localaiming2 -= KB_LOOKSPEED;
		keyboard_look = true;
	}
	else if (gamekeydown[gamecontrolbis[gc_centerview][0]] ||
		gamekeydown[gamecontrolbis[gc_centerview][1]])
		localaiming2 = 0;

	// accept no mlook for network games
	if (!cv_allowmlook.value)
		localaiming2 = 0;

	// look up max (viewheight/2) look down min -(viewheight/2)
	cmd->aiming = G_ClipAimingPitch(&localaiming2);

	if (!mouseaiming && cv_mousemove2.value)
		forward += mouse2y;

	if (strafe || cv_analog2.value
		|| players[secondarydisplayplayer].climbing
		|| (players[secondarydisplayplayer].pflags & PF_SLIDING)) // Analog for mouse
		side = side + mouse2x*2;
	else
		cmd->angleturn = (short)(cmd->angleturn - (mouse2x*8));

	mouse2x = mouse2y = mlook2y = 0;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;
	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	cmd->forwardmove = (signed char)(cmd->forwardmove + forward);
	cmd->sidemove = (signed char)(cmd->sidemove + side);

	localangle2 += (cmd->angleturn<<16);
	cmd->angleturn = (short)(localangle2 >> 16);
}

// User has designated that they want
// analog ON, so tell the game to stop
// fudging with it.
static void UserAnalog_OnChange(void)
{
	if (cv_useranalog.value)
		CV_SetValue(&cv_analog, 1);
	else
		CV_SetValue(&cv_analog, 0);
}

static void UserAnalog2_OnChange(void)
{
	if (cv_useranalog2.value)
		CV_SetValue(&cv_analog2, 1);
	else
		CV_SetValue(&cv_analog2, 0);
}

static void Analog_OnChange(void)
{
	if (!cv_cam_dist.string)
		return;
	if (leveltime > 1)
		CV_SetValue(&cv_cam_dist, 128);

	if (netgame)
		cv_analog.value = 0;
	else if (cv_analog.value)
		CV_SetValue(&cv_cam_dist, 192);
}

static void Analog2_OnChange(void)
{
	if (!cv_cam2_dist.string)
		return;
	if (leveltime > 1)
		CV_SetValue(&cv_cam2_dist, 128);

	if (netgame)
		cv_analog2.value = 0;
	else if (cv_analog2.value)
		CV_SetValue(&cv_cam2_dist, 192);
}

//
// G_DoLoadLevel
//
void G_DoLoadLevel(boolean resetplayer)
{
	int i;

	if (server || (adminplayer == consoleplayer))
		CV_StealthSetValue(&cv_objectplace, 0); // Make sure objectplace is OFF when you first start the level!

	levelstarttic = gametic; // for time calculation

	if (wipegamestate == GS_LEVEL)
		wipegamestate = -1; // force a wipe

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	G_SetGamestate(GS_LEVEL);
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (resetplayer || (playeringame[i] && players[i].playerstate == PST_DEAD))
			players[i].playerstate = PST_REBORN;
	}

	if (!P_SetupLevel(gamemap, gameskill, false))
	{
		// fail so reset game stuff
		Command_ExitGame_f();
		return;
	}

	if (!resetplayer)
		P_FindEmerald();

	displayplayer = consoleplayer; // view the guy you are playing
	if (!splitscreen)
		secondarydisplayplayer = consoleplayer;

	gameaction = ga_nothing;
#ifdef PARANOIA
	Z_CheckHeap(-2);
#endif

	if (cv_chasecam.value)
		P_ResetCamera(&players[displayplayer], &camera);
	if (cv_chasecam2.value && splitscreen)
		P_ResetCamera(&players[secondarydisplayplayer], &camera2);

	// clear cmd building stuff
	memset(gamekeydown, 0, sizeof (gamekeydown));
	for (i = 0;i < JOYAXISSET; i++)
	{
		joyxmove[i] = joyymove[i] = 0;
		joy2xmove[i] = joy2ymove[i] = 0;
	}
	mousex = mousey = 0;
	mouse2x = mouse2y = 0;

	// clear hud messages remains (usually from game startup)
	CON_ClearHUD();

	if (!(cv_debug || devparm || modifiedgame) && !(multiplayer || netgame))
		SetSavedSkin(0, players[0].skin, players[0].prefcolor);
}

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
boolean G_Responder(event_t *ev)
{
	// allow spy mode changes even during the demo
	if (gamestate == GS_LEVEL && ev->type == ev_keydown && ev->data1 == KEY_F12)
	{
		if (!cv_debug && gametype != GT_COOP && gametype != GT_RACE && gametype != GT_CTF && gametype != GT_TAG)
		{
			displayplayer = consoleplayer;
		}
		else
		{
			// spy mode
			while (true)
			{
				displayplayer++;
				if (displayplayer == MAXPLAYERS)
					displayplayer = 0;

				if (displayplayer != consoleplayer && (!playeringame[displayplayer]
					|| (splitscreen && displayplayer == secondarydisplayplayer)))
					continue;

				if (gametype == GT_CTF && players[consoleplayer].ctfteam
					&& players[displayplayer].ctfteam != players[consoleplayer].ctfteam)
					continue;

				if (gametype == GT_TAG && players[consoleplayer].tagit
					&& players[displayplayer].tagit != players[consoleplayer].tagit)
					continue;

				break;
			}

			// change statusbar also if playing back demo
			if (singledemo)
				ST_changeDemoView();

			// tell who's the view
			CONS_Printf("Viewpoint: %s\n", player_names[displayplayer]);

			return true;
		}
	}

	// any other key pops up menu if in demos
	if (gameaction == ga_nothing && !singledemo &&
		(demoplayback || gamestate == GS_DEMOSCREEN || gamestate == GS_TITLESCREEN))
	{
		if (ev->type == ev_keydown && ev->data1 != 301)
		{
			if (timeattacking)
			{
				G_CheckDemoStatus();
				timeattacking = true;
			}
			M_StartControlPanel();
			return true;
		}
		return false;
	}

	if (gamestate == GS_LEVEL)
	{
		if (HU_Responder(ev))
			return true; // chat ate the event
		if (AM_Responder(ev))
			return true; // automap ate it
		// map the event (key/mouse/joy) to a gamecontrol
	}
	// Intro
	else if (gamestate == GS_INTRO || gamestate == GS_INTRO2)
	{
		if (F_IntroResponder(ev))
		{
			D_StartTitle();
			return true;
		}
	}
	else if (gamestate == GS_CUTSCENE)
	{
		if (HU_Responder(ev))
			return true; // chat ate the event

		if (F_CutsceneResponder(ev))
		{
			D_StartTitle();
			return true;
		}
	}

	else if (gamestate == GS_CREDITS)
	{
		if (HU_Responder(ev))
			return true; // chat ate the event

		if (F_CreditResponder(ev))
		{
			F_StartGameEvaluation();
			return true;
		}
	}

	// Demo End
	else if (gamestate == GS_DEMOEND || gamestate == GS_EVALUATION || gamestate == GS_CREDITS)
		return true;

	else if (gamestate == GS_INTERMISSION)
		if (HU_Responder(ev))
			return true; // chat ate the event

	// update keys current state
	G_MapEventsToControls(ev);

	switch (ev->type)
	{
		case ev_keydown:
			if (ev->data1 == gamecontrol[gc_pause][0]
				|| ev->data1 == gamecontrol[gc_pause][1])
			{
				// don't let busy scripts prevent pausing
				char buf = (char)(!paused);

				if (cv_pause.value == 1 || server || (adminplayer == consoleplayer))
				{
					if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
					{
						CONS_Printf(PAUSEINFO);
						return true;
					}

					SendNetXCmd(XD_PAUSE, &buf, 1);
				}
				else
					CONS_Printf(SERVERPAUSE);
				return true;
			}
			return true;

		case ev_keyup:
			return false; // always let key up events filter down

		case ev_mouse:
			return true; // eat events

		case ev_joystick:
			return true; // eat events

		case ev_joystick2:
			return true; // eat events


		default:
			break;
	}

	return false;
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker(void)
{
	ULONG i;
	int buf;
	ticcmd_t *cmd;

	P_MapStart();
	// do player reborns if needed
	if (gamestate == GS_LEVEL)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (players[i].playerstate == PST_REBORN)
				{
					G_DoReborn(i);
				}
			}
		}
	}
	P_MapEnd();

	// do things to change the game state
	while (gameaction != ga_nothing)
		switch (gameaction)
		{
			case ga_completed :
				if (lastdraw)
					goto dontcompleteyet; // Just one more tic man, I swear!
				else
					G_DoCompleted();
				break;
			case ga_worlddone : G_DoWorldDone(); break;
			case ga_nothing : break;
			default : I_Error("gameaction = %d\n", gameaction);
		}

dontcompleteyet:

	buf = gametic % BACKUPTICS;

	// read/write demo and check turbo cheat
	for (i = 0; i < MAXPLAYERS; i++)
	{
		// BP: i == 0 for playback of demos 1.29 now new players is added with xcmd
		if ((playeringame[i] || i == 0) /*&& !dedicated*/)
		{
			cmd = &players[i].cmd;

			if (demoplayback)
				G_ReadDemoTiccmd(cmd, i);
			else
				M_Memcpy(cmd, &netcmds[buf][i], sizeof (ticcmd_t));

			if (demorecording)
				G_WriteDemoTiccmd(cmd, i);
		}
	}

	// do main actions
	switch (gamestate)
	{
		case GS_LEVEL:
			P_Ticker(); // tic the game
			ST_Ticker();
			AM_Ticker();
			HU_Ticker();
			break;

		case GS_INTERMISSION:
			Y_Ticker();
			HU_Ticker();
			break;

		case GS_TIMEATTACK:
			break;

		case GS_INTRO:
		case GS_INTRO2:
			F_IntroTicker();
			break;

		case GS_CUTSCENE:
			F_CutsceneTicker();
			HU_Ticker();
			break;

		case GS_DEMOEND:
			F_DemoEndTicker();
			break;

		case GS_EVALUATION:
			F_GameEvaluationTicker();
			break;

		case GS_CREDITS:
			F_CreditTicker();
			HU_Ticker();
			break;

		case GS_TITLESCREEN:
			F_TitleScreenTicker();
			break;

		case GS_DEMOSCREEN:
			D_PageTicker();
			break;

		case GS_WAITINGPLAYERS:
			F_TitleScreenTicker(); // Fixes title sky stopping
		case GS_DEDICATEDSERVER:
		case GS_NULL:
			break; // do nothing
	}
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_PlayerFinishLevel
// Called when a player completes a level.
//
static inline void G_PlayerFinishLevel(int player)
{
	player_t *p;

	p = &players[player];

	memset(p->powers, 0, sizeof (p->powers));
	p->ringweapons = 0;

	p->mo->flags2 &= ~MF2_SHADOW; // cancel invisibility
	p->bonuscount = 0;
	p->starpostangle = 0;
	p->starposttime = 0;
	p->starpostx = 0;
	p->starposty = 0;
	p->starpostz = 0;
	p->starpostnum = 0;
	p->starpostbit = 0;

	if (rendermode == render_soft)
		V_SetPaletteLump("PLAYPAL"); // Reset the palette
}

//
// G_PlayerReborn
// Called after a player dies. Almost everything is cleared and initialized.
//
void G_PlayerReborn(int player)
{
	player_t *p;
	int score;
	int lives;
	long continues;
	int xtralife;
	int charability;
	int normalspeed;
	int runspeed;
	int thrustfactor;
	int accelstart;
	int acceleration;
	long charflags;
	long pflags = 0;
	mobjtype_t slingitem;
	int thokitem;
	int spinitem;
	int actionspd;
	int mindash;
	int maxdash;
	int supercolor;
	int starttrans;
	int endtrans;
	int prefcolor;
	int tagit;
	int ctfteam;
	int tagcount;
	int starposttime;
	int starpostx;
	int starposty;
	int starpostz;
	int starpostnum;
	int starpostangle;
	unsigned int starpostbit;
	int jumpfactor;
	int exiting;
	int numboxes;
	int laps;
	int totalring;
	int dbginfo;
	byte mare;
	int skincolor;
	int skin;

	score = players[player].score;
	lives = players[player].lives;
	continues = players[player].continues;
	pflags |= (players[player].pflags & PF_TIMEOVER) * PF_TIMEOVER;
	pflags |= (players[player].pflags & PF_AUTOAIM) * PF_AUTOAIM;
	xtralife = players[player].xtralife;
	tagit = players[player].tagit;
	ctfteam = players[player].ctfteam;
	tagcount = players[player].tagcount;
	exiting = players[player].exiting;

	numboxes = players[player].numboxes;
	laps = players[player].laps;
	totalring = players[player].totalring;
	dbginfo = players[player].dbginfo;

	skincolor = players[player].skincolor;
	skin = players[player].skin;
	charability = players[player].charability;
	normalspeed = players[player].normalspeed;
	runspeed = players[player].runspeed;
	thrustfactor = players[player].thrustfactor;
	accelstart = players[player].accelstart;
	acceleration = players[player].acceleration;
	charflags = players[player].charflags;
	supercolor = players[player].supercolor;
	starttrans = players[player].starttranscolor;
	endtrans = players[player].endtranscolor;
	prefcolor = players[player].prefcolor;

	starposttime = players[player].starposttime;
	starpostx = players[player].starpostx;
	starposty = players[player].starposty;
	starpostz = players[player].starpostz;
	starpostnum = players[player].starpostnum;
	starpostangle = players[player].starpostangle;
	starpostbit = players[player].starpostbit;
	jumpfactor = players[player].jumpfactor;
	slingitem = players[player].slingitem;
	thokitem = players[player].thokitem;
	spinitem = players[player].spinitem;
	actionspd = players[player].actionspd;
	mindash = players[player].mindash;
	maxdash = players[player].maxdash;

	mare = players[player].mare;

	p = &players[player];
	memset(p, 0, sizeof (*p));

	p->score = score;
	p->lives = lives;
	p->continues = continues;
	p->pflags = pflags;
	p->xtralife = xtralife;
	p->tagit = tagit;
	p->ctfteam = ctfteam;
	p->tagcount = tagcount;

	// save player config truth reborn
	p->skincolor = skincolor;
	p->skin = skin;
	p->charability = charability;
	p->normalspeed = normalspeed;
	p->runspeed = runspeed;
	p->thrustfactor = thrustfactor;
	p->accelstart = accelstart;
	p->acceleration = acceleration;
	p->charflags = charflags;
	p->supercolor = supercolor;
	p->starttranscolor = starttrans;
	p->endtranscolor = endtrans;
	p->prefcolor = prefcolor;
	p->slingitem = slingitem;
	p->thokitem = thokitem;
	p->spinitem = spinitem;
	p->actionspd = actionspd;
	p->mindash = mindash;
	p->maxdash = maxdash;

	p->starposttime = starposttime;
	p->starpostx = starpostx;
	p->starposty = starposty;
	p->starpostz = starpostz;
	p->starpostnum = starpostnum;
	p->starpostangle = starpostangle;
	p->starpostbit = starpostbit;
	p->jumpfactor = jumpfactor;
	p->exiting = exiting;

	p->numboxes = numboxes;
	p->laps = laps;
	p->totalring = totalring;
	p->dbginfo = dbginfo;

	p->mare = mare;

	// Don't do anything immediately
	p->pflags |= PF_USEDOWN;
	p->pflags |= PF_ATTACKDOWN;
	p->pflags |= PF_JUMPDOWN;

	p->playerstate = PST_LIVE;
	p->health = 1; // 0 rings

	if (netgame || multiplayer)
		p->powers[pw_flashing] = flashingtics-1; // Babysitting deterrent

	if (P_IsLocalPlayer(p))
	{
		if (!(mapmusic & 2048))
			mapmusic = mapheaderinfo[gamemap-1].musicslot;

		S_ChangeMusic(mapmusic & 2047, true);
	}

	if (gametype == GT_COOP)
		P_FindEmerald(); // scan for emeralds to hunt for

	// If NiGHTS, find lowest mare to start with.
	p->mare = P_FindLowestMare();

	if (cv_debug)
		CONS_Printf("Current mare is %d\n", p->mare);

	if (p->mare == 255)
		p->mare = 0;

	// Check to make sure their color didn't change somehow...
	if (gametype == GT_CTF)
	{
		if (p->ctfteam == 1 && p->skincolor != 6)
		{
			if (p == &players[consoleplayer])
				CV_SetValue(&cv_playercolor, 6);
			else if (p == &players[secondarydisplayplayer])
				CV_SetValue(&cv_playercolor2, 6);
		}
		else if (p->ctfteam == 2 && p->skincolor != 7)
		{
			if (p == &players[consoleplayer])
				CV_SetValue(&cv_playercolor, 7);
			else if (p == &players[secondarydisplayplayer])
				CV_SetValue(&cv_playercolor2, 7);
		}
	}
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
static boolean G_CheckSpot(int playernum, mapthing_t *mthing)
{
	fixed_t x;
	fixed_t y;
	subsector_t *ss;
	int i;

	// maybe there is no player start
	if (!mthing)
		return false;

	if (!players[playernum].mo)
	{
		// first spawn of level
		for (i = 0; i < playernum; i++)
			if (playeringame[i] && players[i].mo
				&& players[i].mo->x == mthing->x << FRACBITS
				&& players[i].mo->y == mthing->y << FRACBITS)
			{
				return false;
			}
		return true;
	}

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;
	ss = R_PointInSubsector(x, y);

	if (!P_CheckPosition(players[playernum].mo, x, y, x, y))
		return false;

	return true;
}

//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
void G_DeathMatchSpawnPlayer(int playernum)
{
	int i, j;

	if (numdmstarts)
		for (j = 0; j < 64; j++)
		{
			i = P_Random() % numdmstarts;
			if (G_CheckSpot(playernum, deathmatchstarts[i]))
			{
				P_SpawnPlayer(deathmatchstarts[i], playernum);
				return;
			}
		}

	// Use a coop start dependent on playernum
	CONS_Printf("No deathmatch start in this map - shifting to player starts to avoid crash...\n");

	if (!numcoopstarts)
		I_Error("There aren't enough starts in this map!\n");

	i = playernum % numcoopstarts;
	P_SpawnPlayer(playerstarts[i], playernum);
}

// G_CoopSpawnPlayer
//
void G_CoopSpawnPlayer(int playernum, boolean starpost)
{
	int i, j;

	// CTF Start Spawns
	if (gametype == GT_CTF)
	{
		switch (players[playernum].ctfteam)
		{
			case 1: // Red Team
				if (!numredctfstarts)
				{
					CONS_Printf("No Red Team start in this map, resorting to Deathmatch starts!\n");
					goto startdeath;
				}

				for (j = 0; j < 32; j++)
				{
					i = P_Random() % numredctfstarts;
					if (G_CheckSpot(playernum, redctfstarts[i]))
					{
						P_SpawnPlayer(redctfstarts[i], playernum);
						return;
					}
				}
				break;
			case 2: // Blue Team
				if (!numbluectfstarts)
				{
					CONS_Printf("No Blue Team start in this map, resorting to Deathmatch starts!\n");
					goto startdeath;
				}

				for (j = 0; j < 32; j++)
				{
					i = P_Random() % numbluectfstarts;
					if (G_CheckSpot(playernum, bluectfstarts[i]))
					{
						P_SpawnPlayer(bluectfstarts[i], playernum);
						return;
					}
				}
				break;
			default:
startdeath:
				G_DeathMatchSpawnPlayer(playernum);
				return;
		}
	}

	if (starpost)
	{
		P_SpawnStarpostPlayer(players[playernum].mo, playernum);
		return;
	}

	// no deathmatch use the spot
	if (G_CheckSpot(playernum, playerstarts[playernum]))
	{
		P_SpawnPlayer(playerstarts[playernum], playernum);
		return;
	}

	// use the player start anyway if it exists
	if (playerstarts[playernum])
	{
		P_SpawnPlayer(playerstarts[playernum], playernum);
		return;
	}

	// resort to the player one start, and if there is none, we're screwed
	if (!playerstarts[0])
		I_Error("Not enough starts in this map!\n");

	P_SpawnPlayer(playerstarts[0], playernum);
}

//
// G_DoReborn
//
void G_DoReborn(int playernum)
{
	player_t *player = &players[playernum];
	boolean starpost = false;

	if (countdowntimeup || (!multiplayer && gametype == GT_COOP))
	{
		// reload the level from scratch
		if (countdowntimeup)
		{
			player->starpostangle = 0;
			player->starposttime = 0;
			player->starpostx = 0;
			player->starposty = 0;
			player->starpostz = 0;
			player->starpostnum = 0;
			player->starpostbit = 0;
		}
		if (mapheaderinfo[gamemap-1].noreload && !imcontinuing)
		{
			int i;

			P_LoadThingsOnly();

			P_RehitStarposts();

			// Do a wipe
			wipegamestate = -1;

			if (player->starposttime)
				starpost = true;

			if (server || (adminplayer == consoleplayer))
				CV_StealthSetValue(&cv_objectplace, 0); // Make sure objectplace is OFF when you first start the level!

			if (cv_chasecam.value)
				P_ResetCamera(&players[displayplayer], &camera);
			if (cv_chasecam2.value && splitscreen)
				P_ResetCamera(&players[secondarydisplayplayer], &camera2);

			// clear cmd building stuff
			memset(gamekeydown, 0, sizeof (gamekeydown));
			for (i = 0;i < JOYAXISSET; i++)
			{
				joyxmove[i] = joyymove[i] = 0;
				joy2xmove[i] = joy2ymove[i] = 0;
			}
			mousex = mousey = 0;
			mouse2x = mouse2y = 0;

			// clear hud messages remains (usually from game startup)
			CON_ClearHUD();

			// Starpost support
			G_CoopSpawnPlayer(playernum, starpost);
		}
		else
			G_DoLoadLevel(true);

		imcontinuing = false;
	}
	else
	{
		// respawn at the start

		if (player->starposttime)
			starpost = true;

		// first dissasociate the corpse
		if (player->mo)
		{
			player->mo->player = NULL;
			player->mo->flags2 &= ~MF2_DONTDRAW;
			// Don't leave your carcass stuck 10-billion feet in the ground!
			P_SetMobjState(player->mo, S_DISS);
		}
		// spawn at random spot if in death match
		if (gametype == GT_MATCH || gametype == GT_TAG
#ifdef CHAOSISNOTDEADYET
			|| gametype == GT_CHAOS
#endif
			)
		{
			G_DeathMatchSpawnPlayer(playernum);
			return;
		}

		// Starpost support
		G_CoopSpawnPlayer(playernum, starpost);
	}
}

void G_AddPlayer(int playernum)
{
	player_t *p = &players[playernum];

	p->playerstate = PST_REBORN;
}

static boolean secretexit;

void G_ExitLevel(void)
{
	if (gamestate == GS_LEVEL)
	{
		secretexit = false;
		gameaction = ga_completed;
		lastdraw = true;
	}
}

/** Get the typeoflevel flag needed to indicate support of a gametype.
  * In single-player, this always returns TOL_SP.
  * \param gametype The gametype for which support is desired.
  * \return The typeoflevel flag to check for that gametype.
  * \author Graue <graue@oceanbase.org>
  */
static short TOLFlag(int pgametype)
{
	if (!multiplayer)          return TOL_SP;
	if (pgametype == GT_COOP)  return TOL_COOP;
	if (pgametype == GT_RACE)  return TOL_RACE;
	if (pgametype == GT_MATCH) return TOL_MATCH;
#ifdef CHAOSISNOTDEADYET
	if (pgametype == GT_CHAOS) return TOL_CHAOS;
#endif
	if (pgametype == GT_TAG)   return TOL_TAG;
	if (pgametype == GT_CTF)   return TOL_CTF;

	CONS_Printf("Error: Weird gametype! %d\n", pgametype);
	return MAXSHORT;
}

/** Select a random map with the given typeoflevel flags.
  * If no map has those flags, this arbitrarily gives you map 1.
  * \param tolflags The typeoflevel flags to insist on. Other bits may
  *                 be on too, but all of these must be on.
  * \return A random map with those flags, 1-based, or 1 if no map
  *         has those flags.
  * \author Graue <graue@oceanbase.org>
  */
static int RandMap(short tolflags)
{
	XBOXSTATIC int okmaps[NUMMAPS];
	int numokmaps = 0;
	int ix;
	int mapnum;

	// Find all the maps that are ok and and put them in an array.
	for (ix = 0; ix < NUMMAPS; ix++)
		if ((mapheaderinfo[ix].typeoflevel & tolflags) == tolflags)
			okmaps[numokmaps++] = ix;

	if (numokmaps == 0)
		return 1; // Sorry, none match. You get MAP01.

	mapnum = M_Random() << 8;
	mapnum |= M_Random();
	mapnum %= numokmaps;

	return okmaps[mapnum]+1;
}

//
// G_DoCompleted
//
void G_DoCompleted(void)
{
	int i;
	boolean gottoken = false;

	tokenlist = 0; // Reset the list

	gameaction = ga_nothing;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
			G_PlayerFinishLevel(i); // take away cards and stuff

	if (automapactive)
		AM_Stop();

	S_StopSounds();

	prevmap = gamemap-1;

	// go to next level
	// nextmap is 0-based, unlike gamemap
	if (nextmapoverride != 0)
		nextmap = nextmapoverride-1;
	else
		nextmap = mapheaderinfo[gamemap-1].nextlevel-1;

	// Remember last map for when you come out of the special stage.
	if (gamemap < sstage_start || gamemap > sstage_end)
		lastmap = nextmap;

	// If nextmap is actually going to get used, make sure it points to
	// a map of the proper gametype -- skip levels that don't support
	// the current gametype. (Helps avoid playing boss levels in Race,
	// for instance).
	if (!token && (gamemap < sstage_start || gamemap > sstage_end))
	{
		short tolflag = TOLFlag(gametype);

		if (nextmap >= 0 && nextmap < NUMMAPS
			&& !(mapheaderinfo[nextmap].typeoflevel & tolflag))
		{
			register int cm = nextmap;
			byte visitedmap[(NUMMAPS+7)/8];

			memset(visitedmap, 0, sizeof (visitedmap));

			while (!(mapheaderinfo[cm].typeoflevel & tolflag))
			{
				visitedmap[cm/8] |= (1<<(cm%8));
				cm = mapheaderinfo[cm].nextlevel-1;
				if (cm >= NUMMAPS || cm < 0) // out of range (either 1100-1102 or error)
					break;
				if (visitedmap[cm/8] & (1<<(cm%8))) // smells familiar
				{
					// We got stuck in a loop, came back
					// to the map we started on without
					// finding one supporting the current
					// gametype. Thus, print a warning,
					// and just use this map anyways.
					CONS_Printf("Warning: Can't find a "
						"compatible map after map %d; "
						"using map %d even though it "
						"is not compatible with the "
						"current gametype\n",
							prevmap+1, cm+1);
					break;
				}
			}

			nextmap = cm;
		}
	}

	if (nextmap < 0 || (nextmap >= NUMMAPS && nextmap < 1100-1)
		|| nextmap > 1102-1)
	{
		I_Error("Followed map %d to invalid map %d\n", prevmap + 1, nextmap + 1);
	}

	// wrap around in race
	if (nextmap >= 1100-1 && nextmap <= 1102-1 && gametype == GT_RACE)
		nextmap = racestage_start-1;

	if (gametype == GT_COOP && token)
	{
		if (!(emeralds & EMERALD1))
			nextmap = sstage_start - 1; // Special Stage 1
		else if (!(emeralds & EMERALD2))
			nextmap = sstage_start; // Special Stage 2
		else if (!(emeralds & EMERALD3))
			nextmap = sstage_start + 1; // Special Stage 3
		else if (!(emeralds & EMERALD4))
			nextmap = sstage_start + 2; // Special Stage 4
		else if (!(emeralds & EMERALD5))
			nextmap = sstage_start + 3; // Special Stage 5
		else if (!(emeralds & EMERALD6))
			nextmap = sstage_start + 4; // Special Stage 6
		else if (!(emeralds & EMERALD7))
			nextmap = sstage_start + 5; // Special Stage 7
		else
		{
			gottoken = false;
			goto skipit;
		}

		token--;
		gottoken = true;
	}
skipit:

	if ((gamemap >= sstage_start && gamemap <= sstage_end) && !gottoken)
	{
		nextmap = lastmap; // Exiting from a special stage? Go back to the game. Tails 08-11-2001
	}

	automapactive = false;

	if (gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF
#ifdef CHAOSISNOTDEADYET
		|| gametype == GT_CHAOS
#endif
		)
	{
		if (cv_advancemap.value == 0) // Stay on same map.
			nextmap = prevmap;
		else if (cv_advancemap.value == 2) // Go to random map.
			nextmap = RandMap(TOLFlag(gametype))-1;
	}

	if (skipstats)
		G_AfterIntermission();
	else
	{
		G_SetGamestate(GS_INTERMISSION);
		Y_StartIntermission();
	}
}

void G_AfterIntermission(void)
{
	cechotimer = 0;

	if (mapheaderinfo[gamemap-1].cutscenenum) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[gamemap-1].cutscenenum-1, false, false);
	else
	{
		if (nextmap < 1100-1)
			G_NextLevel();
		else
			Y_EndGame();
	}
}

//
// G_NextLevel (WorldDone)
//
// init next level or go to the final scene
// called by end of intermission screen (y_inter)
//
void G_NextLevel(void)
{
	gameaction = ga_worlddone;
}

static void G_DoWorldDone(void)
{
	// not in demo because demo have the mapcommand on it
	if (server && !demoplayback)
	{
		int nextgametype;

		// for custom exit (linetype 71) that changes gametype
		if (nextmapgametype != -1)
			nextgametype = nextmapgametype;
		else
			nextgametype = gametype;

		if (gametype == GT_COOP && nextgametype == GT_COOP)
			// don't reset player between maps
			D_MapChange(nextmap+1, nextgametype, gameskill, 0, 0, false, false);
		else
			// resetplayer in match/chaos/tag/CTF/race for more equality
			D_MapChange(nextmap+1, nextgametype, gameskill, 1, 0, false, false);
	}

	gameaction = ga_nothing;
}

//
// G_LoadGameSettings
//
// Sets a tad of default info we need.
void G_LoadGameSettings(void)
{
	// defaults
	spstage_start = 1;
	sstage_start = 50;
	sstage_end = 56;
	racestage_start = 1;
}

// G_LoadGameData
// Loads the main data file, which stores information such as emblems found, etc.
void G_LoadGameData(void)
{
	size_t length;
	int i;
	boolean modded = false;
	boolean corrupt = false;

	for (i = 0; i < MAXEMBLEMS; i++)
		emblemlocations[i].collected = false;

	memset(mapvisited, false, sizeof (boolean) * NUMMAPS);

	foundeggs = 0;
	totalplaytime = 0;
	grade = 0;
	timesbeaten = 0;

	length = FIL_ReadFile(va(pandf, srb2home, gamedatafilename), &savebuffer);
	if (!length)
	{
		gamedataloaded = 1; // Aw, no game data. Their loss!
		return;
	}

	save_p = savebuffer;

	foundeggs = (READULONG(save_p)-30)/5;
	totalplaytime = READULONG(save_p);
	grade = (READULONG(save_p)-75)/4;

	for (i = 0; i < NUMMAPS; i++)
		mapvisited[i] = READBYTE(save_p);

	for (i = 0; i < MAXEMBLEMS; i++)
		emblemlocations[i].collected = READBYTE(save_p)-125-(i/4);

	READBYTE(save_p); // DUMMY BYTE REMOVE FOR v1.1

	modded = READBYTE(save_p);
	timesbeaten = (READULONG(save_p)/4)+2;

	// Initialize the table
	memset(timedata, 0, sizeof (timeattack_t) * NUMMAPS);

	for (i = 0; i < NUMMAPS; i++)
		timedata[i].time = READULONG(save_p);

	// Aha! Someone's been screwing with the save file!
	if ((modded && !savemoddata))
		corrupt = true;
	else if (modded != true && modded != false)
		corrupt = true;
	else if (grade > 2047)
		corrupt = true;
	else
	{
		for (i = 0; i < MAXEMBLEMS; i++)
		{
			if (emblemlocations[i].collected != true
				&& emblemlocations[i].collected != false)
			{
				corrupt = true;
				break;
			}
		}

		if (!corrupt)
		{
			for (i = 0; i < NUMMAPS; i++)
			{
				if (mapvisited[i] != true
					&& mapvisited[i] != false)
				{
					corrupt = true;
					break;
				}
			}
		}
	}

	if (corrupt)
	{
		const char *gdfolder = "the SRB2 folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;
		I_Error("Corrupt game data file.\nDelete %s(maybe in %s)\nand try again.", gamedatafilename, gdfolder);
	}

	// done
	Z_Free(savebuffer);
	save_p = NULL;
	gamedataloaded = 1;

	// Is it Spazzo?
	{
		FILE *f = fopen("C:\\tmp.log", "r");

		if (f)
		{
			itsSpazzo = true;
			fclose (f);
		}
	}
}

// G_SaveGameData
// Saves the main data file, which stores information such as emblems found, etc.
void G_SaveGameData(void)
{
	size_t length;
	int i;
	ULONG stemp;
	byte btemp;

	if (!gamedataloaded)
		return;

	save_p = savebuffer = (byte *)malloc(GAMEDATASIZE);
	if (!save_p)
	{
		CONS_Printf("No more free memory for saving game data\n");
		return;
	}

	if (modifiedgame && !savemoddata)
	{
		free(savebuffer);
		save_p = savebuffer = NULL;
		return;
	}

	// Cipher
	// Author: Caesar <caesar@rome.it>
	//
	stemp =(foundeggs*5)+30;
	WRITEULONG(save_p, stemp);
	WRITEULONG(save_p, totalplaytime);
	stemp = (grade*4)+75;
	WRITEULONG(save_p, stemp);

	for (i = 0; i < NUMMAPS; i++)
		WRITEBYTE(save_p, mapvisited[i]);

	for (i = 0; i < MAXEMBLEMS; i++)
	{
		btemp = (byte)(emblemlocations[i].collected+125+(i/4));
		WRITEBYTE(save_p, btemp);
	}

	WRITEBYTE(save_p, 0); // DUMMY BYTE REMOVE FOR v1.1

	btemp = (byte)(savemoddata || modifiedgame);
	WRITEBYTE(save_p, btemp);
	stemp = (timesbeaten-2)*4;
	WRITEULONG(save_p, stemp);

	for (i = 0; i < NUMMAPS; i++)
		WRITEULONG(save_p, timedata[i].time);

	length = save_p - savebuffer;

	FIL_WriteFile(va(pandf, srb2home, gamedatafilename), savebuffer, length);
	free(savebuffer);
	save_p = savebuffer = NULL;
}

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
#define VERSIONSIZE 16

void G_LoadGame(unsigned int slot)
{
	size_t length;
	char vcheck[VERSIONSIZE];
	char savename[255];

	sprintf(savename, savegamename, slot);

	length = FIL_ReadFile(savename, &savebuffer);
	if (!length)
	{
		CONS_Printf(CANTREADFILE, savename);
		return;
	}

	save_p = savebuffer;

	memset(vcheck, 0, sizeof (vcheck));
	sprintf(vcheck, "version %d", VERSION);
	if (strcmp((const char *)save_p, (const char *)vcheck))
	{
		M_StartMessage("Save game from different version\n\nPress ESC\n", NULL, MM_NOTHING);
		Z_Free(savebuffer);
		save_p = savebuffer = NULL;
		return; // bad version
	}
	save_p += VERSIONSIZE;

//	if (demoplayback) // reset game engine
//		G_StopDemo();

//	paused = false;
//	automapactive = false;

	// dearchive all the modifications
	if (!P_LoadGame())
	{
		M_StartMessage("savegame file corrupted\n\nPress ESC\n", NULL, MM_NOTHING);
		Command_ExitGame_f();
		Z_Free(savebuffer);
		save_p = savebuffer = NULL;
		return;
	}

	// done
	Z_Free(savebuffer);
	save_p = savebuffer = NULL;

//	gameaction = ga_nothing;
//	G_SetGamestate(GS_LEVEL);
	displayplayer = consoleplayer;
	multiplayer = splitscreen = false;

//	G_DeferedInitNew(sk_medium, G_BuildMapName(1), 0, 0, 1);
	if (setsizeneeded)
		R_ExecuteSetViewSize();

	CON_ToggleOff();
}

//
// G_SaveGame
// Saves your game.
//
void G_SaveGame(unsigned int savegameslot)
{
	boolean saved;
	char savename[256] = "";
	const char *backup;

	sprintf(savename, savegamename, savegameslot);
	backup = va("%s",savename);

	gameaction = ga_nothing;
	{
		char name[VERSIONSIZE];
		size_t length;

		save_p = savebuffer = (byte *)malloc(SAVEGAMESIZE);
		if (!save_p)
		{
			CONS_Printf("No more free memory for savegame\n");
			return;
		}

		memset(name, 0, sizeof (name));
		sprintf(name, "version %d", VERSION);
		WRITEMEM(save_p, name, VERSIONSIZE);

		P_SaveGame();

		length = SAVEGAMESIZE;
		saved = FIL_WriteFile(backup, savebuffer, length);
		free(savebuffer);
		save_p = savebuffer = NULL;
		lastmapsaved = gamemap;
	}

	gameaction = ga_nothing;

	if (cv_debug && saved)
		CONS_Printf(GGSAVED);
	else if (!saved)
		CONS_Printf("Error while writing to %s for save slot %u, base: %s\n",backup,savegameslot,savegamename);
}

//
// G_DeferedInitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//
void G_DeferedInitNew(skill_t skill, const char *mapname, int pickedchar, boolean SSSG, boolean FLS)
{
	paused = false;

	if (demoplayback)
		COM_BufAddText("stopdemo\n");

	// this leave the actual game if needed
	SV_StartSinglePlayerServer();

	if (splitscreen != SSSG)
	{
		splitscreen = SSSG;
		SplitScreen_OnChange();
	}

	SetSavedSkin(0, pickedchar, atoi(skins[pickedchar].prefcolor));

	if (mapname)
		D_MapChange(M_MapNumber(mapname[3], mapname[4]), gametype, skill, 1, 1, false, FLS);
}

//
// This is the map command interpretation something like Command_Map_f
//
// called at: map cmd execution, doloadgame, doplaydemo
void G_InitNew(skill_t skill, const char *mapname, boolean resetplayer, boolean skipprecutscene)
{
	int i;

	if (paused)
	{
		paused = false;
		S_ResumeSound();
	}

	if (skill < sk_easy)
		skill = sk_easy;
	else if (skill > sk_insane)
		skill = sk_insane;

	if (!(grade & 128) && skill == sk_insane) // Nice try, haxor.
		skill = sk_nightmare;

	if (!netgame)
	{
		if (!(demoplayback || demorecording))
			P_SetRandIndex((byte)(totalplaytime % 256));
		else // Constant seed for demos
			P_SetRandIndex(0);
	}

	if (resetplayer)
	{
		// Clear a bunch of variables
		lastmap = tokenlist = token = sstimer = redscore = bluescore = 0;
		countdown = countdown2 = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			players[i].playerstate = PST_REBORN;
			players[i].starpostangle = players[i].starpostnum = players[i].starposttime = 0;
			players[i].starpostx = players[i].starposty = players[i].starpostz = 0;
			players[i].starpostbit = 0;

			// set lives/continues based on game skill
			switch (skill)
			{
				case sk_insane:
					players[i].lives = 1;
					players[i].continues = 0;
					break;
				case sk_nightmare:
				case sk_hard:
				case sk_medium:
					players[i].lives = 3;
					players[i].continues = 1;
					break;
				case sk_easy:
					players[i].lives = 5;
					players[i].continues = 2;
					break;
				default: // Oops!?
					CONS_Printf(UNKNOWN_GAMESKILL);
					break;
			}

			players[i].score = players[i].tagcount = players[i].xtralife = 0;
		}

		if (!(cv_debug || devparm) && !(multiplayer || netgame))
			SetSavedSkin(0, players[0].skin, players[0].prefcolor);
	}

	// internal game map
	// well this check is useless because it is done before (d_netcmd.c::command_map_f)
	// but in case of for demos....
	if (W_CheckNumForName(mapname) == LUMPERROR)
	{
		I_Error("Internal game map '%s' not found\n", mapname);
		Command_ExitGame_f();
		return;
	}

	gamemap = (short)M_MapNumber(mapname[3], mapname[4]); // get xx out of MAPxx
	maptol = mapheaderinfo[gamemap-1].typeoflevel;
	globalweather = mapheaderinfo[gamemap-1].weather;

	gameskill = skill;
	playerdeadview = false;
	automapactive = false;

	if (!skipprecutscene && mapheaderinfo[gamemap-1].precutscenenum) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[gamemap-1].precutscenenum-1, true, resetplayer);
	else
		G_DoLoadLevel(resetplayer);

	if (netgame)
	{
		const int actnum = mapheaderinfo[gamemap-1].actnum;
		CONS_Printf("Map is now \"%s: %s", mapname, mapheaderinfo[gamemap-1].lvlttl);
		if (!mapheaderinfo[gamemap-1].nozone)
			CONS_Printf(" ZONE");
		if (actnum > 0)
			CONS_Printf(" %2d", actnum);
		CONS_Printf("\"\n");
	}
}

//
// DEMO RECORDING
//

#define ZT_FWD          0x01
#define ZT_SIDE         0x02
#define ZT_ANGLE        0x04
#define ZT_BUTTONS      0x08
#define ZT_AIMING       0x10
#define ZT_BUTTONS2     0x20
#define ZT_EXTRADATA    0x40
#define DEMOMARKER      0x80 // demoend

static ticcmd_t oldcmd[MAXPLAYERS];

static void G_ReadDemoTiccmd(ticcmd_t *cmd, int playernum)
{
	byte ziptic;

	ziptic = READBYTE(demo_p);

	if (ziptic & ZT_FWD)
		oldcmd[playernum].forwardmove = READCHAR(demo_p);
	if (ziptic & ZT_SIDE)
		oldcmd[playernum].sidemove = READCHAR(demo_p);
	if (ziptic & ZT_ANGLE)
		oldcmd[playernum].angleturn = READSHORT(demo_p);
	if (ziptic & ZT_BUTTONS)
		oldcmd[playernum].buttons = (USHORT)(READBYTE(demo_p)<<8); //buttons in a USHORT, not a byte
	if (ziptic & ZT_BUTTONS2)
		oldcmd[playernum].buttons = (USHORT)(oldcmd[playernum].buttons+READBYTE(demo_p)); //ZT_BUTTONS2 always comes with ZT_BUTTONS
	if (ziptic & ZT_AIMING)
		oldcmd[playernum].aiming = READSHORT(demo_p);
	if (ziptic & ZT_EXTRADATA)
		ReadLmpExtraData(&demo_p, playernum);
	else
		ReadLmpExtraData(0, playernum);

	M_Memcpy(cmd, &(oldcmd[playernum]), sizeof (ticcmd_t));

	if (*demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

static void G_WriteDemoTiccmd(ticcmd_t *cmd, int playernum)
{
	char ziptic = 0;
	byte *ziptic_p;

	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	if (cmd->forwardmove != oldcmd[playernum].forwardmove)
	{
		WRITEBYTE(demo_p,cmd->forwardmove);
		oldcmd[playernum].forwardmove = cmd->forwardmove;
		ziptic |= ZT_FWD;
	}

	if (cmd->sidemove != oldcmd[playernum].sidemove)
	{
		WRITEBYTE(demo_p,cmd->sidemove);
		oldcmd[playernum].sidemove = cmd->sidemove;
		ziptic |= ZT_SIDE;
	}

	if (cmd->angleturn != oldcmd[playernum].angleturn)
	{
		WRITESHORT(demo_p,cmd->angleturn);
		oldcmd[playernum].angleturn = cmd->angleturn;
		ziptic |= ZT_ANGLE;
	}

	if (cmd->buttons != oldcmd[playernum].buttons)
	{
		WRITEBYTE(demo_p,cmd->buttons>>8);
		WRITEBYTE(demo_p,cmd->buttons&255);
		oldcmd[playernum].buttons = cmd->buttons;
		ziptic |= ZT_BUTTONS;
		ziptic |= ZT_BUTTONS2;
	}

	if (cmd->aiming != oldcmd[playernum].aiming)
	{
		WRITESHORT(demo_p,cmd->aiming);
		oldcmd[playernum].aiming = cmd->aiming;
		ziptic |= ZT_AIMING;
	}

	if (AddLmpExtradata(&demo_p, playernum))
		ziptic |= ZT_EXTRADATA;

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (ziptic_p > demoend - (5*MAXPLAYERS))
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

//
// G_RecordDemo
//
void G_RecordDemo(const char *name)
{
	int maxsize;

	strcpy(demoname, name);
	strcat(demoname, ".lmp");
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
//	if (demobuffer)
//		free(demobuffer);
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;

	demorecording = true;
}

void G_BeginRecording(void)
{
	int i;

	demo_p = demobuffer;

	WRITEBYTE(demo_p,VERSION);
	WRITEBYTE(demo_p,gameskill);
	WRITEBYTE(demo_p,gamemap);
	WRITEBYTE(demo_p,gametype);
	WRITEBYTE(demo_p,cv_analog.value);
	WRITEBYTE(demo_p,cv_analog2.value);
	WRITEBYTE(demo_p,consoleplayer);
	WRITEBYTE(demo_p,cv_timelimit.value); // just to be compatible with old demo (no longer used)
	WRITEBYTE(demo_p,multiplayer);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			WRITEBYTE(demo_p,1);
		else
			WRITEBYTE(demo_p,0);
	}

	memset(oldcmd, 0, sizeof (oldcmd));
}

//
// G_PlayDemo
//
void G_DeferedPlayDemo(const char *name)
{
	COM_BufAddText("playdemo \"");
	COM_BufAddText(name);
	COM_BufAddText("\"\n");
}

//
// Start a demo from a .LMP file or from a wad resource
//
void G_DoPlayDemo(char *defdemoname)
{
	skill_t skill;
	int i, map;
	lumpnum_t l;

	// load demo file / resource

	// it's an internal demo
	if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		FIL_DefaultExtension(defdemoname, ".lmp");
		if (!FIL_ReadFile(defdemoname, &demobuffer))
		{
			CONS_Printf("\2ERROR: couldn't open file '%s'.\n", defdemoname);
			gameaction = ga_nothing;
			return;
		}
		demo_p = demobuffer;
	}
	else
		demobuffer = demo_p = W_CacheLumpNum(l, PU_STATIC);

	// read demo header
	gameaction = ga_nothing;
	skill = READBYTE(demo_p);
	map = READBYTE(demo_p);

	(void)READBYTE(demo_p);

	(void)READBYTE(demo_p);

	(void)READBYTE(demo_p);

	// pay attention to displayplayer because the status bar links
	// to the display player when playing back a demo.
	displayplayer = consoleplayer = READBYTE(demo_p);

	// support old v1.9 demos with ONLY 4 PLAYERS ! Man! what a shame!!!

	(void)READBYTE(demo_p);

	multiplayer = READBYTE(demo_p);

	for (i = 0; i < 32; i++)
		playeringame[i] = READBYTE(demo_p);

#if MAXPLAYERS > 32
"Please add support for old lmps"
#endif

	memset(oldcmd, 0, sizeof (oldcmd));

	// wait map command in the demo
	G_SetGamestate(GS_WAITINGPLAYERS);
	wipegamestate = GS_WAITINGPLAYERS;

	demoplayback = true;
}

//
// G_TimeDemo
// NOTE: name is a full filename for external demos
//
static int restorecv_vidwait;

void G_TimeDemo(const char *name)
{
	nodrawers = M_CheckParm("-nodraw");
	noblit = M_CheckParm("-noblit");
	restorecv_vidwait = cv_vidwait.value;
	if (cv_vidwait.value)
		CV_Set(&cv_vidwait, "0");
	timingdemo = true;
	singletics = true;
	framecount = 0;
	demostarttime = I_GetTime();
	G_DeferedPlayDemo(name);
}

void G_MovieMode(boolean enable)
{
	if (enable)
	{
		CONS_Printf("Movie mode enabled.\n");
		singletics = true;
		moviemode = true;
#ifdef HAVE_MNG
		M_OpenMNG();
#endif
	}
	else
	{
		CONS_Printf("Movie mode disabled.\n");
		singletics = false;
		moviemode = false;
#ifdef HAVE_MNG
		M_CloseMNG();
#endif
	}
}

void G_DoneLevelLoad(void)
{
	CONS_Printf("Loaded level in %f sec\n", (double)(I_GetTime() - demostarttime) / TICRATE);
	framecount = 0;
	demostarttime = I_GetTime();
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

// reset engine variable set for the demos
// called from stopdemo command, map command, and g_checkdemoStatus.
void G_StopDemo(void)
{
	Z_Free(demobuffer);
	demobuffer = NULL;
	demoplayback = false;
	timingdemo = false;
	singletics = false;

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // cleanup

	G_SetGamestate(GS_NULL);
	wipegamestate = GS_NULL;
	SV_StopServer();
	SV_ResetServer();
}

boolean G_CheckDemoStatus(void)
{
	boolean saved;
	if (timingdemo)
	{
		int time;
		double f1, f2;
		time = I_GetTime() - demostarttime;
		if (!time)
			return true;
		G_StopDemo();
		timingdemo = false;
		f1 = (double)time;
		f2 = (double)framecount*TICRATE;
		CONS_Printf("timed %lu gametics in %d realtics\n"
			"%f seconds, %f avg fps\n",
			leveltime,time,f1/TICRATE,f2/f1);
		if (restorecv_vidwait != cv_vidwait.value)
			CV_SetValue(&cv_vidwait, restorecv_vidwait);
		D_AdvanceDemo();
		return true;
	}

	if (demoplayback)
	{
		if (singledemo)
			I_Quit();
		G_StopDemo();

		if (!timeattacking)
			D_AdvanceDemo();

		return true;
	}

	if (demorecording)
	{
		WRITEBYTE(demo_p, DEMOMARKER);
		saved = FIL_WriteFile(demoname, demobuffer, demo_p - demobuffer);
		free(demobuffer);
		demorecording = false;

		if (!timeattacking)
		{
			if (saved)
				CONS_Printf("\2Demo %s recorded\n",demoname);
			else
				CONS_Printf("\2Demo %s not saved\n",demoname);
		}

		timeattacking = false;

		return true;
	}

	return false;
}

//
// G_SetGamestate
//
// Use this to set the gamestate, please.
//
void G_SetGamestate(gamestate_t newstate)
{
	gamestate = newstate;
	oncontinuescreen = false;
}

/* These functions handle the exitgame flag. Before, when the user
   chose to end a game, it happened immediately, which could cause
   crashes if the game was in the middle of something. Now, a flag
   is set, and the game can then be stopped when it's safe to do
   so.
*/

// Used as a callback function.
void G_SetExitGameFlag(void)
{
	exitgame = true;
}

void G_ClearExitGameFlag(void)
{
	exitgame = false;
}

boolean G_GetExitGameFlag(void)
{
	return exitgame;
}
