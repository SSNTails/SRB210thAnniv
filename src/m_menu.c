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
/// \brief SRB2 selection menu, options, skill, sliders and icons. Kinda widget stuff.
///
///	\warning: \n
///	All V_DrawPatchDirect() has been replaced by V_DrawScaledPatch()
///	so that the menu is scaled to the screen size. The scaling is always
///	an integer multiple of the original size, so that the graphics look
///	good.

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "am_map.h"

#include "doomdef.h"
#include "dstrings.h"
#include "d_main.h"
#include "d_netcmd.h"

#include "console.h"

#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"

#include "m_argv.h"

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"

#include "m_menu.h"
#include "v_video.h"
#include "i_video.h"

#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"

#include "f_finale.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "d_net.h"
#include "mserv.h"
#include "m_misc.h"

#include "byteptr.h"

#include "st_stuff.h"

#include "i_sound.h"

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

boolean menuactive = false;
int fromlevelselect = false;

customsecrets_t customsecretinfo[15];
int inlevelselect = 0;

static int lastmapnum;
static int oldlastmapnum;

#define SKULLXOFF -32
#define LINEHEIGHT 16
#define STRINGHEIGHT 8
#define FONTBHEIGHT 20
#define SMALLLINEHEIGHT 8
#define SLIDER_RANGE 10
#define SLIDER_WIDTH (8*SLIDER_RANGE+6)
#define MAXSTRINGLENGTH 32
#define SERVERS_PER_PAGE 10

// Stuff for customizing the player select screen Tails 09-22-2003
description_t description[15] =
{
	{"             Fastest\n                 Speed Thok\n             Not a good pick\nfor starters, but when\ncontrolled properly,\nSonic is the most\npowerful of the three.", "SONCCHAR", "", "SONIC"},
	{"             Slowest\n                 Fly/Swim\n             Good for\nbeginners. Tails\nhandles the best. His\nflying and swimming\nwill come in handy.", "TAILCHAR", "", "TAILS"},
	{"             Medium\n                 Glide/Climb\n             A well rounded\nchoice, Knuckles\ncompromises the speed\nof Sonic with the\nhandling of Tails.", "KNUXCHAR", "", "KNUCKLES"},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
};

static int saveSlotSelected = 0;
static char joystickInfo[8][25];
static unsigned int serverlistpage;

typedef struct
{
	char playername[SKINNAMESIZE];
	char levelname[32];
	byte actnum;
	byte skincolor;
	byte skinnum;
	byte numemeralds;
	int lives;
	int continues;
	byte netgame;
} saveinfo_t;

static saveinfo_t savegameinfo[10]; // Extra info about the save games.

static char setupm_ip[16];
int startmap; // Mario, NiGHTS, or just a plain old normal game?

static short itemOn = 1; // menu item skull is on, Hack by Tails 09-18-2002
static short skullAnimCounter = 10; // skull animation counter

//
// PROTOTYPES
//
static void M_DrawSaveLoadBorder(int x,int y);

static void M_DrawThermo(int x,int y,consvar_t *cv);
static void M_DrawSlider(int x, int y, const consvar_t *cv);
static void M_CentreText(int y, const char *string); // write text centered
static void M_CustomLevelSelect(int choice);
static void M_StopMessage(int choice);
static void M_GameOption(int choice);
static void M_NetOption(int choice);
static void M_MatchOptions(int choice);
static void M_RaceOptions(int choice);
static void M_TagOptions(int choice);
static void M_CTFOptions(int choice);
#ifdef CHAOSISNOTDEADYET
static void M_ChaosOptions(int choice);
#endif

#ifdef HWRENDER
static void M_OpenGLOption(int choice);
#endif

static void M_NextServerPage(void);
static void M_PrevServerPage(void);

static const char *ALREADYPLAYING = "You are already playing.\nDo you wish to end the\ncurrent game? (Y/N)\n";

// current menudef
menu_t *currentMenu = &MainDef;
//===========================================================================
//Generic Stuffs (more easy to create menus :))
//===========================================================================

static void M_DrawMenuTitle(void)
{
	if (currentMenu->menutitlepic)
	{
		patch_t *p = W_CachePatchName(currentMenu->menutitlepic, PU_CACHE);

		int xtitle = (BASEVIDWIDTH - p->width)/2;
		int ytitle = (currentMenu->y - p->height)/2;

		if (xtitle < 0)
			xtitle = 0;
		if (ytitle < 0)
			ytitle = 0;
		V_DrawScaledPatch(xtitle, ytitle, 0, p);
	}
}

void M_DrawGenericMenu(void)
{
	int x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - p->width)/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					}
				}
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, 0, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string), y + 12,
										'_' | 0x80);
								y += 16;
								break;
							default:
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string), y,
									V_YELLOWMAP, cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(currentMenu->x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

static void M_DrawCenteredMenu(void)
{
	int x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - p->width)/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					}
				}
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawCenteredString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch(currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch(currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, 0, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string), y + 12,
										'_' | 0x80);
								y += 16;
								break;
							default:
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string), y,
									V_YELLOWMAP, cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(x - V_StringWidth(currentMenu->menuitems[itemOn].text)/2 - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawCenteredString(x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 8, i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] == '\n')
			h += 8;

	return h;
}

//===========================================================================
//MAIN MENU
//===========================================================================

static void M_QuitSRB2(int choice);
static void M_OptionsMenu(int choice);
static void M_SecretsMenu(int choice);
static void M_CustomSecretsMenu(int choice);
static void M_MapChange(int choice);

typedef enum
{
	switchmap = 0,
	secrets,
	singleplr,
	multiplr,
	options,
	quitdoom,
	main_end
} main_e;

static menuitem_t MainMenu[] =
{
	{IT_STRING  | IT_CALL,   NULL, "Switch Map...", M_MapChange,     72},
	{IT_CALL    | IT_STRING, NULL, "secrets",   M_SecretsMenu,   84},
	{IT_SUBMENU | IT_STRING, NULL, "1 player", &SinglePlayerDef, 92},
	{IT_SUBMENU | IT_STRING, NULL, "multiplayer",  &MultiPlayerDef, 100},
	{IT_CALL    | IT_STRING, NULL, "options",   M_OptionsMenu,  108},
	{IT_CALL    | IT_STRING, NULL, "quit  game",  M_QuitSRB2,     116},
};

menu_t MainDef =
{
	NULL,
	NULL,
	main_end,
	NULL,
	MainMenu,
	M_DrawCenteredMenu,
	BASEVIDWIDTH/2, 72,
	0,
	NULL
};

static void M_DrawStats(void);
static void M_DrawStats2(void);
static void M_DrawStats3(void);
static void M_DrawStats4(void);
static void M_DrawStats5(void);
static void M_Stats2(int choice);
static void M_Stats3(int choice);
static void M_Stats4(int choice);

// Empty thingy for stats5 menu
typedef enum
{
	statsempty5,
	stats5_end
} stats5_e;

static menuitem_t Stats5Menu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "NEXT", &StatsDef, 192},
};

menu_t Stats5Def =
{
	NULL,
	NULL,
	stats5_end,
	&SinglePlayerDef,
	Stats5Menu,
	M_DrawStats5,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats4 menu
typedef enum
{
	statsempty4,
	stats4_end
} stats4_e;

static menuitem_t Stats4Menu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "NEXT", &Stats5Def, 192},
};

menu_t Stats4Def =
{
	NULL,
	NULL,
	stats4_end,
	&SinglePlayerDef,
	Stats4Menu,
	M_DrawStats4,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats3 menu
typedef enum
{
	statsempty3,
	stats3_end
} stats3_e;

static menuitem_t Stats3Menu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_Stats4, 192},
};

menu_t Stats3Def =
{
	NULL,
	NULL,
	stats3_end,
	&SinglePlayerDef,
	Stats3Menu,
	M_DrawStats3,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats2 menu
typedef enum
{
	statsempty2,
	stats2_end
} stats2_e;

static menuitem_t Stats2Menu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_Stats3, 192},
};

menu_t Stats2Def =
{
	NULL,
	NULL,
	stats2_end,
	&SinglePlayerDef,
	Stats2Menu,
	M_DrawStats2,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats menu
typedef enum
{
	statsempty1,
	stats_end
} stats_e;

static menuitem_t StatsMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_Stats2, 192},
};

menu_t StatsDef =
{
	NULL,
	NULL,
	stats_end,
	&SinglePlayerDef,
	StatsMenu,
	M_DrawStats,
	280, 185,
	0,
	NULL
};

//===========================================================================
//SINGLE PLAYER MENU
//===========================================================================
// Menu Revamp! Tails 11-30-2000
static void M_NewGame(void);
static void M_LoadGame(int choice);
static void M_Statistics(int choice);
static void M_TimeAttack(int choice);

typedef enum
{
	newgame = 0,
	timeattack,
	statistics,
	endgame,
	single_end
} single_e;

static menuitem_t SinglePlayerMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "Start Game", M_LoadGame,     92},
	{IT_CALL | IT_STRING, NULL, "Time Attack",M_TimeAttack,  100},
	{IT_CALL | IT_STRING, NULL, "Statistics", M_Statistics,  108},
	{IT_CALL | IT_STRING, NULL, "End Game",   M_EndGame,     116},
};

menu_t SinglePlayerDef =
{
	0,
	"Single Player",
	single_end,
	&MainDef,
	SinglePlayerMenu,
	M_DrawGenericMenu,
	130, 72, // Tails 11-30-2000
	0,
	NULL
};

//===========================================================================
// Connect Menu
//===========================================================================

static CV_PossibleValue_t serversearch_cons_t[] = {
                                    {0,"Local Lan"},
                                    {1,"Internet"},
                                    {0,NULL}};


static consvar_t cv_serversearch = {"serversearch", "Internet", CV_HIDEN, serversearch_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

#define FIRSTSERVERLINE 6

static void M_Connect(int choice)
{
	// do not call menuexitfunc
	M_ClearMenus(false);

	COM_BufAddText(va("connect node %d\n", serverlist[choice-FIRSTSERVERLINE + serverlistpage * SERVERS_PER_PAGE].node));

	// A little "please wait" message.
	M_DrawTextBox(56, BASEVIDHEIGHT/2-12, 24, 2);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Connecting to server...");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer
}

// Tails 11-19-2002
static void M_ConnectIP(int choice)
{
	(void)choice;
	COM_BufAddText(va("connect %s\n", setupm_ip));

	// A little "please wait" message.
	M_DrawTextBox(56, BASEVIDHEIGHT/2-12, 24, 2);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Connecting to server...");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer
}

static unsigned int localservercount;

static void M_Refresh(int choice)
{
	(void)choice;
	CL_UpdateServerList(cv_serversearch.value);

	// first page of servers
	serverlistpage = 0;
}

static menuitem_t  ConnectMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Search On",&cv_serversearch, 0},
	{IT_STRING | IT_CALL, NULL, "Refresh",  M_Refresh,        0},
	{IT_STRING | IT_CALL, NULL, "Next Page",  M_NextServerPage,        0},
	{IT_STRING | IT_CALL, NULL, "Previous Page",  M_PrevServerPage,        0},
	{IT_STRING | IT_SPACE, NULL, "", NULL, 0},
	{IT_WHITESTRING | IT_SPACE,
	                       NULL, "Server Name                      ping plys gt",
	                                        NULL,              0}, // Tails 01-18-2001
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
	{IT_STRING | IT_SPACE, NULL, "",         M_Connect,        0},
};

static void M_DisplayMSMOTD(void)
{
#if 0
	M_DrawTextBox (32, 0, 28, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, 8, 0, "Master Server MOTD");
#endif
}

static void M_DrawConnectMenu(void)
{
	unsigned int i;
	char *p;
	char cgametype;
	char servername[21];

	for (i = FIRSTSERVERLINE; i < min(localservercount, SERVERS_PER_PAGE)+FIRSTSERVERLINE; i++)
		ConnectMenu[i].status = IT_STRING | IT_SPACE;

	if (serverlistcount <= 0)
		V_DrawString (currentMenu->x,currentMenu->y+FIRSTSERVERLINE*STRINGHEIGHT,0,"No servers found");
	else
	for (i = 0; i < min(serverlistcount - serverlistpage * SERVERS_PER_PAGE, SERVERS_PER_PAGE); i++)
	{
		int slindex = i + serverlistpage * SERVERS_PER_PAGE;

		strncpy(servername, serverlist[slindex].info.servername, 20);
		servername[20] = '\0';
		if (serverlist[slindex].info.modifiedgame)
			V_DrawString(currentMenu->x,currentMenu->y+(FIRSTSERVERLINE+i)*STRINGHEIGHT,V_TRANSLUCENT,servername);
		else
			V_DrawString(currentMenu->x,currentMenu->y+(FIRSTSERVERLINE+i)*STRINGHEIGHT,0,servername);
		p = va("%lu", serverlist[slindex].info.time);
		V_DrawString (currentMenu->x+200-V_StringWidth(p),currentMenu->y+(FIRSTSERVERLINE+i)*STRINGHEIGHT,0,p);

		switch (serverlist[slindex].info.gametype)
		{
			case GT_COOP:
				cgametype = 'C';
				break;
			case GT_MATCH:
				cgametype = 'M';
				break;
			case GT_RACE:
				cgametype = 'R';
				break;
			case GT_TAG:
				cgametype = 'T';
				break;
			case GT_CTF:
				cgametype = 'F';
				break;
			default:
				cgametype = 'U';
				CONS_Printf("M_DrawConnectMenu: Unknown gametype %d\n", serverlist[slindex].info.gametype);
				break;
		}

		p = va("%d/%d  %c", serverlist[slindex].info.numberofplayer,
		                     serverlist[slindex].info.maxplayer,
		                     cgametype); // Tails 01-18-2001
		V_DrawString (currentMenu->x+250-V_StringWidth(p),currentMenu->y+(FIRSTSERVERLINE+i)*STRINGHEIGHT,0,p);

		ConnectMenu[i+FIRSTSERVERLINE].status = IT_STRING | IT_CALL;
	}

	M_DisplayMSMOTD();

	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16, V_YELLOWMAP, "Translucent games are modified.");

	localservercount = serverlistcount;

	M_DrawGenericMenu();
}

static boolean M_CancelConnect(void)
{
	D_CloseConnection();
	return true;
}

menu_t Connectdef =
{
	0,
	"Connect Server",
	sizeof (ConnectMenu)/sizeof (menuitem_t),
	&MultiPlayerDef,
	ConnectMenu,
	M_DrawConnectMenu,
	27,40,
	0,
	M_CancelConnect
};

// Connect using IP address Tails 11-19-2002
static void M_HandleConnectIP(int choice);
static menuitem_t  ConnectIPMenu[] =
{
	{IT_KEYHANDLER | IT_STRING, NULL, "  IP Address:", M_HandleConnectIP, 0},
};

static void M_DrawConnectIPMenu(void);

menu_t ConnectIPdef =
{
	0,
	"Connect Server",
	sizeof (ConnectIPMenu)/sizeof (menuitem_t),
	&MultiPlayerDef,
	ConnectIPMenu,
	M_DrawConnectIPMenu,
	27,40,
	0,
	M_CancelConnect
};

static void M_ConnectMenu(int choice)
{
	(void)choice;
	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (modifiedgame)
	{
		M_StartMessage("You have wad files loaded and/or\nmodified the game in some way.\nPlease restart SRB2 before\nconnecting.", NULL, MM_NOTHING);
		return;
	}

	// Display a little "please wait" message.
	M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Contacting list server...");
	V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer

	// first page of servers
	serverlistpage = 0;

	M_SetupNextMenu(&Connectdef);
	M_Refresh(0);
}

