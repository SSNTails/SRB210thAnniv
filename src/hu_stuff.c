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

#include "doomdef.h"
#include "byteptr.h"
#include "hu_stuff.h"

#include "d_clisrv.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"

#include "dstrings.h"
#include "st_stuff.h" // ST_HEIGHT
#include "r_local.h"

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#include "p_tick.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// coords are scaled
#define HU_INPUTX 0
#define HU_INPUTY 0

// Server message (dedicated) (flag).
#define HU_SERVER_SAY	1

//-------------------------------------------
//              heads up font
//-------------------------------------------
patch_t *hu_font[HU_FONTSIZE];

// Level title and credits fonts
patch_t *lt_font[LT_FONTSIZE];
patch_t *cred_font[CRED_FONTSIZE];

static player_t *plr;
boolean chat_on; // entering a chat message?
static char w_chat[HU_MAXMSGLEN];
static boolean headsupactive = false;
static boolean hu_showscores; // draw rankings
static char hu_tick;

patch_t *rflagico;
patch_t *bflagico;

//-------------------------------------------
//              coop hud
//-------------------------------------------

patch_t *emerald1;
patch_t *emerald2;
patch_t *emerald3;
patch_t *emerald4;
patch_t *emerald5;
patch_t *emerald6;
patch_t *emerald7;
static patch_t *emblemicon;

//-------------------------------------------
//              misc vars
//-------------------------------------------

// crosshair 0 = off, 1 = cross, 2 = angle, 3 = point, see m_menu.c
static patch_t *crosshair[3]; // 3 precached crosshair graphics

// -------
// protos.
// -------
static void HU_DrawRankings(void);
static void HU_DrawCoopOverlay(void);

//======================================================================
//                 KEYBOARD LAYOUTS FOR ENTERING TEXT
//======================================================================

char *shiftxform;