// Connect using IP address Tails 11-19-2002
static void M_ConnectIPMenu(int choice)
{
	(void)choice;
	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (modifiedgame)
	{
		M_StartMessage("You have wad files loaded and/or\nmodified the game in some way.\nPlease restart SRB2 before\nconnecting.", NULL, MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&ConnectIPdef);
}

//===========================================================================
// Start Server Menu
//===========================================================================

CV_PossibleValue_t skill_cons_t[] = {
                             {1,"Easy"}, // Tails 01-18-2001
                             {2,"Normal"}, // Tails 01-18-2001
                             {3,"Hard"}, // Tails 01-18-2001
                             {4,"Very Hard" }, // Tails 01-18-2001
                             {0,NULL}};

static CV_PossibleValue_t map_cons_t[LEVELARRAYSIZE] = {
                                                 {1,"MAP01"},
                                                 {2,"MAP02"},
                                                 {3,"MAP03"},
                                                 {4,"MAP04"},
                                                 {5,"MAP05"},
                                                 {6,"MAP06"},
                                                 {7,"MAP07"},
                                                 {8,"MAP08"},
                                                 {9,"MAP09"},
                                                 {10,"MAP10"},
                                                 {11,"MAP11"},
                                                 {12,"MAP12"},
                                                 {13,"MAP13"},
                                                 {14,"MAP14"},
                                                 {15,"MAP15"},
                                                 {16,"MAP16"},
                                                 {17,"MAP17"},
                                                 {18,"MAP18"},
                                                 {19,"MAP19"},
                                                 {20,"MAP20"},
                                                 {21,"MAP21"},
                                                 {22,"MAP22"},
                                                 {23,"MAP23"},
                                                 {24,"MAP24"},
                                                 {25,"MAP25"},
                                                 {26,"MAP26"},
                                                 {27,"MAP27"},
                                                 {28,"MAP28"},
                                                 {29,"MAP29"},
                                                 {30,"MAP30"},
                                                 {31,"MAP31"},
                                                 {32,"MAP32"},
                                                 {33,"MAP33"},
                                                 {34,"MAP34"},
                                                 {35,"MAP35"},
                                                 {36,"MAP36"},
                                                 {37,"MAP37"},
                                                 {38,"MAP38"},
                                                 {39,"MAP39"},
                                                 {40,"MAP40"},
                                                 {41,"MAP41"},
                                                 {42,"MAP42"},
                                                 {43,"MAP43"},
                                                 {44,"MAP44"},
                                                 {45,"MAP45"},
                                                 {46,"MAP46"},
                                                 {47,"MAP47"},
                                                 {48,"MAP48"},
                                                 {49,"MAP49"},
                                                 {50,"MAP50"},
                                                 {51,"MAP51"},
                                                 {52,"MAP52"},
                                                 {53,"MAP53"},
                                                 {54,"MAP54"},
                                                 {55,"MAP55"},
                                                 {56,"MAP56"},
                                                 {57,"MAP57"},
                                                 {58,"MAP58"},
                                                 {59,"MAP59"},
                                                 {60,"MAP60"},
                                                 {61,"MAP61"},
                                                 {62,"MAP62"},
                                                 {63,"MAP63"},
                                                 {64,"MAP64"},
                                                 {65,"MAP65"},
                                                 {66,"MAP66"},
                                                 {67,"MAP67"},
                                                 {68,"MAP68"},
                                                 {69,"MAP69"},
                                                 {70,"MAP70"},
                                                 {71,"MAP71"},
                                                 {72,"MAP72"},
                                                 {73,"MAP73"},
                                                 {74,"MAP74"},
                                                 {75,"MAP75"},
                                                 {76,"MAP76"},
                                                 {77,"MAP77"},
                                                 {78,"MAP78"},
                                                 {79,"MAP79"},
                                                 {80,"MAP80"},
                                                 {81,"MAP81"},
                                                 {82,"MAP82"},
                                                 {83,"MAP83"},
                                                 {84,"MAP84"},
                                                 {85,"MAP85"},
                                                 {86,"MAP86"},
                                                 {87,"MAP87"},
                                                 {88,"MAP88"},
                                                 {89,"MAP89"},
                                                 {90,"MAP90"},
                                                 {91,"MAP91"},
                                                 {92,"MAP92"},
                                                 {93,"MAP93"},
                                                 {94,"MAP94"},
                                                 {95,"MAP95"},
                                                 {96,"MAP96"},
                                                 {97,"MAP97"},
                                                 {98,"MAP98"},
                                                 {99,"MAP99"},
                                                 {100, "MAP100"},
                                                 {101, "MAP101"},
                                                 {102, "MAP102"},
                                                 {103, "MAP103"},
                                                 {104, "MAP104"},
                                                 {105, "MAP105"},
                                                 {106, "MAP106"},
                                                 {107, "MAP107"},
                                                 {108, "MAP108"},
                                                 {109, "MAP109"},
                                                 {110, "MAP110"},
                                                 {111, "MAP111"},
                                                 {112, "MAP112"},
                                                 {113, "MAP113"},
                                                 {114, "MAP114"},
                                                 {115, "MAP115"},
                                                 {116, "MAP116"},
                                                 {117, "MAP117"},
                                                 {118, "MAP118"},
                                                 {119, "MAP119"},
                                                 {120, "MAP120"},
                                                 {121, "MAP121"},
                                                 {122, "MAP122"},
                                                 {123, "MAP123"},
                                                 {124, "MAP124"},
                                                 {125, "MAP125"},
                                                 {126, "MAP126"},
                                                 {127, "MAP127"},
                                                 {128, "MAP128"},
                                                 {129, "MAP129"},
                                                 {130, "MAP130"},
                                                 {131, "MAP131"},
                                                 {132, "MAP132"},
                                                 {133, "MAP133"},
                                                 {134, "MAP134"},
                                                 {135, "MAP135"},
                                                 {136, "MAP136"},
                                                 {137, "MAP137"},
                                                 {138, "MAP138"},
                                                 {139, "MAP139"},
                                                 {140, "MAP140"},
                                                 {141, "MAP141"},
                                                 {142, "MAP142"},
                                                 {143, "MAP143"},
                                                 {144, "MAP144"},
                                                 {145, "MAP145"},
                                                 {146, "MAP146"},
                                                 {147, "MAP147"},
                                                 {148, "MAP148"},
                                                 {149, "MAP149"},
                                                 {150, "MAP150"},
                                                 {151, "MAP151"},
                                                 {152, "MAP152"},
                                                 {153, "MAP153"},
                                                 {154, "MAP154"},
                                                 {155, "MAP155"},
                                                 {156, "MAP156"},
                                                 {157, "MAP157"},
                                                 {158, "MAP158"},
                                                 {159, "MAP159"},
                                                 {160, "MAP160"},
                                                 {161, "MAP161"},
                                                 {162, "MAP162"},
                                                 {163, "MAP163"},
                                                 {164, "MAP164"},
                                                 {165, "MAP165"},
                                                 {166, "MAP166"},
                                                 {167, "MAP167"},
                                                 {168, "MAP168"},
                                                 {169, "MAP169"},
                                                 {170, "MAP170"},
                                                 {171, "MAP171"},
                                                 {172, "MAP172"},
                                                 {173, "MAP173"},
                                                 {174, "MAP174"},
                                                 {175, "MAP175"},
                                                 {176, "MAP176"},
                                                 {177, "MAP177"},
                                                 {178, "MAP178"},
                                                 {179, "MAP179"},
                                                 {180, "MAP180"},
                                                 {181, "MAP181"},
                                                 {182, "MAP182"},
                                                 {183, "MAP183"},
                                                 {184, "MAP184"},
                                                 {185, "MAP185"},
                                                 {186, "MAP186"},
                                                 {187, "MAP187"},
                                                 {188, "MAP188"},
                                                 {189, "MAP189"},
                                                 {190, "MAP190"},
                                                 {191, "MAP191"},
                                                 {192, "MAP192"},
                                                 {193, "MAP193"},
                                                 {194, "MAP194"},
                                                 {195, "MAP195"},
                                                 {196, "MAP196"},
                                                 {197, "MAP197"},
                                                 {198, "MAP198"},
                                                 {199, "MAP199"},
                                                 {200, "MAP200"},
                                                 {201, "MAP201"},
                                                 {202, "MAP202"},
                                                 {203, "MAP203"},
                                                 {204, "MAP204"},
                                                 {205, "MAP205"},
                                                 {206, "MAP206"},
                                                 {207, "MAP207"},
                                                 {208, "MAP208"},
                                                 {209, "MAP209"},
                                                 {210, "MAP210"},
                                                 {211, "MAP211"},
                                                 {212, "MAP212"},
                                                 {213, "MAP213"},
                                                 {214, "MAP214"},
                                                 {215, "MAP215"},
                                                 {216, "MAP216"},
                                                 {217, "MAP217"},
                                                 {218, "MAP218"},
                                                 {219, "MAP219"},
                                                 {220, "MAP220"},
                                                 {221, "MAP221"},
                                                 {222, "MAP222"},
                                                 {223, "MAP223"},
                                                 {224, "MAP224"},
                                                 {225, "MAP225"},
                                                 {226, "MAP226"},
                                                 {227, "MAP227"},
                                                 {228, "MAP228"},
                                                 {229, "MAP229"},
                                                 {230, "MAP230"},
                                                 {231, "MAP231"},
                                                 {232, "MAP232"},
                                                 {233, "MAP233"},
                                                 {234, "MAP234"},
                                                 {235, "MAP235"},
                                                 {236, "MAP236"},
                                                 {237, "MAP237"},
                                                 {238, "MAP238"},
                                                 {239, "MAP239"},
                                                 {240, "MAP240"},
                                                 {241, "MAP241"},
                                                 {242, "MAP242"},
                                                 {243, "MAP243"},
                                                 {244, "MAP244"},
                                                 {245, "MAP245"},
                                                 {246, "MAP246"},
                                                 {247, "MAP247"},
                                                 {248, "MAP248"},
                                                 {249, "MAP249"},
                                                 {250, "MAP250"},
                                                 {251, "MAP251"},
                                                 {252, "MAP252"},
                                                 {253, "MAP253"},
                                                 {254, "MAP254"},
                                                 {255, "MAP255"},
                                                 {256, "MAP256"},
                                                 {257, "MAP257"},
                                                 {258, "MAP258"},
                                                 {259, "MAP259"},
                                                 {260, "MAP260"},
                                                 {261, "MAP261"},
                                                 {262, "MAP262"},
                                                 {263, "MAP263"},
                                                 {264, "MAP264"},
                                                 {265, "MAP265"},
                                                 {266, "MAP266"},
                                                 {267, "MAP267"},
                                                 {268, "MAP268"},
                                                 {269, "MAP269"},
                                                 {270, "MAP270"},
                                                 {271, "MAP271"},
                                                 {272, "MAP272"},
                                                 {273, "MAP273"},
                                                 {274, "MAP274"},
                                                 {275, "MAP275"},
                                                 {276, "MAP276"},
                                                 {277, "MAP277"},
                                                 {278, "MAP278"},
                                                 {279, "MAP279"},
                                                 {280, "MAP280"},
                                                 {281, "MAP281"},
                                                 {282, "MAP282"},
                                                 {283, "MAP283"},
                                                 {284, "MAP284"},
                                                 {285, "MAP285"},
                                                 {286, "MAP286"},
                                                 {287, "MAP287"},
                                                 {288, "MAP288"},
                                                 {289, "MAP289"},
                                                 {290, "MAP290"},
                                                 {291, "MAP291"},
                                                 {292, "MAP292"},
                                                 {293, "MAP293"},
                                                 {294, "MAP294"},
                                                 {295, "MAP295"},
                                                 {296, "MAP296"},
                                                 {297, "MAP297"},
                                                 {298, "MAP298"},
                                                 {299, "MAP299"},
                                                 {300, "MAP300"},
                                                 {301, "MAP301"},
                                                 {302, "MAP302"},
                                                 {303, "MAP303"},
                                                 {304, "MAP304"},
                                                 {305, "MAP305"},
                                                 {306, "MAP306"},
                                                 {307, "MAP307"},
                                                 {308, "MAP308"},
                                                 {309, "MAP309"},
                                                 {310, "MAP310"},
                                                 {311, "MAP311"},
                                                 {312, "MAP312"},
                                                 {313, "MAP313"},
                                                 {314, "MAP314"},
                                                 {315, "MAP315"},
                                                 {316, "MAP316"},
                                                 {317, "MAP317"},
                                                 {318, "MAP318"},
                                                 {319, "MAP319"},
                                                 {320, "MAP320"},
                                                 {321, "MAP321"},
                                                 {322, "MAP322"},
                                                 {323, "MAP323"},
                                                 {324, "MAP324"},
                                                 {325, "MAP325"},
                                                 {326, "MAP326"},
                                                 {327, "MAP327"},
                                                 {328, "MAP328"},
                                                 {329, "MAP329"},
                                                 {330, "MAP330"},
                                                 {331, "MAP331"},
                                                 {332, "MAP332"},
                                                 {333, "MAP333"},
                                                 {334, "MAP334"},
                                                 {335, "MAP335"},
                                                 {336, "MAP336"},
                                                 {337, "MAP337"},
                                                 {338, "MAP338"},
                                                 {339, "MAP339"},
                                                 {340, "MAP340"},
                                                 {341, "MAP341"},
                                                 {342, "MAP342"},
                                                 {343, "MAP343"},
                                                 {344, "MAP344"},
                                                 {345, "MAP345"},
                                                 {346, "MAP346"},
                                                 {347, "MAP347"},
                                                 {348, "MAP348"},
                                                 {349, "MAP349"},
                                                 {350, "MAP350"},
                                                 {351, "MAP351"},
                                                 {352, "MAP352"},
                                                 {353, "MAP353"},
                                                 {354, "MAP354"},
                                                 {355, "MAP355"},
                                                 {356, "MAP356"},
                                                 {357, "MAP357"},
                                                 {358, "MAP358"},
                                                 {359, "MAP359"},
                                                 {360, "MAP360"},
                                                 {361, "MAP361"},
                                                 {362, "MAP362"},
                                                 {363, "MAP363"},
                                                 {364, "MAP364"},
                                                 {365, "MAP365"},
                                                 {366, "MAP366"},
                                                 {367, "MAP367"},
                                                 {368, "MAP368"},
                                                 {369, "MAP369"},
                                                 {370, "MAP370"},
                                                 {371, "MAP371"},
                                                 {372, "MAP372"},
                                                 {373, "MAP373"},
                                                 {374, "MAP374"},
                                                 {375, "MAP375"},
                                                 {376, "MAP376"},
                                                 {377, "MAP377"},
                                                 {378, "MAP378"},
                                                 {379, "MAP379"},
                                                 {380, "MAP380"},
                                                 {381, "MAP381"},
                                                 {382, "MAP382"},
                                                 {383, "MAP383"},
                                                 {384, "MAP384"},
                                                 {385, "MAP385"},
                                                 {386, "MAP386"},
                                                 {387, "MAP387"},
                                                 {388, "MAP388"},
                                                 {389, "MAP389"},
                                                 {390, "MAP390"},
                                                 {391, "MAP391"},
                                                 {392, "MAP392"},
                                                 {393, "MAP393"},
                                                 {394, "MAP394"},
                                                 {395, "MAP395"},
                                                 {396, "MAP396"},
                                                 {397, "MAP397"},
                                                 {398, "MAP398"},
                                                 {399, "MAP399"},
                                                 {400, "MAP400"},
                                                 {401, "MAP401"},
                                                 {402, "MAP402"},
                                                 {403, "MAP403"},
                                                 {404, "MAP404"},
                                                 {405, "MAP405"},
                                                 {406, "MAP406"},
                                                 {407, "MAP407"},
                                                 {408, "MAP408"},
                                                 {409, "MAP409"},
                                                 {410, "MAP410"},
                                                 {411, "MAP411"},
                                                 {412, "MAP412"},
                                                 {413, "MAP413"},
                                                 {414, "MAP414"},
                                                 {415, "MAP415"},
                                                 {416, "MAP416"},
                                                 {417, "MAP417"},
                                                 {418, "MAP418"},
                                                 {419, "MAP419"},
                                                 {420, "MAP420"},
                                                 {421, "MAP421"},
                                                 {422, "MAP422"},
                                                 {423, "MAP423"},
                                                 {424, "MAP424"},
                                                 {425, "MAP425"},
                                                 {426, "MAP426"},
                                                 {427, "MAP427"},
                                                 {428, "MAP428"},
                                                 {429, "MAP429"},
                                                 {430, "MAP430"},
                                                 {431, "MAP431"},
                                                 {432, "MAP432"},
                                                 {433, "MAP433"},
                                                 {434, "MAP434"},
                                                 {435, "MAP435"},
                                                 {436, "MAP436"},
                                                 {437, "MAP437"},
                                                 {438, "MAP438"},
                                                 {439, "MAP439"},
                                                 {440, "MAP440"},
                                                 {441, "MAP441"},
                                                 {442, "MAP442"},
                                                 {443, "MAP443"},
                                                 {444, "MAP444"},
                                                 {445, "MAP445"},
                                                 {446, "MAP446"},
                                                 {447, "MAP447"},
                                                 {448, "MAP448"},
                                                 {449, "MAP449"},
                                                 {450, "MAP450"},
                                                 {451, "MAP451"},
                                                 {452, "MAP452"},
                                                 {453, "MAP453"},
                                                 {454, "MAP454"},
                                                 {455, "MAP455"},
                                                 {456, "MAP456"},
                                                 {457, "MAP457"},
                                                 {458, "MAP458"},
                                                 {459, "MAP459"},
                                                 {460, "MAP460"},
                                                 {461, "MAP461"},
                                                 {462, "MAP462"},
                                                 {463, "MAP463"},
                                                 {464, "MAP464"},
                                                 {465, "MAP465"},
                                                 {466, "MAP466"},
                                                 {467, "MAP467"},
                                                 {468, "MAP468"},
                                                 {469, "MAP469"},
                                                 {470, "MAP470"},
                                                 {471, "MAP471"},
                                                 {472, "MAP472"},
                                                 {473, "MAP473"},
                                                 {474, "MAP474"},
                                                 {475, "MAP475"},
                                                 {476, "MAP476"},
                                                 {477, "MAP477"},
                                                 {478, "MAP478"},
                                                 {479, "MAP479"},
                                                 {480, "MAP480"},
                                                 {481, "MAP481"},
                                                 {482, "MAP482"},
                                                 {483, "MAP483"},
                                                 {484, "MAP484"},
                                                 {485, "MAP485"},
                                                 {486, "MAP486"},
                                                 {487, "MAP487"},
                                                 {488, "MAP488"},
                                                 {489, "MAP489"},
                                                 {490, "MAP490"},
                                                 {491, "MAP491"},
                                                 {492, "MAP492"},
                                                 {493, "MAP493"},
                                                 {494, "MAP494"},
                                                 {495, "MAP495"},
                                                 {496, "MAP496"},
                                                 {497, "MAP497"},
                                                 {498, "MAP498"},
                                                 {499, "MAP499"},
                                                 {500, "MAP500"},
                                                 {501, "MAP501"},
                                                 {502, "MAP502"},
                                                 {503, "MAP503"},
                                                 {504, "MAP504"},
                                                 {505, "MAP505"},
                                                 {506, "MAP506"},
                                                 {507, "MAP507"},
                                                 {508, "MAP508"},
                                                 {509, "MAP509"},
                                                 {510, "MAP510"},
                                                 {511, "MAP511"},
                                                 {512, "MAP512"},
                                                 {513, "MAP513"},
                                                 {514, "MAP514"},
                                                 {515, "MAP515"},
                                                 {516, "MAP516"},
                                                 {517, "MAP517"},
                                                 {518, "MAP518"},
                                                 {519, "MAP519"},
                                                 {520, "MAP520"},
                                                 {521, "MAP521"},
                                                 {522, "MAP522"},
                                                 {523, "MAP523"},
                                                 {524, "MAP524"},
                                                 {525, "MAP525"},
                                                 {526, "MAP526"},
                                                 {527, "MAP527"},
                                                 {528, "MAP528"},
                                                 {529, "MAP529"},
                                                 {530, "MAP530"},
                                                 {531, "MAP531"},
                                                 {532, "MAP532"},
                                                 {533, "MAP533"},
                                                 {534, "MAP534"},
                                                 {535, "MAP535"},
                                                 {536, "MAP536"},
                                                 {537, "MAP537"},
                                                 {538, "MAP538"},
                                                 {539, "MAP539"},
                                                 {540, "MAP540"},
                                                 {541, "MAP541"},
                                                 {542, "MAP542"},
                                                 {543, "MAP543"},
                                                 {544, "MAP544"},
                                                 {545, "MAP545"},
                                                 {546, "MAP546"},
                                                 {547, "MAP547"},
                                                 {548, "MAP548"},
                                                 {549, "MAP549"},
                                                 {550, "MAP550"},
                                                 {551, "MAP551"},
                                                 {552, "MAP552"},
                                                 {553, "MAP553"},
                                                 {554, "MAP554"},
                                                 {555, "MAP555"},
                                                 {556, "MAP556"},
                                                 {557, "MAP557"},
                                                 {558, "MAP558"},
                                                 {559, "MAP559"},
                                                 {560, "MAP560"},
                                                 {561, "MAP561"},
                                                 {562, "MAP562"},
                                                 {563, "MAP563"},
                                                 {564, "MAP564"},
                                                 {565, "MAP565"},
                                                 {566, "MAP566"},
                                                 {567, "MAP567"},
                                                 {568, "MAP568"},
                                                 {569, "MAP569"},
                                                 {570, "MAP570"},
                                                 {571, "MAP571"},
                                                 {572, "MAP572"},
                                                 {573, "MAP573"},
                                                 {574, "MAP574"},
                                                 {575, "MAP575"},
                                                 {576, "MAP576"},
                                                 {577, "MAP577"},
                                                 {578, "MAP578"},
                                                 {579, "MAP579"},
                                                 {580, "MAP580"},
                                                 {581, "MAP581"},
                                                 {582, "MAP582"},
                                                 {583, "MAP583"},
                                                 {584, "MAP584"},
                                                 {585, "MAP585"},
                                                 {586, "MAP586"},
                                                 {587, "MAP587"},
                                                 {588, "MAP588"},
                                                 {589, "MAP589"},
                                                 {590, "MAP590"},
                                                 {591, "MAP591"},
                                                 {592, "MAP592"},
                                                 {593, "MAP593"},
                                                 {594, "MAP594"},
                                                 {595, "MAP595"},
                                                 {596, "MAP596"},
                                                 {597, "MAP597"},
                                                 {598, "MAP598"},
                                                 {599, "MAP599"},
                                                 {600, "MAP600"},
                                                 {601, "MAP601"},
                                                 {602, "MAP602"},
                                                 {603, "MAP603"},
                                                 {604, "MAP604"},
                                                 {605, "MAP605"},
                                                 {606, "MAP606"},
                                                 {607, "MAP607"},
                                                 {608, "MAP608"},
                                                 {609, "MAP609"},
                                                 {610, "MAP610"},
                                                 {611, "MAP611"},
                                                 {612, "MAP612"},
                                                 {613, "MAP613"},
                                                 {614, "MAP614"},
                                                 {615, "MAP615"},
                                                 {616, "MAP616"},
                                                 {617, "MAP617"},
                                                 {618, "MAP618"},
                                                 {619, "MAP619"},
                                                 {620, "MAP620"},
                                                 {621, "MAP621"},
                                                 {622, "MAP622"},
                                                 {623, "MAP623"},
                                                 {624, "MAP624"},
                                                 {625, "MAP625"},
                                                 {626, "MAP626"},
                                                 {627, "MAP627"},
                                                 {628, "MAP628"},
                                                 {629, "MAP629"},
                                                 {630, "MAP630"},
                                                 {631, "MAP631"},
                                                 {632, "MAP632"},
                                                 {633, "MAP633"},
                                                 {634, "MAP634"},
                                                 {635, "MAP635"},
                                                 {636, "MAP636"},
                                                 {637, "MAP637"},
                                                 {638, "MAP638"},
                                                 {639, "MAP639"},
                                                 {640, "MAP640"},
                                                 {641, "MAP641"},
                                                 {642, "MAP642"},
                                                 {643, "MAP643"},
                                                 {644, "MAP644"},
                                                 {645, "MAP645"},
                                                 {646, "MAP646"},
                                                 {647, "MAP647"},
                                                 {648, "MAP648"},
                                                 {649, "MAP649"},
                                                 {650, "MAP650"},
                                                 {651, "MAP651"},
                                                 {652, "MAP652"},
                                                 {653, "MAP653"},
                                                 {654, "MAP654"},
                                                 {655, "MAP655"},
                                                 {656, "MAP656"},
                                                 {657, "MAP657"},
                                                 {658, "MAP658"},
                                                 {659, "MAP659"},
                                                 {660, "MAP660"},
                                                 {661, "MAP661"},
                                                 {662, "MAP662"},
                                                 {663, "MAP663"},
                                                 {664, "MAP664"},
                                                 {665, "MAP665"},
                                                 {666, "MAP666"},
                                                 {667, "MAP667"},
                                                 {668, "MAP668"},
                                                 {669, "MAP669"},
                                                 {670, "MAP670"},
                                                 {671, "MAP671"},
                                                 {672, "MAP672"},
                                                 {673, "MAP673"},
                                                 {674, "MAP674"},
                                                 {675, "MAP675"},
                                                 {676, "MAP676"},
                                                 {677, "MAP677"},
                                                 {678, "MAP678"},
                                                 {679, "MAP679"},
                                                 {680, "MAP680"},
                                                 {681, "MAP681"},
                                                 {682, "MAP682"},
                                                 {683, "MAP683"},
                                                 {684, "MAP684"},
                                                 {685, "MAP685"},
                                                 {686, "MAP686"},
                                                 {687, "MAP687"},
                                                 {688, "MAP688"},
                                                 {689, "MAP689"},
                                                 {690, "MAP690"},
                                                 {691, "MAP691"},
                                                 {692, "MAP692"},
                                                 {693, "MAP693"},
                                                 {694, "MAP694"},
                                                 {695, "MAP695"},
                                                 {696, "MAP696"},
                                                 {697, "MAP697"},
                                                 {698, "MAP698"},
                                                 {699, "MAP699"},
                                                 {700, "MAP700"},
                                                 {701, "MAP701"},
                                                 {702, "MAP702"},
                                                 {703, "MAP703"},
                                                 {704, "MAP704"},
                                                 {705, "MAP705"},
                                                 {706, "MAP706"},
                                                 {707, "MAP707"},
                                                 {708, "MAP708"},
                                                 {709, "MAP709"},
                                                 {710, "MAP710"},
                                                 {711, "MAP711"},
                                                 {712, "MAP712"},
                                                 {713, "MAP713"},
                                                 {714, "MAP714"},
                                                 {715, "MAP715"},
                                                 {716, "MAP716"},
                                                 {717, "MAP717"},
                                                 {718, "MAP718"},
                                                 {719, "MAP719"},
                                                 {720, "MAP720"},
                                                 {721, "MAP721"},
                                                 {722, "MAP722"},
                                                 {723, "MAP723"},
                                                 {724, "MAP724"},
                                                 {725, "MAP725"},
                                                 {726, "MAP726"},
                                                 {727, "MAP727"},
                                                 {728, "MAP728"},
                                                 {729, "MAP729"},
                                                 {730, "MAP730"},
                                                 {731, "MAP731"},
                                                 {732, "MAP732"},
                                                 {733, "MAP733"},
                                                 {734, "MAP734"},
                                                 {735, "MAP735"},
                                                 {736, "MAP736"},
                                                 {737, "MAP737"},
                                                 {738, "MAP738"},
                                                 {739, "MAP739"},
                                                 {740, "MAP740"},
                                                 {741, "MAP741"},
                                                 {742, "MAP742"},
                                                 {743, "MAP743"},
                                                 {744, "MAP744"},
                                                 {745, "MAP745"},
                                                 {746, "MAP746"},
                                                 {747, "MAP747"},
                                                 {748, "MAP748"},
                                                 {749, "MAP749"},
                                                 {750, "MAP750"},
                                                 {751, "MAP751"},
                                                 {752, "MAP752"},
                                                 {753, "MAP753"},
                                                 {754, "MAP754"},
                                                 {755, "MAP755"},
                                                 {756, "MAP756"},
                                                 {757, "MAP757"},
                                                 {758, "MAP758"},
                                                 {759, "MAP759"},
                                                 {760, "MAP760"},
                                                 {761, "MAP761"},
                                                 {762, "MAP762"},
                                                 {763, "MAP763"},
                                                 {764, "MAP764"},
                                                 {765, "MAP765"},
                                                 {766, "MAP766"},
                                                 {767, "MAP767"},
                                                 {768, "MAP768"},
                                                 {769, "MAP769"},
                                                 {770, "MAP770"},
                                                 {771, "MAP771"},
                                                 {772, "MAP772"},
                                                 {773, "MAP773"},
                                                 {774, "MAP774"},
                                                 {775, "MAP775"},
                                                 {776, "MAP776"},
                                                 {777, "MAP777"},
                                                 {778, "MAP778"},
                                                 {779, "MAP779"},
                                                 {780, "MAP780"},
                                                 {781, "MAP781"},
                                                 {782, "MAP782"},
                                                 {783, "MAP783"},
                                                 {784, "MAP784"},
                                                 {785, "MAP785"},
                                                 {786, "MAP786"},
                                                 {787, "MAP787"},
                                                 {788, "MAP788"},
                                                 {789, "MAP789"},
                                                 {790, "MAP790"},
                                                 {791, "MAP791"},
                                                 {792, "MAP792"},
                                                 {793, "MAP793"},
                                                 {794, "MAP794"},
                                                 {795, "MAP795"},
                                                 {796, "MAP796"},
                                                 {797, "MAP797"},
                                                 {798, "MAP798"},
                                                 {799, "MAP799"},
                                                 {800, "MAP800"},
                                                 {801, "MAP801"},
                                                 {802, "MAP802"},
                                                 {803, "MAP803"},
                                                 {804, "MAP804"},
                                                 {805, "MAP805"},
                                                 {806, "MAP806"},
                                                 {807, "MAP807"},
                                                 {808, "MAP808"},
                                                 {809, "MAP809"},
                                                 {810, "MAP810"},
                                                 {811, "MAP811"},
                                                 {812, "MAP812"},
                                                 {813, "MAP813"},
                                                 {814, "MAP814"},
                                                 {815, "MAP815"},
                                                 {816, "MAP816"},
                                                 {817, "MAP817"},
                                                 {818, "MAP818"},
                                                 {819, "MAP819"},
                                                 {820, "MAP820"},
                                                 {821, "MAP821"},
                                                 {822, "MAP822"},
                                                 {823, "MAP823"},
                                                 {824, "MAP824"},
                                                 {825, "MAP825"},
                                                 {826, "MAP826"},
                                                 {827, "MAP827"},
                                                 {828, "MAP828"},
                                                 {829, "MAP829"},
                                                 {830, "MAP830"},
                                                 {831, "MAP831"},
                                                 {832, "MAP832"},
                                                 {833, "MAP833"},
                                                 {834, "MAP834"},
                                                 {835, "MAP835"},
                                                 {836, "MAP836"},
                                                 {837, "MAP837"},
                                                 {838, "MAP838"},
                                                 {839, "MAP839"},
                                                 {840, "MAP840"},
                                                 {841, "MAP841"},
                                                 {842, "MAP842"},
                                                 {843, "MAP843"},
                                                 {844, "MAP844"},
                                                 {845, "MAP845"},
                                                 {846, "MAP846"},
                                                 {847, "MAP847"},
                                                 {848, "MAP848"},
                                                 {849, "MAP849"},
                                                 {850, "MAP850"},
                                                 {851, "MAP851"},
                                                 {852, "MAP852"},
                                                 {853, "MAP853"},
                                                 {854, "MAP854"},
                                                 {855, "MAP855"},
                                                 {856, "MAP856"},
                                                 {857, "MAP857"},
                                                 {858, "MAP858"},
                                                 {859, "MAP859"},
                                                 {860, "MAP860"},
                                                 {861, "MAP861"},
                                                 {862, "MAP862"},
                                                 {863, "MAP863"},
                                                 {864, "MAP864"},
                                                 {865, "MAP865"},
                                                 {866, "MAP866"},
                                                 {867, "MAP867"},
                                                 {868, "MAP868"},
                                                 {869, "MAP869"},
                                                 {870, "MAP870"},
                                                 {871, "MAP871"},
                                                 {872, "MAP872"},
                                                 {873, "MAP873"},
                                                 {874, "MAP874"},
                                                 {875, "MAP875"},
                                                 {876, "MAP876"},
                                                 {877, "MAP877"},
                                                 {878, "MAP878"},
                                                 {879, "MAP879"},
                                                 {880, "MAP880"},
                                                 {881, "MAP881"},
                                                 {882, "MAP882"},
                                                 {883, "MAP883"},
                                                 {884, "MAP884"},
                                                 {885, "MAP885"},
                                                 {886, "MAP886"},
                                                 {887, "MAP887"},
                                                 {888, "MAP888"},
                                                 {889, "MAP889"},
                                                 {890, "MAP890"},
                                                 {891, "MAP891"},
                                                 {892, "MAP892"},
                                                 {893, "MAP893"},
                                                 {894, "MAP894"},
                                                 {895, "MAP895"},
                                                 {896, "MAP896"},
                                                 {897, "MAP897"},
                                                 {898, "MAP898"},
                                                 {899, "MAP899"},
                                                 {900, "MAP900"},
                                                 {901, "MAP901"},
                                                 {902, "MAP902"},
                                                 {903, "MAP903"},
                                                 {904, "MAP904"},
                                                 {905, "MAP905"},
                                                 {906, "MAP906"},
                                                 {907, "MAP907"},
                                                 {908, "MAP908"},
                                                 {909, "MAP909"},
                                                 {910, "MAP910"},
                                                 {911, "MAP911"},
                                                 {912, "MAP912"},
                                                 {913, "MAP913"},
                                                 {914, "MAP914"},
                                                 {915, "MAP915"},
                                                 {916, "MAP916"},
                                                 {917, "MAP917"},
                                                 {918, "MAP918"},
                                                 {919, "MAP919"},
                                                 {920, "MAP920"},
                                                 {921, "MAP921"},
                                                 {922, "MAP922"},
                                                 {923, "MAP923"},
                                                 {924, "MAP924"},
                                                 {925, "MAP925"},
                                                 {926, "MAP926"},
                                                 {927, "MAP927"},
                                                 {928, "MAP928"},
                                                 {929, "MAP929"},
                                                 {930, "MAP930"},
                                                 {931, "MAP931"},
                                                 {932, "MAP932"},
                                                 {933, "MAP933"},
                                                 {934, "MAP934"},
                                                 {935, "MAP935"},
                                                 {936, "MAP936"},
                                                 {937, "MAP937"},
                                                 {938, "MAP938"},
                                                 {939, "MAP939"},
                                                 {940, "MAP940"},
                                                 {941, "MAP941"},
                                                 {942, "MAP942"},
                                                 {943, "MAP943"},
                                                 {944, "MAP944"},
                                                 {945, "MAP945"},
                                                 {946, "MAP946"},
                                                 {947, "MAP947"},
                                                 {948, "MAP948"},
                                                 {949, "MAP949"},
                                                 {950, "MAP950"},
                                                 {951, "MAP951"},
                                                 {952, "MAP952"},
                                                 {953, "MAP953"},
                                                 {954, "MAP954"},
                                                 {955, "MAP955"},
                                                 {956, "MAP956"},
                                                 {957, "MAP957"},
                                                 {958, "MAP958"},
                                                 {959, "MAP959"},
                                                 {960, "MAP960"},
                                                 {961, "MAP961"},
                                                 {962, "MAP962"},
                                                 {963, "MAP963"},
                                                 {964, "MAP964"},
                                                 {965, "MAP965"},
                                                 {966, "MAP966"},
                                                 {967, "MAP967"},
                                                 {968, "MAP968"},
                                                 {969, "MAP969"},
                                                 {970, "MAP970"},
                                                 {971, "MAP971"},
                                                 {972, "MAP972"},
                                                 {973, "MAP973"},
                                                 {974, "MAP974"},
                                                 {975, "MAP975"},
                                                 {976, "MAP976"},
                                                 {977, "MAP977"},
                                                 {978, "MAP978"},
                                                 {979, "MAP979"},
                                                 {980, "MAP980"},
                                                 {981, "MAP981"},
                                                 {982, "MAP982"},
                                                 {983, "MAP983"},
                                                 {984, "MAP984"},
                                                 {985, "MAP985"},
                                                 {986, "MAP986"},
                                                 {987, "MAP987"},
                                                 {988, "MAP988"},
                                                 {989, "MAP989"},
                                                 {990, "MAP990"},
                                                 {991, "MAP991"},
                                                 {992, "MAP992"},
                                                 {993, "MAP993"},
                                                 {994, "MAP994"},
                                                 {995, "MAP995"},
                                                 {996, "MAP996"},
                                                 {997, "MAP997"},
                                                 {998, "MAP998"},
                                                 {999, "MAP999"},
                                                 {1000, "MAP1000"},
                                                 {1001, "MAP1001"},
                                                 {1002, "MAP1002"},
                                                 {1003, "MAP1003"},
                                                 {1004, "MAP1004"},
                                                 {1005, "MAP1005"},
                                                 {1006, "MAP1006"},
                                                 {1007, "MAP1007"},
                                                 {1008, "MAP1008"},
                                                 {1009, "MAP1009"},
                                                 {1010, "MAP1010"},
                                                 {1011, "MAP1011"},
                                                 {1012, "MAP1012"},
                                                 {1013, "MAP1013"},
                                                 {1014, "MAP1014"},
                                                 {1015, "MAP1015"},
                                                 {1016, "MAP1016"},
                                                 {1017, "MAP1017"},
                                                 {1018, "MAP1018"},
                                                 {1019, "MAP1019"},
                                                 {1020, "MAP1020"},
                                                 {1021, "MAP1021"},
                                                 {1022, "MAP1022"},
                                                 {1023, "MAP1023"},
                                                 {1024, "MAP1024"},
                                                 {1025, "MAP1025"},
                                                 {1026, "MAP1026"},
                                                 {1027, "MAP1027"},
                                                 {1028, "MAP1028"},
                                                 {1029, "MAP1029"},
                                                 {1030, "MAP1030"},
                                                 {1031, "MAP1031"},
                                                 {1032, "MAP1032"},
                                                 {1033, "MAP1033"},
                                                 {1034, "MAP1034"},
                                                 {1035, "MAP1035"},
                                                 {MAXSHORT, NULL}
};

static void Newgametype_OnChange(void);
static void Nextmap_OnChange(void);

// GT_* defined in doomstat.h
static consvar_t cv_skill = {"skill", "Normal", CV_HIDEN, skill_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_nextmap = {"nextmap", "MAP01", CV_HIDEN|CV_CALL, map_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = {"chooseskin", DEFAULTSKIN, CV_HIDEN|CV_CALL, skins_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

// When you add gametypes here, don't forget
// to update them in CV_AddValue!

CV_PossibleValue_t gametype_cons_t[] =
{
	{GT_COOP, "Coop"}, {GT_MATCH, "Match"},
	{GTF_TEAMMATCH, "Team Match"}, {GT_RACE, "Race"},
	{GTF_TIMEONLYRACE, "Time-Only Race"}, {GT_TAG, "Tag"},
	{GT_CTF, "CTF"},
#ifdef CHAOSISNOTDEADYET
	{GT_CHAOS, "Chaos"},
#endif
	{0, NULL}
};

consvar_t cv_newgametype = {"newgametype", "Coop", CV_HIDEN|CV_CALL, gametype_cons_t, Newgametype_OnChange, 0, NULL, NULL, 0, 0, NULL};
boolean StartSplitScreenGame;

static void Newgametype_OnChange(void)
{
	if (menuactive)
	{
		if ((cv_newgametype.value == GT_COOP && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_COOP)) ||
			((cv_newgametype.value == GT_RACE || cv_newgametype.value == GTF_TIMEONLYRACE) && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_RACE)) ||
			((cv_newgametype.value == GT_MATCH || cv_newgametype.value == GTF_TEAMMATCH) && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_MATCH)) ||
#ifdef CHAOSISNOTDEADYET
			(cv_newgametype.value == GT_CHAOS && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_CHAOS)) ||
#endif
			(cv_newgametype.value == GT_TAG && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_TAG)) ||
			(cv_newgametype.value == GT_CTF && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_CTF)))
		{
			CV_SetValue(&cv_nextmap, 1);
			CV_AddValue(&cv_nextmap, -1);
			CV_AddValue(&cv_nextmap, 1);
		}
	}
}