char english_shiftxform[] =
{
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"', // shift-'
	'(', ')', '*', '+',
	'<', // shift-,
	'_', // shift--
	'>', // shift-.
	'?', // shift-/
	')', // shift-0
	'!', // shift-1
	'@', // shift-2
	'#', // shift-3
	'$', // shift-4
	'%', // shift-5
	'^', // shift-6
	'&', // shift-7
	'*', // shift-8
	'(', // shift-9
	':',
	':', // shift-;
	'<',
	'+', // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'[', // shift-[
	'!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
	']', // shift-]
	'"', '_',
	'~', // shift-` for some stupid reason this was a '
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

char cechotext[1024];
tic_t cechotimer = 0;
tic_t cechoduration = 5*TICRATE;
int cechoflags = 0;

//======================================================================
//                          HEADS UP INIT
//======================================================================

// just after
static void Command_Say_f(void);
static void Command_Sayto_f(void);
static void Command_Sayteam_f(void);
static void Got_Saycmd(byte **p, int playernum);

// Initialise Heads up
// once at game startup.
//
void HU_Init(void)
{
	int i, j;
	char *buffer = malloc(9);

	COM_AddCommand("say", Command_Say_f);
	COM_AddCommand("sayto", Command_Sayto_f);
	COM_AddCommand("sayteam", Command_Sayteam_f);
	RegisterNetXCmd(XD_SAY, Got_Saycmd);

	// set shift translation table
	shiftxform = english_shiftxform;

	if (dedicated)
	{
		free(buffer);
		return;
	}

	// cache the heads-up font for entire game execution
	j = HU_FONTSTART;
	for (i = 0; i < HU_FONTSIZE; i++)
	{
		sprintf(buffer, "STCFN%.3d", j);
		j++;
		if (i >= HU_REALFONTSIZE && i != '~' - HU_FONTSTART && i != '`' - HU_FONTSTART) /// \note font end hack
			continue;
		hu_font[i] = (patch_t *)W_CachePatchName(buffer, PU_STATIC);
	}

	// cache the level title font for entire game execution
	lt_font[0] = (patch_t *)W_CachePatchName("LTFNT039", PU_STATIC); /// \note fake start hack

	// Number support
	lt_font[9] = (patch_t *)W_CachePatchName("LTFNT048", PU_STATIC);
	lt_font[10] = (patch_t *)W_CachePatchName("LTFNT049", PU_STATIC);
	lt_font[11] = (patch_t *)W_CachePatchName("LTFNT050", PU_STATIC);
	lt_font[12] = (patch_t *)W_CachePatchName("LTFNT051", PU_STATIC);
	lt_font[13] = (patch_t *)W_CachePatchName("LTFNT052", PU_STATIC);
	lt_font[14] = (patch_t *)W_CachePatchName("LTFNT053", PU_STATIC);
	lt_font[15] = (patch_t *)W_CachePatchName("LTFNT054", PU_STATIC);
	lt_font[16] = (patch_t *)W_CachePatchName("LTFNT055", PU_STATIC);
	lt_font[17] = (patch_t *)W_CachePatchName("LTFNT056", PU_STATIC);
	lt_font[18] = (patch_t *)W_CachePatchName("LTFNT057", PU_STATIC);

	j = LT_REALFONTSTART;
	for (i = LT_REALFONTSTART - LT_FONTSTART; i < LT_FONTSIZE; i++)
	{
		sprintf(buffer, "LTFNT%.3d", j);
		j++;
		lt_font[i] = (patch_t *)W_CachePatchName(buffer, PU_STATIC);
	}

	// cache the credits font for entire game execution (why not?)
	j = CRED_FONTSTART;
	for (i = 0; i < CRED_FONTSIZE; i++)
	{
		sprintf(buffer, "CRFNT%.3d", j);
		j++;
		cred_font[i] = (patch_t *)W_CachePatchName(buffer, PU_STATIC);
	}

	// cache the crosshairs, don't bother to know which one is being used,
	// just cache all 3, they're so small anyway.
	for (i = 0; i < HU_CROSSHAIRS; i++)
	{
		sprintf(buffer, "CROSHAI%c", '1'+i);
		crosshair[i] = (patch_t *)W_CachePatchName(buffer, PU_STATIC);
	}
	free(buffer);

	emblemicon = W_CachePatchName("EMBLICON", PU_STATIC);
	emerald1 = W_CachePatchName("CHAOS1", PU_STATIC);
	emerald2 = W_CachePatchName("CHAOS2", PU_STATIC);
	emerald3 = W_CachePatchName("CHAOS3", PU_STATIC);
	emerald4 = W_CachePatchName("CHAOS4", PU_STATIC);
	emerald5 = W_CachePatchName("CHAOS5", PU_STATIC);
	emerald6 = W_CachePatchName("CHAOS6", PU_STATIC);
	emerald7 = W_CachePatchName("CHAOS7", PU_STATIC);
}

static inline void HU_Stop(void)
{
	headsupactive = false;
}

//
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
	if (headsupactive)
		HU_Stop();

	plr = &players[consoleplayer];
	HU_clearChatChars();

	headsupactive = true;
}

//======================================================================
//                            EXECUTION
//======================================================================

void TeamPlay_OnChange(void)
{
	int i;
	char s[50];

	// Change the name of the teams

	if (cv_teamplay.value == 1)
	{
		// color
		for (i = 0; i < MAXSKINCOLORS; i++)
		{
			sprintf(s, "%s team", Color_Names[i]);
			strcpy(team_names[i],s);
		}
	}
	else if (cv_teamplay.value == 2)
	{
		// skins

		for (i = 0; i < numskins; i++)
		{
			sprintf(s, "%s team", skins[i].name);
			strcpy(team_names[i], s);
		}
	}
}

/** Runs a say command, sending an ::XD_SAY message.
  * A say command consists of a signed 8-bit integer for the target, an
  * unsigned 8-bit flag variable, and then the message itself.
  *
  * The target is 0 to say to everyone, 1 to 32 to say to that player, or -1
  * to -32 to say to everyone on that player's team. Note: This means you
  * have to add 1 to the player number, since they are 0 to 31 internally.
  *
  * The flag HU_SERVER_SAY will be set if it is the dedicated server speaking.
  *
  * This function obtains the message using COM_Argc() and COM_Argv().
  *
  * \param target    Target to send message to.
  * \param usedargs  Number of arguments to ignore.
  * \sa Command_Say_f, Command_Sayteam_f, Command_Sayto_f, Got_Saycmd
  * \author Graue <graue@oceanbase.org>
  */
static void DoSayCommand(signed char target, size_t usedargs)
{
	XBOXSTATIC char buf[254];
	size_t numwords, ix;
	char *msg = &buf[2];
	const int msgspace = sizeof buf - 2;

	numwords = COM_Argc() - usedargs;
	I_Assert(numwords > 0);

	if (cv_mute.value && !(server || adminplayer == consoleplayer))
	{
		CONS_Printf("The chat is muted. You can't say anything at the moment.\n");
		return;
	}

	buf[0] = target;
	buf[1] = (char)(dedicated ? HU_SERVER_SAY : 0);
	msg[0] = '\0';

	for (ix = 0; ix < numwords; ix++)
	{
		if (ix > 0)
			strlcat(msg, " ", msgspace);
		strlcat(msg, COM_Argv(ix + usedargs), msgspace);
	}

	SendNetXCmd(XD_SAY, buf, strlen(msg) + 1 + msg-buf);
}

/** Send a message to everyone.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Say_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf("say <message>: send a message\n");
		return;
	}

	DoSayCommand(0, 1);
}

/** Send a message to a particular person.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Say_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayto_f(void)
{
	int target;

	if (COM_Argc() < 3)
	{
		CONS_Printf("sayto <playername|playernum> <message>: send a message to a player\n");
		return;
	}

	target = nametonum(COM_Argv(1));
	if (target == -1)
	{
		CONS_Printf("sayto: No player with that name!\n");
		return;
	}
	target++; // Internally we use 0 to 31, but say command uses 1 to 32.

	DoSayCommand((signed char)target, 2);
}

/** Send a message to members of the player's team.
  * \sa DoSayCommand, Command_Say_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayteam_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf("sayteam <message>: send a message to your team\n");
		return;
	}

	if (dedicated)
	{
		CONS_Printf("Dedicated servers can't send team messages. Use \"say\".\n");
		return;
	}

	DoSayCommand((signed char)(-(consoleplayer+1)), 1);
}

/** Receives a message, processing an ::XD_SAY command.
  * \sa DoSayCommand
  * \author Graue <graue@oceanbase.org>
  */