static void M_ChangeLevel(int choice)
{
	char mapname[6];
	(void)choice;

	strncpy(mapname, G_BuildMapName(cv_nextmap.value), 5);
	strlwr(mapname);
	mapname[5] = '\0';

	M_ClearMenus(true);
	COM_BufAddText(va("map %s -gametype \"%s\"\n", mapname, cv_newgametype.string));
}

static void M_StartServer(int choice)
{
	(void)choice;
	if (!StartSplitScreenGame)
		netgame = true;

	multiplayer = true;

	// Special Cases
	if (cv_newgametype.value == GTF_TEAMMATCH)
	{
		CV_SetValue(&cv_newgametype, GT_MATCH);
		CV_SetValue(&cv_teamplay, 1);
	}
	else if (cv_newgametype.value == GTF_TIMEONLYRACE)
	{
		CV_SetValue(&cv_newgametype, GT_RACE);
		CV_SetValue(&cv_racetype, 1);
	}
	else if (cv_newgametype.value == GT_MATCH)
		CV_SetValue(&cv_teamplay, 0);
	else if (cv_newgametype.value == GT_RACE)
		CV_SetValue(&cv_racetype, 0);

	if (!StartSplitScreenGame)
	{
		if (demoplayback)
			COM_BufAddText("stopdemo\n");
		D_MapChange(cv_nextmap.value, cv_newgametype.value, cv_skill.value, 1, 1, false, false);
		COM_BufAddText("dummyconsvar 1\n");
	}
	else // split screen
	{
		paused = false;
		if (demoplayback)
			COM_BufAddText("stopdemo\n");

		SV_StartSinglePlayerServer();
		if (!splitscreen)
		{
			splitscreen = true;
			SplitScreen_OnChange();
		}
		D_MapChange(cv_nextmap.value, cv_newgametype.value, cv_skill.value, 1, 1, false, false);
	}

	M_ClearMenus(true);
}

static void M_DrawServerMenu(void)
{
	lumpnum_t lumpnum;
	patch_t *PictureOfLevel;

	M_DrawGenericMenu();

	if (!StartSplitScreenGame)
		M_DisplayMSMOTD();

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	V_DrawSmallScaledPatch((BASEVIDWIDTH*3/4)-(PictureOfLevel->width/4), (BASEVIDHEIGHT*3/4)-(PictureOfLevel->height/4), 0, PictureOfLevel);
}

static menuitem_t ServerMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Skill",                 &cv_skill,           0},
	{IT_STRING|IT_CVAR,              NULL, "Game Type",             &cv_newgametype,    10},

	{IT_STRING|IT_CVAR,              NULL, "Advertise on Internet", &cv_internetserver, 20},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Server Name",           &cv_servername,     30},

	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},


	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                 M_StartServer,     120},
};

menu_t Serverdef =
{
	0,
	"Start Server",
	sizeof (ServerMenu)/sizeof (menuitem_t),
	&MultiPlayerDef,
	ServerMenu,
	M_DrawServerMenu,
	27,40,
	0,
	NULL
};

static menuitem_t ChangeLevelMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",             &cv_newgametype,    30},

	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},


	{IT_WHITESTRING|IT_CALL,         NULL, "Change Level",                 M_ChangeLevel,     120},
};

menu_t ChangeLevelDef =
{
	0,
	"Change Level",
	sizeof (ChangeLevelMenu)/sizeof (menuitem_t),
	&MainDef,
	ChangeLevelMenu,
	M_DrawServerMenu,
	27,40,
	0,
	NULL
};

//
// M_PatchLevelNameTable
//
// Populates the cv_nextmap variable
//
// Modes:
// 0 = Create Server Menu
// 1 = Level Select Menu
// 2 = Time Attack Menu
//
static boolean M_PatchLevelNameTable(int mode)
{
	size_t i;
	int j;
	int currentmap;
	boolean foundone = false;

	for (j = 0; j < LEVELARRAYSIZE-2; j++)
	{
		i = 0;
		currentmap = map_cons_t[j].value-1;

		if (mapheaderinfo[currentmap].lvlttl[0] && ((mode == 0 && !mapheaderinfo[currentmap].hideinmenu) || (mode == 1 && mapheaderinfo[currentmap].levelselect) || (mode == 2 && mapheaderinfo[currentmap].timeattack && mapvisited[currentmap])))
		{
			strncpy(lvltable[j], mapheaderinfo[currentmap].lvlttl, strlen(mapheaderinfo[currentmap].lvlttl));

			i += strlen(mapheaderinfo[currentmap].lvlttl);

			if (!mapheaderinfo[currentmap].nozone)
			{
				lvltable[j][i++] = ' ';
				lvltable[j][i++] = 'Z';
				lvltable[j][i++] = 'O';
				lvltable[j][i++] = 'N';
				lvltable[j][i++] = 'E';
			}

			if (mapheaderinfo[currentmap].actnum)
			{
				char actnum[3];
				int g;

				lvltable[j][i++] = ' ';

				sprintf(actnum, "%d", mapheaderinfo[currentmap].actnum);

				for (g = 0; g < 3; g++)
				{
					if (actnum[g] == '\0')
						break;

					lvltable[j][i++] = actnum[g];
				}
			}

			lvltable[j][i++] = '\0';
			foundone = true;
		}
		else
			lvltable[j][0] = '\0';

		if (lvltable[j][0] == '\0')
			map_cons_t[j].strvalue = NULL;
		else
			map_cons_t[j].strvalue = lvltable[j];
	}

	if (!foundone)
		return false;

	CV_SetValue(&cv_nextmap, cv_nextmap.value); // This causes crash sometimes?!

	if (mode > 0)
	{
		CV_SetValue(&cv_nextmap, map_cons_t[0].value);
		CV_AddValue(&cv_nextmap, -1);
		CV_AddValue(&cv_nextmap, 1);
	}
	else
		Newgametype_OnChange(); // Make sure to start on an appropriate map if wads have been added

	return true;
}

static void M_MapChange(int choice)
{
	(void)choice;
	if (!(netgame || multiplayer) || !Playing())
	{
		M_StartMessage("You aren't in a game!\nPress a key.", NULL, MM_NOTHING);
		return;
	}

	inlevelselect = 0;
	M_PatchLevelNameTable(0);

	// Special Cases
	if (gametype == GT_MATCH && cv_teamplay.value) // Team Match
		CV_SetValue(&cv_newgametype, GTF_TEAMMATCH);
	else if (gametype == GT_RACE && cv_racetype.value) // Time-Only Race
		CV_SetValue(&cv_newgametype, GTF_TIMEONLYRACE);
	else
		CV_SetValue(&cv_newgametype, gametype);

	CV_SetValue(&cv_nextmap, gamemap);

	M_SetupNextMenu(&ChangeLevelDef);
}

//
// M_PatchSkinNameTable
//
// Like M_PatchLevelNameTable, but for cv_chooseskin
//
static void M_PatchSkinNameTable(void)
{
	int j;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	for (j = 0; j < MAXSKINS; j++)
	{
		if (skins[j].name[0] != '\0')
		{
			skins_cons_t[j].strvalue = skins[j].name;
			skins_cons_t[j].value = j+1;
		}
		else
		{
			skins_cons_t[j].strvalue = NULL;
			skins_cons_t[j].value = 0;
		}
	}

	CV_SetValue(&cv_chooseskin, cv_chooseskin.value); // This causes crash sometimes?!

	CV_SetValue(&cv_chooseskin, 1);
	CV_AddValue(&cv_chooseskin, -1);
	CV_AddValue(&cv_chooseskin, 1);

	return;
}

static inline void M_StartSplitServerMenu(void)
{
	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING, M_ExitGameResponse, MM_YESNO);
		return;
	}

	inlevelselect = 0;
	M_PatchLevelNameTable(0);
	StartSplitScreenGame = true;
	ServerMenu[2].status = IT_DISABLED; // No advertise on Internet option.
	ServerMenu[3].status = IT_DISABLED; // No server name.
	M_SetupNextMenu(&Serverdef);
}

static void M_StartServerMenu(int choice)
{
	(void)choice;
	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING, M_ExitGameResponse, MM_YESNO);
		return;
	}

	inlevelselect = 0;
	M_PatchLevelNameTable(0);
	StartSplitScreenGame = false;
	ServerMenu[2].status = IT_STRING|IT_CVAR; // Make advertise on Internet option available.
	ServerMenu[3].status = IT_STRING|IT_CVAR|IT_CV_STRING; // Server name too.
	M_SetupNextMenu(&Serverdef);
}

//===========================================================================
//                            MULTI PLAYER MENU
//===========================================================================
static void M_SetupMultiPlayer(int choice);
static void M_SetupMultiPlayerBis(int choice);
static void M_Splitscreen(int choice);

typedef enum
{
	startserver = 0,
	connectmultiplayermenu,
	connectip,
	startsplitscreengame,
	multiplayeroptions,
	setupplayer1,
	setupplayer2,
	end_game,
	multiplayer_end
} multiplayer_e;

static menuitem_t MultiPlayerMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "HOST GAME",              M_StartServerMenu,      10},
	{IT_CALL | IT_STRING, NULL, "JOIN GAME (Search)",     M_ConnectMenu,          20},
	{IT_CALL | IT_STRING, NULL, "JOIN GAME (Specify IP)", M_ConnectIPMenu,        30},
	{IT_CALL | IT_STRING, NULL, "TWO PLAYER GAME",        M_Splitscreen,          50},
	{IT_CALL | IT_STRING, NULL, "NETWORK OPTIONS",        M_NetOption,            70},
	{IT_CALL | IT_STRING, NULL, "SETUP PLAYER",           M_SetupMultiPlayer,     90},
	{IT_CALL | IT_STRING | IT_DISABLED, NULL, "SETUP PLAYER 2",         M_SetupMultiPlayerBis, 100},
	{IT_CALL | IT_STRING, NULL, "END GAME",               M_EndGame,             120},
};

menu_t  MultiPlayerDef =
{
	"M_MULTI",
	"Multiplayer",
	multiplayer_end,
	&MainDef,
	MultiPlayerMenu,
	M_DrawGenericMenu,
	85,40,
	0,
	NULL
};

static void M_Splitscreen(int choice)
{
	(void)choice;
	M_StartSplitServerMenu();
}

//===========================================================================
// Seconde mouse config for the splitscreen player
//===========================================================================

static menuitem_t  SecondMouseCfgMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Second Mouse Serial Port",
		                                             &cv_mouse2port,      0}, // Tails 01-18-2001

	{IT_STRING | IT_CVAR, NULL, "Use Mouse 2",      &cv_usemouse2,       0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse2 Speed",     &cv_mousesens2,      0},

	{IT_STRING | IT_CVAR, NULL, "Always MouseLook", &cv_alwaysfreelook2, 0},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove2,      0},
	{IT_STRING | IT_CVAR, NULL, "Invert Mouse2", &cv_invertmouse2,    0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mlook Speed", &cv_mlooksens2,      0},
};

menu_t SecondMouseCfgdef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (SecondMouseCfgMenu)/sizeof (menuitem_t),
	&SetupMultiPlayerDef,
	SecondMouseCfgMenu,
	M_DrawGenericMenu,
	27, 40,
	0,
	NULL
};

//===========================================================================
//MULTI PLAYER SETUP MENU
//===========================================================================
static void M_DrawSetupMultiPlayerMenu(void);
static void M_HandleSetupMultiPlayer(int choice);
static void M_Setup1PControlsMenu(int choice);
static void M_Setup2PControlsMenu(int choice);
static boolean M_QuitMultiPlayerMenu(void);

static menuitem_t SetupMultiPlayerMenu[] =
{
	{IT_KEYHANDLER | IT_STRING,  NULL, "Your name",   M_HandleSetupMultiPlayer,   0},

	{IT_CVAR | IT_STRING | IT_CV_NOPRINT,
	                              NULL, "Your color",  &cv_playercolor,           16},

	{IT_KEYHANDLER | IT_STRING,  NULL, "Your player", M_HandleSetupMultiPlayer,  96}, // Tails 01-18-2001

	{IT_CALL | IT_WHITESTRING,  NULL, "Setup Controls...",
	                                                  M_Setup2PControlsMenu,    120},
	{IT_SUBMENU | IT_WHITESTRING, NULL, "Second Mouse config...",
	                                                  &SecondMouseCfgdef,       130},
};

enum
{
	setupmultiplayer_name = 0,
	setupmultiplayer_color,
	setupmultiplayer_skin,
	setupmultiplayer_controls,
	setupmultiplayer_mouse2,
	setupmulti_end
};

menu_t SetupMultiPlayerDef =
{
	"M_PSETUP",
	"Multiplayer",
	sizeof (SetupMultiPlayerMenu)/sizeof (menuitem_t),
	&MultiPlayerDef,
	SetupMultiPlayerMenu,
	M_DrawSetupMultiPlayerMenu,
	27, 40,
	0,
	M_QuitMultiPlayerMenu
};

// Tails 03-02-2002
static void M_DrawSetupChoosePlayerMenu(void);
static boolean M_QuitChoosePlayerMenu(void);
static void M_ChoosePlayer(int choice);
int skillnum;
typedef enum
{
	Player1,
	Player2,
	Player3,
	Player4,
	Player5,
	Player6,
	Player7,
	Player8,
	Player9,
	Player10,
	Player11,
	Player12,
	Player13,
	Player14,
	Player15,
	player_end
} players_e;

menuitem_t PlayerMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "SONIC", M_ChoosePlayer,  0},
	{IT_CALL | IT_STRING, NULL, "TAILS", M_ChoosePlayer,  0},
	{IT_CALL | IT_STRING, NULL, "KNUCKLES", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER4", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER5", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER6", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER7", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER8", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER9", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER10", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER11", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER12", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER13", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER14", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER15", M_ChoosePlayer,  0},
};

menu_t PlayerDef =
{
	"M_PICKP",
	"Choose Your Character",
	sizeof (PlayerMenu)/sizeof (menuitem_t),//player_end,
	&NewDef,
	PlayerMenu,
	M_DrawSetupChoosePlayerMenu,
	24, 16,
	0,
	M_QuitChoosePlayerMenu
};
// Tails 03-02-2002

#define PLBOXW    8
#define PLBOXH    9

static int       multi_tics;
static state_t * multi_state;

// this is set before entering the MultiPlayer setup menu,
// for either player 1 or 2
static char       setupm_name[MAXPLAYERNAME+1];
static player_t * setupm_player;
static consvar_t *setupm_cvskin;
static consvar_t *setupm_cvcolor;
static consvar_t *setupm_cvname;

static void M_SetupMultiPlayer(int choice)
{
	(void)choice;
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		M_StartMessage("You have to be in a game\nto do this.\n\nPress a key\n",NULL,MM_NOTHING);
		return;
	}

	multi_state = &states[mobjinfo[MT_PLAYER].seestate];
	multi_tics = multi_state->tics;
	strcpy(setupm_name, cv_playername.string);

	SetupMultiPlayerDef.numitems = setupmultiplayer_skin +1;      //remove player2 setup controls and mouse2

	// set for player 1
	SetupMultiPlayerMenu[setupmultiplayer_color].itemaction = &cv_playercolor;
	setupm_player = &players[consoleplayer];
	setupm_cvskin = &cv_skin;
	setupm_cvcolor = &cv_playercolor;
	setupm_cvname = &cv_playername;
	M_SetupNextMenu (&SetupMultiPlayerDef);
}

// start the multiplayer setup menu, for secondary player (splitscreen mode)
static void M_SetupMultiPlayerBis(int choice)
{
	(void)choice;
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		M_StartMessage("You have to be in a game\nto do this.\n\nPress a key\n",NULL,MM_NOTHING);
		return;
	}

	multi_state = &states[mobjinfo[MT_PLAYER].seestate];
	multi_tics = multi_state->tics;
	strcpy (setupm_name, cv_playername2.string);
	SetupMultiPlayerDef.numitems = setupmulti_end;          //activate the setup controls for player 2

	// set for splitscreen secondary player
	SetupMultiPlayerMenu[setupmultiplayer_color].itemaction = &cv_playercolor2;
	setupm_player = &players[secondarydisplayplayer];
	setupm_cvskin = &cv_skin2;
	setupm_cvcolor = &cv_playercolor2;
	setupm_cvname = &cv_playername2;
	M_SetupNextMenu (&SetupMultiPlayerDef);
}

// Draw the funky Connect IP menu. Tails 11-19-2002
// So much work for such a little thing!
static void M_DrawConnectIPMenu(void)
{
	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// draw name string
//	M_DrawTextBox(82,8,MAXPLAYERNAME,1);
	V_DrawString (128,40,0,setupm_ip);

	// draw text cursor for name
	if (itemOn == 0 &&
		 skullAnimCounter < 4)   //blink cursor
		V_DrawCharacter(128+V_StringWidth(setupm_ip),40,'_');
}

// called at splitscreen changes
void M_SwitchSplitscreen(void)
{
// activate setup for player 2
	if (splitscreen)
		MultiPlayerMenu[setupplayer2].status = IT_CALL | IT_STRING;
	else
		MultiPlayerMenu[setupplayer2].status = IT_DISABLED;

	if (MultiPlayerDef.lastOn == setupplayer2)
		MultiPlayerDef.lastOn = setupplayer1;
}