static void Got_Saycmd(byte **p, int playernum)
{
	signed char target;
	byte flags;
	const char *dispname;
	char *msg;
	boolean action = false;

	if (cv_mute.value && playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal say command received from %s while muted\n",
			player_names[playernum]);
		if (server)
		{
			XBOXSTATIC char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	target = READCHAR(*p);
	flags = READBYTE(*p);

	msg = (char *)*p;
	SKIPSTRING(*p);

	// Handle "/me" actions, but only in messages to everyone.
	if (target == 0 && strlen(msg) > 4 && strnicmp(msg, "/me ", 4) == 0)
	{
		msg += 4;
		action = true;
	}

	if (flags & HU_SERVER_SAY)
	{
		if (playernum == 0)
			dispname = "SERVER";
		else // HAX!
			return;
	}
	else
		dispname = player_names[playernum];

	// Show messages sent by you, to you, to your team, or to everyone:
	if (consoleplayer == playernum // By you
		|| consoleplayer == target-1 // To you
		|| (target < 0 && ST_SameTeam(&players[consoleplayer],
			&players[-target - 1])) // To your team
		|| target == 0) // To everyone
	{
		const char *cstart = "", *cend = "", *fmt;

		// In CTF, color the player's name.
		if (gametype == GT_CTF)
		{
			cend = "\x80";
			if (players[playernum].ctfteam == 1) // red
				cstart = "\x85";
			else if (players[playernum].ctfteam == 2) // blue
				cstart = "\x84";
		}
		else if (playernum == serverplayer || playernum == adminplayer)
		{
			cend = "\x80";
			cstart = "\x83";
		}

		// Choose the proper format string for display.
		// Each format includes four strings: color start, display
		// name, color end, and the message itself.
		// '\4' makes the message yellow and beeps; '\3' just beeps.
		if (action)
			fmt = "\4* %s%s%s \x82%s\n";
		else if (target == 0) // To everyone
			fmt = "\3<%s%s%s> %s\n";
		else if (target-1 == consoleplayer) // To you
			fmt = "\3*%s%s%s* %s\n";
		else if (target > 0) // By you, to another player
		{
			// Use target's name.
			dispname = player_names[target-1];
			fmt = "\3->*%s%s%s* %s\n";
		}
		else // To your team
			fmt = "\3>>%s%s%s<< (team) %s\n";

		CONS_Printf(fmt, cstart, dispname, cend, msg);
	}
}

// Handles key input and string input
//
static inline boolean HU_keyInChatString(char *s, char ch)
{
	size_t l;

	if (ch >= ' ' && (ch <= '_' || ch == '~' || ch == '`')) /// \note font end hack
	{
		l = strlen(s);
		if (l < HU_MAXMSGLEN - 1)
		{
			s[l++] = ch;
			s[l]=0;
			return true;
		}
		return false;
	}
	else if (ch == KEY_BACKSPACE)
	{
		l = strlen(s);
		if (l)
			s[--l] = 0;
		else
			return false;
	}
	else if (ch != KEY_ENTER)
		return false; // did not eat key

	return true; // ate the key
}

//
//
void HU_Ticker(void)
{
	if (dedicated)
		return;

	hu_tick++;
	hu_tick &= 7; // currently only to blink chat input cursor

	if ((gamekeydown[gamecontrol[gc_scores][0]] || gamekeydown[gamecontrol[gc_scores][1]]))
		hu_showscores = !chat_on;
	else
	{
		if (gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF
#ifdef CHAOSISNOTDEADYET
			|| gametype == GT_CHAOS
#endif
			)
		{
			hu_showscores = playerdeadview;
		}
		else
			hu_showscores = false;
	}
}

#define QUEUESIZE 256

static boolean teamtalk = false;
static char chatchars[QUEUESIZE];
static int head = 0, tail = 0;

//
// HU_dequeueChatChar
//
char HU_dequeueChatChar(void)
{
	char c;

	if (head != tail)
	{
		c = chatchars[tail];
		tail = (tail + 1) & (QUEUESIZE-1);
	}
	else
		c = 0;

	return c;
}

//
//
static void HU_queueChatChar(char c)
{
	if (((head + 1) & (QUEUESIZE-1)) == tail)
		CONS_Printf(HUSTR_MSGU); // message not sent
	else
	{
		if (c == KEY_BACKSPACE)
		{
			if (tail != head)
				head = (head - 1) & (QUEUESIZE-1);
		}
		else
		{
			chatchars[head] = c;
			head = (head + 1) & (QUEUESIZE-1);
		}
	}

	// send automaticly the message (no more chat char)
	if (c == KEY_ENTER)
	{
		char buf[255];
		size_t ci = 0;

		do {
			c = HU_dequeueChatChar();
			if (c != 13) // Graue 07-04-2004: don't know why this has to be done
				buf[ci++]=c;
		} while (c);
		// Graue 09-04-2004: 1 not 2, hell if I know why
		if (ci > 1) // Graue 07-02-2004: 2 not 3, with HU_BROADCAST disposed of
		{
			if (teamtalk)
				COM_BufInsertText(va("sayteam \"%s\"", buf)); // Graue 07-04-2004: quote it!
			else
				COM_BufInsertText(va("say \"%s\"", buf)); // Graue 07-04-2004: quote it!
		}
	}
}

void HU_clearChatChars(void)
{
	while (tail != head)
		HU_queueChatChar(KEY_BACKSPACE);
	chat_on = false;
}

//
// Returns true if key eaten
//
boolean HU_Responder(event_t *ev)
{
	static boolean shiftdown = false, altdown = false;
	boolean eatkey = false;
	byte c;

	if (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT)
	{
		shiftdown = (ev->type == ev_keydown);
		return chat_on;
	}
	else if (ev->data1 == KEY_LALT || ev->data1 == KEY_RALT)
	{
		altdown = (ev->type == ev_keydown);
		return false;
	}

	if (ev->type != ev_keydown)
		return false;

	// only KeyDown events now...

	if (!chat_on)
	{
		// enter chat mode
		if ((ev->data1 == gamecontrol[gc_talkkey][0] || ev->data1 == gamecontrol[gc_talkkey][1])
			&& netgame && (!cv_mute.value || server || (adminplayer == consoleplayer)))
		{
			eatkey = chat_on = true;
			w_chat[0] = 0;
			teamtalk = false;
		}
		if ((ev->data1 == gamecontrol[gc_teamkey][0] || ev->data1 == gamecontrol[gc_teamkey][1])
			&& netgame && (!cv_mute.value || server || (adminplayer == consoleplayer)))
		{
			eatkey = chat_on = true;
			w_chat[0] = 0;
			teamtalk = true;
		}
	}
	else
	{
		c = (byte)ev->data1;

		// use console translations
		if (shiftdown)
			c = shiftxform[c];

		// Graue 07-04-2004: on following line, added a ! before shiftdown
		//                   and changed || to &&
		// if shiftdown, then the shiftxform has ALREADY been done
		// (this is what was causing ^ to show up as ", shiftxform being done twice)
		if (!shiftdown && (c >= 'a' && c <= 'z'))
			c = shiftxform[c];

		if (c == '"') // Graue 07-04-2004: quote marks mess it up
			c = '\'';

		eatkey = HU_keyInChatString(w_chat,c);
		if (eatkey)
			HU_queueChatChar(c);
		if (c == KEY_ENTER)
			chat_on = false;
		else if (c == KEY_ESCAPE)
			chat_on = false;

		eatkey = true;
	}

	return eatkey;
}

//======================================================================
//                         HEADS UP DRAWING
//======================================================================

//
// HU_DrawChat
//
// Draw chat input
//
static void HU_DrawChat(void)
{
	int t = 0, c = 0, y = HU_INPUTY;
	size_t i = 0;
	const char *ntalk = "Say: ", *ttalk = "Say-Team: ";
	const char *talk = ntalk;

	if (teamtalk)
	{
		talk = ttalk;
#if 0
		     if (players[consoleplayer].ctfteam == 1)
			t = 0x500;  // Red
		else if (players[consoleplayer].ctfteam == 2)
			t = 0x400; // Bule
#endif
	}

	while (talk[i])
	{
		V_DrawCharacter(HU_INPUTX + (c<<3), y, talk[i++] | V_NOSCALEPATCH|V_NOSCALESTART);
		c++;
	}

	i = 0;
	while (w_chat[i])
	{
		//Hurdler: isn't it better like that?
		V_DrawCharacter(HU_INPUTX + (c<<3), y, w_chat[i++] | V_NOSCALEPATCH|V_NOSCALESTART|t);

		c++;
		if (c >= (vid.width>>3))
		{
			c = 0;
			y += 8;
		}
	}

	if (hu_tick < 4)
		V_DrawCharacter(HU_INPUTX + (c<<3), y, '_' | V_NOSCALEPATCH|V_NOSCALESTART|t);
}


// draw the Crosshair, at the exact center of the view.
//
// Crosshairs are pre-cached at HU_Init

static inline void HU_DrawCrosshair(void)
{
	int i, y;

	i = cv_crosshair.value & 3;
	if (!i)
		return;

	if (gametype == GT_CTF && !players[consoleplayer].ctfteam)
		return;

#ifdef HWRENDER
	if (rendermode != render_soft)
		y = (int)gr_basewindowcentery;
	else
#endif
		y = viewwindowy + (viewheight>>1);

	V_DrawTranslucentPatch(vid.width>>1, y, V_NOSCALESTART, crosshair[i - 1]);
}

static inline void HU_DrawCrosshair2(void)
{
	int i, y;

	i = cv_crosshair2.value & 3;
	if (!i)
		return;

	if (gametype == GT_CTF && !players[secondarydisplayplayer].ctfteam)
		return;

#ifdef HWRENDER
	if (rendermode != render_soft)
		y = (int)gr_basewindowcentery;
	else
#endif
		y = viewwindowy + (viewheight>>1);

	if (splitscreen)
	{
#ifdef HWRENDER
		if (rendermode != render_soft)
			y += (int)gr_viewheight;
		else
#endif
			y += viewheight;

		V_DrawTranslucentPatch(vid.width>>1, y, V_NOSCALESTART, crosshair[i - 1]);
	}
}

// Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
	// draw chat string plus cursor
	if (chat_on)
		HU_DrawChat();

	if (gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE || gamestate == GS_CREDITS)
		return;

	// draw multiplayer rankings
	if (hu_showscores)
	{
		if (gametype == GT_MATCH || gametype == GT_RACE || gametype == GT_TAG || gametype == GT_CTF
#ifdef CHAOSISNOTDEADYET
			|| gametype == GT_CHAOS
#endif
			)
			HU_DrawRankings();
		else if (gametype == GT_COOP)
		{
			HU_DrawCoopOverlay();

			if (multiplayer || netgame)
				HU_DrawRankings();
		}
	}

	// draw the crosshair, not when viewing demos nor with chasecam
	if (!automapactive && cv_crosshair.value && !demoplayback && !cv_chasecam.value)
		HU_DrawCrosshair();

	if (!automapactive && cv_crosshair2.value && !demoplayback && !cv_chasecam2.value)
		HU_DrawCrosshair2();

	if (cechotimer)
	{
		int i = 0;
		int y = (BASEVIDHEIGHT/2)-4;
		int pnumlines = 0;
		char *line;
		char *echoptr;
		char temp[1024];

		cechotimer--;

		while (cechotext[i] != '\0')
		{
			if (cechotext[i] == '\\')
				pnumlines++;

			i++;
		}

		y -= (pnumlines-1)*6;

		strcpy(temp, cechotext);

		echoptr = &temp[0];

		while (*echoptr != '\0')
		{
			line = strchr(echoptr, '\\');

			if (line == NULL)
				return;

			*line = '\0';

			V_DrawCenteredString(BASEVIDWIDTH/2, y, cechoflags, echoptr);
			y += 12;

			echoptr = line;
			echoptr++;
		}
	}
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

// Clear old messages from the borders around the view window
// (only for reduced view, refresh the borders when needed)
//
// startline: y coord to start clear,
// clearlines: how many lines to clear.
//
static int oldclearlines;

void HU_Erase(void)
{
	int topline, bottomline;
	int y, yoffset;

	// clear hud msgs on double buffer (Glide mode)
	boolean secondframe;
	static int secondframelines;

	if (con_clearlines == oldclearlines && !con_hudupdate && !chat_on)
		return;

	// clear the other frame in double-buffer modes
	secondframe = (con_clearlines != oldclearlines);
	if (secondframe)
		secondframelines = oldclearlines;

	// clear the message lines that go away, so use _oldclearlines_
	bottomline = oldclearlines;
	oldclearlines = con_clearlines;
	if (chat_on)
		if (bottomline < 8)
			bottomline = 8;

	if (automapactive || viewwindowx == 0) // hud msgs don't need to be cleared
		return;

	// software mode copies view border pattern & beveled edges from the backbuffer
	if (rendermode == render_soft)
	{
		topline = 0;
		for (y = topline, yoffset = y*vid.width; y < bottomline; y++, yoffset += vid.width)
		{
			if (y < viewwindowy || y >= viewwindowy + viewheight)
				R_VideoErase(yoffset, vid.width); // erase entire line
			else
			{
				R_VideoErase(yoffset, viewwindowx); // erase left border
				// erase right border
				R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
			}
		}
		con_hudupdate = false; // if it was set..
	}
#ifdef HWRENDER
	else if (rendermode != render_none)
	{
		// refresh just what is needed from the view borders
		HWR_DrawViewBorder(secondframelines);
		con_hudupdate = secondframe;
	}
#endif
}

//======================================================================
//                   IN-LEVEL MULTIPLAYER RANKINGS
//======================================================================

// count frags for each team
int HU_CreateTeamScoresTbl(playersort_t *tab, ULONG dmtotals[])
{
	int i, j, scorelines, team;

	scorelines = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			if (cv_teamplay.value == 1)
				team = players[i].skincolor;
			else
				team = players[i].skin;

			for (j = 0; j < scorelines; j++)
				if (tab[j].num == team)
				{
					// found team
					tab[j].count += players[i].score;
					if (dmtotals)
						dmtotals[team] = tab[j].count;
					break;
				}
			if (j == scorelines)
			{
				// team not found, so add it
				tab[scorelines].count = players[i].score;
				tab[scorelines].num = team;
				tab[scorelines].color = players[i].skincolor;
				tab[scorelines].name = team_names[team];

				if (dmtotals)
					dmtotals[team] = tab[scorelines].count;

				scorelines++;
			}
		}
	}
	return scorelines;
}