//
//  Draw the multi player setup menu, had some fun with player anim
//
static void M_DrawSetupMultiPlayerMenu(void)
{
	int mx, my, st;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;

	mx = SetupMultiPlayerDef.x;
	my = SetupMultiPlayerDef.y;

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// draw name string
	M_DrawTextBox(mx + 90, my - 8, MAXPLAYERNAME, 1);
	V_DrawString(mx + 98, my, 0, setupm_name);

	// draw skin string
	V_DrawString(mx + 90, my + 96, 0, setupm_cvskin->string);

	// draw the name of the color you have chosen
	// Just so people don't go thinking that "Default" is Green.
	V_DrawString(208, 72, 0, setupm_cvcolor->string);

	// draw text cursor for name
	if (!itemOn && skullAnimCounter < 4) // blink cursor
		V_DrawCharacter(mx + 98 + V_StringWidth(setupm_name), my, '_');

	// anim the player in the box
	if (--multi_tics <= 0)
	{
		st = multi_state->nextstate;
		if (st != S_NULL)
			multi_state = &states[st];
		multi_tics = multi_state->tics;
		if (multi_tics == -1)
			multi_tics = 15;
	}

	// skin 0 is default player sprite
	if (R_SkinAvailable(setupm_cvskin->string) != -1)
		sprdef = &skins[R_SkinAvailable(setupm_cvskin->string)].spritedef;
	else
		sprdef = &skins[0].spritedef;

	sprframe = &sprdef->spriteframes[multi_state->frame & FF_FRAMEMASK];
	patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);

	// draw box around guy
	M_DrawTextBox(mx + 90, my + 8, PLBOXW, PLBOXH);

	// draw player sprite
	if (!setupm_cvcolor->value)
	{
		if (atoi(skins[setupm_cvskin->value].highres))
			V_DrawScaledPatch(mx + 98 + (PLBOXW*8/2), my + 16 + (PLBOXH*8) - 8, 0, patch);
		else
			V_DrawScaledPatch(mx + 98 + (PLBOXW*8/2), my + 16 + (PLBOXH*8) - 8, 0, patch);
	}
	else
	{
		byte *colormap = (byte *)translationtables[setupm_player->skin] - 256
			+ (setupm_cvcolor->value<<8);

		if (atoi(skins[setupm_cvskin->value].highres))
			V_DrawSmallMappedPatch(mx + 98 + (PLBOXW*8/2), my + 16 + (PLBOXH*8) - 8, 0, patch, colormap);
		else
			V_DrawMappedPatch(mx + 98 + (PLBOXW*8/2), my + 16 + (PLBOXH*8) - 8, 0, patch, colormap);
	}
}

// Tails 11-19-2002
static void M_HandleConnectIP(int choice)
{
	size_t   l;
	boolean  exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_ENTER:
			S_StartSound(NULL,sfx_menu1); // Tails
			M_ClearMenus(true);
			M_ConnectIP(1);
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			if ((l = strlen(setupm_ip))!=0 && itemOn == 0)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l-1] =0;
			}
			break;

		default:
			l = strlen(setupm_ip);
			if (l < 16-1 && (choice == 46 || (choice >= 48 && choice <= 57))) // Rudimentary number and period enforcing
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l] =(char)choice;
				setupm_ip[l+1] =0;
			}
			break;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu (currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

//
// Handle Setup MultiPlayer Menu
//
static void M_HandleSetupMultiPlayer(int choice)
{
	size_t   l;
	boolean  exitmenu = false;  // exit to previous menu and send name change
	int      myskin = setupm_cvskin->value;

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL,sfx_menu1); // Tails
			if (itemOn+1 >= SetupMultiPlayerDef.numitems)
				itemOn = 0;
			else itemOn++;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL,sfx_menu1); // Tails
			if (!itemOn)
				itemOn = (short)(SetupMultiPlayerDef.numitems-1);
			else itemOn--;
			break;

		case KEY_LEFTARROW:
			if (itemOn == 2)       //player skin
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				myskin--;
			}
			break;

		case KEY_RIGHTARROW:
			if (itemOn == 2)       //player skin
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				myskin++;
			}
			break;

		case KEY_ENTER:
			S_StartSound(NULL,sfx_menu1); // Tails
			exitmenu = true;
		break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			if ((l = strlen(setupm_name))!=0 && itemOn == 0)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_name[l-1] =0;
			}
			break;

		default:
			if (choice < 32 || choice > 127 || itemOn != 0)
				break;
			l = strlen(setupm_name);
			if (l < MAXPLAYERNAME-1)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_name[l] =(char)choice;
				setupm_name[l+1] =0;
			}
			break;
	}

	// check skin
	if (myskin <0)
		myskin = numskins-1;
	if (myskin >numskins-1)
		myskin = 0;

	// check skin change
	if (myskin != setupm_player->skin)
		COM_BufAddText (va("%s \"%s\"",setupm_cvskin->name,skins[myskin].name));

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu (currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

static boolean M_QuitMultiPlayerMenu(void)
{
	size_t l;
	// send name if changed
	if (strcmp(setupm_name, setupm_cvname->string))
	{
		// remove trailing whitespaces
		for (l= strlen(setupm_name)-1;
		 (signed)l >= 0 && setupm_name[l] ==' '; l--)
			setupm_name[l] =0;
		COM_BufAddText (va("%s \"%s\"\n",setupm_cvname->name,setupm_name));
	}
	return true;
}


////////////////////////////////////////////////////////////////
//                   CHARACTER SELECT SCREEN                  //
////////////////////////////////////////////////////////////////

static inline void M_SetupChoosePlayer(int choice)
{
	(void)choice;
	if (Playing() == false)
	{
		S_StopMusic();
		S_ChangeMusic(mus_chrsel, true);
	}

	M_SetupNextMenu (&PlayerDef);
}

//
//  Draw the choose player setup menu, had some fun with player anim
//
static void M_DrawSetupChoosePlayerMenu(void)
{
	int      mx = PlayerDef.x, my = PlayerDef.y;
	patch_t *patch;

	// Black BG
	V_DrawFill(0, 0, vid.width, vid.height, 31);

	{
		// Compact the menu
		int i;
		byte alpha = 0;
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if (currentMenu->menuitems[i].status == 0
			|| currentMenu->menuitems[i].status == IT_DISABLED)
				continue;

			currentMenu->menuitems[i].alphaKey = alpha;
			alpha += 8;
		}
	}

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// TEXT BOX!
	// For the character
	M_DrawTextBox(mx+152,my, 16, 16);

	// For description
	M_DrawTextBox(mx-24, my+72, 20, 10);

	patch = W_CachePatchName(description[itemOn].picname, PU_CACHE);

	V_DrawString(mx-16, my+80, V_YELLOWMAP, "Speed:\nAbility:\nNotes:");

	V_DrawScaledPatch(mx+160,my+8,0,patch);
	V_DrawString(mx-16, my+80, 0, description[itemOn].info);
}

//
// Handle Setup Choose Player Menu
//
#if 0
static void M_HandleSetupChoosePlayer(int choice)
{
	boolean  exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL,sfx_menu1); // Tails
			if (itemOn+1 >= SetupMultiPlayerDef.numitems)
				itemOn = 0;
			else itemOn++;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL,sfx_menu1); // Tails
			if (!itemOn)
				itemOn = (short)(SetupMultiPlayerDef.numitems-1);
			else itemOn--;
			break;

		case KEY_ENTER:
			S_StartSound(NULL,sfx_menu1); // Tails
			exitmenu = true;
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		default:
			break;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu (currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}
#endif

static boolean M_QuitChoosePlayerMenu(void)
{
	// Stop music
	S_StopMusic();
	return true;
}


//===========================================================================
//                           NEW GAME FOR SINGLE PLAYER
//===========================================================================
static void M_DrawNewGame(void);
// overhaul! Tails 11-30-2000
static void M_ChooseSkill(int choice);

typedef enum
{
	easy,
	normal,
	hard,
	veryhard,
	ultimate,
	newg_end
} newgame_e;

static menuitem_t NewGameMenu[] =
{
	// Tails
	{IT_CALL | IT_STRING, NULL, "Easy",      M_ChooseSkill,  90},
	{IT_CALL | IT_STRING, NULL, "Normal",    M_ChooseSkill, 100},
	{IT_CALL | IT_STRING, NULL, "Hard",      M_ChooseSkill, 110},
	{IT_CALL | IT_STRING, NULL, "Very Hard", M_ChooseSkill, 120},
	{IT_CALL | IT_STRING, NULL, "Ultimate",  M_ChooseSkill, 130},
};

menu_t NewDef =
{
	"M_NEWG",
	"NEW GAME",
	newg_end,           // # of menu items
	&SinglePlayerDef,            // previous menu
	NewGameMenu,        // menuitem_t ->
	M_DrawNewGame,      // drawing routine ->
	48, 63,             // x, y
	normal,             // lastOn
	NULL
};

static void M_DrawNewGame(void)
{
	if (modifiedgame && !savemoddata)
	{
		M_DrawTextBox(24,64-4,32,3);

		V_DrawCenteredString(160, 64+4, 0, "Note: Game must be reset to record");
		V_DrawCenteredString(160, 64+16, 0, "statistics or unlock secrets.");
	}

	M_DrawGenericMenu();
}

static void M_Statistics(int choice)
{
	(void)choice;
	if (modifiedgame && !savemoddata)
	{
		M_StartMessage("Statistics not available\nin modified games.", NULL, MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&StatsDef);
}

static void M_Stats2(int choice)
{
	(void)choice;
	oldlastmapnum = lastmapnum;
	M_SetupNextMenu(&Stats2Def);
}

static void M_Stats3(int choice)
{
	(void)choice;
	oldlastmapnum = lastmapnum;
	M_SetupNextMenu(&Stats3Def);
}

static void M_Stats4(int choice)
{
	(void)choice;
	oldlastmapnum = lastmapnum;
	M_SetupNextMenu(&Stats4Def);
}

//
// M_GetLevelEmblem
//
// Returns pointer to an emblem if an emblem exists
// for that level, and exists for that player.
// NULL if not found.
//
static emblem_t *M_GetLevelEmblem(int mapnum, int player)
{
	int i;

	for (i = 0; i < numemblems; i++)
	{
		if (emblemlocations[i].level == mapnum
			&& emblemlocations[i].player == player)
			return &emblemlocations[i];
	}
	return NULL;
}

static void M_DrawStats(void)
{
	int found = 0;
	int i;
	char hours[4];
	char minutes[4];
	char seconds[4];
	tic_t besttime = 0;
	boolean displaytimeattack = true;

	for (i = 0; i < MAXEMBLEMS; i++)
	{
		if (emblemlocations[i].collected)
			found++;
	}

	V_DrawString(64, 32, 0, va("x %d/%d", found, numemblems));
	V_DrawScaledPatch(32, 32-4, 0, W_CachePatchName("EMBLICON", PU_STATIC));

	if (totalplaytime/(3600*TICRATE) < 10)
		sprintf(hours, "0%lu", totalplaytime/(3600*TICRATE));
	else
		sprintf(hours, "%lu:", totalplaytime/(3600*TICRATE));

	if (totalplaytime/(60*TICRATE)%60 < 10)
		sprintf(minutes, "0%lu", totalplaytime/(60*TICRATE)%60);
	else
		sprintf(minutes, "%lu", totalplaytime/(60*TICRATE)%60);

	if (((totalplaytime/TICRATE) % 60) < 10)
		sprintf(seconds, "0%lu", (totalplaytime/TICRATE) % 60);
	else
		sprintf(seconds, "%lu", (totalplaytime/TICRATE) % 60);

	V_DrawCenteredString(224, 8, 0, "Total Play Time:");
	V_DrawCenteredString(224, 20, 0, va("%s:%s:%s", hours, minutes, seconds));

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!(mapheaderinfo[i].timeattack))
			continue;

		if (timedata[i].time > 0)
			besttime += timedata[i].time;
		else
			displaytimeattack = false;
	}

	if (displaytimeattack)
	{
		if (besttime/(3600*TICRATE) < 10)
			sprintf(hours, "0%lu", besttime/(3600*TICRATE));
		else
			sprintf(hours, "%lu", besttime/(3600*TICRATE));

		if (besttime/(60*TICRATE)%60 < 10)
			sprintf(minutes, "0%lu", besttime/(60*TICRATE)%60);
		else
			sprintf(minutes, "%lu", besttime/(60*TICRATE)%60);

		if (((besttime/TICRATE) % 60) < 10)
			sprintf(seconds, "0%lu", (besttime/TICRATE) % 60);
		else
			sprintf(seconds, "%lu", (besttime/TICRATE) % 60);

		V_DrawCenteredString(224, 36, 0, "Best Time Attack:");
		V_DrawCenteredString(224, 48, 0, va("%s:%s:%s", hours, minutes, seconds));
	}

	if (eastermode)
	{
		found = 0;

		if (foundeggs & 1)
			found++;
		if (foundeggs & 2)
			found++;
		if (foundeggs & 4)
			found++;
		if (foundeggs & 8)
			found++;
		if (foundeggs & 16)
			found++;
		if (foundeggs & 32)
			found++;
		if (foundeggs & 64)
			found++;
		if (foundeggs & 128)
			found++;
		if (foundeggs & 256)
			found++;
		if (foundeggs & 512)
			found++;
		if (foundeggs & 1024)
			found++;
		if (foundeggs & 2048)
			found++;

		V_DrawCenteredString(BASEVIDWIDTH/2, 16, V_YELLOWMAP, va("Eggs Found: %d/%d", found, NUMEGGS));
	}

	{
		int y = 80;
		char names[8];
		emblem_t *emblem;

		V_DrawString(32+36, y-16, 0, "LEVEL NAME");
		V_DrawString(224+28, y-16, 0, "BEST TIME");

		lastmapnum = 0;
		oldlastmapnum = 0;

		sprintf(names, "%c %c %c", skins[0].name[0], skins[1].name[0], skins[2].name[0]);
		V_DrawString(32, y-16, 0, names);

		for (i = oldlastmapnum; i < NUMMAPS; i++)
		{

			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (timedata[i].time/(60*TICRATE) < 10)
					sprintf(minutes, "0%lu", timedata[i].time/(60*TICRATE));
				else
					sprintf(minutes, "%lu", timedata[i].time/(60*TICRATE));

				if (((timedata[i].time/TICRATE) % 60) < 10)
					sprintf(seconds, "0%lu", (timedata[i].time/TICRATE) % 60);
				else
					sprintf(seconds, "%lu", (timedata[i].time/TICRATE) % 60);

				if ((timedata[i].time % TICRATE) < 10)
					sprintf(hours, "0%lu", timedata[i].time % TICRATE);
				else
					sprintf(hours, "%lu", timedata[i].time % TICRATE);

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats2(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		int i;
		int y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 2");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (timedata[i].time/(60*TICRATE) < 10)
					sprintf(minutes, "0%lu", timedata[i].time/(60*TICRATE));
				else
					sprintf(minutes, "%lu", timedata[i].time/(60*TICRATE));

				if (((timedata[i].time/TICRATE) % 60) < 10)
					sprintf(seconds, "0%lu", (timedata[i].time/TICRATE) % 60);
				else
					sprintf(seconds, "%lu", (timedata[i].time/TICRATE) % 60);

				if ((timedata[i].time % TICRATE) < 10)
					sprintf(hours, "0%lu", timedata[i].time % TICRATE);
				else
					sprintf(hours, "%lu", timedata[i].time % TICRATE);

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats3(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		int i;
		int y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 3");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (timedata[i].time/(60*TICRATE) < 10)
					sprintf(minutes, "0%lu", timedata[i].time/(60*TICRATE));
				else
					sprintf(minutes, "%lu", timedata[i].time/(60*TICRATE));

				if (((timedata[i].time/TICRATE) % 60) < 10)
					sprintf(seconds, "0%lu", (timedata[i].time/TICRATE) % 60);
				else
					sprintf(seconds, "%lu", (timedata[i].time/TICRATE) % 60);

				if ((timedata[i].time % TICRATE) < 10)
					sprintf(hours, "0%lu", timedata[i].time % TICRATE);
				else
					sprintf(hours, "%lu", timedata[i].time % TICRATE);

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats4(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		int i;
		int y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 4");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (timedata[i].time/(60*TICRATE) < 10)
					sprintf(minutes, "0%lu", timedata[i].time/(60*TICRATE));
				else
					sprintf(minutes, "%lu", timedata[i].time/(60*TICRATE));

				if (((timedata[i].time/TICRATE) % 60) < 10)
					sprintf(seconds, "0%lu", (timedata[i].time/TICRATE) % 60);
				else
					sprintf(seconds, "%lu", (timedata[i].time/TICRATE) % 60);

				if ((timedata[i].time % TICRATE) < 10)
					sprintf(hours, "0%lu", timedata[i].time % TICRATE);
				else
					sprintf(hours, "%lu", timedata[i].time % TICRATE);

				if ((timedata[i].time % TICRATE) < 10)
					sprintf(hours, "0%lu", timedata[i].time % TICRATE);
				else
					sprintf(hours, "%lu", timedata[i].time % TICRATE);

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats5(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		int i;
		int y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 5");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (timedata[i].time/(60*TICRATE) < 10)
					sprintf(minutes, "0%lu", timedata[i].time/(60*TICRATE));
				else
					sprintf(minutes, "%lu", timedata[i].time/(60*TICRATE));

				if (((timedata[i].time/TICRATE) % 60) < 10)
					sprintf(seconds, "0%lu", (timedata[i].time/TICRATE) % 60);
				else
					sprintf(seconds, "%lu", (timedata[i].time/TICRATE) % 60);

				if ((timedata[i].time % TICRATE) < 10)
					sprintf(hours, "0%lu", timedata[i].time % TICRATE);
				else
					sprintf(hours, "%lu",timedata[i].time % TICRATE);

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_NewGame(void)
{
	fromlevelselect = false;

	if (grade & 128) // Ultimate skill level is unlockable Tails 05-19-2003
		NewGameMenu[ultimate].status = IT_STRING | IT_CALL;
	else
		NewGameMenu[ultimate].status = IT_DISABLED;

	startmap = spstage_start;
	CV_SetValue(&cv_newgametype, GT_COOP); // Graue 09-08-2004

	NewDef.prevMenu = currentMenu;

	M_SetupNextMenu(&NewDef);

	StartSplitScreenGame = false;
}

static void M_AdventureGame(int choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(NEWGAME,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (grade & 128) // Ultimate skill level is unlockable Tails 05-19-2003
		NewGameMenu[ultimate].status = IT_STRING | IT_CALL;
	else
		NewGameMenu[ultimate].status = IT_DISABLED;

	startmap = 42;

	NewDef.prevMenu = currentMenu;

	M_SetupNextMenu(&NewDef);

	StartSplitScreenGame = false;
}

static void M_ChristmasGame(int choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(NEWGAME,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (grade & 128) // Ultimate skill level is unlockable Tails 05-19-2003
		NewGameMenu[ultimate].status = IT_STRING | IT_CALL;
	else
		NewGameMenu[ultimate].status = IT_DISABLED;

	startmap = 40;

	NewDef.prevMenu = currentMenu;

	M_SetupNextMenu(&NewDef);

	StartSplitScreenGame = false;
}

static void M_NightsGame(int choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(NEWGAME,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (grade & 128) // Ultimate skill level is unlockable Tails 05-19-2003
		NewGameMenu[ultimate].status = IT_STRING | IT_CALL;
	else
		NewGameMenu[ultimate].status = IT_DISABLED;

	startmap = 29;

	NewDef.prevMenu = currentMenu;

	M_SetupNextMenu(&NewDef);

	StartSplitScreenGame = false;
}

static void M_MarioGame(int choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(NEWGAME,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (grade & 128) // Ultimate skill level is unlockable Tails 05-19-2003
		NewGameMenu[ultimate].status = IT_STRING | IT_CALL;
	else
		NewGameMenu[ultimate].status = IT_DISABLED;

	startmap = 30;

	NewDef.prevMenu = currentMenu;

	M_SetupNextMenu(&NewDef);

	StartSplitScreenGame = false;
}

static void M_CustomWarp(int choice)
{
	if (netgame && Playing())
	{
		M_StartMessage(NEWGAME,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (grade & 128) // Ultimate skill level is unlockable Tails 05-19-2003
		NewGameMenu[ultimate].status = IT_STRING | IT_CALL;
	else
		NewGameMenu[ultimate].status = IT_DISABLED;

	startmap = customsecretinfo[choice-1].variable;

	NewDef.prevMenu = currentMenu;

	M_SetupNextMenu(&NewDef);

	StartSplitScreenGame = false;
}

// Chose the player you want to use Tails 03-02-2002
static void M_ChoosePlayer(int choice)
{
	int skinnum;

	M_ClearMenus(true);

	strlwr(description[choice].skinname);

	skinnum = R_SkinAvailable(description[choice].skinname);

	lastmapsaved = 0;

	G_DeferedInitNew(skillnum, G_BuildMapName(startmap), skinnum, StartSplitScreenGame, fromlevelselect);
	COM_BufAddText("dummyconsvar 1\n"); // G_DeferedInitNew doesn't do this
}

static void M_ReplayTimeAttack(int choice);
static void M_ChooseTimeAttackNoRecord(int choice);
static void M_ChooseTimeAttack(int choice);
static void M_DrawTimeAttackMenu(void);

typedef enum
{
	taplayer = 0,
	talevel,
	tareplay,
	tastart,
	timeattack_end
} timeattack_e;

static menuitem_t TimeAttackMenu[] =
{
	{IT_STRING|IT_CVAR,      NULL, "Player",     &cv_chooseskin,   50},
	{IT_STRING|IT_CVAR,      NULL, "Level",      &cv_nextmap,      65},

	{IT_WHITESTRING|IT_CALL, NULL, "Replay",   M_ReplayTimeAttack,        100},
	{IT_WHITESTRING|IT_CALL, NULL, "Start (No Record)",  M_ChooseTimeAttackNoRecord,115},
	{IT_WHITESTRING|IT_CALL, NULL, "Start (Record)",     M_ChooseTimeAttack,        130},
};

menu_t TimeAttackDef =
{
	0,
	"Time Attack",
	sizeof (TimeAttackMenu)/sizeof (menuitem_t),
	&SinglePlayerDef,
	TimeAttackMenu,
	M_DrawTimeAttackMenu,
	40, 40,
	0,
	NULL
};

// Used only for time attack menu
static void Nextmap_OnChange(void)
{
	if (currentMenu != &TimeAttackDef)
		return;

	TimeAttackMenu[tareplay].status = IT_DISABLED;

	// Check if file exists, if not, disable REPLAY option
	if (FIL_FileExists(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%02d.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1)))
		TimeAttackMenu[tareplay].status = IT_WHITESTRING|IT_CALL;
}

//
// M_TimeAttack
//
static void M_TimeAttack(int choice)
{
	(void)choice;

	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (modifiedgame && !savemoddata)
	{
		M_StartMessage("This cannot be done in a modified game.\n",NULL,MM_NOTHING);
		return;
	}

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	if (!(M_PatchLevelNameTable(2)))
	{
		M_StartMessage("No time-attackable levels found.\n",NULL,MM_NOTHING);
		return;
	}

	inlevelselect = 2; // Don't be dependent on cv_newgametype

	M_PatchSkinNameTable();

	M_SetupNextMenu(&TimeAttackDef);

	CV_AddValue(&cv_nextmap, 1);
	CV_AddValue(&cv_nextmap, -1);

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusic(mus_racent, true);
}

//
// M_DrawTimeAttackMenu
//
void M_DrawTimeAttackMenu(void)
{
	patch_t *PictureOfLevel;
	lumpnum_t lumpnum;
	char hours[4];
	char minutes[4];
	char seconds[4];
	char tics[4];
	tic_t besttime = 0;
	int i;

	S_ChangeMusic(mus_racent, true); // Eww, but needed for when user hits escape during demo playback

	V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

	if (W_CheckNumForName(description[cv_chooseskin.value-1].picname) != LUMPERROR)
		V_DrawSmallScaledPatch(224, 16, 0, W_CachePatchName(description[cv_chooseskin.value-1].picname, PU_CACHE));

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	V_DrawSmallScaledPatch(208, 128, 0, PictureOfLevel);

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!(mapheaderinfo[i].timeattack))
			continue;

		if (timedata[i].time > 0)
			besttime += timedata[i].time;
	}

	sprintf(hours,   "%02lu", besttime/(3600*TICRATE));
	sprintf(minutes, "%02lu", besttime/(60*TICRATE)%60);
	sprintf(seconds, "%02lu", (besttime/TICRATE) % 60);
	sprintf(tics,    "%02lu", besttime%TICRATE);

	V_DrawCenteredString(128, 36, 0, "Best Time Attack:");
	V_DrawCenteredString(128, 48, 0, va("%s:%s:%s.%s", hours, minutes, seconds, tics));

	if (cv_nextmap.value)
	{
		if (timedata[cv_nextmap.value-1].time > 0)
			V_DrawCenteredString(BASEVIDWIDTH/2, 116, 0, va("Best Time: %lu:%02lu.%02lu", timedata[cv_nextmap.value-1].time/(60*TICRATE)%60,
				(timedata[cv_nextmap.value-1].time/TICRATE) % 60, timedata[cv_nextmap.value-1].time % TICRATE));
	}

	M_DrawGenericMenu();
}

//
// M_ChooseTimeAttackNoRecord
//
// Like M_ChooseTimeAttack, but doesn't record a demo.
static void M_ChooseTimeAttackNoRecord(int choice)
{
	(void)choice;
	emeralds = 0;
	M_ClearMenus(true);
	timeattacking = true;
	remove(va("%s"PATHSEP"temp.lmp", srb2home));
	G_DeferedInitNew(sk_medium, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1, false, false);
	timeattacking = true;
}

//
// M_ChooseTimeAttack
//
// Player has selected the "START" from the time attack screen
static void M_ChooseTimeAttack(int choice)
{
	(void)choice;
	emeralds = 0;
	M_ClearMenus(true);
	timeattacking = true;
	G_RecordDemo("temp");
	G_BeginRecording();
	G_DeferedInitNew(sk_medium, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1, false, false);
	timeattacking = true;
}

//
// M_ReplayTimeAttack
//
// Player has selected the "REPLAY" from the time attack screen
static void M_ReplayTimeAttack(int choice)
{
	(void)choice;
	M_ClearMenus(true);
	G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%02d", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1));

	timeattacking = true;
}

static void M_ChooseSkill(int choice)
{
	skillnum = choice+1; // Tails 03-02-2002
	M_SetupChoosePlayer(0);
}

static void M_EraseData(int choice);

// Tails 08-11-2002
//===========================================================================
//                        Data OPTIONS MENU
//===========================================================================

static menuitem_t DataOptionsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Erase Time Attack Data", M_EraseData, 0},
	{IT_STRING | IT_CALL, NULL, "Erase Secrets Data", M_EraseData, 0},
};

menu_t DataOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (DataOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	DataOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

static void M_TimeDataResponse(int ch)
{
	int i;
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Delete the data
	for (i = 0; i < NUMMAPS; i++)
		timedata[i].time = 0;

	M_SetupNextMenu(&DataOptionsDef);
}

static void M_SecretsDataResponse(int ch)
{
	int i;
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Delete the data
	for (i = 0; i < MAXEMBLEMS; i++)
		emblemlocations[i].collected = false;

	foundeggs = 0;
	grade = 0;
	timesbeaten = 0;

	M_ClearMenus(true);
}

static void M_EraseData(int choice)
{
	if (Playing())
	{
		M_StartMessage("A game cannot be running.\nEnd it first.",NULL,MM_NOTHING);
		return;
	}

	else if (choice == 0)
		M_StartMessage("Are you sure you want to delete\nthe time attack data?\n(Y/N)\n",M_TimeDataResponse,MM_YESNO);
	else // 1
		M_StartMessage("Are you sure you want to delete\nthe secrets data?\n(Y/N)\n",M_SecretsDataResponse,MM_YESNO);
}

static menuitem_t ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Player 1 Controls...", M_Setup1PControlsMenu,   0},
	{IT_CALL    | IT_STRING, NULL, "Player 2 Controls...", M_Setup2PControlsMenu,  10},

	{IT_STRING  | IT_CVAR, NULL, "Analog Control(1P)", &cv_useranalog,  30}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING  | IT_CVAR, NULL, "Analog Control(2P)", &cv_useranalog2,  40}, // Changed all to normal string Tails 11-30-2000

	{IT_SUBMENU | IT_STRING, NULL, "Setup Joysticks...", &JoystickDef  ,  60},

	{IT_STRING  | IT_CVAR, NULL, "Camera (1P)"  , &cv_chasecam  ,  80}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING  | IT_CVAR, NULL, "Autoaim (1P)" , &cv_autoaim   ,  90}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING  | IT_CVAR, NULL, "Crosshair (1P)", &cv_crosshair , 100}, // Changed all to normal string Tails 11-30-2000

	{IT_STRING  | IT_CVAR, NULL, "Camera (2P)"  , &cv_chasecam2 , 110}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING  | IT_CVAR, NULL, "Autoaim (2P)" , &cv_autoaim2  , 120}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING  | IT_CVAR, NULL, "Crosshair (2P)", &cv_crosshair2, 130}, // Changed all to normal string Tails 11-30-2000

	{IT_STRING  | IT_CVAR, NULL, "Control per key", &cv_controlperkey, 150}, // Changed all to normal string Tails 11-30-2000
};

menu_t ControlsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (ControlsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	ControlsMenu,
	M_DrawGenericMenu,
	60, 24,
	0,
	NULL
};

//===========================================================================
//                             OPTIONS MENU
//===========================================================================
//
// M_Options
//

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t OptionsMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Setup Controls...", &ControlsDef,      20},
	{IT_CALL    | IT_STRING, NULL, "Game Options...",   M_GameOption,      40},
	{IT_SUBMENU | IT_STRING, NULL, "Server options...", &ServerOptionsDef, 50},
	{IT_SUBMENU | IT_STRING, NULL, "Sound Volume...",   &SoundDef,         60},
	{IT_SUBMENU | IT_STRING, NULL, "Video Options...",  &VideoOptionsDef,  70},
	{IT_SUBMENU | IT_STRING, NULL, "Mouse Options...",  &MouseOptionsDef,  80},
	{IT_SUBMENU | IT_STRING, NULL, "Data Options...",   &DataOptionsDef,   90},
};

menu_t OptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OptionsMenu)/sizeof (menuitem_t),
	&MainDef,
	OptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

// Tails 08-18-2002
static void M_OptionsMenu(int choice)
{
	(void)choice;
	M_SetupNextMenu (&OptionsDef);
}

FUNCNORETURN static ATTRNORETURN void M_UltimateCheat(int choice)
{
	(void)choice;
	I_Quit ();
}

static void M_DestroyRobotsResponse(int ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Destroy all robots
	P_DestroyRobots();

	M_ClearMenus(true);
}

static void M_DestroyRobots(int choice)
{
	(void)choice;
	if (!(Playing() && gamestate == GS_LEVEL))
	{
		M_StartMessage("You need to be playing and in\na level to do this!",NULL,MM_NOTHING);
		return;
	}

	if (multiplayer || netgame)
	{
		M_StartMessage("You can't do this in\na network game!",NULL,MM_NOTHING);
		return;
	}

	M_StartMessage("Do you want to destroy all\nrobots in the current level?\n(Y/N)\n",M_DestroyRobotsResponse,MM_YESNO);
}

static void M_LevelSelectWarp(int choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(NEWGAME,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (W_CheckNumForName(G_BuildMapName(cv_nextmap.value)) == LUMPERROR)
	{
//		CONS_Printf("\2Internal game map '%s' not found\n", G_BuildMapName(cv_nextmap.value));
		return;
	}

	// Ultimate always disabled in level select.
	NewGameMenu[ultimate].status = IT_DISABLED;

	startmap = cv_nextmap.value;

	fromlevelselect = true;

	NewDef.prevMenu = currentMenu;

	M_SetupNextMenu(&NewDef);

	StartSplitScreenGame = false;
}

/** Checklist of unlockable bonuses.
  */
typedef struct
{
	const char *name;        ///< What you get.
	const char *requirement; ///< What you have to do.
	boolean unlocked;        ///< Whether you've done it.
} checklist_t;

// Tails 12-19-2003
static void M_DrawUnlockChecklist(void)
{
#define NUMCHECKLIST 7
	checklist_t checklist[NUMCHECKLIST];
	int i;

	checklist[0].name = "Level Select";
	checklist[0].requirement = "Finish 1P";
	checklist[0].unlocked = (grade & 1);

	checklist[1].name = "Adventure Example";
	checklist[1].requirement = "Finish NiGHTS";
	checklist[1].unlocked = (grade & 64);

	checklist[2].name = "Mario Koopa Blast";
	checklist[2].requirement = "Finish 1P\nw/ Emeralds";
	checklist[2].unlocked = (grade & 8);

	checklist[3].name = "Sonic Into Dreams";
	checklist[3].requirement = "Find All Emblems";
	checklist[3].unlocked = (grade & 16);

	checklist[4].name = "Christmas Hunt";
	checklist[4].requirement = "Finish Mario";
	checklist[4].unlocked = (grade & 32);

	checklist[5].name = "Time Attack Bonus";
	checklist[5].requirement = "Finish 1P\nin 6 minutes";
	checklist[5].unlocked = (grade & 256);

	checklist[6].name = "Easter Egg Bonus";
	checklist[6].requirement = "Find all\nEaster Eggs";
	checklist[6].unlocked = (grade & 512);

	for (i = 0; i < NUMCHECKLIST; i++)
	{
		V_DrawString(8, 8+(24*i), V_RETURN8, checklist[i].name);
		V_DrawString(160, 8+(24*i), V_RETURN8, checklist[i].requirement);

		if (checklist[i].unlocked)
			V_DrawString(308, 8+(24*i), V_YELLOWMAP, "Y");
		else
			V_DrawString(308, 8+(24*i), V_YELLOWMAP, "N");
	}
}

boolean M_GotEnoughEmblems(int number)
{
	int i;
	int gottenemblems = 0;

	for (i = 0; i < MAXEMBLEMS; i++)
	{
		if (emblemlocations[i].collected)
			gottenemblems++;
	}

	if (gottenemblems >= number)
		return true;

	return false;
}

boolean M_GotLowEnoughTime(int time)
{
	int seconds = 0;
	int i;

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!(mapheaderinfo[i].timeattack))
			continue;

		if (timedata[i].time > 0)
			seconds += timedata[i].time;
		else
			seconds += 800*TICRATE;
	}

	seconds /= TICRATE;

	if (seconds <= time)
		return true;

	return false;
}