//
// HU_DrawTabRankings
//
void HU_DrawTabRankings(int x, int y, playersort_t *tab, int scorelines, int whiteplayer)
{
	int i;
	byte *colormap;

	for (i = 0; i < scorelines; i++)
	{
		V_DrawString(cv_teamplay.value ? x : x+24, y, (tab[i].num == whiteplayer && !cv_teamplay.value) ? V_YELLOWMAP : 0, tab[i].name);

		if (!cv_teamplay.value)
		{
			if (tab[i].color == 0)
			{
				colormap = colormaps;
				V_DrawSmallScaledPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin]);
			}
			else
			{
				colormap = (byte *) translationtables[players[tab[i].num].skin] - 256 + (tab[i].color<<8);
				V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}

		if (gametype == GT_CTF)
		{
			patch_t *p;

			if (players[tab[i].num].gotflag & MF_REDFLAG) // Red
			{
				p = rflagico;
				V_DrawSmallScaledPatch(x-32, y-4, 0, p);
			}
			else if (players[tab[i].num].gotflag & MF_BLUEFLAG) // Blue
			{
				p = bflagico;
				V_DrawSmallScaledPatch(x-32, y-4, 0, p);
			}
		}

		if (gametype == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedString(x+240, y, 0, va("%lu:%02lu.%02lu", players[tab[i].num].realtime/(60*TICRATE), players[tab[i].num].realtime/TICRATE % 60, players[tab[i].num].realtime % TICRATE));
				else
					V_DrawRightAlignedString(x+240, y, 0, va("%lu", tab[i].count));
			}
			else
				V_DrawRightAlignedString(x+240, y, 0, va("%lu:%02lu.%02lu", tab[i].count/(60*TICRATE), tab[i].count/TICRATE % 60, tab[i].count % TICRATE));
		}
		else
			V_DrawRightAlignedString(x+240, y, 0, va("%lu", tab[i].count));

		y += 16;
	}
}


//
// HU_DrawRankings
//
static void HU_DrawRankings(void)
{
	patch_t *p;
	playersort_t tab[MAXPLAYERS];
	int i, scorelines;
	ULONG whiteplayer;

	// draw the ranking title panel
/*	if (gametype != GT_CTF)
	{
		p = W_CachePatchName("RESULT", PU_CACHE);
		V_DrawScaledPatch((BASEVIDWIDTH - p->width)/2, 5, 0, p);
	}*/

	if (gametype == GT_CTF)
	{
		p = bflagico;
		V_DrawSmallScaledPatch(128 - p->width/4, 4, 0, p);

		V_DrawCenteredString(128, 16, 0, va("%lu", bluescore));

		p = rflagico;
		V_DrawSmallScaledPatch(192 - p->width/4, 4, 0, p);

		V_DrawCenteredString(192, 16, 0, va("%lu", redscore));
	}

	if (gametype != GT_RACE)
	{
		if (cv_timelimit.value && timelimitintics > 0)
		{
			int timeval = (timelimitintics+1-leveltime)/TICRATE;

			if (timeval > 0)
			{
				V_DrawCenteredString(64, 8, 0, "TIME LEFT");
				V_DrawCenteredString(64, 16, 0, va("%u", timeval));
			}
		}

		if (cv_pointlimit.value > 0)
		{
			V_DrawCenteredString(256, 8, 0, "POINT LIMIT");
			V_DrawCenteredString(256, 16, 0, va("%d", cv_pointlimit.value));
		}
	}

	// When you play, you quickly see your score because your name is displayed in white.
	// When playing back a demo, you quickly see who's the view.
	whiteplayer = demoplayback ? displayplayer : consoleplayer;

	if (!cv_teamplay.value)
	{
		int j;
		boolean completed[MAXPLAYERS];

		scorelines = 0;
		memset(completed, 0, sizeof (completed));
		memset(tab, 0, sizeof (playersort_t)*MAXPLAYERS);

		for (i = 0; i < MAXPLAYERS; i++)
		{
			tab[i].num = -1;
			tab[i].name = 0;

			if (gametype == GT_RACE && !circuitmap)
				tab[i].count = MAXINT;
		}

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j])
				continue;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i])
				{
					if (gametype == GT_TAG)
					{
						if (players[i].tagcount >= tab[scorelines].count && completed[i] == false)
						{
							tab[scorelines].count = players[i].tagcount;
							tab[scorelines].num = i;
							tab[scorelines].color = players[i].skincolor;
							tab[scorelines].name = player_names[i];
						}
					}
					else if (gametype == GT_RACE)
					{
						if (circuitmap)
						{
							if (players[i].laps+1 >= tab[scorelines].count && completed[i] == false)
							{
								tab[scorelines].count = players[i].laps+1;
								tab[scorelines].num = i;
								tab[scorelines].color = players[i].skincolor;
								tab[scorelines].name = player_names[i];
							}
						}
						else
						{
							if (players[i].realtime <= tab[scorelines].count && completed[i] == false)
							{
								tab[scorelines].count = players[i].realtime;
								tab[scorelines].num = i;
								tab[scorelines].color = players[i].skincolor;
								tab[scorelines].name = player_names[i];
							}
						}
					}
					else
					{
						if (players[i].score >= tab[scorelines].count && completed[i] == false)
						{
							tab[scorelines].count = players[i].score;
							tab[scorelines].num = i;
							tab[scorelines].color = players[i].skincolor;
							tab[scorelines].name = player_names[i];
						}
					}
				}
			}
			completed[tab[scorelines].num] = true;
			scorelines++;
		}

		if (scorelines > 10)
			scorelines = 10; //dont draw past bottom of screen, show the best only

		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
	}
	else
	{
		memset(tab, 0, sizeof (tab));

		scorelines = HU_CreateTeamScoresTbl(tab, NULL);

		if (scorelines > 10)
			scorelines = 10; // only show 10 best teams

		// and the team frag to the left
		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
		//	cv_teamplay.value == 1 ? players[whiteplayer].skincolor : players[whiteplayer].skin);
		// first param was "Teams"
		// Graue 04-08-2004: only display team counts not individual counts
		// there can be up to 16 teams by color (teamplay 1) or 32 by skin (teamplay 2)
	}
}