static void M_DrawCustomChecklist(void)
{
	int numcustom = 0;
	int i;
	int totalnum = 0;
	checklist_t checklist[15];

	memset(checklist, 0, sizeof (checklist));

	for (i = 0; i < 15; i++)
	{
		if (customsecretinfo[i].neededemblems)
		{
			checklist[i].unlocked = M_GotEnoughEmblems(customsecretinfo[i].neededemblems);

			if (checklist[i].unlocked && customsecretinfo[i].neededtime)
				checklist[i].unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (checklist[i].unlocked && customsecretinfo[i].neededgrade)
				checklist[i].unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else if (customsecretinfo[i].neededtime)
		{
			checklist[i].unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (checklist[i].unlocked && customsecretinfo[i].neededgrade)
				checklist[i].unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else
			checklist[i].unlocked = (grade & customsecretinfo[i].neededgrade);

		if (checklist[i].unlocked)
			totalnum++;
	}

	for (i = 0; i < 15; i++)
	{
		if (checklist[i].unlocked && totalnum > 7)
			continue;

		if (customsecretinfo[i].name[0] == 0)
			continue;

		V_DrawString(8, 8+(24*numcustom), V_RETURN8, customsecretinfo[i].name);
		V_DrawString(160, 8+(24*numcustom), V_RETURN8|V_WORDWRAP, customsecretinfo[i].objective);

		if (checklist[i].unlocked)
			V_DrawString(308, 8+(24*numcustom), V_YELLOWMAP, "Y");
		else
			V_DrawString(308, 8+(24*numcustom), V_YELLOWMAP, "N");

		numcustom++;

		if (numcustom > 6)
			break;
	}
}

static void M_DrawTimeAttackBonus(void)
{
	V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("CONCEPT1", PU_CACHE));
}

static void M_DrawEasterEggBonus(void)
{
	V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("CONCEPT2", PU_CACHE));
}

// Empty thingy for checklist menu
typedef enum
{
	unlockchecklistempty1,
	unlockchecklist_end
} unlockchecklist_e;

static menuitem_t UnlockChecklistMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_SecretsMenu, 192},
};

menu_t UnlockChecklistDef =
{
	NULL,
	NULL,
	unlockchecklist_end,
	&SecretsDef,
	UnlockChecklistMenu,
	M_DrawUnlockChecklist,
	280, 185,
	0,
	NULL
};

// Empty thingy for custom checklist menu
typedef enum
{
	customchecklistempty1,
	customchecklist_end
} customchecklist_e;

static menuitem_t CustomChecklistMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_CustomSecretsMenu, 192},
};

menu_t CustomChecklistDef =
{
	NULL,
	NULL,
	customchecklist_end,
	&CustomSecretsDef,
	CustomChecklistMenu,
	M_DrawCustomChecklist,
	280, 185,
	0,
	NULL
};

// Empty thingy for bonus menu
typedef enum
{
	timeattackbonusempty1,
	timeattackbonus_end
} timeattackbonus_e;

static menuitem_t TimeAttackBonusMenu[] =
{
	{IT_CALL | IT_STRING, 0, "NEXT", M_SecretsMenu, 192},
};

menu_t TimeAttackBonusDef =
{
	NULL,
	NULL,
	timeattackbonus_end,
	&MainDef,
	TimeAttackBonusMenu,
	M_DrawTimeAttackBonus,
	280, 185,
	0,
	NULL
};

// Empty thingy for bonus menu
typedef enum
{
	eastereggbonusempty1,
	eastereggbonus_end
} easteregg_e;

static menuitem_t EasterEggBonusMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_SecretsMenu, 192},
};

menu_t EasterEggBonusDef =
{
	NULL,
	NULL,
	eastereggbonus_end,
	&MainDef,
	EasterEggBonusMenu,
	M_DrawEasterEggBonus,
	280, 185,
	0,
	NULL
};

static void M_UnlockChecklist(int choice)
{
	(void)choice;
	if (savemoddata)
	{
		M_StartMessage("Checklist does not apply\nfor this mod.\nUse statistics screen instead.", NULL, MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&UnlockChecklistDef);
}

static void M_CustomChecklist(int choice)
{
	(void)choice;
	M_SetupNextMenu(&CustomChecklistDef);
}

static void M_TimeAttackBonus(int choice)
{
	(void)choice;
	M_SetupNextMenu(&TimeAttackBonusDef);
}

static void M_EasterEggBonus(int choice)
{
	(void)choice;
	M_SetupNextMenu(&EasterEggBonusDef);
}

static void M_BetaShowcase(int choice)
{
	(void)choice;
}

//===========================================================================
//                             ??? MENU
//===========================================================================
//
// M_Options
//
static void M_Reward(int choice);
static void M_LevelSelect(int choice);

typedef enum
{
	unlockchecklist = 0,
	ultimatecheat,
	secretsgravity,
	ringslinger,
	levelselect,
	rings,
	lives,
	continues,
	norobots,
	betashowcase,
	reward,
	timeattackbonus,
	eastereggbonus,
	secrets_end
} secrets_e;

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t SecretsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Secrets Checklist",  M_UnlockChecklist,    0}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CALL, NULL, "Ultimate Cheat",     M_UltimateCheat,     20}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR, NULL, "Gravity",            &cv_gravity,         40}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR, NULL, "Throw Rings",        &cv_ringslinger,     50}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_DISABLED | IT_CALL
	             , NULL, "Level Select",       M_LevelSelect,       60}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR, NULL, "Modify Rings",       &cv_startrings,      70}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR, NULL, "Modify Lives",       &cv_startlives,      80}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR, NULL, "Modify Continues",   &cv_startcontinues,  90}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CALL, NULL, "Destroy All Robots", M_DestroyRobots,    100}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_DISABLED | IT_CALL
	             , NULL, "Beta Showcase",      M_BetaShowcase,     110}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CALL, NULL, "Bonus Levels",       M_Reward,           120}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CALL, NULL, "Time Attack Bonus",  M_TimeAttackBonus,  130},
	{IT_STRING | IT_CALL, NULL, "Easter Egg Bonus",   M_EasterEggBonus,   140},
};

menu_t SecretsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	secrets_end,
	&MainDef,
	SecretsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                             Custom Secrets MENU
//===========================================================================
//
//
//

typedef enum
{
	customchecklist = 0,
	custom1,
	custom2,
	custom3,
	custom4,
	custom5,
	custom6,
	custom7,
	custom8,
	custom9,
	custom10,
	custom11,
	custom12,
	custom13,
	custom14,
	custom15,
	customsecrets_end
} customsecrets_e;

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t CustomSecretsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Secrets Checklist",  M_CustomChecklist,    0},
	{IT_STRING | IT_CALL, NULL, "Custom1",   M_CustomLevelSelect,       10},
	{IT_STRING | IT_CALL, NULL, "Custom2",   M_CustomWarp,    20},
	{IT_STRING | IT_CALL, NULL, "Custom3",   M_CustomWarp,    30},
	{IT_STRING | IT_CALL, NULL, "Custom4",   M_CustomWarp,    40},
	{IT_STRING | IT_CALL, NULL, "Custom5",   M_CustomWarp,    50},
	{IT_STRING | IT_CALL, NULL, "Custom6",   M_CustomWarp,    60},
	{IT_STRING | IT_CALL, NULL, "Custom7",   M_CustomWarp,    70},
	{IT_STRING | IT_CALL, NULL, "Custom8",   M_CustomWarp,    80},
	{IT_STRING | IT_CALL, NULL, "Custom9",   M_CustomWarp,    90},
	{IT_STRING | IT_CALL, NULL, "Custom10",   M_CustomWarp,    100},
	{IT_STRING | IT_CALL, NULL, "Custom11",   M_CustomWarp,    110},
	{IT_STRING | IT_CALL, NULL, "Custom12",   M_CustomWarp,    120},
	{IT_STRING | IT_CALL, NULL, "Custom13",   M_CustomWarp,    130},
	{IT_STRING | IT_CALL, NULL, "Custom14",   M_CustomWarp,    140},
	{IT_STRING | IT_CALL, NULL, "Custom15",   M_CustomWarp,    150},
};

menu_t CustomSecretsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	customsecrets_end,
	&MainDef,
	CustomSecretsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                             Reward MENU
//===========================================================================
//
// M_Reward
//
typedef enum
{
	nights,
	mario,
	adventure,
	xmasfind,
	reward_end
} reward_e;

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t RewardMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Sonic Into Dreams", M_NightsGame,    30},
	{IT_STRING | IT_CALL, NULL, "Mario Koopa Blast", M_MarioGame,     50},
	{IT_STRING | IT_CALL, NULL, "Adventure Example", M_AdventureGame, 70},
	{IT_STRING | IT_CALL, NULL, "Christmas Hunt",    M_ChristmasGame, 90},
};

menu_t RewardDef =
{
	"M_OPTTTL",
	"OPTIONS",
	reward_end,
	&SecretsDef,
	RewardMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

static void M_Reward(int choice)
{
	(void)choice;
	if (grade & 8)
		RewardMenu[mario].status = IT_STRING | IT_CALL;
	else
		RewardMenu[mario].status |= IT_DISABLED;

	if (grade & 16)
		RewardMenu[nights].status = IT_STRING | IT_CALL;
	else
		RewardMenu[nights].status |= IT_DISABLED;

	if (grade & 32)
		RewardMenu[xmasfind].status = IT_STRING | IT_CALL;
	else
		RewardMenu[xmasfind].status |= IT_DISABLED;

	if (grade & 64)
		RewardMenu[adventure].status = IT_STRING | IT_CALL;
	else
		RewardMenu[adventure].status |= IT_DISABLED;

	M_SetupNextMenu (&RewardDef);
}

//===========================================================================
//                             Level Select Menu
//===========================================================================
//
// M_LevelSelect
//

static void M_DrawLevelSelectMenu(void)
{
	M_DrawGenericMenu();

	if (cv_nextmap.value)
	{
		lumpnum_t lumpnum;
		patch_t *PictureOfLevel;

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

		V_DrawSmallScaledPatch(200, 110, 0, PictureOfLevel);
	}
}

static menuitem_t LevelSelectMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                 M_LevelSelectWarp,     120},
};

menu_t LevelSelectDef =
{
	0,
	"Level Select",
	sizeof (LevelSelectMenu)/sizeof (menuitem_t),
	&SecretsDef,
	LevelSelectMenu,
	M_DrawLevelSelectMenu,
	40, 40,
	0,
	NULL
};

static void M_LevelSelect(int choice)
{
	(void)choice;
	LevelSelectDef.prevMenu = &SecretsDef;
	inlevelselect = 1;

	if (!(M_PatchLevelNameTable(1)))
	{
		M_StartMessage("No selectable levels found.\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&LevelSelectDef);
}

static void M_CustomLevelSelect(int choice)
{
	(void)choice;
	LevelSelectDef.prevMenu = &CustomSecretsDef;
	inlevelselect = 1;

	if (!(M_PatchLevelNameTable(1)))
	{
		M_StartMessage("No selectable levels found.\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&LevelSelectDef);
}

static void M_SecretsMenu(int choice)
{
	int i;

	// Disable all the menu choices
	(void)choice;
	for (i = ultimatecheat;i < secrets_end;i++)
		SecretsMenu[i].status = IT_DISABLED;

	// Check grade and enable options as appropriate
	switch (grade&7)
	{
		case 7:
		case 6:
//			SecretsMenu[betashowcase].status = IT_STRING | IT_CALL;
			SecretsMenu[norobots].status = IT_STRING | IT_CALL;
			SecretsMenu[rings].status = IT_STRING | IT_CVAR;
		case 5:
		case 4:
			SecretsMenu[lives].status = IT_STRING | IT_CVAR;
			SecretsMenu[continues].status = IT_STRING | IT_CVAR;
		case 3:
			SecretsMenu[ringslinger].status = IT_STRING | IT_CVAR;
		case 2:
			SecretsMenu[secretsgravity].status = IT_STRING | IT_CVAR;
			SecretsMenu[ultimatecheat].status = IT_STRING | IT_CALL;
		case 1:
//			SecretsMenu[reward].status = IT_STRING | IT_CALL;
			break;
		default: // You shouldn't even be here, weirdo.
			return;
			break;
	}

	if ((grade & 8) ||
	(grade & 16) ||
	(grade & 32) ||
	(grade & 64) ||
	(grade & 1024))
		SecretsMenu[reward].status = IT_STRING | IT_CALL;
	else
		SecretsMenu[reward].status = IT_DISABLED;


	if (grade & 1)
		SecretsMenu[levelselect].status = IT_STRING | IT_CALL;

	if (grade & 256)
		SecretsMenu[timeattackbonus].status = IT_STRING | IT_CALL;

	if (grade & 512)
		SecretsMenu[eastereggbonus].status = IT_STRING | IT_CALL;

	M_SetupNextMenu(&SecretsDef);
}

static void M_CustomSecretsMenu(int choice)
{
	int i;
	boolean unlocked;

	// Disable all the menu choices
	(void)choice;
	for (i = custom1;i < customsecrets_end;i++)
		CustomSecretsMenu[i].status = IT_DISABLED;

	for (i = 0; i < 15; i++)
	{
		unlocked = false;

		if (customsecretinfo[i].neededemblems)
		{
			unlocked = M_GotEnoughEmblems(customsecretinfo[i].neededemblems);

			if (unlocked && customsecretinfo[i].neededtime)
				unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (unlocked && customsecretinfo[i].neededgrade)
				unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else if (customsecretinfo[i].neededtime)
		{
			unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (unlocked && customsecretinfo[i].neededgrade)
				unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else
			unlocked = (grade & customsecretinfo[i].neededgrade);

		if (unlocked)
		{
			CustomSecretsMenu[custom1+i].status = IT_STRING|IT_CALL;

			switch (customsecretinfo[i].type)
			{
				case 0:
					CustomSecretsMenu[custom1+i].itemaction = M_CustomLevelSelect;
					break;
				case 1:
					CustomSecretsMenu[custom1+i].itemaction = M_CustomWarp;
				default:
					break;
			}

			CustomSecretsMenu[custom1+i].text = customsecretinfo[i].name;
		}
	}

	M_SetupNextMenu(&CustomSecretsDef);
}

//
//  A smaller 'Thermo', with range given as percents (0-100)
//
static void M_DrawSlider(int x, int y, const consvar_t *cv)
{
	int i;
	int range;
	patch_t *p;

	for (i = 0; cv->PossibleValue[i+1].strvalue; i++);

	range = ((cv->value - cv->PossibleValue[0].value) * 100 /
	 (cv->PossibleValue[i].value - cv->PossibleValue[0].value));

	if (range < 0)
		range = 0;
	if (range > 100)
		range = 100;

	x = BASEVIDWIDTH - x - SLIDER_WIDTH;

	V_DrawScaledPatch(x - 8, y, 0, W_CachePatchName("M_SLIDEL", PU_CACHE));

	p =  W_CachePatchName("M_SLIDEM", PU_CACHE);
	for (i = 0; i < SLIDER_RANGE; i++)
		V_DrawScaledPatch (x+i*8, y, 0,p);

	p = W_CachePatchName("M_SLIDER", PU_CACHE);
	V_DrawScaledPatch(x+SLIDER_RANGE*8, y, 0, p);

	// draw the slider cursor
	p = W_CachePatchName("M_SLIDEC", PU_CACHE);
	V_DrawMappedPatch(x + ((SLIDER_RANGE-1)*8*range)/100, y, 0, p, yellowmap);
}

//===========================================================================
//                        Video OPTIONS MENU
//===========================================================================

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t VideoOptionsMenu[] =
{
	// Tails
	{IT_STRING | IT_SUBMENU, NULL, "Video Modes...",      &VidModeDef,      0},
#if defined (UNIXLIKE) || defined (SDL)
	{IT_STRING|IT_CVAR,      NULL, "Fullscreen",          &cv_fullscreen,  10},
#endif
#ifdef HWRENDER
	//17/10/99: added by Hurdler
	{IT_CALL|IT_WHITESTRING, NULL, "3D Card Options...",  M_OpenGLOption,  20},
#endif
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                         NULL, "Brightness",          &cv_usegamma,    40},

	{IT_STRING | IT_CVAR,    NULL, "V-SYNC",              &cv_vidwait,     50},

	{IT_STRING | IT_CVAR,    NULL, "Snow Density",        &cv_numsnow,     70}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR,    NULL, "Rain Density",        &cv_raindensity, 80}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR,    NULL, "Rain/Snow Draw Dist", &cv_precipdist,  90}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR,    NULL, "FPS Meter",           &cv_ticrate,    100},
};

menu_t VideoOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (VideoOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	VideoOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Mouse OPTIONS MENU
//===========================================================================

//added : 24-03-00: note: alphaKey member is the y offset
static menuitem_t MouseOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse",        &cv_usemouse,         0},
	{IT_STRING | IT_CVAR, NULL, "Always MouseLook", &cv_alwaysfreelook,   0},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove,        0},
	{IT_STRING | IT_CVAR, NULL, "Invert Mouse",     &cv_invertmouse,      0},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Speed",      &cv_mousesens,        0},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mlook Speed",      &cv_mlooksens,        0},
};

menu_t MouseOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (MouseOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	MouseOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Game OPTIONS MENU
//===========================================================================

static menuitem_t GameOptionsMenu[] =
{
	// Tails
	{IT_STRING | IT_CVAR, NULL, "Number of Sound Channels", &cv_numChannels,   0},
	{IT_STRING | IT_CVAR, NULL, "Camera Speed",             &cv_cam_speed,     0},
	{IT_STRING | IT_CVAR, NULL, "Camera Height",            &cv_cam_height,    0},
	{IT_STRING | IT_CVAR, NULL, "Camera Distance",          &cv_cam_dist,      0},
	{IT_STRING | IT_CVAR, NULL, "Hold Camera Angle",        &cv_cam_still,     0},
	{IT_STRING | IT_CVAR, NULL, "Camera Rotation",          &cv_cam_rotate,    0},
	{IT_STRING | IT_CV_SLIDER | IT_CVAR,
	                      NULL, "Camera Rotation Speed",    &cv_cam_rotspeed,  0},
	{IT_STRING | IT_CVAR, NULL, "2P Camera Speed",          &cv_cam2_speed,    0},
	{IT_STRING | IT_CVAR, NULL, "2P Camera Height",         &cv_cam2_height,   0},
	{IT_STRING | IT_CVAR, NULL, "2P Camera Distance",       &cv_cam2_dist,     0},
	{IT_STRING | IT_CVAR, NULL, "2P Hold Camera Angle",     &cv_cam2_still,    0},
	{IT_STRING | IT_CVAR, NULL, "2P Camera Rotation",       &cv_cam2_rotate,   0},
	{IT_STRING | IT_CV_SLIDER | IT_CVAR,
	                      NULL, "2P Camera Rotation Speed", &cv_cam2_rotspeed, 0},
	{IT_STRING | IT_CVAR, NULL, "High Resolution Timer",    &cv_timetic,       0},
//	{IT_CALL   | IT_WHITESTRING, 0, "Network Options...",   M_NetOption,       0},
};

menu_t GameOptionDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (GameOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	GameOptionsMenu,
	M_DrawGenericMenu,
	24, 32,
	0,
	NULL
};

static void M_GameOption(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&GameOptionDef);
}

static void M_MonitorToggles(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&MonitorToggleDef);
}

//===========================================================================
//                        Network OPTIONS MENU
//===========================================================================

// Graue 12-06-2003
static menuitem_t NetOptionsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Match Mode Options...",  M_MatchOptions,      20},
	{IT_STRING | IT_CALL, NULL, "Race Mode Options...",  M_RaceOptions,       30},
	{IT_STRING | IT_CALL, NULL, "Tag Mode Options...",  M_TagOptions,        40},
	{IT_STRING | IT_CALL, NULL, "CTF Mode Options...",  M_CTFOptions,        50},
#ifdef CHAOSISNOTDEADYET
	{IT_STRING | IT_CALL, NULL, "Chaos Mode Options...",  M_ChaosOptions,      60},
#endif
	{IT_STRING | IT_CVAR, NULL, "Server controls skin",   &cv_forceskin,       80},
	{IT_STRING | IT_CVAR, NULL, "Allow autoaim",          &cv_allowautoaim,    90},
	{IT_STRING | IT_CVAR, NULL, "Allow join player",      &cv_allownewplayer, 100},
	{IT_STRING | IT_CVAR, NULL, "Allow WAD Downloading",  &cv_downloading,    110},
	// Moved level timelimit to the individual mode options Graue 12-13-2003
	{IT_STRING | IT_CVAR, NULL, "Max Players",            &cv_maxplayers,     130},

	{IT_STRING | IT_CALL, NULL, "Monitor Toggles...",     M_MonitorToggles,   140},

	//{IT_CALL | IT_WHITESTRING,NULL, "Game Options...",  M_GameOption,       150},
};

menu_t NetOptionDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (NetOptionsMenu)/sizeof (menuitem_t),
	&MultiPlayerDef,
	NetOptionsMenu,
	M_DrawGenericMenu,
	60, 30,
	0,
	NULL
};

//===========================================================================
//                        Match Mode OPTIONS MENU
//===========================================================================

static menuitem_t MatchOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Special Ring Weapons",  &cv_specialrings,    10},
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",            &cv_matchboxes,      20},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn",          &cv_itemrespawn,     30},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn time",     &cv_itemrespawntime, 40},
	{IT_STRING | IT_CVAR, NULL, "Scoring Type",          &cv_match_scoring,   50},
	{IT_STRING | IT_CVAR, NULL, "Sudden Death",          &cv_suddendeath,     60},

	{IT_STRING | IT_CVAR, NULL, "Time Limit",            &cv_timelimit,       80}, // Graue 12-13-2003
	{IT_STRING | IT_CVAR, NULL, "Point Limit",           &cv_pointlimit,      90}, // Graue 12-13-2003

	{IT_STRING | IT_CVAR, NULL, "Intermission Timer",    &cv_inttime,        110},
	{IT_STRING | IT_CVAR, NULL, "Advance to next level", &cv_advancemap,     120},
};

menu_t MatchOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (MatchOptionsMenu)/sizeof (menuitem_t),
	&NetOptionDef,
	MatchOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Race Mode OPTIONS MENU
//===========================================================================

static menuitem_t RaceOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",   &cv_raceitemboxes, 40},
	{IT_STRING | IT_CVAR, NULL, "Number of Laps", &cv_numlaps, 60},
};

menu_t RaceOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (RaceOptionsMenu)/sizeof (menuitem_t),
	&NetOptionDef,
	RaceOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Tag Mode OPTIONS MENU
//===========================================================================

static menuitem_t TagOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Special Ring Weapons",  &cv_specialrings,    10},
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",            &cv_matchboxes,      20},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn",          &cv_itemrespawn,     30},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn time",     &cv_itemrespawntime, 40},

	{IT_STRING | IT_CVAR, NULL, "Time Limit",            &cv_timelimit,       60}, // Graue 12-13-2003
	{IT_STRING | IT_CVAR, NULL, "Point Limit",           &cv_pointlimit,      70}, // Graue 12-13-2003

	{IT_STRING | IT_CVAR, NULL, "Intermission Timer",    &cv_inttime,         90},
	{IT_STRING | IT_CVAR, NULL, "Advance to next level", &cv_advancemap,     100},
};

menu_t TagOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (TagOptionsMenu)/sizeof (menuitem_t),
	&NetOptionDef,
	TagOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        CTF Mode OPTIONS MENU
//===========================================================================

static menuitem_t CTFOptionsMenu[] =
{ // Reordered the options nicely Graue 12-13-2003
	{IT_STRING | IT_CVAR, NULL, "Special Ring Weapons",  &cv_specialrings,    10},
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",            &cv_matchboxes,      20},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn",          &cv_itemrespawn,     30},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn time",     &cv_itemrespawntime, 40},
	{IT_STRING | IT_CVAR, NULL, "Flag Respawn Time",     &cv_flagtime,        50},

	{IT_STRING | IT_CVAR, NULL, "Time Limit",            &cv_timelimit,      100},
	{IT_STRING | IT_CVAR, NULL, "Point Limit",           &cv_pointlimit,     110},

	{IT_STRING | IT_CVAR, NULL, "Intermission Timer",    &cv_inttime,        130},
	{IT_STRING | IT_CVAR, NULL, "Advance to next level", &cv_advancemap,     140},
};

menu_t CTFOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (CTFOptionsMenu)/sizeof (menuitem_t),
	&NetOptionDef,
	CTFOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Chaos Mode OPTIONS MENU
//===========================================================================

#ifdef CHAOSISNOTDEADYET
static menuitem_t ChaosOptionsMenu[] =
{
	{IT_STRING | IT_CVAR,    NULL, "Item Boxes",            &cv_matchboxes,      20},
	{IT_STRING | IT_CVAR,    NULL, "Item Respawn",          &cv_itemrespawn,     30},
	{IT_STRING | IT_CVAR,    NULL, "Item Respawn time",     &cv_itemrespawntime, 40},
	{IT_STRING | IT_CVAR,    NULL, "Enemy respawn rate",    &cv_chaos_spawnrate, 50},
	{IT_SUBMENU | IT_STRING, NULL, "Enemy Toggles...",      &EnemyToggleDef,     60},

	{IT_STRING | IT_CVAR,    NULL, "Time Limit",            &cv_timelimit,       80},
	{IT_STRING | IT_CVAR,    NULL, "Point Limit",           &cv_pointlimit,      90},

	{IT_STRING | IT_CVAR,    NULL, "Intermission Timer",    &cv_inttime,        110},
	{IT_STRING | IT_CVAR,    NULL, "Advance to next level", &cv_advancemap,     120},

};

menu_t ChaosOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (ChaosOptionsMenu)/sizeof (menuitem_t),
	&NetOptionDef,
	ChaosOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Enemy Toggle MENU
//===========================================================================

static menuitem_t EnemyToggleMenu[] =
{
	{IT_STRING|IT_CVAR, NULL, "Blue Crawla",      &cv_chaos_bluecrawla,      30},
	{IT_STRING|IT_CVAR, NULL, "Red Crawla",       &cv_chaos_redcrawla,       40},
	{IT_STRING|IT_CVAR, NULL, "Crawla Commander", &cv_chaos_crawlacommander, 50},
	{IT_STRING|IT_CVAR, NULL, "Skim",             &cv_chaos_skim,            60},
	{IT_STRING|IT_CVAR, NULL, "JettySyn Bomber",  &cv_chaos_jettysynbomber,  70},
	{IT_STRING|IT_CVAR, NULL, "JettySyn Gunner",  &cv_chaos_jettysyngunner,  80},
	{IT_STRING|IT_CVAR, NULL, "Boss 1",           &cv_chaos_eggmobile1,      90},
	{IT_STRING|IT_CVAR, NULL, "Boss 2",           &cv_chaos_eggmobile2,     100},
};

menu_t EnemyToggleDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (EnemyToggleMenu)/sizeof (menuitem_t),
	&ChaosOptionsDef,
	EnemyToggleMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};
#endif

//===========================================================================
//                        Monitor Toggle MENU
//===========================================================================

static menuitem_t MonitorToggleMenu[] =
{
	{IT_STRING|IT_CVAR, NULL, "Teleporters",       &cv_teleporters,   20},
	{IT_STRING|IT_CVAR, NULL, "Super Ring",        &cv_superring,     30},
	{IT_STRING|IT_CVAR, NULL, "Super Sneakers",    &cv_supersneakers, 40},
	{IT_STRING|IT_CVAR, NULL, "Invincibility",     &cv_invincibility, 50},
	{IT_STRING|IT_CVAR, NULL, "Jump Shield",       &cv_jumpshield,    60},
	{IT_STRING|IT_CVAR, NULL, "Elemental Shield",  &cv_watershield,   70},
	{IT_STRING|IT_CVAR, NULL, "Attraction Shield", &cv_ringshield,    80},
	{IT_STRING|IT_CVAR, NULL, "Force Shield",      &cv_forceshield,   90},
	{IT_STRING|IT_CVAR, NULL, "Armageddon Shield", &cv_bombshield,   100},
	{IT_STRING|IT_CVAR, NULL, "1 Up",              &cv_1up,          110},
	{IT_STRING|IT_CVAR, NULL, "Eggman Box",        &cv_eggmanbox,    120},
};

menu_t MonitorToggleDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (MonitorToggleMenu)/sizeof (menuitem_t),
	&NetOptionDef,
	MonitorToggleMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

static void M_NetOption(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n", NULL, MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&NetOptionDef);
}

static void M_MatchOptions(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n", NULL, MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&MatchOptionsDef);
}

static void M_RaceOptions(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&RaceOptionsDef);
}

static void M_TagOptions(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&TagOptionsDef);
}

static void M_CTFOptions(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&CTFOptionsDef);
}

#ifdef CHAOSISNOTDEADYET
static void M_ChaosOptions(int choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&ChaosOptionsDef);
}
#endif

//===========================================================================
//                        Server OPTIONS MENU
//===========================================================================
static menuitem_t ServerOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Internet server", &cv_internetserver,     0},
	{IT_STRING | IT_CVAR | IT_CV_STRING,
	                      NULL, "Master server",   &cv_masterserver,       0},
	{IT_STRING | IT_CVAR | IT_CV_STRING,
	                      NULL, "Server name",     &cv_servername,         0},
};

menu_t ServerOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (ServerOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	ServerOptionsMenu,
	M_DrawGenericMenu,
	28, 40,
	0,
	NULL
};

//===========================================================================
//                          Read This! MENU 1
//===========================================================================

static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);

typedef enum
{
	rdthsempty1,
	read1_end
} read_e;

static menuitem_t ReadMenu1[] =
{
	{IT_SUBMENU | IT_NOTHING, NULL, "", &MainDef, 0},
};

menu_t ReadDef1 =
{
	NULL,
	NULL,
	read1_end,
	NULL,
	ReadMenu1,
	M_DrawReadThis1,
	330, 165,
	0,
	NULL
};

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
static void M_DrawReadThis1(void)
{
	V_DrawScaledPatch (0,0,0,W_CachePatchName("HELP",PU_CACHE));
	return;
}

//===========================================================================
//                          *B^D 'Menu'
//===========================================================================

typedef enum
{
	rdthsempty2,
	read2_end
} read_e2;

static menuitem_t ReadMenu2[] =
{
	{IT_SUBMENU | IT_NOTHING, NULL, "", &MainDef, 0},
};

menu_t ReadDef2 =
{
	NULL,
	NULL,
	read2_end,
	NULL,
	ReadMenu2,
	M_DrawReadThis2,
	330, 175,
	0,
	NULL
};


//
// Read This Menus - optional second page.
//
static void M_DrawReadThis2(void)
{
	V_DrawScaledPic (0,0,0,W_GetNumForName ("BULMER"));
	HU_Drawer();
	return;
}

// M_ToggleSFX
// M_ToggleDigital
// M_ToggleMIDI
//
// Toggles sound systems in-game.
//
static void M_ToggleSFX(void)
{
	if (nosound)
	{
		nosound = false;
		I_StartupSound();
		if (nosound) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		M_StartMessage("SFX Enabled", NULL, MM_NOTHING);
	}
	else
	{
		if (sound_disabled)
		{
			sound_disabled = false;
			M_StartMessage("SFX Enabled", NULL, MM_NOTHING);
		}
		else
		{
			sound_disabled = true;
			S_StopSounds();
			M_StartMessage("SFX Disabled", NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleDigital(void)
{
	if (nofmod)
	{
		nofmod = false;
		I_InitDigMusic();
		if (nofmod) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_StopMusic();
		S_ChangeMusic(mus_lclear, false);
		M_StartMessage("Digital Music Enabled", NULL, MM_NOTHING);
	}
	else
	{
		if (digital_disabled)
		{
			digital_disabled = false;
			M_StartMessage("Digital Music Enabled", NULL, MM_NOTHING);
		}
		else
		{
			digital_disabled = true;
			S_StopMusic();
			M_StartMessage("Digital Music Disabled", NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleMIDI(void)
{
	if (nomusic)
	{
		nomusic = false;
		I_InitMIDIMusic();
		if (nomusic) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_ChangeMusic(mus_lclear, false);
		M_StartMessage("MIDI Music Enabled", NULL, MM_NOTHING);
	}
	else
	{
		if (music_disabled)
		{
			music_disabled = false;
			M_StartMessage("MIDI Music Enabled", NULL, MM_NOTHING);
		}
		else
		{
			music_disabled = true;
			S_StopMusic();
			M_StartMessage("MIDI Music Disabled", NULL, MM_NOTHING);
		}
	}
}

//===========================================================================
//                        SOUND VOLUME MENU
//===========================================================================

typedef enum
{
	sfx_vol,
	sfx_empty1,
	digmusic_vol,
	sfx_empty2,
	midimusic_vol,
	sfx_empty3,
#ifdef PC_DOS
	cdaudio_vol,
	sfx_empty4,
#endif
	tog_sfx,
	tog_dig,
	tog_midi,
	sound_end
} sound_e;

static menuitem_t SoundMenu[] =
{
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Sound Volume" , &cv_soundvolume,     0},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Music Volume" , &cv_digmusicvolume,  0},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "MIDI Volume"  , &cv_midimusicvolume, 0},
#ifdef PC_DOS
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "CD Volume"    , &cd_volume,          0},
#endif
	{IT_STRING    | IT_CALL,  NULL,  "Toggle SFX"   , M_ToggleSFX,         0},
	{IT_STRING    | IT_CALL,  NULL,  "Toggle Digital Music", M_ToggleDigital,     0},
	{IT_STRING    | IT_CALL,  NULL,  "Toggle MIDI Music", M_ToggleMIDI,        0},
};

menu_t SoundDef =
{
	"M_SVOL",
	"Sound Volume",
	sizeof (SoundMenu)/sizeof (menuitem_t),
	&OptionsDef,
	SoundMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                          JOYSTICK MENU
//===========================================================================
static void M_Setup1PJoystickMenu(int choice);
static void M_Setup2PJoystickMenu(int choice);

typedef enum
{
	p1joy,
	p1set,
	p1turn,
	p1move,
	p1side,
	p1look,
	p1fire,
	p1nfire,
	p2joy,
	p2set,
	p2turn,
	p2move,
	p2side,
	p2look,
	p2fire,
	p2nfire,
	joystick_end
} joy_e;


static menuitem_t JoystickMenu[] =
{
	{IT_WHITESTRING | IT_SPACE, NULL, "Player 1 Joystick" , NULL                 ,  10},
	{IT_STRING      | IT_CALL,  NULL, "Select Joystick...", M_Setup1PJoystickMenu,  20},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Turning"  , &cv_turnaxis         ,  30},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Moving"   , &cv_moveaxis         ,  40},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Strafe"   , &cv_sideaxis         ,  50},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Looking"  , &cv_lookaxis         ,  60},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Firing"   , &cv_fireaxis         ,  70},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For NFiring"  , &cv_firenaxis        ,  80},
	{IT_WHITESTRING | IT_SPACE, NULL, "Player 2 Joystick" , NULL                 ,  90},
	{IT_STRING      | IT_CALL,  NULL, "Select Joystick...", M_Setup2PJoystickMenu, 100},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Turning"  , &cv_turnaxis2        , 110},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Moving"   , &cv_moveaxis2        , 120},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Strafe"   , &cv_sideaxis2        , 130},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Looking"  , &cv_lookaxis2        , 140},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Firing"   , &cv_fireaxis2        , 150},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For NFiring"  , &cv_firenaxis2       , 160},

};

menu_t JoystickDef =
{
	"M_CONTRO",
	"Setup Joystick",
	joystick_end,
	&ControlsDef,
	JoystickMenu,
	M_DrawGenericMenu,
	50, 20,
	1,
	NULL
};

static void M_DrawJoystick(void);
static void M_AssignJoystick(int choice);

typedef enum
{
	joy0 = 0,
	joy1,
	joy2,
	joy3,
	joy4,
	joy5,
	joy6,
	joystickset_end
} joyset_e;

static menuitem_t JoystickSetMenu[] =
{
	{IT_CALL | IT_NOTHING, "None", NULL, M_AssignJoystick, '0'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '1'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '2'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '3'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '4'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '5'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '6'},
};

static menu_t JoystickSetDef =
{
	"M_CONTRO",
	"Select Joystick",
	sizeof (JoystickSetMenu)/sizeof (menuitem_t),
	&JoystickDef,
	JoystickSetMenu,
	M_DrawJoystick,
	50, 40,
	0,
	NULL
};

//===========================================================================
//                          CONTROLS MENU
//===========================================================================
static void M_DrawControl(void);               // added 3-1-98
static void M_ChangeControl(int choice);
static void M_ControlDef2(void);

//
// this is the same for all control pages
//
static menuitem_t ControlMenu[] =
{
	{IT_CALL | IT_STRING2, NULL, "Forward",      M_ChangeControl, gc_forward    },
	{IT_CALL | IT_STRING2, NULL, "Reverse",      M_ChangeControl, gc_backward   },
	{IT_CALL | IT_STRING2, NULL, "Turn Left",    M_ChangeControl, gc_turnleft   },
	{IT_CALL | IT_STRING2, NULL, "Turn Right",   M_ChangeControl, gc_turnright  },
	{IT_CALL | IT_STRING2, NULL, "Jump",         M_ChangeControl, gc_jump       },
	{IT_CALL | IT_STRING2, NULL, "Spin",         M_ChangeControl, gc_use        }, // Tails 12-04-99
	{IT_CALL | IT_STRING2, NULL, "Ring Toss",    M_ChangeControl, gc_fire       },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss Normal",
	                                             M_ChangeControl, gc_firenormal },
	{IT_CALL | IT_STRING2, NULL, "Taunt",        M_ChangeControl, gc_taunt      },
	{IT_CALL | IT_STRING2, NULL, "Light Dash/Toss Flag",   M_ChangeControl, gc_lightdash  },
	{IT_CALL | IT_STRING2, NULL, "Strafe On",    M_ChangeControl, gc_strafe     },
	{IT_CALL | IT_STRING2, NULL, "Strafe Left",  M_ChangeControl, gc_strafeleft },
	{IT_CALL | IT_STRING2, NULL, "Strafe Right", M_ChangeControl, gc_straferight},
	{IT_CALL | IT_STRING2, NULL, "Look Up",      M_ChangeControl, gc_lookup     },
	{IT_CALL | IT_STRING2, NULL, "Look Down",    M_ChangeControl, gc_lookdown   },
	{IT_CALL | IT_STRING2, NULL, "Center View",  M_ChangeControl, gc_centerview },
	{IT_CALL | IT_STRING2, NULL, "Mouselook",    M_ChangeControl, gc_mouseaiming},

	{IT_CALL | IT_WHITESTRING,
	                       NULL, "next",         M_ControlDef2,               144},
};

menu_t ControlDef =
{
	"M_CONTRO",
	"Setup Controls",
	sizeof (ControlMenu)/sizeof (menuitem_t),
	&OptionsDef,
	ControlMenu,
	M_DrawControl,
	24, 40,
	0,
	NULL
};

//
//  Controls page 2
//
// WARNING!: IF YOU MODIFY THIS CHECK "UGLY HACK"
// COMMENTS BELOW TO MAINTAIN CONSISTENCY!!!
//
static menuitem_t ControlMenu2[] =
{
	{IT_CALL | IT_STRING2, NULL, "Talk key",         M_ChangeControl, gc_talkkey   },
	{IT_CALL | IT_STRING2, NULL, "Team-Talk key",    M_ChangeControl, gc_teamkey   },
	{IT_CALL | IT_STRING2, NULL, "Rankings/Scores",  M_ChangeControl, gc_scores    },
	{IT_CALL | IT_STRING2, NULL, "Console",          M_ChangeControl, gc_console   },
	{IT_CALL | IT_STRING2, NULL, "Next Weapon",      M_ChangeControl, gc_weaponnext},
	{IT_CALL | IT_STRING2, NULL, "Prev Weapon",      M_ChangeControl, gc_weaponprev},
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera L",  M_ChangeControl, gc_camleft   },
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera R",  M_ChangeControl, gc_camright  },
	{IT_CALL | IT_STRING2, NULL, "Reset Camera",     M_ChangeControl, gc_camreset  },
	{IT_CALL | IT_STRING2, NULL, "Pause",            M_ChangeControl, gc_pause     },

	{IT_SUBMENU | IT_WHITESTRING,
	                       NULL, "next",             &ControlDef,     140          },
};

menu_t ControlDef2 =
{
	"M_CONTRO",
	"Setup Controls",
	sizeof (ControlMenu2)/sizeof (menuitem_t),
	&OptionsDef,
	ControlMenu2,
	M_DrawControl,
	24, 40,
	0,
	NULL
};


//
// Start the controls menu, setting it up for either the console player,
// or the secondary splitscreen player
//
static  boolean setupcontrols_secondaryplayer;
static  int   (*setupcontrols)[2];  // pointer to the gamecontrols of the player being edited

static void M_ControlDef2(void)
{
	M_SetupNextMenu(&ControlDef2);
}