static void HU_DrawCoopOverlay(void)
{
	if (!netgame && (!modifiedgame || savemoddata))
	{
		char emblemsfound[20];
		int found = 0;
		int i;

		for (i = 0; i < MAXEMBLEMS; i++)
		{
			if (emblemlocations[i].collected)
				found++;
		}

		sprintf(emblemsfound, "- %d/%d", found, numemblems);
		V_DrawString(160, 96, 0, emblemsfound);
		V_DrawScaledPatch(128, 96 - emblemicon->height/4, 0, emblemicon);
	}

	if (emeralds & EMERALD1)
		V_DrawScaledPatch(64, 128, V_TRANSLUCENT, emerald1);
	if (emeralds & EMERALD2)
		V_DrawScaledPatch(96, 128, V_TRANSLUCENT, emerald2);
	if (emeralds & EMERALD3)
		V_DrawScaledPatch(128, 128, V_TRANSLUCENT, emerald3);
	if (emeralds & EMERALD4)
		V_DrawScaledPatch(160, 128, V_TRANSLUCENT, emerald4);
	if (emeralds & EMERALD5)
		V_DrawScaledPatch(192, 128, V_TRANSLUCENT, emerald5);
	if (emeralds & EMERALD6)
		V_DrawScaledPatch(224, 128, V_TRANSLUCENT, emerald6);
	if (emeralds & EMERALD7)
		V_DrawScaledPatch(256, 128, V_TRANSLUCENT, emerald7);
}