static void M_DrawJoystick(void)
{
	int i;

	M_DrawGenericMenu();

	for (i = joy0;i < joystickset_end; i++)
	{
		M_DrawSaveLoadBorder(JoystickSetDef.x,JoystickSetDef.y+LINEHEIGHT*i);

		if ((setupcontrols_secondaryplayer && (i == cv_usejoystick2.value))
			|| (!setupcontrols_secondaryplayer && (i == cv_usejoystick.value)))
			V_DrawString(JoystickSetDef.x,JoystickSetDef.y+LINEHEIGHT*i,V_YELLOWMAP,joystickInfo[i]);
		else
			V_DrawString(JoystickSetDef.x,JoystickSetDef.y+LINEHEIGHT*i,0,joystickInfo[i]);
	}
}

static void M_SetupJoystickMenu(int choice)
{
	int i = 0;
	const char *joyname = "None";
	const char *joyNA = "Unavailable";
	int n = I_NumJoys();
	(void)choice;

	strcpy(joystickInfo[i], joyname);

	for (i = joy1; i < joystickset_end; i++)
	{
		if (i <= n && (joyname = I_GetJoyName(i)) != NULL)
		{
			strncpy(joystickInfo[i], joyname, 24);
			joystickInfo[i][24] = '\0';
		}
		else
			strcpy(joystickInfo[i], joyNA);
	}

	M_SetupNextMenu(&JoystickSetDef);
}

static void M_Setup1PJoystickMenu(int choice)
{
	setupcontrols_secondaryplayer = false;
	M_SetupJoystickMenu(choice);
}

static void M_Setup2PJoystickMenu(int choice)
{
	setupcontrols_secondaryplayer = true;
	M_SetupJoystickMenu(choice);
}

static void M_AssignJoystick(int choice)
{
	if (setupcontrols_secondaryplayer)
		CV_SetValue(&cv_usejoystick2, choice);
	else
		CV_SetValue(&cv_usejoystick, choice);
}

static void M_Setup1PControlsMenu(int choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = false;
	setupcontrols = gamecontrol;        // was called from main Options (for console player, then)
	currentMenu->lastOn = itemOn;
	M_SetupNextMenu(&ControlDef);
}

static void M_Setup2PControlsMenu(int choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = true;
	setupcontrols = gamecontrolbis;
	currentMenu->lastOn = itemOn;
	M_SetupNextMenu(&ControlDef);
}

static void M_DrawControlsGenerics(void)
{
	int x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	// UGLY HACK!
	if (setupcontrols_secondaryplayer
		&& currentMenu == &ControlDef2)
	{
		for (i = 0; i < 4; i++)
		{
			if (currentMenu->menuitems[i].alphaKey)
				y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
		}
		if (itemOn < 3)
			itemOn = 3;
	}

	for (i = 0; i < currentMenu->numitems; i++)
	{
		// UGLY HACK!
		if (setupcontrols_secondaryplayer
			&& currentMenu == &ControlDef2
			&& i < 4)
			continue;

		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					V_DrawScaledPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
				}
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, 0, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string), y + 12,
										'_' | 0x80);
								y += 16;
								break;
							default:
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string), y,
									V_YELLOWMAP, cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(currentMenu->x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 22, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}
//
//  Draws the Customise Controls menu
//
static void M_DrawControl(void)
{
	char     tmp[50];
	int      i;
	int      keys[2];

	// draw title, strings and submenu
	M_DrawControlsGenerics();

	M_CentreText (ControlDef.y-12,
		 (setupcontrols_secondaryplayer ? "SET CONTROLS FOR SECONDARY PLAYER" :
		                                  "PRESS ENTER TO CHANGE, BACKSPACE TO CLEAR"));

	for (i = 0;i < currentMenu->numitems;i++)
	{
		if (currentMenu->menuitems[i].status != IT_CONTROL)
			continue;

		if (setupcontrols_secondaryplayer
			&& currentMenu == &ControlDef2
			&& i < 3)
			continue;

		keys[0] = setupcontrols[currentMenu->menuitems[i].alphaKey][0];
		keys[1] = setupcontrols[currentMenu->menuitems[i].alphaKey][1];

		tmp[0] ='\0';
		if (keys[0] == KEY_NULL && keys[1] == KEY_NULL)
		{
			strcpy(tmp, "---");
		}
		else
		{
			if (keys[0] != KEY_NULL)
				strcat (tmp, G_KeynumToString (keys[0]));

			if (keys[0] != KEY_NULL && keys[1] != KEY_NULL)
				strcat(tmp," or ");

			if (keys[1] != KEY_NULL)
				strcat (tmp, G_KeynumToString (keys[1]));


		}
		V_DrawString(BASEVIDWIDTH-ControlDef.x-V_StringWidth(tmp), ControlDef.y + i*8,V_YELLOWMAP, tmp);
	}

}

static int controltochange;

static void M_ChangecontrolResponse(event_t *ev)
{
	int        control;
	int        found;
	int        ch = ev->data1;

	// ESCAPE cancels
	if (ch != KEY_ESCAPE)
	{

		switch (ev->type)
		{
			// ignore mouse/joy movements, just get buttons
			case ev_mouse:
			case ev_mouse2:
			case ev_joystick:
			case ev_joystick2:
				ch = KEY_NULL;      // no key
			break;

			// keypad arrows are converted for the menu in cursor arrows
			// so use the event instead of ch
			case ev_keydown:
				ch = ev->data1;
			break;

			default:
			break;
		}

		control = controltochange;

		// check if we already entered this key
		found = -1;
		if (setupcontrols[control][0] ==ch)
			found = 0;
		else
		 if (setupcontrols[control][1] ==ch)
			found = 1;
		if (found >= 0)
		{
			// replace mouse and joy clicks by double clicks
			if (ch >= KEY_MOUSE1 && ch <= KEY_MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_MOUSE1+KEY_DBLMOUSE1;
			else
			 if (ch >= KEY_JOY1 && ch <= KEY_JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_JOY1+KEY_DBLJOY1;
			else
			 if (ch >= KEY_2MOUSE1 && ch <= KEY_2MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_2MOUSE1+KEY_DBL2MOUSE1;
			else
			 if (ch >= KEY_2JOY1 && ch <= KEY_2JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_2JOY1+KEY_DBL2JOY1;

		}
		else
		{
			// check if change key1 or key2, or replace the two by the new
			found = 0;
			if (setupcontrols[control][0] == KEY_NULL)
				found++;
			if (setupcontrols[control][1] == KEY_NULL)
				found++;
			if (found == 2)
			{
				found = 0;
				setupcontrols[control][1] = KEY_NULL;  //replace key 1,clear key2
			}
			G_CheckDoubleUsage(ch);
			setupcontrols[control][found] = ch;
		}

	}

	M_StopMessage(0);
}

static void M_ChangeControl(int choice)
{
	static char tmp[55];

	controltochange = currentMenu->menuitems[choice].alphaKey;
	sprintf(tmp, "Hit the new key for\n%s\nESC for Cancel",
		currentMenu->menuitems[choice].text);

	M_StartMessage(tmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
}

//===========================================================================
//                        VIDEO MODE MENU
//===========================================================================
static void M_DrawVideoMode(void);             //added : 30-01-98:

static void M_HandleVideoMode(int ch);

static menuitem_t VideoModeMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleVideoMode, '\0'},     // dummy menuitem for the control func
};

menu_t VidModeDef =
{
	"M_VIDEO",
	"Video Mode",
	1,                  // # of menu items
	//sizeof (VideoModeMenu)/sizeof (menuitem_t),
	&VideoOptionsDef,   // previous menu
	VideoModeMenu,      // menuitem_t ->
	M_DrawVideoMode,    // drawing routine ->
	48, 36,             // x,y
	0,                  // lastOn
	NULL
};

//added : 30-01-98:
#define MAXCOLUMNMODES   10     //max modes displayed in one column
#define MAXMODEDESCS     (MAXCOLUMNMODES*3)

// shhh... what am I doing... nooooo!
static int vidm_testingmode = 0;
static int vidm_previousmode;
static int vidm_current = 0;
static int vidm_nummodes;
static int vidm_column_size;

typedef struct
{
	int modenum; // video mode number in the vidmodes list
	const char *desc;  // XXXxYYY
	int iscur;   // 1 if it is the current active mode
} modedesc_t;

static modedesc_t modedescs[MAXMODEDESCS];

//
// Draw the video modes list, a-la-Quake
//
static void M_DrawVideoMode(void)
{
	int i, j, vdup, row, col, nummodes;
	const char *desc;
	char temp[80];

	// draw title
	M_DrawMenuTitle();

#if defined (UNIXLIKE) || defined (SDL)
	VID_PrepareModeList(); // FIXME: hack
#endif
	vidm_nummodes = 0;
	nummodes = VID_NumModes();

#ifdef _WINDOWS
	// clean that later: skip windowed mode 0, video modes menu only shows FULL SCREEN modes
	if (nummodes < 1)
	{
		// put the windowed mode so that there is at least one mode
		modedescs[0].modenum = 0;
		modedescs[0].desc = VID_GetModeName(0);
		modedescs[0].iscur = 1;
		vidm_nummodes = 1;
	}
	for (i = 1; i <= nummodes && vidm_nummodes < MAXMODEDESCS; i++)
#else
	// DOS does not skip mode 0, because mode 0 is ALWAYS present
	for (i = 0; i < nummodes && vidm_nummodes < MAXMODEDESCS; i++)
#endif
	{
		desc = VID_GetModeName(i);
		if (desc)
		{
			vdup = 0;

			// when a resolution exists both under VGA and VESA, keep the
			// VESA mode, which is always a higher modenum
			for (j = 0; j < vidm_nummodes; j++)
			{
				if (!strcmp(modedescs[j].desc, desc))
				{
					// mode(0): 320x200 is always standard VGA, not vesa
					if (modedescs[j].modenum)
					{
						modedescs[j].modenum = i;
						vdup = 1;

						if (i == vid.modenum)
							modedescs[j].iscur = 1;
					}
					else
						vdup = 1;

					break;
				}
			}

			if (!vdup)
			{
				modedescs[vidm_nummodes].modenum = i;
				modedescs[vidm_nummodes].desc = desc;
				modedescs[vidm_nummodes].iscur = 0;

				if (i == vid.modenum)
					modedescs[vidm_nummodes].iscur = 1;

				vidm_nummodes++;
			}
		}
	}

	vidm_column_size = (vidm_nummodes+2) / 3;

	row = 41;
	col = VidModeDef.y;
	for (i = 0; i < vidm_nummodes; i++)
	{
		V_DrawString(row, col, modedescs[i].iscur ? V_YELLOWMAP : 0, modedescs[i].desc);

		col += 8;
		if ((i % vidm_column_size) == (vidm_column_size-1))
		{
			row += 7*13;
			col = 36;
		}
	}

	if (vidm_testingmode > 0)
	{
		sprintf(temp, "TESTING MODE %s", modedescs[vidm_current].desc);
		M_CentreText(VidModeDef.y + 80 + 24, temp);
		M_CentreText(VidModeDef.y + 90 + 24, "Please wait 5 seconds...");
	}
	else
	{
		M_CentreText(VidModeDef.y + 60 + 24, "Press ENTER to set mode");
		M_CentreText(VidModeDef.y + 70 + 24, "T to test mode for 5 seconds");

		sprintf(temp, "D to make %s the default", VID_GetModeName(vid.modenum));
		M_CentreText(VidModeDef.y + 80 + 24,temp);

		sprintf(temp, "Current default is %dx%d (%d bits)", cv_scr_width.value,
			cv_scr_height.value, cv_scr_depth.value);
		M_CentreText(VidModeDef.y + 90 + 24,temp);

		M_CentreText(VidModeDef.y + 100 + 24,"Press ESC to exit");
	}

	// Draw the cursor for the VidMode menu
	if (skullAnimCounter < 4) // use the Skull anim counter to blink the cursor
	{
		i = 41 - 10 + ((vidm_current / vidm_column_size)*7*13);
		j = VidModeDef.y + ((vidm_current % vidm_column_size)*8);
		V_DrawCharacter(i - 8, j, '*');
	}
}

// special menuitem key handler for video mode list
static void M_HandleVideoMode(int ch)
{
	if (vidm_testingmode > 0)
	{
		// change back to the previous mode quickly
		if (ch == KEY_ESCAPE)
		{
			setmodeneeded = vidm_previousmode + 1;
			vidm_testingmode = 0;
		}
		return;
	}

	switch (ch)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current++;
			if (vidm_current >= vidm_nummodes)
				vidm_current = 0;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current--;
			if (vidm_current < 0)
				vidm_current = vidm_nummodes - 1;
			break;

		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current -= vidm_column_size;
			if (vidm_current < 0)
				vidm_current = (vidm_column_size*3) + vidm_current;
			if (vidm_current >= vidm_nummodes)
				vidm_current = vidm_nummodes - 1;
			break;

		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current += vidm_column_size;
			if (vidm_current >= (vidm_column_size*3))
				vidm_current %= vidm_column_size;
			if (vidm_current >= vidm_nummodes)
				vidm_current = vidm_nummodes - 1;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			if (!setmodeneeded) // in case the previous setmode was not finished
				setmodeneeded = modedescs[vidm_current].modenum + 1;
			break;

		case KEY_ESCAPE: // this one same as M_Responder
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu);
			else
				M_ClearMenus(true);
			break;

		case 'T':
		case 't':
			vidm_testingmode = TICRATE*5;
			vidm_previousmode = vid.modenum;
			if (!setmodeneeded) // in case the previous setmode was not finished
				setmodeneeded = modedescs[vidm_current].modenum + 1;
			break;

		case 'D':
		case 'd':
			// current active mode becomes the default mode.
			SCR_SetDefaultMode();
			break;

		default:
			break;
	}
}

//===========================================================================
//LOAD GAME MENU
//===========================================================================
static void M_DrawLoad(void);

static void M_LoadSelect(int choice);

typedef enum
{
	load1,
	load2,
	load3,
	load4,
	load5,
	load6,
	load_end
} load_e;

static menuitem_t LoadGameMenu[] =
{
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '1'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '2'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '3'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '4'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '5'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '6'},
};

menu_t LoadDef =
{
	"M_PICKG",
	"Load Game",
	load_end,
	&SinglePlayerDef,
	LoadGameMenu,
	M_DrawLoad,
	80, 54,
	0,
	NULL
};

static void M_DrawGameStats(void)
{
	int ecks;
	saveSlotSelected = itemOn;

	ecks = LoadDef.x + 24;
	M_DrawTextBox(LoadDef.x-8,144, 23, 4);

	if (savegameinfo[saveSlotSelected].lives == -42) // Empty
	{
		V_DrawString(ecks + 16, 152, 0, "EMPTY");
		return;
	}

	if (savegameinfo[saveSlotSelected].skincolor == 0)
		V_DrawScaledPatch ((int)((LoadDef.x)*vid.fdupx),(int)((144+8)*vid.fdupy), V_NOSCALESTART,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].faceprefix, PU_CACHE));
	else
	{
		byte *colormap = (byte *) translationtables[savegameinfo[saveSlotSelected].skinnum] - 256 + (savegameinfo[saveSlotSelected].skincolor<<8);
		V_DrawMappedPatch ((int)((LoadDef.x)*vid.fdupx),(int)((144+8)*vid.fdupy), V_NOSCALESTART,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].faceprefix, PU_CACHE), colormap);
	}

	V_DrawString(ecks + 16, 152, 0, savegameinfo[saveSlotSelected].playername);

	if (savegameinfo[saveSlotSelected].actnum == 0)
		V_DrawString(ecks + 16, 160, 0, va("%s", savegameinfo[saveSlotSelected].levelname));
	else
		V_DrawString(ecks + 16, 160, 0, va("%s %d", savegameinfo[saveSlotSelected].levelname, savegameinfo[saveSlotSelected].actnum));

	V_DrawScaledPatch(ecks + 16, 168, 0, W_CachePatchName("CHAOS1", PU_CACHE));
	V_DrawString(ecks + 36, 172, 0, va("x %d", savegameinfo[saveSlotSelected].numemeralds));

	V_DrawScaledPatch(ecks + 64, 169, 0, W_CachePatchName("ONEUP", PU_CACHE));
	V_DrawString(ecks + 84, 172, 0, va("x %d", savegameinfo[saveSlotSelected].lives));

	V_DrawScaledPatch(ecks + 120, 168, 0, W_CachePatchName("CONTINS", PU_CACHE));
	V_DrawString(ecks + 140, 172, 0, va("x %d", savegameinfo[saveSlotSelected].continues));
}

//
// M_LoadGame & Cie.
//
static void M_DrawLoad(void)
{
	int i;

	M_DrawGenericMenu();

	V_DrawCenteredString(BASEVIDWIDTH/2, 40, 0, "Hit backspace to delete a save.");

	for (i = 0;i < load_end; i++)
	{
		M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
		V_DrawString(LoadDef.x,LoadDef.y+LINEHEIGHT*i,0,va("Save Slot %d", i+1));
	}
	M_DrawGameStats();
}

//
// User wants to load this game
//
static void M_LoadSelect(int choice)
{
	if (netgame && Playing())
	{
		M_StartMessage(NEWGAME,M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (!FIL_ReadFileOK(va(savegamename, choice)))
	{
		// This slot is empty, so start a new game here.
		M_NewGame();
	}
	else
	{
		G_LoadGame((unsigned int)choice);
		M_ClearMenus(true);
	}

	cursaveslot = choice;
}

#define VERSIONSIZE             16
// Reads the save file to list lives, level, player, etc.
// Tails 05-29-2003
static void M_ReadSavegameInfo(unsigned int slot)
{
#define BADSAVE I_Error("Bad savegame in slot %u", slot);
#define CHECKPOS if (save_p >= end_p) BADSAVE
	size_t length;
	char savename[255];
	byte *savebuffer;
	byte *end_p; // buffer end point, don't read past here
	byte *save_p;
	int fake; // Dummy variable

	sprintf(savename, savegamename, slot);

	length = FIL_ReadFile(savename, &savebuffer);
	if (length == 0)
	{
		CONS_Printf(text[HUSTR_MSGU_NUM], savename);
		savegameinfo[slot].lives = -42;
		return;
	}

	end_p = savebuffer + length;

	// skip the description field
	save_p = savebuffer;

	save_p += VERSIONSIZE;

	// dearchive all the modifications
	// P_UnArchiveMisc()

	CHECKPOS
	(void)READBYTE(save_p);

	CHECKPOS
	fake = READSHORT(save_p);
	if (fake-1 >= NUMMAPS) BADSAVE
	strcpy(savegameinfo[slot].levelname, mapheaderinfo[fake-1].lvlttl);

	savegameinfo[slot].actnum = mapheaderinfo[fake-1].actnum;

	CHECKPOS
	fake = READUSHORT(save_p)-357; // emeralds

	savegameinfo[slot].numemeralds = 0;

	if (fake & EMERALD1)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD2)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD3)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD4)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD5)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD6)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD7)
		savegameinfo[slot].numemeralds++;

	CHECKPOS
	(void)READBYTE(save_p); // modifiedgame

	// P_UnArchivePlayer()
	CHECKPOS
	savegameinfo[slot].skincolor = READBYTE(save_p);
	CHECKPOS
	savegameinfo[slot].skinnum = READBYTE(save_p);
	strcpy(savegameinfo[slot].playername,
		skins[savegameinfo[slot].skinnum].name);

	CHECKPOS
	(void)READLONG(save_p); // Score

	CHECKPOS
	savegameinfo[slot].lives = READLONG(save_p); // lives
	CHECKPOS
	savegameinfo[slot].continues = READLONG(save_p); // continues

	// done
	Z_Free(savebuffer);
#undef CHECKPOS
#undef BADSAVE
}

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//  and put it in savegamestrings global variable
//
static void M_ReadSaveStrings(void)
{
	FILE *handle;
	unsigned int i;
	char name[256];

	for (i = 0; i < load_end; i++)
	{
		snprintf(name, sizeof name, savegamename, i);

		handle = fopen(name, "rb");
		if (handle == NULL)
		{
			LoadGameMenu[i].status = 0;
			savegameinfo[i].lives = -42;
			continue;
		}
		fclose(handle);
		LoadGameMenu[i].status = 1;
		M_ReadSavegameInfo(i);
	}
}

static int curSaveSelected;

//
// User wants to delete this game
//
static void M_SaveGameDeleteResponse(int ch)
{
	char name[256];

	if (ch != 'y')
		return;

	// delete savegame
	snprintf(name, sizeof name, savegamename, curSaveSelected);
	remove(name);

	// Refresh savegame menu info
	M_ReadSaveStrings();
}

//
// Selected from DOOM menu
//
static void M_LoadGame(int choice)
{
	(void)choice;
	// change can't load message to can't load in server mode
	if (netgame && !server)
	{
		M_StartMessage(LOADNET,NULL,MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&LoadDef);
	M_ReadSaveStrings();
}

//
// Draw border for the savegame description
//
static void M_DrawSaveLoadBorder(int x,int y)
{
	int i;

	V_DrawScaledPatch (x-8,y+7,0,W_CachePatchName("M_LSLEFT",PU_CACHE));

	for (i = 0;i < 24;i++)
	{
		V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSCNTR",PU_CACHE));
		x += 8;
	}

	V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSRGHT",PU_CACHE));
}

//===========================================================================
//                                 END GAME
//===========================================================================

//
// M_EndGame
//
static void M_EndGameResponse(int ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	currentMenu->lastOn = itemOn;
	M_ClearMenus(true);
	//Command_ExitGame_f();
	G_SetExitGameFlag();
}

void M_EndGame(int choice)
{
	(void)choice;
	if (demoplayback || demorecording)
		return;

	M_StartMessage(ENDGAME,M_EndGameResponse,MM_YESNO);
}

//===========================================================================
//                                 Quit Game
//===========================================================================

//
// M_QuitSRB2
//
static int quitsounds2[8] =
{
	sfx_spring, // Tails 11-09-99
	sfx_itemup, // Tails 11-09-99
	sfx_jump, // Tails 11-09-99
	sfx_pop,
	sfx_gloop, // Tails 11-09-99
	sfx_splash, // Tails 11-09-99
	sfx_floush, // Tails 11-09-99
	sfx_chchng // Tails 11-09-99
};

void M_ExitGameResponse(int ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	//Command_ExitGame_f();
	G_SetExitGameFlag();
}

void M_QuitResponse(int ch)
{
	tic_t time;
	if (ch != 'y' && ch != KEY_ENTER)
		return;
	if (!(netgame || cv_debug))
	{
		if (quitsounds2[(gametic>>2)&7]) S_StartSound(NULL, quitsounds2[(gametic>>2)&7]); // Use quitsounds2, not quitsounds Tails 11-09-99

		//added : 12-02-98: do that instead of I_WaitVbl which does not work
		time = I_GetTime() + TICRATE*3; // Shortened the quit time, used to be 2 seconds Tails 03-26-2001
		while (time > I_GetTime())
		{
			V_DrawScaledPatch(0, 0, 0, W_CachePatchName("GAMEQUIT", PU_CACHE)); // Demo 3 Quit Screen Tails 06-16-2001
			I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
			I_Sleep();
		}
	}
	I_Quit();
}

static void M_QuitSRB2(int choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	static char s[200];
	(void)choice;
	sprintf(s, text[DOSY_NUM], text[QUITMSG_NUM + (gametic % NUM_QUITMESSAGES)]);
	M_StartMessage(s, M_QuitResponse, MM_YESNO);
}

//===========================================================================
//                              Some Draw routine
//===========================================================================

//
// Menu Functions
//
static void M_DrawThermo(int x, int y, consvar_t *cv)
{
	int xx = x, i;
	lumpnum_t leftlump, rightlump, centerlump[2], cursorlump;
	patch_t *p;

	leftlump = W_GetNumForName("M_THERML");
	rightlump = W_GetNumForName("M_THERMR");
	centerlump[0] = W_GetNumForName("M_THERMM");
	centerlump[1] = W_GetNumForName("M_THERMM");
	cursorlump = W_GetNumForName("M_THERMO");

	V_DrawScaledPatch(xx, y, 0, p = W_CachePatchNum(leftlump,PU_CACHE));
	xx += SHORT(p->width) - SHORT(p->leftoffset);
	for (i = 0; i < 16; i++)
	{
		V_DrawScaledPatch(xx, y, V_WRAPX, W_CachePatchNum(centerlump[i & 1], PU_CACHE));
		xx += 8;
	}
	V_DrawScaledPatch(xx, y, 0, W_CachePatchNum(rightlump, PU_CACHE));

	xx = (cv->value - cv->PossibleValue[0].value) * (15*8) /
		(cv->PossibleValue[1].value - cv->PossibleValue[0].value);

	V_DrawScaledPatch((x + 8) + xx, y, 0, W_CachePatchNum(cursorlump, PU_CACHE));
}

//
//  Draw a textbox, like Quake does, because sometimes it's difficult
//  to read the text with all the stuff in the background...
//
//added : 06-02-98:
void M_DrawTextBox(int x, int y, int width, int boxlines)
{
	patch_t *p;
	int cx = x, cy = y, n;
	int step = 8, boff = 8;

	// draw left side
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TL], PU_CACHE));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_L], PU_CACHE);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BL], PU_CACHE));

	// draw middle
	V_DrawFlatFill(x + boff, y + boff, width*step, boxlines*step, st_borderpatchnum);

	cx += boff;
	cy = y;
	while (width > 0)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_T], PU_CACHE));
		V_DrawScaledPatch(cx, y + boff + boxlines*step, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_B], PU_CACHE));
		width--;
		cx += step;
	}

	// draw right side
	cy = y;
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TR], PU_CACHE));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_R], PU_CACHE);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BR], PU_CACHE));
}

//==========================================================================
//                        Message is now a (hackable) Menu
//==========================================================================
static void M_DrawMessageMenu(void);

static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0,NULL, NULL, NULL,0}
};

menu_t MessageDef =
{
	NULL,               // title
	NULL,
	1,                  // # of menu items
	NULL,               // previous menu       (TO HACK)
	MessageMenu,        // menuitem_t ->
	M_DrawMessageMenu,  // drawing routine ->
	0, 0,               // x, y                (TO HACK)
	0,                  // lastOn, flags       (TO HACK)
	NULL
};


void M_StartMessage(const char *string, void *routine,
	menumessagetype_t itemtype)
{
	int max = 0, start = 0, i, strlines;
	static char *message = NULL;
	Z_Free(message);
	message = Z_StrDup(string);
	DEBFILE(message);

	M_StartControlPanel(); // can't put menuactive to true
	MessageDef.prevMenu = currentMenu;
	MessageDef.menuitems[0].text     = message;
	MessageDef.menuitems[0].alphaKey = (byte)itemtype;
	if (!routine && itemtype != MM_NOTHING) itemtype = MM_NOTHING;
	switch (itemtype)
	{
		case MM_NOTHING:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = M_StopMessage;
			break;
		case MM_YESNO:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
		case MM_EVENTHANDLER:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
	}
	//added : 06-02-98: now draw a textbox around the message
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0;i < (int)strlen(message+start);i++)
		{
			if (*(message+start+i) == '\n')
			{
				if (i > max)
					max = i;
				start += i+1;
				i = -1; //added : 07-02-98 : damned!
				break;
			}
		}

		if (i == (int)strlen(message+start))
			start += i;
	}

	MessageDef.x = (short)((BASEVIDWIDTH  - 8*max-16)/2);
	MessageDef.y = (short)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	MessageDef.lastOn = (short)((strlines<<8)+max);

	//M_SetupNextMenu();
	currentMenu = &MessageDef;
	itemOn = 0;
}

#define MAXMSGLINELEN 256

static void M_DrawMessageMenu(void)
{
	int y = currentMenu->y;
	short i,max;
	char string[MAXMSGLINELEN];
	int start = 0, mlines;
	const char *msg = currentMenu->menuitems[0].text;

	mlines = currentMenu->lastOn>>8;
	max = (short)((byte)(currentMenu->lastOn & 0xFF)*8);
	M_DrawTextBox(currentMenu->x, y - 8, (max+7)>>3, mlines);

	while (*(msg+start))
	{
		for (i = 0; i < (int)strlen(msg+start); i++)
		{
			if (*(msg+start+i) == '\n')
			{
				memset(string, 0, MAXMSGLINELEN);
				if (i >= MAXMSGLINELEN)
				{
					CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
					return;
				}
				else
				{
					strncpy(string,msg+start, i);
					start += i+1;
					i = -1; //added : 07-02-98 : damned!
				}

				break;
			}
		}

		if (i == (int)strlen(msg+start))
		{
			if (i >= MAXMSGLINELEN)
			{
				CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
				return;
			}
			else
			{
				strcpy(string, msg + start);
				start += i;
			}
		}

		V_DrawString((BASEVIDWIDTH - V_StringWidth(string))/2,y,0,string);
		y += 8; //SHORT(hu_font[0]->height);
	}
}

// default message handler
static void M_StopMessage(int choice)
{
	(void)choice;
	M_SetupNextMenu(MessageDef.prevMenu);
}

//==========================================================================
//                        Menu stuffs
//==========================================================================

//added : 30-01-98:
//
//  Write a string centered using the hu_font
//
static void M_CentreText(int y, const char *string)
{
	int x;
	//added : 02-02-98 : centre on 320, because V_DrawString centers on vid.width...
	x = (BASEVIDWIDTH - V_StringWidth(string))>>1;
	V_DrawString(x,y,0,string);
}


//
// CONTROL PANEL
//

static void M_ChangeCvar(int choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

	if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
	 ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD))
	{
		CV_SetValue(cv,cv->value+choice*2-1);
	}
	else
	 if (cv->flags & CV_FLOAT)
	{
		char s[20];
		sprintf(s,"%f",FIXED_TO_FLOAT(cv->value)+(choice*2-1)*(1.0f/16.0f));
		CV_Set(cv,s);
	}
	else
		CV_AddValue(cv,choice*2-1);
}

static boolean M_ChangeStringCvar(int choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
	char buf[255];
	size_t len;

	switch (choice)
	{
		case KEY_BACKSPACE:
			len = strlen(cv->string);
			if (len > 0)
			{
				memcpy(buf, cv->string, len);
				buf[len-1] = 0;
				CV_Set(cv, buf);
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(cv->string);
				if (len < MAXSTRINGLENGTH - 1)
				{
					memcpy(buf, cv->string, len);
					buf[len++] = (char)choice;
					buf[len] = 0;
					CV_Set(cv, buf);
				}
				return true;
			}
			break;
	}
	return false;
}

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	int ch = -1;
//	int i;
	static tic_t joywait = 0, mousewait = 0;
	static boolean shiftdown = false;
	static int pmousex = 0, pmousey = 0;
	static int lastx = 0, lasty = 0;
	void (*routine)(int choice); // for some casting problem

	if (dedicated || gamestate == GS_INTRO || gamestate == GS_INTRO2 || gamestate == GS_CUTSCENE)
		return false;

	if (ev->type == ev_keyup && (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT))
	{
		shiftdown = false;
		return false;
	}
	else if (ev->type == ev_keydown)
	{
		ch = ev->data1;

		// added 5-2-98 remap virtual keys (mouse & joystick buttons)
		switch (ch)
		{
			case KEY_LSHIFT:
			case KEY_RSHIFT:
				shiftdown = true;
				break; //return false;
			case KEY_MOUSE1:
			case KEY_JOY1:
			case KEY_JOY1 + 2:
				ch = KEY_ENTER;
				break;
			case KEY_JOY1 + 3:
				ch = 'n';
				break;
			case KEY_MOUSE1 + 1:
			case KEY_JOY1 + 1:
				ch = KEY_BACKSPACE;
				break;
			case KEY_HAT1:
				ch = KEY_UPARROW;
				break;
			case KEY_HAT1 + 1:
				ch = KEY_DOWNARROW;
				break;
			case KEY_HAT1 + 2:
				ch = KEY_LEFTARROW;
				break;
			case KEY_HAT1 + 3:
				ch = KEY_RIGHTARROW;
				break;
		}
	}
	else if (menuactive)
	{
		if (ev->type == ev_joystick  && ev->data1 == 0 && joywait < I_GetTime())
		{
			if (ev->data3 == -1)
			{
				ch = KEY_UPARROW;
				joywait = I_GetTime() + TICRATE/7;
			}
			else if (ev->data3 == 1)
			{
				ch = KEY_DOWNARROW;
				joywait = I_GetTime() + TICRATE/7;
			}

			if (ev->data2 == -1)
			{
				ch = KEY_LEFTARROW;
				joywait = I_GetTime() + TICRATE/17;
			}
			else if (ev->data2 == 1)
			{
				ch = KEY_RIGHTARROW;
				joywait = I_GetTime() + TICRATE/17;
			}
		}
		else if (ev->type == ev_mouse && mousewait < I_GetTime())
		{
			pmousey += ev->data3;
			if (pmousey < lasty-30)
			{
				ch = KEY_DOWNARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousey = lasty -= 30;
			}
			else if (pmousey > lasty + 30)
			{
				ch = KEY_UPARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousey = lasty += 30;
			}

			pmousex += ev->data2;
			if (pmousex < lastx - 30)
			{
				ch = KEY_LEFTARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousex = lastx -= 30;
			}
			else if (pmousex > lastx+30)
			{
				ch = KEY_RIGHTARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousex = lastx += 30;
			}
		}
	}

	if (ch == -1)
		return false;

	// F-Keys
	if (!menuactive)
	{
		switch (ch)
		{
			case KEY_F1: // Help key
				M_StartControlPanel();
				currentMenu = &ReadDef1;
				itemOn = 0;
				S_StartSound(NULL, sfx_swtchn);
				return true;

			case KEY_F2: // Empty
				return true;

			case KEY_F3: // Empty
				return true;

			case KEY_F4: // Sound Volume
				M_StartControlPanel();
				currentMenu = &SoundDef;
				itemOn = sfx_vol;
				return true;

#ifndef DC
			case KEY_F5: // Video Mode
				M_StartControlPanel();
				M_SetupNextMenu(&VidModeDef);
				return true;
#endif

			case KEY_F6: // Empty
				return true;

			case KEY_F7: // Options
				M_StartControlPanel();
				M_OptionsMenu(0);
				return true;

			case KEY_F8: // Screenshot
				COM_BufAddText("screenshot\n");
				return true;

			case KEY_F9: // Empty
				return true;

			case KEY_F10: // Quit SRB2
				M_QuitSRB2(0);
				return true;

			case KEY_F11: // Gamma Level
				CV_AddValue(&cv_usegamma, 1);
				return true;

			case KEY_ESCAPE: // Pop up menu
				if (chat_on)
				{
					HU_clearChatChars();
					chat_on = false;
				}
				else
					M_StartControlPanel();
				return true;
		}
		return false;
	}

	routine = currentMenu->menuitems[itemOn].itemaction;

	// Handle menuitems which need a specific key handling
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER)
	{
		if (shiftdown && ch >= 32 && ch <= 127)
			ch = shiftxform[ch];
		routine(ch);
		return true;
	}

	if (currentMenu->menuitems[itemOn].status == IT_MSGHANDLER)
	{
		if (currentMenu->menuitems[itemOn].alphaKey != MM_EVENTHANDLER)
		{
			if (ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE || ch == KEY_ENTER)
			{
				if (routine)
					routine(ch);
				M_StopMessage(0);
				return true;
			}
			return true;
		}
		else
		{
			// dirty hack: for customising controls, I want only buttons/keys, not moves
			if (ev->type == ev_mouse || ev->type == ev_mouse2 || ev->type == ev_joystick
				|| ev->type == ev_joystick2)
				return true;
			if (routine)
			{
				void (*otherroutine)(event_t *ev) = currentMenu->menuitems[itemOn].itemaction;
				otherroutine(ev); //Alam: what a hack
			}
			return true;
		}
	}

	// BP: one of the more big hack i have never made
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	{
		if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING)
		{
			if (shiftdown && ch >= 32 && ch <= 127)
				ch = shiftxform[ch];
			if (M_ChangeStringCvar(ch))
				return true;
			else
				routine = NULL;
		}
		else
			routine = M_ChangeCvar;
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEY_DOWNARROW:
			do
			{
				if (itemOn + 1 > currentMenu->numitems - 1)
					itemOn = 0;
				else
					itemOn++;
			} while ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

			S_StartSound(NULL, sfx_menu1);
			return true;

		case KEY_UPARROW:
			do
			{
				if (!itemOn)
					itemOn = (short)(currentMenu->numitems - 1);
				else
					itemOn--;
			} while ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

			S_StartSound(NULL, sfx_menu1);
			return true;

		case KEY_LEFTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				S_StartSound(NULL, sfx_menu1);
				routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				S_StartSound(NULL, sfx_menu1);
				routine(1);
			}
			return true;

		case KEY_ENTER:
			currentMenu->lastOn = itemOn;
			if (routine)
			{
				switch (currentMenu->menuitems[itemOn].status & IT_TYPE)
				{
					case IT_CVAR:
					case IT_ARROWS:
						routine(1); // right arrow
						S_StartSound(NULL, sfx_menu1);
						break;
					case IT_CALL:
						routine(itemOn);
						S_StartSound(NULL, sfx_menu1);
						break;
					case IT_SUBMENU:
						currentMenu->lastOn = itemOn;
						M_SetupNextMenu((menu_t *)currentMenu->menuitems[itemOn].itemaction);
						S_StartSound(NULL, sfx_menu1);
						break;
				}
			}
			return true;

		case KEY_ESCAPE:
			currentMenu->lastOn = itemOn;
			if (currentMenu->prevMenu)
			{
				if (currentMenu == &TimeAttackDef)
				{
					// Fade to black first
					if (rendermode == render_soft)
					{
						tic_t nowtime, tics, wipestart, y;
						boolean done;
						V_DrawFill(0, 0, vid.width, vid.height, 31);
						F_WipeEndScreen(0, 0, vid.width, vid.height);

						wipestart = I_GetTime() - 1;
						y = wipestart + 2*TICRATE; // init a timeout
						do
						{
							do
							{
								nowtime = I_GetTime();
								tics = nowtime - wipestart;
								if (!tics) I_Sleep();
							} while (!tics);
							wipestart = nowtime;
							done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
							I_OsPolling();
							I_UpdateNoBlit();
							I_FinishUpdate(); // page flip or blit buffer
						} while (!done && I_GetTime() < y);
					}
					menuactive = false;
					D_StartTitle();
				}
				else
				{
					currentMenu = currentMenu->prevMenu;
					itemOn = currentMenu->lastOn;
				}
			}
			else
			{
				M_ClearMenus(true);
				S_StartSound(NULL,sfx_swtchx);
			}
			return true;

		case KEY_BACKSPACE:
			if ((currentMenu->menuitems[itemOn].status) == IT_CONTROL)
			{
				S_StartSound(NULL, sfx_stnmov);
				// detach any keys associated with the game control
				G_ClearControlKeys(setupcontrols, currentMenu->menuitems[itemOn].alphaKey);
				return true;
			}
			else if (currentMenu == &LoadDef)
			{
				curSaveSelected = itemOn; // Eww eww!
				M_StartMessage("Are you sure you want to delete\nthis save game?\n(Y/N)\n",M_SaveGameDeleteResponse,MM_YESNO);
				return true;
			}
			currentMenu->lastOn = itemOn;
			if (currentMenu->prevMenu)
			{
				currentMenu = currentMenu->prevMenu;
				itemOn = currentMenu->lastOn;
			}
			return true;

		default:
/*			for (i = itemOn + 1; i < currentMenu->numitems; i++)
				if (currentMenu->menuitems[i].alphaKey == ch && !(currentMenu->menuitems[i].status & IT_DISABLED))
				{
					itemOn = (short)i;
					S_StartSound(NULL, sfx_menu1);
					return true;
				}
			for (i = 0; i <= itemOn; i++)
				if (currentMenu->menuitems[i].alphaKey == ch && !(currentMenu->menuitems[i].status & IT_DISABLED))
				{
					itemOn = (short)i;
					S_StartSound(NULL, sfx_menu1);
					return true;
				}*/
			break;
	}

	return true;
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
	if (currentMenu == &MessageDef)
		menuactive = true;

	if (!menuactive)
		return;

	// now that's more readable with a faded background (yeah like Quake...)
	if (!WipeInAction)
		V_DrawFadeScreen();

	if (currentMenu->drawroutine)
		currentMenu->drawroutine(); // call current menu Draw routine
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	// intro might call this repeatedly
	if (menuactive)
	{
		CON_ToggleOff(); // move away console
		return;
	}

	menuactive = true;

	if (!(netgame || multiplayer) || !Playing()
		|| !(server || adminplayer == consoleplayer))
	{
		MainMenu[switchmap].status = IT_DISABLED;
	}
	else
		MainMenu[switchmap].status = IT_STRING | IT_CALL;

	MainMenu[secrets].status = IT_DISABLED;

	// Check for the ??? menu
	if (grade > 0)
		MainMenu[secrets].status = IT_STRING | IT_CALL;

	if (savemoddata)
		MainMenu[secrets].itemaction = M_CustomSecretsMenu;
	else
		MainMenu[secrets].itemaction = M_SecretsMenu;

	currentMenu = &MainDef;
	itemOn = singleplr;

	CON_ToggleOff(); // move away console

	if (timeattacking) // Cancel recording
	{
		G_CheckDemoStatus();

		if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
			Command_ExitGame_f();

		currentMenu = &TimeAttackDef;
		itemOn = currentMenu->lastOn;
		timeattacking = false;
		G_SetGamestate(GS_TIMEATTACK);
		S_ChangeMusic(mus_racent, true);
		CV_AddValue(&cv_nextmap, 1);
		CV_AddValue(&cv_nextmap, -1);
		return;
	}
}

//
// M_ClearMenus
//
void M_ClearMenus(boolean callexitmenufunc)
{
	if (!menuactive)
		return;

	if (currentMenu->quitroutine && callexitmenufunc && !currentMenu->quitroutine())
		return; // we can't quit this menu (also used to set parameter from the menu)

#ifndef DC // Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));
#endif //Alam: But not on the Dreamcast's VMUs

	if (currentMenu != &MessageDef)
		menuactive = false;
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
	int i;

	if (currentMenu->quitroutine)
	{
		if (!currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
	}
	currentMenu = menudef;
	itemOn = currentMenu->lastOn;

	// in case of...
	if (itemOn >= currentMenu->numitems)
		itemOn = (short)(currentMenu->numitems - 1);

	// the curent item can be desabled,
	// this code go up until a enabled item found
	if (currentMenu->menuitems[itemOn].status == IT_DISABLED)
	{
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if ((currentMenu->menuitems[i].status != IT_DISABLED))
			{
				itemOn = (short)i;
				break;
			}
		}
	}
}

//
// M_Ticker
//
void M_Ticker(void)
{
	if (dedicated)
		return;

	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8 * NEWTICRATERATIO;

	//added : 30-01-98 : test mode for five seconds
	if (vidm_testingmode > 0)
	{
		// restore the previous video mode
		if (--vidm_testingmode == 0)
			setmodeneeded = vidm_previousmode + 1;
	}
}

//
// M_Init
//
void M_Init(void)
{
	CV_RegisterVar(&cv_skill);
	CV_RegisterVar(&cv_nextmap);
	CV_RegisterVar(&cv_newgametype);
	CV_RegisterVar(&cv_chooseskin);

	if (dedicated)
		return;

	// This is used because DOOM 2 had only one HELP
	//  page. I use CREDIT as second page now, but
	//  kept this hack for educational purposes.
	ReadMenu1[0].itemaction = &MainDef;

	CV_RegisterVar(&cv_serversearch);
}

//======================================================================
// OpenGL specific options
//======================================================================

#ifdef HWRENDER

static void M_DrawOpenGLMenu(void);
static void M_OGL_DrawFogMenu(void);
static void M_OGL_DrawColorMenu(void);
static void M_HandleFogColor(int choice);

static menuitem_t OpenGLOptionsMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Field of view",   &cv_grfov,            10},
	{IT_STRING|IT_CVAR,         NULL, "Quality",         &cv_scr_depth,        20},
	{IT_STRING|IT_CVAR,         NULL, "Texture Filter",  &cv_grfiltermode,     30},
	{IT_STRING|IT_CVAR,         NULL, "Anisotropic",     &cv_granisotropicmode,40},
#ifdef _WINDOWS
	{IT_STRING|IT_CVAR,         NULL, "Fullscreen",      &cv_fullscreen,       50},
#endif
	{IT_STRING|IT_CVAR|IT_CV_SLIDER,
	                            NULL, "Translucent HUD", &cv_grtranslucenthud, 60},
	{IT_SUBMENU|IT_WHITESTRING, NULL, "Fog...",          &OGL_FogDef,          80},
	{IT_SUBMENU|IT_WHITESTRING, NULL, "Gamma...",        &OGL_ColorDef,        90},
	{IT_SUBMENU|IT_WHITESTRING, NULL, "Development...",  &OGL_DevDef,          100},
};

static menuitem_t OGL_FogMenu[] =
{
	{IT_STRING|IT_CVAR,       NULL, "Fog",         &cv_grfog,         0},
	{IT_STRING|IT_KEYHANDLER, NULL, "Fog color",   M_HandleFogColor, 10},
	{IT_STRING|IT_CVAR,       NULL, "Fog density", &cv_grfogdensity, 20},
};

static menuitem_t OGL_ColorMenu[] =
{
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "red",   &cv_grgammared,   10},
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "green", &cv_grgammagreen, 20},
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "blue",  &cv_grgammablue,  30},
};

static menuitem_t OGL_DevMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Translucent walls", &cv_grtranswall, 20},
};

menu_t OpenGLOptionDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OpenGLOptionsMenu)/sizeof (menuitem_t),
	&VideoOptionsDef,
	OpenGLOptionsMenu,
	M_DrawOpenGLMenu,
	30, 40,
	0,
	NULL
};


menu_t OGL_FogDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OGL_FogMenu)/sizeof (menuitem_t),
	&OpenGLOptionDef,
	OGL_FogMenu,
	M_OGL_DrawFogMenu,
	60, 40,
	0,
	NULL
};

menu_t OGL_ColorDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OGL_ColorMenu)/sizeof (menuitem_t),
	&OpenGLOptionDef,
	OGL_ColorMenu,
	M_OGL_DrawColorMenu,
	60, 40,
	0,
	NULL
};

menu_t OGL_DevDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OGL_DevMenu)/sizeof (menuitem_t),
	&OpenGLOptionDef,
	OGL_DevMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//======================================================================
// M_DrawOpenGLMenu()
//======================================================================
static void M_DrawOpenGLMenu(void)
{
	int mx, my;

	mx = OpenGLOptionDef.x;
	my = OpenGLOptionDef.y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
	V_DrawString(BASEVIDWIDTH - mx - V_StringWidth(cv_scr_depth.string),
		my + currentMenu->menuitems[2].alphaKey, V_YELLOWMAP, cv_scr_depth.string);
}

#define FOG_COLOR_ITEM  1
//======================================================================
// M_OGL_DrawFogMenu()
//======================================================================
static void M_OGL_DrawFogMenu(void)
{
	int mx, my;

	mx = OGL_FogDef.x;
	my = OGL_FogDef.y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
	V_DrawString(BASEVIDWIDTH - mx - V_StringWidth(cv_grfogcolor.string),
		my + currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey, V_YELLOWMAP, cv_grfogcolor.string);
	// blink cursor on FOG_COLOR_ITEM if selected
	if (itemOn == FOG_COLOR_ITEM && skullAnimCounter < 4)
		V_DrawCharacter(BASEVIDWIDTH - mx,
			my + currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey, '_' | 0x80);
}

//======================================================================
// M_OGL_DrawColorMenu()
//======================================================================
static void M_OGL_DrawColorMenu(void)
{
	int mx, my;

	mx = OGL_ColorDef.x;
	my = OGL_ColorDef.y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
	V_DrawString(mx, my + currentMenu->menuitems[0].alphaKey - 10,
		V_YELLOWMAP, "Gamma correction");
}

//======================================================================
// M_OpenGLOption()
//======================================================================
static void M_OpenGLOption(int choice)
{
	(void)choice;
	if (rendermode != render_soft)
		M_SetupNextMenu(&OpenGLOptionDef);
	else
		M_StartMessage("You are in software mode\nYou can't change the options\n", NULL, MM_NOTHING);
}

//======================================================================
// M_HandleFogColor()
//======================================================================
static void M_HandleFogColor(int choice)
{
	size_t i, l;
	char temp[8];
	boolean exitmenu = false; // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn++;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn--;
			break;

		case KEY_ESCAPE:
			S_StartSound(NULL, sfx_menu1);
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			S_StartSound(NULL, sfx_menu1);
			strcpy(temp, cv_grfogcolor.string);
			strcpy(cv_grfogcolor.zstring, "000000");
			l = strlen(temp)-1;
			for (i = 0; i < l; i++)
				cv_grfogcolor.zstring[i + 6 - l] = temp[i];
			break;

		default:
			if ((choice >= '0' && choice <= '9') || (choice >= 'a' && choice <= 'f')
				|| (choice >= 'A' && choice <= 'F'))
			{
				S_StartSound(NULL, sfx_menu1);
				strcpy(temp, cv_grfogcolor.string);
				strcpy(cv_grfogcolor.zstring, "000000");
				l = strlen(temp);
				for (i = 0; i < l; i++)
					cv_grfogcolor.zstring[5 - i] = temp[l - i];
					cv_grfogcolor.zstring[5] = (char)choice;
			}
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}
#endif


static void M_NextServerPage(void)
{
	if ((serverlistpage + 1) * SERVERS_PER_PAGE < serverlistcount) serverlistpage++;
}

static void M_PrevServerPage(void)
{
	if (serverlistpage > 0) serverlistpage--;
}
